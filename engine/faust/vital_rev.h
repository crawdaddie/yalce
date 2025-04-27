#ifndef _FAUST_VITAL_REV_H
#define _FAUST_VITAL_REV_H

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
#define mydsp vital_rev
#define newmydsp new_vital_rev
#define deletemydsp delete_vital_rev 
#define metadatamydsp metadata_vital_rev 
#define getSampleRatemydsp getSampleRate_vital_rev
#define getNumInputsmydsp getNumInputs_vital_rev
#define getNumOutputsmydsp getNumOutputs_vital_rev
#define classInitmydsp classInit_vital_rev
#define instanceResetUserInterfacemydsp instanceResetUserInterface_vital_rev
#define mydsp_faustpower2_f vital_rev_faustpower2_f
#define instanceResetUserInterfacemydsp instanceResetUserInterface_vital_rev
#define instanceClearmydsp instanceClear_vital_rev
#define instanceConstantsmydsp instanceConstants_vital_rev
#define instanceInitmydsp instanceInit_vital_rev
#define initmydsp init_vital_rev
#define buildUserInterfacemydsp buildUserInterface_vital_rev
#define computemydsp compute_vital_rev
#define faust_perform vital_rev_perform
#define faust_node vital_rev_node

#ifdef __cplusplus
}
#endif

#endif // _FAUST_ENGINE/FAUST/VITAL_REV_H
