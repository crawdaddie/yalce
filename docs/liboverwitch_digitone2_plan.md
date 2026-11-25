# Digitone II integration through liboverwitch

Status: proposed. Target: Linux, Digitone II, audio_jit, and one fixed playback
interface. Companion to the [external MIDI plan](external_midi_implementation_plan.md).

Use the existing `liboverwitch` C library as the USB audio driver. Expose the
Digitone through ordinary engine audio nodes and a small ylc instrument API.
Keep USB mechanics below the engine boundary. Sample-accurate external note
onset remains a feasibility requirement, not an established library capability.

## What already exists upstream

The inspected upstream source builds `liboverwitch.la` and installs
`overwitch.h`. The library contains the USB engine, device discovery, clock
tracking, and resampling. `jclient.c` supplies JACK integration separately.
This is library integration work; extracting a library from the JACK executable
is unnecessary.

| Upstream component | Reuse |
| --- | --- |
| `overwitch.h` | Public device, engine, buffer, clock, and resampler API. |
| `engine.c` | USB setup, transfers, audio decoding, and stream lifecycle. |
| `resampler.c` and `dll.c` | Adapt the device clock to the host clock. DLL here means delay-locked loop. |
| `res/devices.json` | Device identity and channel layout, including Digitone II. |
| `jclient.c` | Reference for host integration; do not link its JACK frontend. |
| `main-record.c` | Example of consuming the USB engine without JACK or the host-clock resampler. |

Sources: [build definition](https://github.com/dagargo/overwitch/blob/master/src/Makefile.am),
[public API](https://github.com/dagargo/overwitch/blob/master/src/overwitch.h),
[JACK adapter](https://github.com/dagargo/overwitch/blob/master/src/jclient.c),
and [native recorder](https://github.com/dagargo/overwitch/blob/master/src/main-record.c).
Pin the upstream revision when implementation starts; these links track master.

The core links libusb, JSON-GLib, libsamplerate, and threading support. It does
not require the JACK frontend, GTK, or the D-Bus service. Upstream configuration
may still check frontend dependencies; verify the library-only build rather
than assuming the whole package configures without JACK installed.

The public library header and resampler carry GPL-3.0-or-later notices. Preserve
notices and resolve distribution compatibility before distributing linked builds.

## What first class means in ylc

A Digitone instance owns one connected instrument, its audio stream, selected
return channels, note destination, and timing configuration. Its return behaves
like another engine signal: it can feed audio_jit filters, delays, reverbs,
mixing, and recording.

It differs from an internal synth in two ways: the hardware produces audio
continuously, and MIDI triggers voices outside ylc. A note call must not create
a USB stream or a capture node. One stream serves all active notes and tracks.

Proposed API sketch, not existing executable code:

```ylc
# Setup: open once, handle failure, configure routing and timing, await readiness.
let dn = ...;

# Build and play the return once, outside the note coroutine.
let effect = @Audio fn input -> input |> Filter.lpf1p 5000.;;
Digitone.audio dn 0 |> effect |> play_node;

# Inside the coroutine passed to play_pattern:
Digitone.note dn 0 60.;
kick () |> play_node;
```

`Digitone.audio dn 0` selects a configured return signal, initially the stereo
main output. `Digitone.note dn 0 60.` targets zero-based MIDI channel 0 at the
current logical tick. Return selection and MIDI channel selection are separate.
Individual hardware tracks can later expose separate stereo nodes using the
same stream and channel descriptors.

The originally requested `send_midi_note 0 60.` can refer to the configured
default instrument. It must share the same scheduling implementation.

Minimum binding behavior:

| Operation | Contract |
| --- | --- |
| Open | Return an instrument handle or explicit error. Reject ambiguous device matches. |
| Configure | Set MIDI endpoint, default velocity/gate, return selection, and measured timing before playback. |
| Readiness | Distinguish opening, clock tuning, ready, recovering, disconnected, and failed states. |
| Audio | Return a persistent engine node for a configured signal. |
| Note | Validate pitch/channel, schedule note-on and note-off at logical times, return submission status. |
| Status | Report stream state, timing validity, discontinuities, and latency. |
| Close | Stop submissions and USB work safely while existing nodes remain memory-safe. |

Use an opaque handle. No public access to Overwitch structures is needed in
ylc. Reject unsupported fractional pitches; do not silently add pitch bend.
Apply the note defaults, cancellation, and ownership rules from the companion
plan. These operations belong on the control/scheduler side, not inside an
`@Audio` sample function.

## Layer boundaries

```text
ylc Digitone bindings                   audio_jit processing
         |                                     ^
         v                                     |
engine external-instrument API --------> engine source nodes
         |
         v
Overwitch driver adapter
         |
         v
liboverwitch
         |
         v
libusb and Digitone II
```

Proposed placement:

- `engine/elektron.c` and `.h`: instrument lifecycle, timeline integration,
  persistent source nodes, and a private driver boundary.
- `libs/overwitch/`: build integration and the Overwitch driver adapter.
- `engine/bindings/Digitone.ylc`: user-facing bindings.
- `test/`: deterministic adapter tests, lifecycle tests, and a runnable example.

The driver owns liboverwitch calls. The engine owns node lifetime and scheduled
events. audio_jit consumes normal node buffers through existing connections.
Add no compiler special case unless a concrete integration test demonstrates
that ordinary node inputs cannot represent the return.

## Native library integration

### Open and start

1. Discover devices with `ow_get_device_list` and select the intended Digitone.
   Resolve MIDI separately; matching the audio device does not identify the
   correct ALSA MIDI endpoint automatically.
2. Read `ow_device_desc` for names, channel counts, and formats. Deploy the
   required `devices.json` data at the library's expected location.
3. Allocate the driver, bounded audio rings, conversion buffers, and node storage
   before entering real-time processing.
4. Create the resampler with `ow_resampler_init_from_device`.
5. Supply `ow_context` buffer callbacks and a consistent microsecond clock.
   Start with `ow_resampler_start`, passing host sample rate and block size.
6. Wait for steady operation on the control thread before scheduling notes.

The library retains the context pointer. Its callbacks, buffers, and clock
provider must outlive the USB worker. Startup and shutdown can wait; audio
processing must not.

### Replace JACK host services

The current JACK client supplies rings, time, cycle timing, real-time priority,
sample rate, buffer size, latency propagation, and xrun notifications. Supply
equivalents through ylc's audio layer.

| JACK responsibility | ylc replacement |
| --- | --- |
| Ring read/write callbacks | Bounded single-producer/single-consumer buffers with byte counts matching the public API. |
| `jack_get_time` | Consistent monotonic clock, converted to the library's microsecond units. |
| `jack_get_cycle_times` | Timestamp for the relevant host processing cycle from the audio clock mapping. |
| Process callback | Once-per-block preparation before graph evaluation. |
| Sample rate and block changes | Control-side reconfiguration and timing invalidation. |
| Latency callbacks | Publish instrument path latency to the engine's compensation logic. |
| Xrun callbacks | Recovery state, silence, reset/retune, and invalidation of pending timing. |

SoundIo can provide variable callback sizes containing several ylc subblocks.
Use a fixed adapter block or a bounded block assembler, with a defined timestamp
for each block. Do not pass an arbitrary callback-entry wall time for every
subblock; that would distort the clock estimator.

Changing the resampler buffer size can allocate and reset state. Keep that out
of the audio callback. Audit the reused callback path for spinlocks and logging;
JACK usage does not imply that every library operation is lock-free.

### Prepare return buffers

For each adapter block:

1. Update host/device clock ratios with `ow_resampler_compute_ratios`.
2. Read device audio with `ow_resampler_read_audio`.
3. Obtain its float buffer with `ow_resampler_get_o2h_audio_buffer`.
4. Copy selected lanes into preallocated engine buffers, converting to double.
5. Evaluate the graph using those buffers exactly once for that frame range.

Several nodes may select channels from one stream. They must not each consume
the ring or advance the resampler. Preserve channel alignment across selections.
Use descriptor names and validated maps rather than assuming MIDI channel N
corresponds to USB channel N.

Start with device-to-host capture. The API also supports host-to-device audio
through its write buffer and `ow_resampler_write_audio`; audio sends into the
Digitone's processing are a separate feature, not a prerequisite for capture.

### Lifetime and recovery

Nodes retain the instrument's buffer owner. Closing a handle transitions nodes
to silence; free shared memory only after the audio graph stops referencing it.
Do not depend on undocumented automatic finalization of ylc pointers.

On disconnection or an invalid timing state, stop accepting timed notes, release
active notes when possible, and output silence. Report discontinuities instead
of repeating stale audio. Restart through control-side teardown and setup;
require clock stabilization before resuming a synchronized group.

## Is this a better device connection

For audio capture, yes: the existing library supplies direct digital multitrack
transport, device layouts, and clock adaptation. It avoids the external JACK
client and patch routing while preserving the existing playback interface.
It also avoids the analogue synth-output/interface-input conversion path.

Embedding is not itself proof of lower latency or better jitter. It replaces
host integration responsibilities that JACK currently supplies. Compare the
embedded adapter with upstream Overwitch before attributing improvements to it.

For MIDI control, the inspected public API has no note, CC, transport, or
device-sample-timestamped event interface. USB control transfers in the engine
must not be interpreted as a general musical-event protocol; inspected control
operations include device setup and naming.

Initially, scheduled MIDI remains a separate backend. Device-sample-addressed
note delivery would require additional protocol evidence and implementation,
not just a new ylc wrapper around liboverwitch.

## Sample accuracy boundaries

| Property | What can be established |
| --- | --- |
| Device-stream sample ordering | Capture successive decoded audio frames and detect/report stream discontinuities. |
| Internal audio activation | Verify activation at the exact logical sample using deterministic tests. |
| Device audio aligned to the playback clock | Reuse clock adaptation, account for buffering and resampler delay, then measure the result. |
| A MIDI note sounding at the requested sample | Not established by liboverwitch's audio API. Requires a verified event transport and device response. |

A USB packet arriving late need not change the sample order within the audio
stream. Its arrival timestamp still does not identify the exact time the synth
acted on a MIDI note. Missing frames must invalidate continuity, not disappear
inside compensation.

The Digitone and playback interface remain independently clocked even when the
same playback interface is always used. The adaptive resampler follows their
relative rate. Keeping it at nominal ratio 1 merely because both report 48 kHz
would allow drift. Resampled host frames are not the untouched native device
samples, and resampler delay belongs in the timing budget.

The public audio-buffer callbacks do not carry a per-buffer native sample
timestamp. If verification requires native frame counters and discontinuity
metadata, identify what the engine actually validates and expose a small
upstream metadata API. Do not read private engine fields from ylc or assume
the protocol's counter fields already constitute a supported timing contract.

To reach the required live result, establish whether the device accepts musical
events tied to its audio sample timeline. Investigate documented behavior,
upstream work, and controlled protocol experiments before modifying unknown
packet regions. Audio sample counters alone do not prove such support.

If ordinary MIDI plus this capture path fails the one-sample criterion,
liboverwitch integration alone does not meet the requirement. Preserve that
result explicitly. Do not conceal it with per-note transient alignment or
replace live synthesis with recorded sample playback without user acceptance.

## Implementation and evidence

| Stage | Deliverable and acceptance |
| --- | --- |
| 1 Library consumption | Pin and build liboverwitch; compile a small consumer against only its installed public header. Confirm symbols, dependencies, and device-data installation. |
| 2 Capture feasibility | Open the Digitone, capture selected tracks, report continuity and timing state, and compare against upstream recording. No compiler changes. |
| 3 Clock adapter | Fake-clock tests for rate mismatch, block assembly, tuning, underruns, and xruns; establish buffer timestamps and latency accounting. |
| 4 Engine integration | Persistent source nodes with mono/stereo tests, shared-stream reads, safe close, and disconnect silence. |
| 5 audio_jit example | One long-lived return through a simple effect, mixed with an internal reference using the existing interface. |
| 6 MIDI timing gate | Repeated note-triggered transients meet the companion plan's one-sample maximum-error criterion after one fixed calibration. If they fail, investigate event transport before building the complete instrument API. |
| 7 Instrument bindings | Open/configure/status/audio/note/close bindings, task cancellation, note-off handling, and a runnable coroutine example. |

For existing bugs encountered during integration, add a failing regression test
before fixing shared code. Keep dependency and driver changes isolated from
unrelated compiler, audio synthesis, and tracker work.

Hardware measurements must distinguish raw device capture from resampled host
audio and the final monitored output. Record individual errors, maximum error,
drift, and discontinuities across callback phases, sustained notes, chords,
traffic, load, and restart. A stable median alone is insufficient.

The first useful milestone is direct digital capture as an ordinary audio_jit
input. The decisive milestone is demonstrated sample-accurate external note
onset; shipping the capture adapter does not imply that milestone passed.
