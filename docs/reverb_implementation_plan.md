# Reverb implementation plan

Build a 224-inspired hall and plate alongside modern ambient FDN4 and FDN8
reverbs. Add freeze and shimmer to the modern networks after validating their
decay and stability. Keep the current `Reverb` functions available unchanged.

The first release should be mono-input, stereo-output, wet-only processors.
Create one persistent effect node per send bus, rather than a new reverb for
every note. Dry/wet mixing belongs at the routing layer.

## Algorithms and intended sound

These are proposed implementations and names, not existing APIs.

| Processor | Structure | Intended character | First release |
| --- | --- | --- | --- |
| `ambient4` | Four-line orthogonal FDN with input diffusion | Spacious, economical, audible movement | Yes |
| `ambient8` | Eight-line FDN with denser diffusion and stereo taps | Smooth, long pad and instrument tails | Yes |
| `freeze8` | Lossless endpoint of the ambient FDN8 | Sustain the current tank without new input | Yes |
| `shimmer8` | Ambient FDN8 with filtered, pitch-shifted feedback | Octave-up evolving ambience | Yes |
| `hall224` | Dedicated vintage hall topology and tuning | Darker, modulated, spacious hall | After modern tank validation |
| `plate224` | Dedicated dense plate topology and tuning | Smooth sustained plate colour | After hall voicing |

Freeze is a state of the same running ambient tank. `freeze8` can be a
convenience wrapper; it must not capture silence by constructing a new node.
Four versus eight lines alone does not establish quality: delay distribution,
diffusion, damping, modulation, and stereo observation need separate tuning.

## Existing code to reuse

[DSP_common.ylc](../libs/audio_jit/DSP_common.ylc) already contains delay
allpasses, filters, `lag`, `pitchshift`, `Reverb.fdn4`, and `Reverb.fdn8`.
[audio_jit.c](../libs/audio_jit/audio_jit.c) provides per-node audio state,
static DSP iteration, and the `feedback_net` and `delay_proc` lowering paths.

Important differences between the existing helpers and the proposed tanks:

- `fdn4`, `fdn8`, and `feedback_net` return input plus their delayed output.
  New send effects need explicit wet-only output, not another hidden dry path.
- `fdn8` applies fixed low-pass and high-pass filtering to its feedback. Those
  losses prevent an indefinite freeze even when its scalar loop gain reaches one.
- `feedback_net` instantiates its processing callback per feedback line, but
  currently exposes neither a line index to that callback nor stereo output
  taps. It is useful for prototypes, not a complete new reverb interface.
- Existing delay and pitch-shift kernels use linear interpolation. Fractional
  delay reads introduce frequency-dependent loss; pitch shifting also needs
  separate alias and gain measurements.
- `ylc_audio_dl_allpass_kernel` currently computes `out = delayed - g * input`
  but writes `input + g * delayed`. For that output convention, a standard
  allpass writes `input + g * out`. Add a failing unity-gain regression before
  correcting it; measure the effect on existing patches that use `dl_allpass`.

Reuse the existing building blocks where they preserve the required signal
flow. Add a dedicated kernel only for behaviour that the current audio
composition cannot express cleanly. Do not build a general reverb graph
framework before these concrete algorithms work.

## Modern FDN signal flow

```text
input -> input tone -> predelay -> allpass diffusion -> injection vector
                                                        |
                         +------------------------------v---------+
                         | delay lines -> decay filters -> matrix |
                         |      ^                          |      |
                         +------|--------------------------+------+
                                |                         |
                                +--------- feedback <-----+

delay-line taps -> independent L/R observation -> output tone -> wet L/R
                       |
                       +-> filtered pitch shift -> limited return to tank
```

Read every delay output before writing any delay input. This prevents update
order from changing the network or creating an accidental zero-delay loop.
Count every fixed delay and allpass in the effective round-trip duration.

Start with the Householder matrix already used in the repository:

```text
A = (2 / N) * J - I
mixed[i] = (2 / N) * sum(delayed) - delayed[i]
```

This avoids a dense matrix multiplication. Verify `AᵀA = I` in a small test.
An orthogonal matrix with ideal delays forms a lossless reference network;
loss filters then set its decay.
[Julius O. Smith describes the lossless matrix condition](https://www.dsprelated.com/freebooks/pasp/Most_General_Lossless_Feedback.html).

Choose input vectors and signed stereo observation vectors independently.
Normalize their gains, and measure left/right correlation and the mono sum.
Summing every delay with the same positive coefficient for both channels will
not create a useful stereo field. Avoid duplicating an entire mono tank through
automatic lane expansion when the intended result is one joint stereo tank.

Choose nonuniform delay lengths in seconds, convert them to samples at node
construction, and inspect the rounded lengths at every supported sample rate.
Avoid common factors and obvious echo periodicity, but do not treat prime
lengths as proof of good sound. Tune from impulse responses and listening.
Size buffers from the actual supported maximum delay, not one second per line
by default. For context, eight one-second double buffers at 48 kHz alone cost
about 3.1 MB per effect instance.

## Decay, diffusion, and modulation

For an ideal delay of `d[i]` seconds and target amplitude RT60 `T`:

```text
gain[i] = 10^(-3 * d[i] / T)
```

Give bass and midrange separate target decay times and fit bounded loop
filters around a crossover. Add high-frequency damping for the upper tail.
Measure achieved decay by frequency band; a post-output low-pass changes
colour but does not create frequency-dependent decay inside the tank.

Begin with a few short input allpasses, then add internal diffusion only if
the tail remains sparse. Validate the implemented allpass transfer function
and gain before relying on it in a feedback loop. Keep diffusion separate from
decay; increasing one must not silently increase the other.

Use small, smooth, independent modulation trajectories. Express depths in
seconds and document their equivalent sample excursions. Avoid discontinuous
random delay jumps, a single identical LFO on every line, and shared global RNG
state. For repeatable tests, use fixed phases and deterministic seeds.

First make the feedback delays fixed integers. For the freeze-capable version,
initial movement can come from output taps outside the recirculating loop.
This gives an unambiguous lossless freeze reference. Add modulated feedback
delays only after choosing interpolation with measured loop loss and a defined
transition into freeze. A fixed allpass interpolator has different behaviour
from a time-varying one; do not assume modulation preserves energy.

## Freeze behaviour

At the fully frozen endpoint:

- Input injection is zero, including queued predelay, diffuser, and shimmer
  returns entering the tank.
- The feedback matrix remains orthogonal, with unit loop gains.
- Loop damping, DC blockers, saturation, and other lossy stages are bypassed.
- Delay lengths are fixed, and reads introduce no interpolation loss in the
  baseline implementation.
- Pitch shifting is removed from feedback. Its existing buffers may keep
  processing, but their output cannot enter the frozen tank.

Use finite ramps that reach these endpoint values exactly. An asymptotic
`lag` alone can leave a small loss or injection indefinitely. Keep tone and
DC control before injection or after observation when a lossless loop is
required. Output gain remains available while the tank is frozen.

Define capture, sustain, and release transitions explicitly. Closing the
input first avoids adding energy while feedback approaches unity. Restore
normal loss before fully reopening injection on release. Keep the same tank
state; resetting or reallocating it defeats freeze.

Test stored tank energy separately from observed output RMS: beating between
modes can make output level fluctuate even when total stored energy is stable.
Floating-point drift still needs measurement over long captures. Do not hide
an unstable loop with a permanent nonlinear limiter inside the frozen tank.

## Shimmer behaviour

Start with one octave-up branch using the existing four-head `pitchshift`
kernel. Feed a bounded amount of shifted tail back into the tank, then diffuse
it on subsequent passes. This makes successive generations accumulate pitch
shifts; shifting only the final wet output creates a different effect.
[Valhalla describes pitch-shifted feedback as the basis of shimmer](https://valhalladsp.com/2010/12/02/valhallashimmer-tips-and-tricks-shimmering/).

Keep pitch interval, return amount, base RT60, and wet output level independent.
Derive the pitch ratio as `2^(semitones / 12)`. Use an octave as the initial
sound; add fifth and downward variants after the octave branch passes tests.

Filter before upward pitch shifting. As a starting bound, keep its input
bandwidth below `0.45 * fs / max(1, ratio)`, then measure actual aliases and
windowing sidebands. Filter the returned branch to prevent successive shifts
from turning into high-frequency buildup. Evaluate a longer window for smooth
tails, while measuring onset delay, graininess, and transient smearing.

A convex blend with a pitch shifter is not automatically energy-preserving:
measure its gain and reserve feedback headroom. Keep a documented maximum
return amount, test worst-case sustained input and automation, and check for
NaNs and runaway growth. Freeze defaults to preserving the current spectrum;
an intentionally evolving shimmer hold should be a separate, explicitly lossy
or gain-controlled behaviour.

Do not instantiate eight pitch shifters merely because there are eight FDN
lines. Start with a stereo return branch and distribute its output through
signed injection vectors. Instantiate no pitch-shift state in plain ambient
variants. In YLC audio expressions, a zero multiplier or conditional may still
evaluate the unused branch; verify generated state and processing cost.

## Lexicon 224 target

Use the 224's final v4.4 program family as the behavioural reference. It has
eight reverb programs plus chorus, backed by seven distinct algorithms.
Small and Large Concert Hall B share an algorithm; Vocal Plate and Percussion
Plate A also share one. Its controls include separate bass/mid decay,
crossover, treble decay, predelay, depth, diffusion, mode enhancement, and
level-dependent decay optimization.
[Universal Audio documents these program relationships and controls](https://help.uaudio.com/hc/en-us/articles/4419497193492-Lexicon-224-Digital-Reverb-Manual).

First voice a Large Concert Hall B-inspired processor, then a plate-inspired
processor. Give each its own topology, delay set, tap pattern, and parameter
mapping. Do not label the existing FDN8 plus a dark filter an exact 224 model.
A generic Dattorro-style plate is a useful separate reference, not evidence
that the 224 used that exact published diagram.

For a 224-inspired implementation, prototype:

1. Input bandwidth shaping and predelay.
2. An early/later energy distribution controlled by depth.
3. Input and tank diffusion with separate density and decay controls.
4. Program-specific modulated delay/allpass structures and signed output taps.
5. Bass/mid loop losses and treble damping.
6. Program-specific modulation behaviour, tuned against recordings.
7. Level-dependent diffusion/decay adjustment only after the static version
   matches the reference's broad envelope and colour.

These are design choices to investigate, not a recovered 224 schematic.
Record which parameters alter structure and which alter gains. Test whether
parameter changes preserve the tail or require a second tank and a crossfade.

An exact recreation is a separate milestone: specify the hardware/firmware
revision, recover and verify the algorithm graph and coefficients, reproduce
its update rates and arithmetic, and compare against that revision. Use the
[224 service documentation](https://www.benmook.com/tech/documents/Manuals/Lexicon%20224/Lexicon%20224%20Service%20Manual.pdf)
and reference captures to establish facts before choosing a vintage sample
rate, quantization law, or modulation schedule. Converter colour, gain stepping,
and internal arithmetic are different mechanisms; a final bitcrusher cannot
stand in for all three. Start clean, then add measured colour separately.

## Repository changes

| Location | Planned responsibility |
| --- | --- |
| `libs/audio_jit/DSP_common.ylc` | Reuse existing DSP primitives; preserve existing reverb APIs |
| New `libs/audio_jit/Ambient.ylc` | Modern wet FDNs, freeze control, shimmer composition |
| New `libs/audio_jit/Lex224.ylc` | Program-specific vintage hall/plate composition and tuning |
| `osc_kernels.h` and `osc_kernels.c` | Add only required stateful primitives that cannot be expressed cleanly in YLC |
| `audio_jit.c` | Register any new primitives using existing state and lane conventions |
| `libs/audio_jit/Makefile` | Track any new kernel/include dependencies |
| New offline render tests | Impulses, spectral decay, freeze transitions, stereo, shimmer, automation |
| New example patch | Persistent send bus, bounded levels, parameter changes without resetting tails |

Start with explicit functions and a small set of named presets. Avoid a public
matrix editor, arbitrary graph configuration, or a new dependency for simple
delay/filter operations. Keep all allocations at construction and preserve
state ownership across node replacement and REPL use. Normal operation must
not allocate, lock, log, or read files from the sample callback.

## Implementation order and acceptance

| Stage | Deliverable | Required evidence |
| --- | --- | --- |
| 1 | Offline render harness, allpass correction, existing FDN baselines | Unity-gain allpass response, correct channels, finite samples, repeatable state |
| 2 | Wet FDN4 and FDN8 with integer delays | Orthogonal matrix, independent instances, measured decay, no hidden dry path |
| 3 | Diffusion, band decay, stereo observation | Dense tail without obvious ringing; band RT60 and mono compatibility reports |
| 4 | Freeze transitions | Input isolation and bounded stored energy through capture, hold, and release |
| 5 | Ambient motion | Smooth modulation; no clicks, excessive pitch wobble, or changed freeze guarantees |
| 6 | Octave shimmer | Measured shifted spectrum, bounded feedback, controllable buildup, freeze isolation |
| 7 | 224-inspired Hall B | Reference captures, documented topology, level-matched listening comparisons |
| 8 | 224-inspired plate | Distinct diffusion/envelope/taps; validated parameter behaviour |
| 9 | Optional historical colour | Measured colour contribution and explicit clean comparison |

Exercise 44.1, 48, and 96 kHz. Use impulses, impulses at several levels,
band-limited noise bursts, sine sweeps, sustained tones, silence, and extreme
parameter automation. Include short RT60, very long RT60, and at least several
minutes of freeze. For RT60 analysis, use band-filtered Schroeder energy decay
and report the fitting interval and extrapolation instead of treating an
unfinished tail as a measured 60 dB decay.

Keep audio-rate finite checks and CPU/memory budgets separate from listening
judgments. Measure early echo density, late-tail spectra, stereo correlation,
mono cancellation, modulation sidebands, and transition peaks. Store small
reference renders or reproducible render commands alongside numerical reports.
No new algorithm should replace an existing patch until it passes these checks
and a level-matched listening comparison on bass, pads, percussion, and vocals.
