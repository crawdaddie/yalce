#ifndef _FAUST_IMP_H
#define _FAUST_IMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "../audio_graph.h"
#include "../node.h"
#include "../ctx.h"
#include "../faust.h"

#ifndef FAUSTFLOAT
#define FAUSTFLOAT double
#endif

/* The DSP class */
#define mydsp imp
#define newmydsp new_imp
#define deletemydsp delete_imp 
#define metadatamydsp metadata_imp 
#define getSampleRatemydsp getSampleRate_imp
#define getNumInputsmydsp getNumInputs_imp
#define getNumOutputsmydsp getNumOutputs_imp
#define classInitmydsp classInit_imp
#define instanceResetUserInterfacemydsp instanceResetUserInterface_imp
#define mydsp_faustpower2_f imp_faustpower2_f
#define instanceResetUserInterfacemydsp instanceResetUserInterface_imp
#define instanceClearmydsp instanceClear_imp
#define instanceConstantsmydsp instanceConstants_imp
#define instanceInitmydsp instanceInit_imp
#define initmydsp init_imp
#define buildUserInterfacemydsp buildUserInterface_imp
#define computemydsp compute_imp
#define faust_perform imp_perform
#define faust_node imp_node

#ifdef __cplusplus
}
#endif

#endif // _FAUST_ENGINE/FAUST/IMP_TRAIN_H
