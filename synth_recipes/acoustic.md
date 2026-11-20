# Acoustic instruments: parts 24–44

Review of Gordon Reid's articles in the supplied mirror. These are implementation
notes, not transcribed patches. Kick and snare adapt `../drums.ylc`
(`Kick2` and `Snare`), preserving the simple recipe signatures. Numeric settings in `acoustic.ylc` are starting
points chosen here, except the 80 Hz brass rasp and 180/330 Hz snare modes discussed
in the articles. No circuit emulation or acoustic realism is claimed.

Additional voices live in [percussion.ylc](../synth-recipes/percussion.ylc).
Cymbal, bell, hi-hat and cowbell adapt the existing `drums.ylc` recipes.
Timpani uses the article's mode ratios, relative gains and decay ratios; the
remaining settings are practical starting points, not exact hardware presets.

| Part | Implementation details and available primitives | Status |
| --- | --- | --- |
| [24](https://github.com/micjamking/synth-secrets/blob/master/part-24.md) Wind | Bore geometry selects odd-only or complete harmonics. `sq_osc` suits a basic clarinet; `saw_osc` supplies brass harmonics. Filter brightness must vary with level and pitch. | Basis for `brass`; woodwind deferred. |
| [25](https://github.com/micjamking/synth-secrets/blob/master/part-25.md) Brass | Separate amplitude/filter attacks reproduce slower development of upper harmonics. Brief audio-rate filter modulation supplies rasp; delayed vibrato and filtered breath add expression. | `brass` implements separate ADSRs and transient 80 Hz filter modulation; breath and vibrato deferred. |
| [26](https://github.com/micjamking/synth-secrets/blob/master/part-26.md) Minimoog brass | Single saw, separate contours, tracking resonant low-pass and filter modulation. Unshaped noise weakens the imitation. | `brass` cascades two biquads for a four-pole slope, not a Moog circuit response. |
| [27](https://github.com/micjamking/synth-secrets/blob/master/part-27.md) SH101/Axxe brass | A shared envelope constrains filter and amplitude independently of oscillator choice. SH101 sub-oscillator adds body; available LFO rate limits filter rasp. | Existing `adsr`, `sq_osc`, `Filter.lpf`; hardware-specific variants deferred. |
| [28](https://github.com/micjamking/synth-secrets/blob/master/part-28.md) Plucked strings | Plucking position removes partials; direction changes the two-stage decay. String, plates and air exchange energy; stiffness introduces inharmonicity. | [`acoustic_guitar`](../synth-recipes/guitars.ylc) uses weighted decaying partials and fixed body bands; no body/string coupling. |
| [29](https://github.com/micjamking/synth-secrets/blob/master/part-29.md) Acoustic guitar | Six independent voices need body resonances, feedback, decaying brightness and performance articulation. `Filter.bpf`, `feedback_net`, `lag` supply components, not a finished model. | [`acoustic_guitar`](../synth-recipes/guitars.ylc) adds body bands and key tracking; six-string assignment and coupling remain unmodelled. |
| [30](https://github.com/micjamking/synth-secrets/blob/master/part-30.md) Electric guitar | Pulse/ramp mixtures and faster filter than amplitude decay give useful synthetic plucks. Pickup position creates pitch-dependent cancellations. | [`electric_guitar`](../synth-recipes/guitars.ylc) uses an enveloped quarter-duty pulse and low-pass decay; pulse notches only approximate pickup coloration. |
| [31](https://github.com/micjamking/synth-secrets/blob/master/part-31.md) Drum modes | Membrane modes are inharmonic; the kettle shifts important radial modes toward harmonic relationships. Each mode needs its own amplitude and decay. | `timpani` supplies four modes with distinct gains and decays. |
| [32](https://github.com/micjamking/synth-secrets/blob/master/part-32.md) Timpani | Four principal sine modes use ratios 1, 1.50, 1.99, 2.44 and distinct decays. Short filtered ring modulation or noise supplies the initial inharmonic cluster. | `timpani` adds a brief high-passed ring-modulated strike to the four pitched modes. |
| [33](https://github.com/micjamking/synth-secrets/blob/master/part-33.md) Bass-drum theory | Low modes resemble a frequency-shifted harmonic series; mid modes, falling pitch and beater click need separate paths. `freq_shift` can supply the fixed-Hz offset. | `kick` adapts `Kick2`: filtered triangle body, slight pitch fall, inharmonic phase modulation and a transient filter contour. |
| [34](https://github.com/micjamking/synth-secrets/blob/master/part-34.md) Practical kick | Swept self-oscillating filters give simple kicks. TR909 uses a sine-like oscillator plus noise/click; TR808 uses a struck bridged-T resonator. | `kick` uses oscillator-plus-modulation modes, not either circuit. |
| [35](https://github.com/micjamking/synth-secrets/blob/master/part-35.md) Snare theory | Coupled heads and wires yield tonal modes plus broad noise. Harder strikes increase noise and high-frequency content; spectral holes need notch filters. | `snare` combines two head modes, seven upper modes and notched wire noise; velocity control deferred. |
| [36](https://github.com/micjamking/synth-secrets/blob/master/part-36.md) Practical snare | Two independently decaying modes plus high-passed white noise capture the simpler architecture. Separate noise bands add TR909-style detail. | `snare` adapts the oscillator-bank and point-envelope approach in `drums.ylc`; no circuit matching. |
| [37](https://github.com/micjamking/synth-secrets/blob/master/part-37.md) Metallic percussion | Cymbal modes are discrete at light strikes, then increasingly nonlinear. Energy moves from initial mids into highs before the final midrange tail. | `cymbal` supplies three modulated oscillator pairs and separate ping/tail filters; no nonlinear plate model. |
| [38](https://github.com/micjamking/synth-secrets/blob/master/part-38.md) Cymbal synthesis | Complex modulation feeds a short band-passed ping and a longer high-passed tail; evolving filters describe energy migration. | `cymbal` implements the two filter paths and evolving brightness using point envelopes. |
| [39](https://github.com/micjamking/synth-secrets/blob/master/part-39.md) Practical cymbals | TR808 combines six square oscillators with filtering; cross-modulation enriches metallic spectra beyond filtered noise. TR909 cymbals use samples. | `cymbal` uses three pairs of square-wave FM sources from `drums.ylc`'s `Cymbal2`; no samples. |
| [40](https://github.com/micjamking/synth-secrets/blob/master/part-40.md) Bells/hi-hats | Stretched sine partials, short FM impact and long beating hum need separate envelopes. Closed hats must choke open hats. | `bell` combines stretched modes, clapper and beating hum; `hihat` filters six square sources. Independent hats do not choke one another. |
| [41](https://github.com/micjamking/synth-secrets/blob/master/part-41.md) Cowbells/claves | Cowbell mixes two square sources through filtering and a short contour. Claves reduce to a struck, rapidly decaying resonator. | `cowbell` adapts the article's triangle-pair approximation via `drums.ylc`; `clave` uses a rapidly damped filtered triangle. |
| [42](https://github.com/micjamking/synth-secrets/blob/master/part-42.md) Piano | Hammer impact, evolving stretched partials, beating strings and body resonances need separate treatment. Velocity and pitch affect both decay and brightness. | `acoustic_piano` uses a synthetic impact and key-tracked decay; no coupled strings or soundboard. |
| [43](https://github.com/micjamking/synth-secrets/blob/master/part-43.md) Sync piano | A master resets a swept slave; slave pitch changes timbre while master sets perceived pitch. Separate contours mix master and slave differently over time. | `acoustic_piano` mixes a square master with a swept saw slave reset by `trig`/`phasor_sinc`. |
| [44](https://github.com/micjamking/synth-secrets/blob/master/part-44.md) Piano dynamics | JX10 filter and amplifier use different velocity curves. Key tracking and contours shape brightness; LFO modulation is inactive in this tone. | `acoustic_piano` shapes brightness and amplitude separately; playing velocity is fixed, so JX10 dynamics curves are not reproduced. |

`brass freq duration` and `pluck freq duration` take Hz and gate seconds.
Start with 110–880 Hz and 0.1–2 seconds; these patches do not validate inputs.
`kick ()` and `snare ()` use fixed tuning and durations. Every voice creates one
trigger and frees itself after release. Gains leave headroom for individual
voices; overlapping notes still need mixer gain management.

New percussion voices end at their requested duration. `timpani freq duration`
and `bell freq duration` take Hz and seconds; `cymbal duration`, `hihat duration`,
`cowbell duration` and `clave duration` take seconds. `acoustic_piano freq duration`
uses gate seconds plus an 80 ms release. Bell works best around middle C; start
timpani around 150 Hz and claves around 40 ms. Hard-sync and metallic FM can alias;
these recipes do not add oversampling.
