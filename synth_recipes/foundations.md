# Foundations: parts 1–23

Reviewed Gordon Reid's article text in the [repository](https://github.com/micjamking/synth-secrets).
Linked diagrams were not independently inspected. These are implementation notes,
not transcriptions or claims of circuit emulation.

| Part | Implementation detail | ylc mapping / status |
| --- | --- | --- |
| [1](https://github.com/micjamking/synth-secrets/blob/master/part-1.md) | Saw harmonics decrease as 1/n; filtering changes their balance. | `saw_osc` → `Filter.lpf`; `subtractive_pluck`. |
| [2](https://github.com/micjamking/synth-secrets/blob/master/part-2.md) | Membrane modes are inharmonic and decay independently; noise and ring modulation approximate dense spectra. | `white`, sine banks, multiplication; `ring_chime` illustrates the metallic branch only. |
| [3](https://github.com/micjamking/synth-secrets/blob/master/part-3.md) | Shape amplitude and brightness independently; modulation signals are ordinary signals. | `adsr`, `decay`, `scale`, multiplication; `subtractive_pluck`. |
| [4](https://github.com/micjamking/synth-secrets/blob/master/part-4.md) | Delayed copies interfere by frequency; filters also shift phase. | `delay` plus dry signal for feedforward combs; existing filters preserve their own phase response. No separate recipe. |
| [5](https://github.com/micjamking/synth-secrets/blob/master/part-5.md) | Filter order, knee and topology matter; cascading stages is not a hardware model. | `Filter.lpf1p`, `Filter.lpf`, `Filter.ladder`; pluck uses one biquad, not a four-pole emulation. |
| [6](https://github.com/micjamking/synth-secrets/blob/master/part-6.md) | Modulate cutoff and Q; keyboard tracking changes how resonance follows notes. | Pluck cutoff tracks frequency; moderate Q. Self-oscillation is not attempted with the biquad. |
| [7](https://github.com/micjamking/synth-secrets/blob/master/part-7.md) | A trigger starts a contour; a gate determines note release. Retrigger policy affects articulation. | `trig 0` → `rect duration` → `adsr`; each invocation creates one independent note. |
| [8](https://github.com/micjamking/synth-secrets/blob/master/part-8.md) | Multi-stage contours can produce a spit, dip and swell that one ADSR cannot. | `spit_brass` uses `array_env` for the spit/dip/swell, then holds until release. |
| [9](https://github.com/micjamking/synth-secrets/blob/master/part-9.md) | VCAs multiply audio or control signals; offsets and excess gain change results. | Multiplication scales envelopes and final output in all three voices. |
| [10](https://github.com/micjamking/synth-secrets/blob/master/part-10.md) | LFO destination distinguishes vibrato, tremolo and filter motion; pulse width changes harmonics. | `pwm_pad` subtracts a variably delayed saw; `sample_hold` modulates the filter. |
| [11](https://github.com/micjamking/synth-secrets/blob/master/part-11.md) | AM retains a carrier through a DC offset; zero-centred ring modulation removes it. Ratios control sideband relationships. | `ring_chime` multiplies two bipolar sines at an inharmonic ratio. |
| [12](https://github.com/micjamking/synth-secrets/blob/master/part-12.md) | FM index is frequency deviation divided by modulation frequency; index controls spectral breadth. | `fm_tone` scales deviation by modulation frequency and applies a separate decay. |
| [13](https://github.com/micjamking/synth-secrets/blob/master/part-13.md) | Integer carrier/modulator ratios produce harmonic spectra; operator envelopes change timbre. | `fm_tone` uses 1:2. `fm_op` offers phase modulation for other recipes; feedback algorithms deferred. |
| [14](https://github.com/micjamking/synth-secrets/blob/master/part-14.md) | Additive synthesis gives each partial its own contour; higher partials can decay sooner. | `additive_pluck` sums eight harmonics with 1/n levels and decay times. |
| [15](https://github.com/micjamking/synth-secrets/blob/master/part-15.md) | Vocoders apply analysis-band envelopes to corresponding carrier bands; consonants need noise. | `vocoder` implements eight matching bandpasses, rectification/`lag`, carrier multiplication and high-frequency noise. Pitch following remains deferred. |
| [16](https://github.com/micjamking/synth-secrets/blob/master/part-16.md) | Sample-and-hold captures on clock edges; pitch quantization precedes portamento. | `sample_hold` clocks `rand_trig` at 8 Hz and slews filter cutoff. Scale quantization remains deferred. |
| [17](https://github.com/micjamking/synth-secrets/blob/master/part-17.md) | Sampling rate and amplitude quantization are separate; filtering after aliasing cannot undo it. | `tabread`/`bufplay`; keep synthesized sidebands below Nyquist. No converter recipe. |
| [18](https://github.com/micjamking/synth-secrets/blob/master/part-18.md) | Note priority and envelope retrigger policy are separate choices. | Belongs in note/event handling, not one-shot DSP; deferred. |
| [19](https://github.com/micjamking/synth-secrets/blob/master/part-19.md) | Two oscillators sharing articulation are not two independent voices; separate paths also permit layered attack/body sounds. | Mix independently enveloped branches or instantiate separate nodes; keyboard duophony deferred. |
| [20](https://github.com/micjamking/synth-secrets/blob/master/part-20.md) | True polyphony needs independent amplitude and timbre contours per note. | Instantiate a recipe per note using existing `play_node`; no shared envelope state. |
| [21](https://github.com/micjamking/synth-secrets/blob/master/part-21.md) | Voice allocation and stealing are control-layer concerns; small voice differences affect repeated notes. | Existing scheduler/node lifecycle; no allocator or vintage scanning simulation added. |
| [22](https://github.com/micjamking/synth-secrets/blob/master/part-22.md) | Short parallel feedback delays approximate resonant bodies inside a patch. | `body_pluck` places three fixed, parallel `comb` resonators inside a plucked saw voice. The article's 500 Hz example needs 2 ms, not its stated 2 microseconds. |
| [23](https://github.com/micjamking/synth-secrets/blob/master/part-23.md) | Formant centres stay fixed as source pitch changes; each band needs gain and width. | `vowel_ee` uses three fixed `Filter.bpf` branches and the article's frequencies/gains/Q values; vowel morphing remains deferred. Use the primitive's Q convention rather than copying the article's ambiguous bandwidth wording. |

`foundations.ylc` exports three `@Audio fn freq duration` voices. Frequency is Hz;
duration is gate time in seconds. Use positive durations and 40–2000 Hz.
Release adds at most 0.12 seconds. `ring_chime` decays even while its gate remains open.

The patch values are chosen examples, not article presets:

- `subtractive_pluck`: saw → two-pole low-pass → amplitude envelope. Cutoff falls
  from eight times the pitch to 1.5 times it, independently of loudness.
- `ring_chime`: two sines with ratio √2 → multiplication → decaying envelope.
  This illustrates inharmonic sidebands; it does not model a bell's modal spectrum.
- `fm_tone`: sine modulator at twice the carrier frequency → carrier frequency
  input. Peak index 0.4 decays separately from amplitude. This is linear FM in Hz,
  not the phase-in-cycles input of `fm_op`.

No hardware filter matching, voice-priority emulation, spectral analysis/resynthesis,
or anti-aliasing beyond the existing primitives is included.

[advanced.ylc](../synth-recipes/advanced.ylc) adds six `fn freq duration` voices
and the continuous `vocoder carrier modulator` effect. Use 40–2000 Hz and positive
gate durations; `vowel_ee` is most useful at 80–400 Hz. Release is at most 0.2 s.

`body_pluck` models a resonant cavity, not a calibrated guitar. `pwm_pad` uses
a delayed saw difference, removes DC, and inherits the existing oscillators'
aliasing limits. `vocoder` expects normalized audio inputs; its eight broad bands
produce rough articulation, with 8 ms voiced and 3 ms sibilant smoothing. It does
not allocate or terminate voices; the caller supplies input and output lifetimes.
The fixed `ee` parameters come from part 23; all other new values are chosen
examples rather than exact instrument presets.
