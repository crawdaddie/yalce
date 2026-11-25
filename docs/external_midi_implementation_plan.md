# External MIDI instrument implementation plan

Status: proposed. Initial target: Linux/ALSA, Digitone 2, one fixed audio
interface for capture and playback.

## Available MIDI output

`fm_hmm_midi.ylc` now targets `Elektron Digitone II MIDI 1`, channels 1 and 2.
`bpm` defaults to 120. The parent coroutine sends timestamped MIDI Start,
then 24 MIDI clocks per quarter note while the note pattern runs. Enable
`MIDI CONFIG > SYNC > CLOCK RECEIVE` and `TRANSPORT RECEIVE` on the Digitone.
Call `stop ()` in the REPL to cancel both patterns and send MIDI Stop. Existing
note durations remain seconds; changing BPM changes the external tempo.
No `aconnect` output connection is required. The Digitone supplies the sounds;
this example does not capture its audio.

```ylc
let port = MIDI.midi_open "Elektron Digitone II MIDI 1";
# Inside play_pattern: both attacks use the same logical sample tick.
MIDI.midi_note port 0 60. 100 0.25;
MIDI.midi_note port 1 67. 100 0.25;
```

Channels are zero-based; pitch is an integer-valued Double, velocity is 1–127,
and duration is seconds. `midi_open` returns zero for a missing destination.
Note calls return zero on submission success or -1 on failure. `midi_note_at`
adds an explicit Uint64 sample tick as its final argument.

Linux queues real-time ALSA events using one sample-to-queue mapping. Equal
ticks produce equal queue deadlines; MIDI 1.0 does not transmit those timestamps
to the device. Late events can be delivered immediately.

The initial Linux mapping now uses a coherent audio frame/presentation-time
snapshot, including SoundIo's reported output latency, and the running ALSA
queue's time measured against CLOCK_MONOTONIC. Calibration occurs on the first
scheduled send and stays fixed so queued timestamps do not change. Scheduled
sends fail until an audio snapshot is available. An underrun or invalid latency
reading invalidates calibration; restart the process before resuming scheduled
MIDI. Immediate MIDI sends remain available.

This compensates the reported audio output latency, not synth response, USB/DIN
latency, or long-term clock drift. It does not establish the sample-accuracy
requirement below. macOS retains its previous timing mapping.

Note-offs are submitted with each note. Cancelling a pattern leaves already
queued MIDI events intact. Stop patterns and allow their gates to finish before
calling `midi_panic port channel`, which sends All Sound Off. Process termination
can discard pending releases; overlapping notes of the same pitch/channel share
MIDI 1.0 note-off semantics. macOS changes have not been hardware-tested.

## Sample accuracy requirement

Sample accuracy is required. Musically tight playback or compensation of the
average offset does not satisfy this requirement. The ALSA plan below is a
candidate implementation, not evidence that the complete hardware path meets it.

Use an intended output sample as the reference. After one fixed calibration,
the measured external onset must remain within one output sample of its target
under the supported workload, without per-note correction. At 48 kHz, one
sample is approximately 20.83 microseconds. Define the onset detector and its
measurement uncertainty before accepting hardware results.

Distinguish three boundaries:

| Boundary | Requirement |
| --- | --- |
| ylc event and internal audio activation | Exact target sample in deterministic tests. |
| MIDI delivery to the synth | Verify physical delivery timing relative to the audio clock; kernel timestamps alone are insufficient evidence. |
| Synth audio returned to the output | Verify the complete path within one sample after fixed calibration. Accurate MIDI output alone does not establish this. |

DAW external-instrument routing and delay compensation do not establish a
sample-accurate external onset guarantee. Ableton documents timing variation
outside its control in its [MIDI fact sheet](https://www.ableton.com/en/manual/midi-fact-sheet/).
Its published tests are examples, not measurements of the proposed setup.

Audio-clocked MIDI hardware is a separate candidate. Expert Sleepers describes
[USAMO](https://www.expert-sleepers.co.uk/usamo.html) as generating MIDI from an
audio output, with sample-accurate MIDI timing. That claim concerns MIDI output,
not the receiving synth's audio onset. Its supplied software lists Mac and
Windows support; Linux/ylc integration must be established before choosing it.

[Elektron Overbridge](https://www.elektron.se/support-downloads/overbridge)
supports Digitone II. Investigate its timing contract and platform integration
as another candidate; support for the instrument does not prove a sample-accurate
onset guarantee. It is no longer excluded from feasibility research.

## Feasibility gate before implementation

Run this gate before building the broad scheduling API:

1. Fix the intended monitoring route, interface, MIDI transport, sample rate,
   buffer settings, and a reproducible sharp-transient Digitone patch.
2. Record repeated external transients and an internal sample-timed reference
   into a common measurement clock. Include the actual monitored output path.
3. Measure individual onset errors after a single fixed offset. Report maximum
   absolute error as well as median and percentiles. A passing average is not
   sufficient.
4. Repeat across callback phases, simultaneous notes, sustained traffic, CPU
   load, and long sessions. Separate clock drift from delivery jitter and synth
   response variation. Account for measurement error.
5. If ordinary MIDI fails the one-sample requirement, evaluate an audio-clocked
   MIDI transport and Overbridge before committing to the ALSA-only design.
6. If no live path passes, mark the live requirement unmet. Capturing and
   aligning audio for later sample-timed playback is a different workflow and
   requires user acceptance; it is not a substitute hidden behind this API.

Finite measurements establish performance within tested conditions, not an
unconditional hardware guarantee. Identify which timing bounds the selected
transport and device actually document. Delay general API work until the live
path has enough evidence to justify it.

## Required behavior

Inside a coroutine scheduled with `play_pattern`:

```ylc
send_midi_note 0 60.;
kick () |> play_node;
```

Both calls refer to the same logical event time. The Digitone and kick should
be heard together, whether the Digitone is monitored externally or returned
through audio_jit. Different monitoring paths require different compensation.

MIDI transmission happens before the intended sound, when necessary. Calling
the functions consecutively does not mean transmitting MIDI immediately.

```text
                          +--> kick ------------------------+
coroutine event at T -----+                                 +--> output
                          +--> scheduled MIDI --> Digitone  |
                                                    |      |
                                           interface input |
                                                    |      |
                                             audio_jit ----+
```

The fixed interface avoids supporting independently clocked input and output
devices in this implementation. Calibration is reusable while the MIDI route,
sample rate, buffer settings, and monitoring path remain unchanged. Using the
same interface does not make these latencies zero or perfectly constant.

## How DAWs handle external instruments

DAWs expose an external instrument as a MIDI destination plus an audio return.
The return enters the normal mixer and effect chain, alongside internal tracks.
Latency compensation aligns the paths by preparing playback ahead of time and
delaying shorter paths where needed.

| Program | External instrument behavior |
| --- | --- |
| Ableton Live | External Instrument selects MIDI output and audio input. Its Hardware Latency control corrects device delay that Live cannot detect automatically. Delay Compensation aligns tracks to the longest path. |
| Bitwig Studio | HW Instrument routes notes to a MIDI port and receives audio. It provides an audio-return latency offset in samples and an audio effect chain. |

Sources: [Live External Instrument](https://www.ableton.com/en/manual/live-instrument-reference/#external-instrument),
[Live delay compensation](https://help.ableton.com/hc/en-us/articles/209072409-Delay-Compensation-FAQ),
and [Bitwig hardware devices](https://www.bitwig.com/userguide/latest/hardware/).

Playback can be prepared because future notes are known. A newly played live
note cannot be sent before it exists. Keeping live internal and external parts
aligned can therefore add monitoring latency. Moving a recording afterward
does not correct what was heard during monitoring.

Fixed latency and jitter are separate. Compensation removes a repeatable
offset; it cannot remove unpredictable MIDI or synth timing variation. See
[Live MIDI timing](https://www.ableton.com/en/manual/midi-fact-sheet/).

## Existing code and missing behavior

| Area | Existing behavior | Required change |
| --- | --- | --- |
| `engine/scheduling.c` | Coroutines execute ahead of their logical tick. `get_tl_tick()` exposes that tick. | Preserve it when submitting MIDI; provide enough preparation time. |
| `engine/engine_core.c` | `play_node` queues activation at `get_tl_tick()`. | Keep sample-timed audio activation. Expose hardware return through existing nodes. |
| `engine/audio_loop.c` | SoundIo capture and playback; capture ring; render-frame counter. | Map frames to output presentation time and account for capture buffering. |
| `engine/midi_linux.c` | Immediate output; destination arguments ignored; timestamped sends use the direct queue. | Explicit destinations and a real scheduled ALSA queue. |
| `engine/bindings/MIDI.ylc` | Low-level MIDI externs; no scheduled note helper. | Add the timeline-aware API without silently changing immediate APIs. |
| `libs/audio_jit/DSP.ylc` | Import starts audio immediately. | Permit input configuration before startup; preserve ordinary imports. |
| `engine/bindings/Config.ylc` | Input device and channel configuration. | Reuse for the fixed interface. |
| `engine/bindings/SynthEngine.ylc` | `hw_inlet` exposes configured input signals. | Reuse for the audio return; verify audio_jit integration. |

The current frame counter describes rendering progress, not the exact frame
reaching the converter now. Scheduler wakeups derived only from that counter
are insufficient for accurate MIDI deadlines between audio callbacks.

## Timing model

Keep musical time, rendering time, and wall-clock deadlines distinct:

- `T`: logical sample tick shared by both calls.
- `P(T)`: estimated monotonic time when output sample `T` reaches the interface
  output.
- `M`: MIDI delivery plus synth response latency.
- `C`: synth audio output to availability in the capture stream consumed by DSP,
  including interface input and the application's capture buffering.
- `O`: DSP output to interface output latency.
- `F`: declared algorithmic latency in the external return's effect chain.

For external monitoring through a mixer:

```text
MIDI deadline = P(T) - M
kick audible = P(T)
```

For the Digitone returned through audio_jit:

```text
MIDI deadline = P(T) - (M + C + F + O)
return audible = P(T)
kick audible   = P(T)
```

These are conceptual equations. Backend timestamp conversion uses one clock
mapping and must not count output latency twice. A measured aggregate path
latency replaces its estimated components; a manual residual adjusts it.

For example, if the kick is due at wall time 1000 ms and the measured MIDI-to-
monitored-output path takes 18 ms, transmit at 982 ms. These numbers illustrate
the calculation; they are not Digitone measurements.

An intentional slow envelope, reverb tail, or musical delay is not automatically
removed. Calibrate with a sharp transient. Compensate only declared processing
latency, such as lookahead, plus the hardware and buffering path.

## API and configuration

Preserve the requested two-argument call:

```ylc
send_midi_note 0 60.;
```

Proposed semantics:

- First argument is a zero-based MIDI channel on the configured output port.
  It is not a device index. User channel 1 is represented by `0`.
- Pitch accepts an integer-valued `Double` for consistency with pattern data.
  Reject nonfinite, fractional, and out-of-range values rather than silently
  wrapping or introducing pitch bend.
- Velocity and gate duration come from instrument setup, using named defaults.
  Gate duration is in seconds and independent of the coroutine's next yield.
- Schedule both note-on and note-off. Return an error status on invalid input,
  missing configuration, or submission failure.
- Keep lower-level explicit velocity, duration, and note-off operations available
  for sustained notes. They must use the same timeline when scheduled.

Configure the destination once outside the pattern. Store the MIDI endpoint,
default velocity and gate, monitoring mode, return signal, and compensation.
Do not embed port discovery or calibration in every note call.

For audio_jit return, select the fixed interface and input channels before
starting audio. Configure one stereo or mono return, then keep its processing
node alive. Notes trigger the Digitone, not a new capture stream or effect node.

## Implementation stages

### 1 Establish failing tests and the timing contract

Add deterministic tests before repairing existing destination and timestamp
defects. Use fake clocks and a fake MIDI sink; hardware is not needed.

Test that two calls in one coroutine share `T`, an early coroutine queues MIDI
instead of sending immediately, and both monitoring modes calculate the correct
deadline. Cover sample-rate conversion and deadlines between audio callbacks.

Observe the regressions failing before modifying their implementations.

### 2 Publish an audio presentation clock

In `engine/audio_loop.c`, publish a coherent snapshot containing sample tick,
monotonic timestamp, sample rate, and measured output buffering. Account for
frames already queued and offsets within each callback.

Use SoundIo latency queries in their permitted callbacks. Check backend errors;
do not substitute the requested software buffer size for measured latency.
Keep the conversion behind the audio layer so MIDI callers use sample ticks.

Refresh the mapping during playback to track audio-clock drift. Convert into
the ALSA queue's time domain explicitly; do not mix raw monotonic timestamps,
queue-relative time, and audio sample counts.

An xrun or device restart invalidates the mapping. Flush pending external notes,
release active notes, rebuild the mapping, and resume at a future boundary.

### 3 Implement scheduled MIDI output

Repair destination addressing in the shared Linux backend. Resolve the selected
ALSA client and port explicitly; avoid broadcasting to every subscriber.

Allocate and start a real ALSA sequencer queue. Submit events ahead of time with
real-time timestamps, converted from the presentation clock. ALSA musical ticks
are not audio sample ticks. Leave final delivery to the backend queue rather
than sleeping or sending MIDI from the audio callback.

ALSA documents queued delivery, direct delivery, and timestamp domains in its
[sequencer interface](https://www.alsa-project.org/alsa-doc/alsa-lib/seq.html).

Use bounded submissions, stable ordering for equal deadlines, explicit errors,
and a clear late-event counter. Submit from the scheduling/control side; audio
callbacks must not acquire MIDI locks, allocate, or perform MIDI I/O.

Preserve immediate low-level calls. Give new scheduled entry points explicit
timestamp semantics; do not silently reinterpret existing `_ts` callers.

### 4 Integrate notes with coroutine ownership

Use `get_tl_tick()` and the current task token when queuing notes. Add the helper
binding and update every affected declaration, caller, and fixture.

Natural coroutine completion must leave its queued notes and note-offs intact.
Explicit cancellation must remove pending notes and release notes already sent.
Track ownership by destination, channel, pitch, and task. A stale note-off must
not cut off a newer retrigger. Define overlapping same-pitch notes explicitly;
MIDI 1 does not provide independent note identities.

Panic, disconnect, queue failure, and shutdown must release active notes. A
failure after note-on cannot simply discard its note-off. Reserve cleanup
capacity or provide a reliable emergency release path.

Kernel-queued events need removable ownership tags or a small dispatch horizon
with equivalent cancellation guarantees. Do not use a whole-port panic for
normal cancellation when other tasks share the channel.

### 5 Provide sufficient preparation time

The existing lookahead is `BUF_SIZE * SCHEDULER_LOOKAHEAD_SUBBLOCKS`. Compare it
with the required external-path advance and scheduling margin.

All parts in a synchronized group need a common future start with enough lead
time. Apply this to immediate starts, quantized starts, and nested patterns.
Shifting only the MIDI pattern would break the requested contract.

Reject an unsupported compensation budget or increase preparation lead through
an explicit setup policy. Do not silently saturate a past MIDI deadline to now
and claim alignment. Report genuinely late events and define whether they are
sent immediately or dropped; either outcome is a timing failure.

Preparation lookahead is not an audio delay line. Advancing computation alone
does not repair an incorrectly mapped output clock or missing capture latency.

### 6 Validate capture and audio_jit return

Reuse `hw_inlet` and existing audio_jit node connections. Verify mono and stereo
lane handling, configuration order, and sustained processing across blocks.

Before relying on capture timing, test the touched capture path for sample
format conversion, channel bounds, partial reads, silence on underrun, and ring
occupancy. The current reader copies native input samples while playback reads
them as floats; supported formats need correct conversion or explicit rejection.

Keep capture buffering stable and measurable. Never replay stale samples on an
underrun. Use the same interface for input and output; multiple-device clock
reconciliation and resampling are outside this implementation.

Start with an effect having no added algorithmic latency. A return chain with
lookahead or other declared latency requires an additional configured allowance;
automatic graph-wide plugin delay compensation is a later feature.

### 7 Calibrate and demonstrate the complete path

Create one runnable Digitone example using the requested pattern body, the
configured MIDI destination, a hardware return, and a simple audio_jit effect.

Record an internal reference impulse and repeated sharp Digitone transients.
Measure the median offset, adjust compensation, and repeat. Report residual
median, P95/P99 deviation, and worst error. Do not mistake patch attack time
for transport latency.

Calibrate external monitoring separately from audio_jit return. Disable duplicate
direct monitoring when listening through audio_jit. Preserve the measured setup
in explicit configuration; no device-profile database is needed initially.

Recalibrate after changing MIDI transport, sample rate, buffers, interface
settings, monitoring route, or return-chain latency. A fixed interface permits
reuse; it does not justify a universal hard-coded offset.

## Verification and completion criteria

| Check | Required evidence |
| --- | --- |
| Timeline arithmetic | Fake-clock tests pass at multiple sample rates and callback sizes, without block-boundary quantization. |
| Routing | Events reach only the selected destination; unavailable and ambiguous devices produce errors. |
| ALSA scheduling | A virtual receiver observes events delivered at scheduled queue times rather than submission time. |
| Note lifecycle | Cancellation before and after transmission, natural completion, retriggers, overlapping tasks, queue failure, and shutdown leave no stuck notes. |
| Capture | Known sample buffers preserve channel order and format; partial reads and underruns produce defined silence. |
| Integration | The example compiles with the current compiler, runs through `play_pattern`, and processes the hardware return through audio_jit. |
| Hardware alignment | Repeated recordings demonstrate the calibrated offset without callback-sized steps or accumulating drift; publish measured jitter under normal load and CPU load. |
| Restart | Xrun, reconnect, and device restart invalidate calibration timing state and recover without stale queued notes. |

Internal activation can be sample accurate. External onset remains limited by
driver reporting, MIDI delivery, synth response, and measurement uncertainty.
The hardware tolerance is the one-sample requirement above. Measurements decide
whether the path passes; they must not be used to relax the requirement.

## Boundaries and remaining setup

Initial scope is one configured instrument port, the fixed duplex interface,
scheduled notes, manual measured compensation, and an optional audio_jit return.
MIDI clock synchronization is not required when ylc sends individual notes.

Defer MPE, fractional pitch, production Overbridge integration, automatic instrument
discovery, multiple audio interfaces, general graph-wide delay compensation,
and faster-than-real-time rendering of external hardware. macOS scheduling
requires a separate CoreMIDI implementation of the same timing contract.
Overbridge feasibility research remains part of the sample-accuracy gate.

Before hardware verification, identify USB MIDI versus DIN, the interface name,
input channels, and the Digitone receive channel. These determine routing and
calibration, not the pattern's timing semantics.
