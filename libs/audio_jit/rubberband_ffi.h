#ifndef _LIBS_AUDIO_JIT_RUBBERBAND_FFI_H
#define _LIBS_AUDIO_JIT_RUBBERBAND_FFI_H

#include "../../lang/ylc_datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

void *ylc_rubberband_new_realtime(int sample_rate, int channels,
                                  double time_ratio, double pitch_scale);
void *ylc_rubberband_new_realtime_finer(int sample_rate, int channels,
                                        double time_ratio,
                                        double pitch_scale);
void ylc_rubberband_delete(void *handle);
void ylc_rubberband_reset(void *handle);
void ylc_rubberband_set_time_ratio(void *handle, double ratio);
void ylc_rubberband_set_pitch_scale(void *handle, double scale);
int ylc_rubberband_get_latency(void *handle);
int ylc_rubberband_get_samples_required(void *handle);
int ylc_rubberband_get_channel_count(void *handle);
_DoubleArray ylc_rubberband_process(void *handle, _DoubleArray input,
                                    int final_block);
void ylc_rubberband_free_array(_DoubleArray arr);

#ifdef __cplusplus
}
#endif

#endif
