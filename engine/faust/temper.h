#ifndef _FAUST_TEMPER_H
#define _FAUST_TEMPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../audio_graph.h"
#include "../ctx.h"
#include "../faust.h"
#include "../node.h"
#include <stdlib.h>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT double
#endif

/* The DSP class */
#define mydsp temper
#define newmydsp new_temper
#define deletemydsp delete_temper
#define metadatamydsp metadata_temper
#define getSampleRatemydsp getSampleRate_temper
#define getNumInputsmydsp getNumInputs_temper
#define getNumOutputsmydsp getNumOutputs_temper
#define classInitmydsp classInit_temper
#define instanceResetUserInterfacemydsp instanceResetUserInterface_temper
#define mydsp_faustpower2_f temper_faustpower2_f
#define instanceResetUserInterfacemydsp instanceResetUserInterface_temper
#define instanceClearmydsp instanceClear_temper
#define instanceConstantsmydsp instanceConstants_temper
#define instanceInitmydsp instanceInit_temper
#define initmydsp init_temper
#define buildUserInterfacemydsp buildUserInterface_temper
#define computemydsp compute_temper

#ifdef __cplusplus
}
#endif

#endif // _FAUST_ENGINE/FAUST/TEMPER_H
