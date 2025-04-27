#include "./imp_train.h"
#include <string.h>
/* ------------------------------------------------------------
author: ""
name: "imptrain"
version: "0.1"
Code generated with Faust 2.79.3 (https://faust.grame.fr)
Compilation options: -a pure.c -lang c -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33
-single -ftz 0
------------------------------------------------------------ */

#ifndef __mydsp_H__
#define __mydsp_H__

/************************************************************************
 ************************************************************************
    FAUST Architecture File
    Copyright (C) 2009-2011 Albert Graef <Dr.Graef@t-online.de>
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2.1 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the GNU C Library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA.
 ************************************************************************
 ************************************************************************/

/* Pure C architecture for Faust. This provides essentially the same interface
   as the C++ architecture in pure.cpp, but is supposed to be used with Pure's
   native Faust interface. In particular, this architecture enables you to use
   faust2 to compile a Faust dsp to a C module (faust -lang c -a pure.c) which
   can then be compiled to LLVM bitcode using an LLVM-capable C compiler such
   as clang, llvm-gcc, or gcc with the dragonegg plugin. The resulting bitcode
   module can then be loaded with Pure's built-in Faust bitcode linker. Please
   check the Pure manual at https://agraef.github.io/pure-lang/ for details. */

#include <math.h>
#include <stdlib.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

/* Define FAUSTFLOAT so that audio buffers and control values are always
   represented as double pointers. */

#define FAUSTFLOAT double

//----------------------------------------------------------------------------
//  Abstract user interface
//----------------------------------------------------------------------------

// -- layout groups

typedef void (*openTabBoxFun)(void *interface, const char *label);
typedef void (*openHorizontalBoxFun)(void *interface, const char *label);
typedef void (*openVerticalBoxFun)(void *interface, const char *label);
typedef void (*closeBoxFun)(void *interface);

// -- active widgets

typedef void (*addButtonFun)(void *interface, const char *label,
                             FAUSTFLOAT *zone);
typedef void (*addCheckButtonFun)(void *interface, const char *label,
                                  FAUSTFLOAT *zone);
typedef void (*addVerticalSliderFun)(void *interface, const char *label,
                                     FAUSTFLOAT *zone, FAUSTFLOAT init,
                                     FAUSTFLOAT min, FAUSTFLOAT max,
                                     FAUSTFLOAT step);
typedef void (*addHorizontalSliderFun)(void *interface, const char *label,
                                       FAUSTFLOAT *zone, FAUSTFLOAT init,
                                       FAUSTFLOAT min, FAUSTFLOAT max,
                                       FAUSTFLOAT step);
typedef void (*addNumEntryFun)(void *interface, const char *label,
                               FAUSTFLOAT *zone, FAUSTFLOAT init,
                               FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step);

// -- passive display widgets

typedef void (*addHorizontalBargraphFun)(void *interface, const char *label,
                                         FAUSTFLOAT *zone, FAUSTFLOAT min,
                                         FAUSTFLOAT max);
typedef void (*addVerticalBargraphFun)(void *interface, const char *label,
                                       FAUSTFLOAT *zone, FAUSTFLOAT min,
                                       FAUSTFLOAT max);

typedef void (*declareFun)(void *interface, FAUSTFLOAT *zone, const char *key,
                           const char *value);

typedef struct {
  void *uiInterface;
  openTabBoxFun openTabBox;
  openHorizontalBoxFun openHorizontalBox;
  openVerticalBoxFun openVerticalBox;
  closeBoxFun closeBox;
  addButtonFun addButton;
  addCheckButtonFun addCheckButton;
  addVerticalSliderFun addVerticalSlider;
  addHorizontalSliderFun addHorizontalSlider;
  addNumEntryFun addNumEntry;
  addHorizontalBargraphFun addHorizontalBargraph;
  addVerticalBargraphFun addVerticalBargraph;
  declareFun declare;
} UIGlue;

typedef void (*metaDeclareFun)(void *, const char *key, const char *value);

typedef struct {
  void *metaInterface;
  metaDeclareFun declare;
} MetaGlue;

//----------------------------------------------------------------------------
//  FAUST generated signal processor
//----------------------------------------------------------------------------
// clang-format off

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 


#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static float mydsp_faustpower2_f(float value) {
	return value * value;
}

#ifndef FAUSTCLASS 
#define FAUSTCLASS mydsp
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

typedef struct {
	int iVec0[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fConst2;
	float fRec0[2];
	float fVec1[2];
	float fVec2[2];
} mydsp;

mydsp* newmydsp() { 
	mydsp* dsp = (mydsp*)calloc(1, sizeof(mydsp));
	return dsp;
}

void deletemydsp(mydsp* dsp) { 
	free(dsp);
}

void metadatamydsp(MetaGlue* m) { 
	m->declare(m->metaInterface, "author", "");
	m->declare(m->metaInterface, "compile_options", "-a pure.c -lang c -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
	m->declare(m->metaInterface, "description", "alias-suppressed impulse train");
	m->declare(m->metaInterface, "filename", "imp.dsp");
	m->declare(m->metaInterface, "maths.lib/author", "GRAME");
	m->declare(m->metaInterface, "maths.lib/copyright", "GRAME");
	m->declare(m->metaInterface, "maths.lib/license", "LGPL with exception");
	m->declare(m->metaInterface, "maths.lib/name", "Faust Math Library");
	m->declare(m->metaInterface, "maths.lib/version", "2.8.1");
	m->declare(m->metaInterface, "name", "imptrain");
	m->declare(m->metaInterface, "oscillators.lib/lf_sawpos:author", "Bart Brouns, revised by Stéphane Letz");
	m->declare(m->metaInterface, "oscillators.lib/lf_sawpos:licence", "STK-4.3");
	m->declare(m->metaInterface, "oscillators.lib/name", "Faust Oscillator Library");
	m->declare(m->metaInterface, "oscillators.lib/sawN:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "oscillators.lib/sawN:license", "STK-4.3");
	m->declare(m->metaInterface, "oscillators.lib/version", "1.6.0");
	m->declare(m->metaInterface, "platform.lib/name", "Generic Platform Library");
	m->declare(m->metaInterface, "platform.lib/version", "1.3.0");
	m->declare(m->metaInterface, "version", "0.1");
}

int getSampleRatemydsp(mydsp* RESTRICT dsp) {
	return dsp->fSampleRate;
}

int getNumInputsmydsp(mydsp* RESTRICT dsp) {
	return 1;
}
int getNumOutputsmydsp(mydsp* RESTRICT dsp) {
	return 1;
}

void classInitmydsp(int sample_rate) {
}

void instanceResetUserInterfacemydsp(mydsp* dsp) {
}

void instanceClearmydsp(mydsp* dsp) {
	/* C99 loop */
	{
		int l0;
		for (l0 = 0; l0 < 2; l0 = l0 + 1) {
			dsp->iVec0[l0] = 0;
		}
	}
	/* C99 loop */
	{
		int l1;
		for (l1 = 0; l1 < 2; l1 = l1 + 1) {
			dsp->fRec0[l1] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l2;
		for (l2 = 0; l2 < 2; l2 = l2 + 1) {
			dsp->fVec1[l2] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l3;
		for (l3 = 0; l3 < 2; l3 = l3 + 1) {
			dsp->fVec2[l3] = 0.0f;
		}
	}
}

void instanceConstantsmydsp(mydsp* dsp, int sample_rate) {
	dsp->fSampleRate = sample_rate;
	dsp->fConst0 = fminf(1.92e+05f, fmaxf(1.0f, (float)(dsp->fSampleRate)));
	dsp->fConst1 = 0.125f * dsp->fConst0;
	dsp->fConst2 = 1.0f / dsp->fConst0;
}
	
void instanceInitmydsp(mydsp* dsp, int sample_rate) {
	instanceConstantsmydsp(dsp, sample_rate);
	instanceResetUserInterfacemydsp(dsp);
	instanceClearmydsp(dsp);
}

void initmydsp(mydsp* dsp, int sample_rate) {
	classInitmydsp(sample_rate);
	instanceInitmydsp(dsp, sample_rate);
}

void buildUserInterfacemydsp(mydsp* dsp, UIGlue* ui_interface) {
	ui_interface->openVerticalBox(ui_interface->uiInterface, "imptrain");
	ui_interface->closeBox(ui_interface->uiInterface);
}

void computemydsp(mydsp* dsp, int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
	FAUSTFLOAT* input0 = inputs[0];
	FAUSTFLOAT* output0 = outputs[0];
	/* C99 loop */
	{
		int i0;
		for (i0 = 0; i0 < count; i0 = i0 + 1) {
			dsp->iVec0[0] = 1;
			int iTemp0 = 1 - dsp->iVec0[1];
			float fTemp1 = fmaxf(2e+01f, fabsf((float)(input0[i0])));
			float fTemp2 = ((iTemp0) ? 0.0f : dsp->fRec0[1] + dsp->fConst2 * fTemp1);
			dsp->fRec0[0] = fTemp2 - floorf(fTemp2);
			float fTemp3 = mydsp_faustpower2_f(2.0f * dsp->fRec0[0] + -1.0f);
			dsp->fVec1[0] = fTemp3;
			float fTemp4 = (float)(dsp->iVec0[1]) * (fTemp3 - dsp->fVec1[1]) / fTemp1;
			dsp->fVec2[0] = fTemp4;
			output0[i0] = (FAUSTFLOAT)((float)(iTemp0) - dsp->fConst1 * (fTemp4 - dsp->fVec2[1]));
			dsp->iVec0[1] = dsp->iVec0[0];
			dsp->fRec0[1] = dsp->fRec0[0];
			dsp->fVec1[1] = dsp->fVec1[0];
			dsp->fVec2[1] = dsp->fVec2[0];
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif

typedef struct {
  mydsp dsp;
} imp_train_state;

// The impulse train perform function that will be called by the audio graph
void *imp_train_perform(Node *node, imp_train_state *state, Node *inputs[],
                       int nframes, double spf) {
  double *out = node->output.buf;
  double *freq = inputs[0]->output.buf;
  
  
  // Process the audio
  computemydsp(&state->dsp, nframes, &freq, &out);
  
  
  return node->output.buf;
}

// Create a static (fixed parameter) impulse train node
NodeRef imp_train_node(NodeRef freq) {
  AudioGraph *graph = _graph;
  NodeRef node = allocate_node_in_graph(graph, sizeof(imp_train_state));
  
  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)imp_train_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(imp_train_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(imp_train_state)),
      .output = (Signal){.layout = 1, // Mono output
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "imp_train",
  };
  
  // Initialize state
  imp_train_state *state = graph != NULL ? 
      (imp_train_state *)(graph->nodes_state_memory + node->state_offset) : (node + 1);
      
  // Initialize all memory to zero before setting parameters
  memset(state, 0, sizeof(imp_train_state));
  
  // Initialize the Faust DSP
  int sample_rate = ctx_sample_rate();
  instanceConstantsmydsp(&state->dsp, sample_rate);
  instanceResetUserInterfacemydsp(&state->dsp);
  instanceClearmydsp(&state->dsp);
  
  // Connect frequency input
  node->connections[0].source_node_index = freq->node_index;
  
  return node;
}

