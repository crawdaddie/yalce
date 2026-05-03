# Rubber Band Scaffolding

`libs/rubberband` is tracked as a git submodule and `libs/audio_jit/libaudio_jit.so`
now builds against Rubber Band using its upstream C header
`libs/rubberband/rubberband/rubberband-c.h` plus the bundled single-file C++
implementation.

## Exported FFI

These symbols are exported from `libaudio_jit.so`:

- `ylc_rubberband_new_realtime`
- `ylc_rubberband_new_realtime_finer`
- `ylc_rubberband_delete`
- `ylc_rubberband_reset`
- `ylc_rubberband_set_time_ratio`
- `ylc_rubberband_set_pitch_scale`
- `ylc_rubberband_get_latency`
- `ylc_rubberband_get_samples_required`
- `ylc_rubberband_get_channel_count`
- `ylc_rubberband_process`
- `ylc_rubberband_free_array`

`DSP.ylc` exposes these under `DSP.RubberBand`.

## Data Model

- `process` accepts an interleaved `Array of Double`
- input length must be divisible by channel count
- output is a newly allocated interleaved `Array of Double`
- free returned output with `RubberBand.free_array`

## Intended Use

This is block-level scaffolding, not yet a sample-by-sample `compile_audio_fn`
ugen. A typical flow is:

1. create a processor with `new_realtime` or `new_realtime_finer`
2. update `time_ratio` and/or `pitch_scale` as needed
3. feed blocks through `process`
4. free each returned block with `free_array`
5. delete the processor when done
