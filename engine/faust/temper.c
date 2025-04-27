#include "./temper.h"
/* ------------------------------------------------------------
name: "temper"
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
	FAUSTFLOAT fHslider0;
	float fRec3[2];
	FAUSTFLOAT fHslider1;
	float fRec4[2];
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider2;
	float fRec6[2];
	FAUSTFLOAT fHslider3;
	float fRec7[2];
	float fRec5[3];
	float fConst2;
	float fConst3;
	float fRec8[2];
	FAUSTFLOAT fHslider4;
	float fRec9[2];
	FAUSTFLOAT fHslider5;
	float fRec10[2];
	float fVec0[2];
	float fRec2[2];
	float fRec1[2];
	float fRec0[2];
	FAUSTFLOAT fHslider6;
	float fRec11[2];
} mydsp;

mydsp* newmydsp() { 
	mydsp* dsp = (mydsp*)calloc(1, sizeof(mydsp));
	return dsp;
}

void deletemydsp(mydsp* dsp) { 
	free(dsp);
}

void metadatamydsp(MetaGlue* m) { 
	m->declare(m->metaInterface, "analyzers.lib/name", "Faust Analyzer Library");
	m->declare(m->metaInterface, "analyzers.lib/version", "1.2.0");
	m->declare(m->metaInterface, "basics.lib/name", "Faust Basic Element Library");
	m->declare(m->metaInterface, "basics.lib/version", "1.21.0");
	m->declare(m->metaInterface, "compile_options", "-a pure.c -lang c -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
	m->declare(m->metaInterface, "filename", "temper.dsp");
	m->declare(m->metaInterface, "filters.lib/dcblocker:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/dcblocker:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/dcblocker:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/fir:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/fir:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/iir:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/iir:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/name", "Faust Filters Library");
	m->declare(m->metaInterface, "filters.lib/pole:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/pole:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/pole:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/tf1:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/tf1:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/tf2:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/tf2:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/version", "1.7.1");
	m->declare(m->metaInterface, "filters.lib/zero:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/zero:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/zero:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "maths.lib/author", "GRAME");
	m->declare(m->metaInterface, "maths.lib/copyright", "GRAME");
	m->declare(m->metaInterface, "maths.lib/license", "LGPL with exception");
	m->declare(m->metaInterface, "maths.lib/name", "Faust Math Library");
	m->declare(m->metaInterface, "maths.lib/version", "2.8.1");
	m->declare(m->metaInterface, "name", "temper");
	m->declare(m->metaInterface, "platform.lib/name", "Generic Platform Library");
	m->declare(m->metaInterface, "platform.lib/version", "1.3.0");
	m->declare(m->metaInterface, "signals.lib/name", "Faust Signal Routing Library");
	m->declare(m->metaInterface, "signals.lib/version", "1.6.0");
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
	dsp->fHslider0 = (FAUSTFLOAT)(1.0f);
	dsp->fHslider1 = (FAUSTFLOAT)(-6e+01f);
	dsp->fHslider2 = (FAUSTFLOAT)(2e+04f);
	dsp->fHslider3 = (FAUSTFLOAT)(1.0f);
	dsp->fHslider4 = (FAUSTFLOAT)(4.0f);
	dsp->fHslider5 = (FAUSTFLOAT)(1.0f);
	dsp->fHslider6 = (FAUSTFLOAT)(-3.0f);
}

void instanceClearmydsp(mydsp* dsp) {
	/* C99 loop */
	{
		int l0;
		for (l0 = 0; l0 < 2; l0 = l0 + 1) {
			dsp->fRec3[l0] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l1;
		for (l1 = 0; l1 < 2; l1 = l1 + 1) {
			dsp->fRec4[l1] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l2;
		for (l2 = 0; l2 < 2; l2 = l2 + 1) {
			dsp->fRec6[l2] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l3;
		for (l3 = 0; l3 < 2; l3 = l3 + 1) {
			dsp->fRec7[l3] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l4;
		for (l4 = 0; l4 < 3; l4 = l4 + 1) {
			dsp->fRec5[l4] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l5;
		for (l5 = 0; l5 < 2; l5 = l5 + 1) {
			dsp->fRec8[l5] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l6;
		for (l6 = 0; l6 < 2; l6 = l6 + 1) {
			dsp->fRec9[l6] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l7;
		for (l7 = 0; l7 < 2; l7 = l7 + 1) {
			dsp->fRec10[l7] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l8;
		for (l8 = 0; l8 < 2; l8 = l8 + 1) {
			dsp->fVec0[l8] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l9;
		for (l9 = 0; l9 < 2; l9 = l9 + 1) {
			dsp->fRec2[l9] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l10;
		for (l10 = 0; l10 < 2; l10 = l10 + 1) {
			dsp->fRec1[l10] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l11;
		for (l11 = 0; l11 < 2; l11 = l11 + 1) {
			dsp->fRec0[l11] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l12;
		for (l12 = 0; l12 < 2; l12 = l12 + 1) {
			dsp->fRec11[l12] = 0.0f;
		}
	}
}

void instanceConstantsmydsp(mydsp* dsp, int sample_rate) {
	dsp->fSampleRate = sample_rate;
	dsp->fConst0 = fminf(1.92e+05f, fmaxf(1.0f, (float)(dsp->fSampleRate)));
	dsp->fConst1 = 3.1415927f / dsp->fConst0;
	dsp->fConst2 = expf(-(25.0f / dsp->fConst0));
	dsp->fConst3 = 1.0f - dsp->fConst2;
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
	ui_interface->openVerticalBox(ui_interface->uiInterface, "temper");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Curve", &dsp->fHslider5, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.1f, (FAUSTFLOAT)4.0f, (FAUSTFLOAT)0.001f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Cutoff", &dsp->fHslider2, (FAUSTFLOAT)2e+04f, (FAUSTFLOAT)1e+02f, (FAUSTFLOAT)2e+04f, (FAUSTFLOAT)1.0f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Drive", &dsp->fHslider4, (FAUSTFLOAT)4.0f, (FAUSTFLOAT)-1e+01f, (FAUSTFLOAT)1e+01f, (FAUSTFLOAT)0.001f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Feedback", &dsp->fHslider1, (FAUSTFLOAT)-6e+01f, (FAUSTFLOAT)-6e+01f, (FAUSTFLOAT)-24.0f, (FAUSTFLOAT)1.0f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Level", &dsp->fHslider6, (FAUSTFLOAT)-3.0f, (FAUSTFLOAT)-24.0f, (FAUSTFLOAT)24.0f, (FAUSTFLOAT)1.0f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Resonance", &dsp->fHslider3, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)8.0f, (FAUSTFLOAT)0.001f);
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Saturation", &dsp->fHslider0, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.001f);
	ui_interface->closeBox(ui_interface->uiInterface);
}

void computemydsp(mydsp* dsp, int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
	FAUSTFLOAT* input0 = inputs[0];
	FAUSTFLOAT* output0 = outputs[0];
	float fSlow0 = 0.005f * (float)(dsp->fHslider0);
	float fSlow1 = 0.005f * powf(1e+01f, 0.05f * (float)(dsp->fHslider1));
	float fSlow2 = 0.005f / tanf(dsp->fConst1 * (float)(dsp->fHslider2));
	float fSlow3 = 0.005f * (float)(dsp->fHslider3);
	float fSlow4 = 0.005f * (float)(dsp->fHslider4);
	float fSlow5 = 0.005f * (float)(dsp->fHslider5);
	float fSlow6 = 0.005f * powf(1e+01f, 0.05f * (float)(dsp->fHslider6));
	/* C99 loop */
	{
		int i0;
		for (i0 = 0; i0 < count; i0 = i0 + 1) {
			dsp->fRec3[0] = fSlow0 + 0.995f * dsp->fRec3[1];
			dsp->fRec4[0] = fSlow1 + 0.995f * dsp->fRec4[1];
			dsp->fRec6[0] = fSlow2 + 0.995f * dsp->fRec6[1];
			dsp->fRec7[0] = fSlow3 + 0.995f * dsp->fRec7[1];
			float fTemp0 = 1.0f / dsp->fRec7[0];
			float fTemp1 = dsp->fRec6[0] * (dsp->fRec6[0] + fTemp0) + 1.0f;
			dsp->fRec5[0] = (float)(input0[i0]) - (dsp->fRec5[2] * (dsp->fRec6[0] * (dsp->fRec6[0] - fTemp0) + 1.0f) + 2.0f * dsp->fRec5[1] * (1.0f - mydsp_faustpower2_f(dsp->fRec6[0]))) / fTemp1;
			float fTemp2 = dsp->fRec4[0] * dsp->fRec0[1] + (dsp->fRec5[2] + dsp->fRec5[0] + 2.0f * dsp->fRec5[1]) / fTemp1;
			float fTemp3 = fabsf(fTemp2);
			dsp->fRec8[0] = fmaxf(fTemp3, dsp->fConst2 * dsp->fRec8[1] + dsp->fConst3 * fTemp3);
			dsp->fRec9[0] = fSlow4 + 0.995f * dsp->fRec9[1];
			float fTemp4 = fminf(3.0f, fmaxf(-3.0f, dsp->fRec8[0] + dsp->fRec9[0] * fTemp2));
			dsp->fRec10[0] = fSlow5 + 0.995f * dsp->fRec10[1];
			float fTemp5 = mydsp_faustpower2_f(dsp->fRec10[0]);
			float fTemp6 = fTemp5 * mydsp_faustpower2_f(fTemp4);
			float fTemp7 = fTemp6 + 27.0f;
			float fTemp8 = 9.0f * fTemp5 + 27.0f;
			float fTemp9 = (9.0f * fTemp6 + 27.0f) * (fTemp5 + 27.0f);
			float fTemp10 = (1.0f - dsp->fRec3[0]) * fTemp2 + 0.24f * (dsp->fRec3[0] * fTemp4 * fTemp7 * fTemp8 / fTemp9);
			dsp->fVec0[0] = fTemp10;
			dsp->fRec2[0] = dsp->fVec0[1] - 0.24f * (fTemp4 * fTemp7 * fTemp8 * (dsp->fRec2[1] - fTemp10) / fTemp9);
			dsp->fRec1[0] = dsp->fRec2[0] + 0.995f * dsp->fRec1[1] - dsp->fRec2[1];
			dsp->fRec0[0] = dsp->fRec1[0];
			dsp->fRec11[0] = fSlow6 + 0.995f * dsp->fRec11[1];
			output0[i0] = (FAUSTFLOAT)(4.0f * dsp->fRec0[0] * dsp->fRec11[0]);
			dsp->fRec3[1] = dsp->fRec3[0];
			dsp->fRec4[1] = dsp->fRec4[0];
			dsp->fRec6[1] = dsp->fRec6[0];
			dsp->fRec7[1] = dsp->fRec7[0];
			dsp->fRec5[2] = dsp->fRec5[1];
			dsp->fRec5[1] = dsp->fRec5[0];
			dsp->fRec8[1] = dsp->fRec8[0];
			dsp->fRec9[1] = dsp->fRec9[0];
			dsp->fRec10[1] = dsp->fRec10[0];
			dsp->fVec0[1] = dsp->fVec0[0];
			dsp->fRec2[1] = dsp->fRec2[0];
			dsp->fRec1[1] = dsp->fRec1[0];
			dsp->fRec0[1] = dsp->fRec0[0];
			dsp->fRec11[1] = dsp->fRec11[0];
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif

typedef struct {
  mydsp dsp;
  double saturation; // 0.0 to 1.0
  double feedback;   // -60.0 to -24.0 dB
  double cutoff;     // 100.0 to 20000.0 Hz
  double resonance;  // 1.0 to 8.0
  double drive;      // -10.0 to 10.0
  double curve;      // 0.1 to 4.0
  double level;      // -24.0 to 24.0 dB
} temper_state;

// Update the DSP parameters based on node state
static void update_temper_parameters(temper_state *state) {
  state->dsp.fHslider0 = (FAUSTFLOAT)state->saturation; // Saturation (0.0-1.0)
  state->dsp.fHslider1 =
      (FAUSTFLOAT)state->feedback;                  // Feedback (-60 to -24 dB)
  state->dsp.fHslider2 = (FAUSTFLOAT)state->cutoff; // Cutoff (100-20000 Hz)
  state->dsp.fHslider3 = (FAUSTFLOAT)state->resonance; // Resonance (1.0-8.0)
  state->dsp.fHslider4 = (FAUSTFLOAT)state->drive;     // Drive (-10 to 10)
  state->dsp.fHslider5 = (FAUSTFLOAT)state->curve;     // Curve (0.1-4.0)
  state->dsp.fHslider6 = (FAUSTFLOAT)state->level;     // Level (-24 to 24 dB)
}

// The temper_perform function that will be called by the audio graph
void *temper_perform(Node *node, temper_state *state, Node *inputs[],
                     int nframes, double spf) {
  double *out = node->output.buf;
  double *in = inputs[0]->output.buf;

  // Setup temporary buffers for FAUST processing
  FAUSTFLOAT *faust_input[1];
  FAUSTFLOAT *faust_output[1];

  FAUSTFLOAT in_buffer[nframes];
  FAUSTFLOAT out_buffer[nframes];

  // Copy input to FAUST format buffer and convert to float
  if (inputs[0]->output.layout > 1) {
    int lo = inputs[0]->output.layout;
    for (int i = 0; i < nframes; i++) {
      in_buffer[i] = 0.;
      for (int j = 0; j < lo; j++) {
        in_buffer[i] += (FAUSTFLOAT)in[j + lo * i];
      }
    }
  } else {
    for (int i = 0; i < nframes; i++) {
      in_buffer[i] = (FAUSTFLOAT)in[i];
    }
  }

  faust_input[0] = in_buffer;
  faust_output[0] = out_buffer;

  // Update parameters
  update_temper_parameters(state);

  // Process the audio
  computemydsp(&state->dsp, nframes, faust_input, faust_output);

  // Copy the output back
  for (int i = 0; i < nframes; i++) {
    out[i] = (double)out_buffer[i];
  }

  return node->output.buf;
}

// Create a static (fixed parameter) temper node
NodeRef temper_node(double saturation, double feedback, double cutoff,
                  double resonance, double drive, double curve, double level,
                  NodeRef input) {
  AudioGraph *graph = _graph;
  NodeRef node = allocate_node_in_graph(graph, sizeof(temper_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)temper_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(temper_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(temper_state)),
      // .output = (Signal){.layout = input->output.layout,
      //                    .size = BUF_SIZE,
      //                    .buf = allocate_buffer_from_pool(graph,
      //                    input->output.layout * BUF_SIZE)},

      .output = (Signal){.layout = 1,
                         .size = BUF_SIZE,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE)},
      .meta = "temper",
  };

  // Initialize state
  temper_state *state =
      (temper_state *)(graph->nodes_state_memory + node->state_offset);

  // Set initial parameter values
  state->saturation = saturation; // 0.0 to 1.0
  state->feedback = feedback;     // -60.0 to -24.0 dB
  state->cutoff = cutoff;         // 100.0 to 20000.0 Hz
  state->resonance = resonance;   // 1.0 to 8.0
  state->drive = drive;           // -10.0 to 10.0
  state->curve = curve;           // 0.1 to 4.0
  state->level = level;           // -24.0 to 24.0 dB

  double spf = ctx_spf();
  // Initialize the Faust DSP
  instanceConstantsmydsp(&state->dsp, (int)(1.0 / spf));
  instanceResetUserInterfacemydsp(&state->dsp);
  instanceClearmydsp(&state->dsp);

  // Connect input
  node->connections[0].source_node_index = input->node_index;

  return node;
}
