#ifndef _FAUST_ZITA_REV_H
#define _FAUST_ZITA_REV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "./faust.h"

#ifndef FAUSTFLOAT
#define FAUSTFLOAT double
#endif

/* The DSP class */
#define mydsp zita_rev
#define newmydsp new_zita_rev
#define deletemydsp delete_zita_rev
#define metadatamydsp metadata_zita_rev
#define getSampleRatemydsp getSampleRate_zita_rev
#define getNumInputsmydsp getNumInputs_zita_rev
#define getNumOutputsmydsp getNumOutputs_zita_rev
#define classInitmydsp classInit_zita_rev
#define instanceResetUserInterfacemydsp instanceResetUserInterface_zita_rev
#define mydsp_faustpower2_f zita_rev_faustpower2_f
#define instanceResetUserInterfacemydsp instanceResetUserInterface_zita_rev
#define instanceClearmydsp instanceClear_zita_rev
#define instanceConstantsmydsp instanceConstants_zita_rev
#define instanceInitmydsp instanceInit_zita_rev
#define initmydsp init_zita_rev
#define buildUserInterfacemydsp buildUserInterface_zita_rev
#define computemydsp compute_zita_rev
#define faust_perform zita_rev_perform
#define faust_node zita_rev_node

#ifdef __cplusplus
}
#endif

#endif // _FAUST_ENGINE/FAUST/ZITA_REV_H
