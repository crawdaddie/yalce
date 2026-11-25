# Band-limited oscillators for A4 voices

Build a clean oscillator first. Add thickness through controlled beating,
waveform character, and the voice's nonlinear mixer/filter. Aliasing adds
pitch-dependent interference; it is not a substitute for those behaviours.

This is an implementation plan. Proposed names and tuning values below are
starting points, not existing APIs or measurements of Elektron hardware.

## Existing implementation

- [osc_kernels.c](../libs/audio_jit/osc_kernels.c) implements `saw_osc` and
  `sq_osc` with one 2048-sample table each and linear interpolation. Neither
  kernel selects a different table according to playback frequency.
- [osc_kernels.h](../libs/audio_jit/osc_kernels.h) defines per-instance phase
  state. The Audio compiler reserves this state inside each synth node.
- [audio_jit.c](../libs/audio_jit/audio_jit.c) registers oscillator builtins,
  links precompiled kernel bitcode, and emits one kernel call per sample and
  channel lane. Independent builtin calls have independent state.
- [DSP_common.ylc](../libs/audio_jit/DSP_common.ylc) declares the public audio
  functions. [a4.ylc](../a4.ylc) currently constructs several waveforms directly
  from phase, including pulse comparisons and phase resets for sync.

A table can contain a clean cycle and still alias when pitched upward. Table
length controls lookup accuracy and representable harmonics; playback rate
determines which harmonics fit below Nyquist. These are separate constraints.
[EarLevel explains that distinction](https://www.earlevel.com/main/2012/11/18/a-wavetable-oscillator-end-notes/).

## First implementation

Add a band-limited saw builtin and derive variable-width pulses from it.
Keep the current oscillators available so existing patches retain their sound.
Compose detuning, sub-oscillators, gain, and filtering in YLC rather than hiding
an entire synth voice inside the oscillator.

```text
Immutable table bank             Per oscillator instance
  shape / harmonic limit         phase, transition state
           |                              |
           +--------> table lookup <------+
                            |
                      oscillator mixer
                            |
                   drive and voice filters
                            |
                         envelope
```

Use one waveform bank shared by instances within its JIT lifetime. Do not
allocate or generate tables from the sample callback. Keep phase and any
transition history private to the oscillator instance and channel lane.

## Build the table bank

Generate tables offline. For an analytic saw, Fourier synthesis is sufficient;
an FFT is useful when importing measured or drawn cycles. The runtime need
not acquire an FFT dependency merely to read generated tables.

For an imported cycle:

1. Establish a stable periodic cycle and remove DC.
2. Choose a common phase origin for every derived table.
3. Transform the cycle to its spectrum.
4. Remove harmonics above each level's limit, preserving conjugate symmetry.
5. Inverse-transform each level and append its first sample as a wrap guard.

Use one gain calibration across a waveform's bank. Independently peak- or
RMS-normalizing every level changes the fundamental level when levels blend.
High notes naturally contain less energy once their upper harmonics are gone.

A practical initial layout is a fixed table length with approximately octave
spaced harmonic limits. Constant lengths simplify interpolation, alignment,
and phase-coherent transitions. Smaller high-register tables are a later
memory optimization. Progressive bandwidth reduction is the basis of
[EarLevel's table-bank implementation](https://www.earlevel.com/main/2012/05/09/a-wavetable-oscillator-part-3/).

For `N` unique samples, keep positive harmonics below `N / 2`. With `N = 2048`,
an example bank is `1023, 511, 255, ..., 3, 1` harmonics. The guard sample is
additional storage; it is not part of the cycle length.

Name meaningful constants: table length, maximum harmonic count, usable
bandwidth fraction, overlap width, and any shape calibration gains.

## Select and interpolate safely

Let `fs = 1 / spf`, `f` be frequency in Hz, and `H` a table's highest harmonic.
Use a guard below Nyquist:

```text
H * abs(f) <= bandwidth_fraction * fs
```

`bandwidth_fraction = 0.45` is an initial design choice, not a universal
anti-aliasing guarantee. It reserves transition space below `0.5 * fs`.

Select the richest table satisfying this bound. Crossfade to the next poorer
table before the richer table becomes unsafe. At the richer table's upper
safe frequency its contribution must already be zero. Blending an unsafe
table after that boundary reintroduces aliasing.

Use one phase accumulator for both lookups. A table transition must not reset
phase. For each lookup:

```text
position = phase * table_length
index    = floor(position)
fraction = position - index
sample   = table[index] + fraction * (table[index + 1] - table[index])
phase    = phase + frequency * spf
phase    = phase - floor(phase)
```

Linear interpolation is a useful first implementation. Measure its image
rejection before adding cubic or higher-order lookup. A larger table improves
interpolation accuracy but cannot replace pitch-dependent band-limiting.

Define behaviour for zero, negative, nonfinite, and above-band frequencies.
Either support reverse phase motion or explicitly clamp negative frequency.
Reject nonfinite inputs before indexing. Above the usable fundamental range,
mute or apply an explicitly documented fade rather than wrapping into an
unrelated audible pitch. Make table choice depend on the actual sample rate.

## Pulse width without a width table bank

For an increasing saw `s(p) = 2 * frac(p) - 1`, a zero-mean pulse is:

```text
pulse(p, duty) = saw(p) - saw(frac(p + duty))
```

Use the same phase and harmonic level for both reads. At `duty = 0.5` this
produces a square; as duty approaches either endpoint it tends toward silence.
Its ideal levels are `-2*duty` and `2*(1-duty)`, so peak gain varies with width.
Choose and document a gain calibration instead of assuming constant peak level.
[EarLevel uses phase-offset saw subtraction for PWM](https://www.earlevel.com/main/2012/05/25/a-wavetable-oscillator-the-code/).

The current A4 pulse is bipolar with a duty-dependent DC component. Decide
deliberately whether to preserve that behaviour or use the zero-mean version.
DC changes can alter drive and filter behaviour. Keep the existing `pw` mapping
from `-1..1` to duty `0..1`, and preserve its endpoint behaviour explicitly.

Slow PWM can work well with this bank. Audio-rate PWM creates sidebands beyond
the unmodulated harmonic limit; selecting tables from the carrier alone is
insufficient. Smooth abrupt control changes and measure the modulation range.

## Thickness without unstable pitch

Keep these controls separate so their effect remains predictable:

| Component | Purpose | Initial approach |
| --- | --- | --- |
| Static detune | Repeatable beating | Two oscillators, independent phase |
| Linear detune | Similar beat rate across registers | Preserve A4's separate Hz offset |
| Cents detune | Similar interval across registers | `f * 2^(cents / 1200)` |
| Slow drift | Gentle pitch movement | Independent, bounded modulation in cents |
| Waveform character | Harmonic balance | Phase-aligned, DC-free source cycles |
| Sub-oscillator | Stable bass weight | Separate tuned oscillator with restrained gain |
| Drive | Level-dependent colour | Nonlinear mixer/filter with alias control |

Start by listening to two oscillators at roughly opposite small detunings,
for example `-3` and `+3` cents. This is a listening experiment, not a hardware
setting. At low pitches their beating is much slower than at high pitches.
A fixed Hz offset behaves differently; the existing A4 `detune_hz` parameter
should retain that meaning.

Use smooth, bounded drift rather than independent random pitch every sample.
Keep its state per oscillator. Deterministic seeds make offline renders and
regressions repeatable; do not introduce a shared random-number generator into
the new kernel. A new zeroed node otherwise resets its phases on each note:
continuing phase across notes requires explicit voice lifetime support.

For bass, preserve mono compatibility and fundamental strength. Keep the sub
centred and avoid treating a wide stereo unison as the default. Compare at
matched loudness: doubling oscillator count also increases level and can make
a comparison misleading. Reserve mixer headroom before drive and resonance.

## Morphing and analog source cycles

A measured cycle can preserve asymmetry, curvature, and harmonic roll-off.
It does not model oscillator drift, level-dependent circuitry, sync, or filter
interaction. Those need separate mechanisms or measurements.

If adding waveform morphing, phase-align the source cycles and band-limit
every morph frame consistently. Start with interpolation between aligned
cycles; rapid morphing adds modulation sidebands and needs separate alias
tests. Avoid arbitrary phase alignment changes that produce cancellation or
apparent detuning while morphing.

## Sync and oversampling

Hard sync introduces a discontinuity when the slave is reset. Band-limited
free-running tables do not band-limit that new reset. Track the master's
crossing at fractional sample time; resetting only at sample boundaries causes
additional timing error. BLEP-style correction is an established approach to
discontinuities, including hard sync.
[This DAFx paper discusses the connection](https://dafx.de/paper-archive/2025/DAFx25_paper_29.pdf).

Oversampling is another part of alias control, particularly for sync and drive:

```text
Output rate: 48 kHz                 Internal example: 192 kHz

                         +-------------------------------------+
controls ----------------> oscillators -> sync -> drive/filter |
                         +-------------------------------------+
                                            |
                                  anti-alias low-pass
                                            |
                                      decimate by 4
                                            |
                                       48 kHz output
```

Generate the oscillator directly at the internal rate. Upsampling an already
aliased output cannot undo aliasing. Use `internal_spf = spf / factor` and
advance every included stateful stage once per internal step. Interpolate
changing controls where required. A one-sample output trigger must become
one internal event, not four retriggers.

Keep feedback connections inside one consistently clocked region. Calling an
already rendered neighbouring node four times does not produce four new input
samples. For existing audio inputs, proper upsampling is required. The present
per-sample compiler has no general oversampling-region operator; adding one
would require explicit lowering and state/clock rules. A dedicated kernel can
oversample its own internals without changing the whole engine.

Use a persistent decimation filter, not just the final internal sample or an
unqualified four-sample average. Specify passband, transition band, stopband
attenuation, gain, and latency. Match the delay of any parallel dry path.
Oversampling filters add cost and latency, and finite oversampling still
allows some aliasing. [JUCE documents these implementation tradeoffs](https://docs.juce.com/develop/classjuce_1_1dsp_1_1Oversampling.html).

## Repository integration

| File | Change when implementing |
| --- | --- |
| `libs/audio_jit/osc_kernels.h` | New instance-state struct; keep table storage separate |
| `libs/audio_jit/osc_kernels.c` | Scalar lookup kernel and validation |
| `libs/audio_jit/audio_jit.c` | Builtin name, arity, state size/alignment, lane expansion |
| `libs/audio_jit/DSP_common.ylc` | Public extern declaration and YLC composition |
| `libs/audio_jit/Makefile` | Track generated table includes as bitcode prerequisites |
| `a4.ylc` | Opt-in replacement after comparisons |

Use the existing `AudioBuiltin` registration and per-node state allocation.
Do not expose table pointers through the YLC voice API. Keep existing shape
helpers separate from pitch-dependent oscillator lookups: a phase-only API
does not by itself provide the frequency information needed for table choice.

Generated immutable tables embedded in kernel bitcode follow that code's JIT
lifetime. Runtime-generated banks need an explicit owner that survives every
referencing synth and REPL chunk. Do not retain pointers to compiler stack
buffers or temporary MIR data as audio state.

## Validation before replacing A4 oscillators

1. **Numerics:** finite output, correct tuning, phase wrapping, zero frequency,
   chosen negative-frequency policy, parameter endpoints, sample-rate changes.
2. **Static spectra:** coherent FFT tones in several registers at 44.1, 48,
   and 96 kHz. Compare against additive references using the same harmonic
   limit. Separate interpolation images from intended harmonics.
3. **Transitions:** slow sweeps and abrupt pitch jumps across every table
   boundary; measure gain changes, clicks, and unsafe harmonic leakage.
4. **Modulation:** PWM, vibrato, FM/PM, sync ratios, detune, morphing, and rapid
   parameter automation. Static pitch tests cannot establish their alias floor.
5. **Voice behaviour:** independent node/lane state, repeatable initialization,
   retrigger behaviour, clipping headroom, mono sum, and low-frequency DC.
6. **Cost:** many simultaneous voices, worst-case modulation, construction
   memory, and render time. Verify no sample-path allocations, locks, file I/O,
   or table regeneration.
7. **Listening:** level-matched dry oscillator and complete A4 bass/lead renders.
   Include long notes so beating, transition noise, and drift are audible.

Keep reference renders and measured thresholds with the tests. Implement the
table bank and transitions first, then pulse/PWM, then thickness controls.
Add sync correction and oversampling after measuring those specific cases.
