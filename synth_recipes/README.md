# More Synth Secrets recipes

Additional `@Audio` recipes using the existing YLC/audio_jit primitives.
The first nine voices remain in [synth-secrets](../synth-secrets/README.md).
Percussion follows [drums.ylc](../drums.ylc); string layering follows
[strings.ylc](../strings.ylc). These files define voices without starting playback.

| File | Voices | Arguments |
| --- | --- | --- |
| [guitars.ylc](guitars.ylc) | `acoustic_guitar`, `electric_guitar` | frequency Hz, gate seconds |
| [advanced.ylc](advanced.ylc) | `additive_pluck`, `vowel_ee`, `sample_hold`, `body_pluck`, `spit_brass`, `pwm_pad` | frequency Hz, gate seconds |
| [percussion.ylc](percussion.ylc) | `timpani`, `bell` | frequency Hz, total seconds |
| [percussion.ylc](percussion.ylc) | `cymbal`, `hihat`, `cowbell`, `clave` | total seconds |
| [percussion.ylc](percussion.ylc) | `acoustic_piano` | frequency Hz, gate seconds |
| [organ.ylc](organ.ylc) | `drawbar_organ` | frequency Hz, gate seconds |
| [winds.ylc](winds.ylc) | `pan_pipe`, `flute`, `recorder`, `clarinet`, `violin` | frequency Hz, gate seconds |

The organ uses registration `888400000`, third-harmonic percussion, key click
and saturation. Drawbar steps use the approximate 3 dB law described in
[this organ synthesis paper](https://www.mdpi.com/2076-3417/6/7/185).
`acoustic_piano` is a subtractive, hard-sync piano patch, not a sampled piano.

## Play

From the repository root:

```sh
./build/ylc --base . -i synth-recipes/percussion.ylc
```

Then enter:

```ocaml
demo () |> play_pattern 0.;
```

Every recipe file defines `demo`: a finite coroutine that announces and plays
each sound once, leaving at least one second of silence after its tail.
Load one file at a time. Effects include their own test signals;
loading a file does not start its demo.

Or play individual sounds:

```ocaml
timpani 110. 1.5 |> play_node;
bell 220. 3. |> play_node;
cymbal 1.5 |> play_node;
acoustic_piano 261.63 0.8 |> play_node;
```

Use finite positive frequencies and durations. Start at 110–880 Hz; lower
pitches usually suit the vowel voice better. Gate durations add release tails;
percussion durations scale the full envelope. Each voice marks itself finished.

## Effects

| File | Effects | Arguments |
| --- | --- | --- |
| [advanced.ylc](advanced.ylc) | `vocoder` | carrier signal, modulator signal |
| [effects.ylc](effects.ylc) | `chorus`, `flanger`, `rotary` | modulation rate Hz, input signal |
| [effects.ylc](effects.ylc) | `echo`, `short_room` | input signal |

Effects output mono audio. `echo` uses a fixed quarter-second repeat;
`short_room` uses parallel combs and an allpass diffuser.

`vocoder carrier modulator` in [advanced.ylc](advanced.ylc) transfers eight
analysis-band envelopes onto matching carrier bands. Arguments are audio signals,
not frequencies. Use it inside another `@Audio` function:

```ocaml
let robot = @Audio fn freq duration ->
  let gate = trig 0 |> rect duration;
  vocoder (saw_osc freq) (saw_osc (freq * 0.5))
  |> (*) (asr 0.01 1. 0.1 gate |> kill_on_end)
;;

robot 220. 1. |> play_node;
```

Effects do not terminate their host voice. Apply a final envelope when building
a one-shot voice, allowing delay/reverb tails before `kill_on_end`.

## Scope and checks

Article mappings remain in [foundations](../synth-secrets/foundations.md),
[acoustic instruments](../synth-secrets/acoustic.md) and
[keys/effects](../synth-secrets/keys-effects.md). Numeric settings are demonstration
choices unless attributed. Models omit coupled acoustic bodies, shared organ
tonewheels, keyboard articulation and hi-hat choking. Modulated/pulse oscillators
are not oversampled; high notes can alias.

```sh
python3 test/test_synth_secrets.py
```

The shared headless test covers both directories at 44.1/48 kHz: finite bounded
output, nonzero energy, release completion and silence, plus silent-input effect
checks. It also advances every demo to completion and renders its voices offline.
No listening or audio-device playback has been checked.
