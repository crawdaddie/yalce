#!/bin/bash
# Simple script to compile Faust DSP to C with a matching header file

if [ $# -lt 1 ]; then
    echo "Usage: $0 <dsp_file.dsp> [output_name]"
    echo "Example: $0 temper.dsp temper"
    exit 1
fi

DSP_FILE=$1
BASE_NAME=$(basename "$DSP_FILE" .dsp)
DSP_NAME=$BASE_NAME
OUTPUT_NAME=${2:-${BASE_NAME}}
HEADER_FILENAME=$(basename "$OUTPUT_NAME")

echo $DSP_NAME
echo $BASE_NAME
echo $OUTPUT_NAME

if ! command -v faust &> /dev/null; then
    echo "Faust compiler not found. Please install it first."
    exit 1
fi

echo "Compiling $DSP_FILE to $OUTPUT_NAME.c and $OUTPUT_NAME.h..."

faust -lang c -a pure.c "$DSP_FILE" -o "${OUTPUT_NAME}.c"


cat > "${OUTPUT_NAME}.h" << EOL
#ifndef $(echo "_FAUST_${DSP_NAME}" | tr '[:lower:]' '[:upper:]')_H
#define $(echo "_FAUST_${DSP_NAME}" | tr '[:lower:]' '[:upper:]')_H

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
#define mydsp ${DSP_NAME}
#define newmydsp new_${DSP_NAME}
#define deletemydsp delete_${DSP_NAME} 
#define metadatamydsp metadata_${DSP_NAME} 
#define getSampleRatemydsp getSampleRate_${DSP_NAME}
#define getNumInputsmydsp getNumInputs_${DSP_NAME}
#define getNumOutputsmydsp getNumOutputs_${DSP_NAME}
#define classInitmydsp classInit_${DSP_NAME}
#define instanceResetUserInterfacemydsp instanceResetUserInterface_${DSP_NAME}
#define mydsp_faustpower2_f ${DSP_NAME}_faustpower2_f
#define instanceResetUserInterfacemydsp instanceResetUserInterface_${DSP_NAME}
#define instanceClearmydsp instanceClear_${DSP_NAME}
#define instanceConstantsmydsp instanceConstants_${DSP_NAME}
#define instanceInitmydsp instanceInit_${DSP_NAME}
#define initmydsp init_${DSP_NAME}
#define buildUserInterfacemydsp buildUserInterface_${DSP_NAME}
#define computemydsp compute_${DSP_NAME}
#define faust_perform ${DSP_NAME}_perform
#define faust_node ${DSP_NAME}_node

#ifdef __cplusplus
}
#endif

#endif // $(echo "_FAUST_${OUTPUT_NAME}" | tr '[:lower:]' '[:upper:]')_H
EOL

TEMP_FILE=$(mktemp)
echo "#include \"./${BASE_NAME}.h\"" > $TEMP_FILE
cat "${OUTPUT_NAME}.c" >> $TEMP_FILE
mv $TEMP_FILE "${OUTPUT_NAME}.c"

echo "Compilation complete!"
echo "Generated files:"
echo "- ${OUTPUT_NAME}.c - C implementation with header include"
echo "- ${OUTPUT_NAME}.h - Header file for inclusion in your project"
