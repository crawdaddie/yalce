# Synth Secrets recipes

Nine starter voices using YLC's `@Audio` and existing `audio_jit` primitives.
Based on Gordon Reid's [Synth Secrets series](https://github.com/micjamking/synth-secrets).
Percussion and stereo strings adapt [drums.ylc](../drums.ylc) and
[strings.ylc](../strings.ylc), without their playback demos.
These demonstrate synthesis techniques; they are not exact hardware presets.
Additional instruments and effects are in [synth-recipes](../synth-recipes/README.md).

| File | Voices | Arguments |
| --- | --- | --- |
| [foundations.ylc](foundations.ylc) | `subtractive_pluck`, `ring_chime`, `fm_tone` | frequency Hz, gate seconds |
| [acoustic.ylc](acoustic.ylc) | `brass`, `pluck` | frequency Hz, gate seconds |
| [acoustic.ylc](acoustic.ylc) | `kick`, `snare` | `()` |
| [keys-effects.ylc](keys-effects.ylc) | `string_pad`, `tonewheel` | frequency Hz, gate seconds |

From the repository root, load a file into the REPL:

```sh
./build/ylc --base . -i synth-secrets/acoustic.ylc
```

Then play individual notes:

```ocaml
brass 220. 0.5 |> play_node;
pluck 440. 0.8 |> play_node;
kick () |> play_node;
snare () |> play_node;
```

Loading initializes audio but schedules no notes. Each call creates a one-shot
voice that marks itself finished through `kill_on_end`. Duration is gate time,
not total lifetime: envelopes add their tails, and ADSR attack/decay can finish
after a short gate closes. `ring_chime` naturally decays even with a long gate.
Use finite positive arguments; start at 110–880 Hz and 0.1–2 seconds. Individual
voices leave headroom; chords still need output gain management.

## Article review

All 63 repository articles are covered by implementation notes:

- [Parts 1–23: foundations](foundations.md)
- [Parts 24–44: acoustic instruments](acoustic.md)
- [Parts 45–63: keys, strings and effects](keys-effects.md)

The notes map techniques to available primitives and distinguish implemented
recipes from deferred work. Parts 52–63 are truncated in the supplied mirror;
the newer wind/effect notes also reference recovered original articles. Linked
diagrams were not inspected. Parameter values
are demonstration choices unless explicitly attributed.

## Check

With `build/ylc` and `libs/audio_jit` built:

```sh
python3 test/test_synth_secrets.py
```

The headless check covers both recipe directories at 44.1/48 kHz, including short
gates and several pitches. It checks finite output, nonzero energy, peak headroom,
silent tails, completion flags and stereo separation for `string_pad`. Effects
also receive silence and delay-tail checks. It does not assess perceptual
resemblance or audio-device playback.
