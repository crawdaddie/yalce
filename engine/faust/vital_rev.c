#include "./vital_rev.h"
#include <string.h>
#include <stdio.h>
/* ------------------------------------------------------------
author: "David Braun"
name: "vital_rev"
version: "0.1"
Code generated with Faust 2.79.3 (https://faust.grame.fr)
Compilation options: -a pure.c -lang c -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __mydsp_H__
#define  __mydsp_H__

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
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fHslider0;
	float fConst2;
	int iVec0[2];
	float fRec18[2];
	FAUSTFLOAT fHslider1;
	float fRec19[2];
	float fConst3;
	FAUSTFLOAT fHslider2;
	float fRec21[2];
	float fRec20[2];
	float fConst4;
	FAUSTFLOAT fHslider3;
	float fRec23[2];
	FAUSTFLOAT fHslider4;
	float fRec25[2];
	float fRec24[2];
	float fRec26[2];
	FAUSTFLOAT fHslider5;
	float fRec27[2];
	float fVec1[2];
	float fRec22[2];
	float fRec28[2];
	FAUSTFLOAT fHslider6;
	float fRec29[2];
	FAUSTFLOAT fHslider7;
	float fRec30[2];
	int IOTA0;
	float fVec2[131072];
	float fConst5;
	FAUSTFLOAT fHslider8;
	float fRec32[2];
	FAUSTFLOAT fHslider9;
	float fRec34[2];
	float fConst6;
	FAUSTFLOAT fHslider10;
	float fRec35[2];
	float fVec3[65536];
	float fVec4[2];
	float fRec33[2];
	float fRec31[2];
	float fVec5[1024];
	float fRec16[2];
	float fRec38[2];
	float fRec40[2];
	float fRec41[2];
	float fVec6[2];
	float fRec39[2];
	float fRec42[2];
	float fVec7[131072];
	float fVec8[65536];
	float fVec9[2];
	float fRec44[2];
	float fRec43[2];
	float fVec10[1024];
	float fRec36[2];
	float fRec48[2];
	float fRec49[2];
	float fVec11[2];
	float fRec47[2];
	float fRec50[2];
	float fVec12[131072];
	float fVec13[1024];
	float fRec45[2];
	float fRec53[2];
	float fRec55[2];
	float fRec56[2];
	float fVec14[2];
	float fRec54[2];
	float fRec57[2];
	float fVec15[131072];
	float fVec16[1024];
	float fRec51[2];
	float fRec61[2];
	float fRec62[2];
	float fVec17[2];
	float fRec60[2];
	float fRec63[2];
	float fVec18[131072];
	float fVec19[1024];
	float fRec58[2];
	float fRec66[2];
	float fRec68[2];
	float fRec69[2];
	float fVec20[2];
	float fRec67[2];
	float fRec70[2];
	float fVec21[131072];
	float fVec22[1024];
	float fRec64[2];
	float fRec74[2];
	float fRec75[2];
	float fVec23[2];
	float fRec73[2];
	float fRec76[2];
	float fVec24[65536];
	float fVec25[1024];
	float fRec71[2];
	float fRec79[2];
	float fRec81[2];
	float fRec82[2];
	float fVec26[2];
	float fRec80[2];
	float fRec83[2];
	float fVec27[131072];
	float fVec28[1024];
	float fRec77[2];
	float fRec87[2];
	float fRec88[2];
	float fVec29[2];
	float fRec86[2];
	float fRec89[2];
	float fVec30[131072];
	float fVec31[1024];
	float fRec84[2];
	float fRec92[2];
	float fRec94[2];
	float fRec95[2];
	float fVec32[2];
	float fRec93[2];
	float fRec96[2];
	float fVec33[131072];
	float fVec34[1024];
	float fRec90[2];
	float fRec100[2];
	float fRec101[2];
	float fVec35[2];
	float fRec99[2];
	float fRec102[2];
	float fVec36[65536];
	float fVec37[1024];
	float fRec97[2];
	float fRec105[2];
	float fRec107[2];
	float fRec108[2];
	float fVec38[2];
	float fRec106[2];
	float fRec109[2];
	float fVec39[131072];
	float fVec40[1024];
	float fRec103[2];
	float fRec113[2];
	float fRec114[2];
	float fVec41[2];
	float fRec112[2];
	float fRec115[2];
	float fVec42[131072];
	float fVec43[1024];
	float fRec110[2];
	float fRec118[2];
	float fRec120[2];
	float fRec121[2];
	float fVec44[2];
	float fRec119[2];
	float fRec122[2];
	float fVec45[65536];
	float fVec46[1024];
	float fRec116[2];
	float fRec126[2];
	float fRec127[2];
	float fVec47[2];
	float fRec125[2];
	float fRec128[2];
	float fVec48[65536];
	float fVec49[1024];
	float fRec123[2];
	float fRec132[2];
	float fRec133[2];
	float fVec50[2];
	float fRec131[2];
	float fRec134[2];
	float fVec51[131072];
	float fVec52[1024];
	float fRec129[2];
	float fRec0[3];
	float fRec1[3];
	float fRec2[3];
	float fRec3[3];
	float fRec4[3];
	float fRec5[3];
	float fRec6[3];
	float fRec7[3];
	float fRec8[3];
	float fRec9[3];
	float fRec10[3];
	float fRec11[3];
	float fRec12[3];
	float fRec13[3];
	float fRec14[3];
	float fRec15[3];
	FAUSTFLOAT fHslider11;
	float fRec135[2];
} mydsp;

mydsp* newmydsp() { 
	mydsp* dsp = (mydsp*)calloc(1, sizeof(mydsp));
	return dsp;
}

void deletemydsp(mydsp* dsp) { 
	free(dsp);
}

void metadatamydsp(MetaGlue* m) { 
	m->declare(m->metaInterface, "aanl.lib/name", "Faust Antialiased Nonlinearities");
	m->declare(m->metaInterface, "aanl.lib/version", "1.4.1");
	m->declare(m->metaInterface, "analyzers.lib/name", "Faust Analyzer Library");
	m->declare(m->metaInterface, "analyzers.lib/version", "1.2.0");
	m->declare(m->metaInterface, "author", "David Braun");
	m->declare(m->metaInterface, "basics.lib/name", "Faust Basic Element Library");
	m->declare(m->metaInterface, "basics.lib/version", "1.21.0");
	m->declare(m->metaInterface, "compile_options", "-a pure.c -lang c -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
	m->declare(m->metaInterface, "delays.lib/fdelayltv:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "delays.lib/name", "Faust Delay Library");
	m->declare(m->metaInterface, "delays.lib/version", "1.1.0");
	m->declare(m->metaInterface, "demos.lib/name", "Faust Demos Library");
	m->declare(m->metaInterface, "demos.lib/version", "1.2.0");
	m->declare(m->metaInterface, "demos.lib/vital_rev_demo:author", "David Braun");
	m->declare(m->metaInterface, "demos.lib/vital_rev_demo:license", "GPL-3.0");
	m->declare(m->metaInterface, "description", "Vital demo application.");
	m->declare(m->metaInterface, "filename", "vital_rev.dsp");
	m->declare(m->metaInterface, "filters.lib/allpass_comb:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/allpass_comb:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/allpass_comb:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/filterbank:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/filterbank:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/filterbank:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/highpass:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/highshelf:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/highshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/highshelf:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/lowpass:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/lowpass:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/lowshelf:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/lowshelf:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/lowshelf:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/name", "Faust Filters Library");
	m->declare(m->metaInterface, "filters.lib/tf1:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/tf1:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/tf1s:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/tf1s:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/version", "1.7.1");
	m->declare(m->metaInterface, "interpolators.lib/interpolate_linear:author", "Stéphane Letz");
	m->declare(m->metaInterface, "interpolators.lib/interpolate_linear:licence", "MIT");
	m->declare(m->metaInterface, "interpolators.lib/name", "Faust Interpolator Library");
	m->declare(m->metaInterface, "interpolators.lib/remap:author", "David Braun");
	m->declare(m->metaInterface, "interpolators.lib/version", "1.4.0");
	m->declare(m->metaInterface, "maths.lib/author", "GRAME");
	m->declare(m->metaInterface, "maths.lib/copyright", "GRAME");
	m->declare(m->metaInterface, "maths.lib/license", "LGPL with exception");
	m->declare(m->metaInterface, "maths.lib/name", "Faust Math Library");
	m->declare(m->metaInterface, "maths.lib/version", "2.8.1");
	m->declare(m->metaInterface, "misceffects.lib/dryWetMixerConstantPower:author", "David Braun, revised by Stéphane Letz");
	m->declare(m->metaInterface, "misceffects.lib/name", "Misc Effects Library");
	m->declare(m->metaInterface, "misceffects.lib/version", "2.5.1");
	m->declare(m->metaInterface, "name", "vital_rev");
	m->declare(m->metaInterface, "platform.lib/name", "Generic Platform Library");
	m->declare(m->metaInterface, "platform.lib/version", "1.3.0");
	m->declare(m->metaInterface, "reverbs.lib/name", "Faust Reverb Library");
	m->declare(m->metaInterface, "reverbs.lib/version", "1.4.0");
	m->declare(m->metaInterface, "reverbs.lib/vital_rev:author", "David Braun");
	m->declare(m->metaInterface, "reverbs.lib/vital_rev:license", "GPL-3.0");
	m->declare(m->metaInterface, "routes.lib/name", "Faust Signal Routing Library");
	m->declare(m->metaInterface, "routes.lib/version", "1.2.0");
	m->declare(m->metaInterface, "signals.lib/name", "Faust Signal Routing Library");
	m->declare(m->metaInterface, "signals.lib/version", "1.6.0");
	m->declare(m->metaInterface, "spats.lib/name", "Faust Spatialization Library");
	m->declare(m->metaInterface, "spats.lib/version", "1.2.0");
	m->declare(m->metaInterface, "version", "0.1");
}

int getSampleRatemydsp(mydsp* RESTRICT dsp) {
	return dsp->fSampleRate;
}

int getNumInputsmydsp(mydsp* RESTRICT dsp) {
	return 2;
}
int getNumOutputsmydsp(mydsp* RESTRICT dsp) {
	return 2;
}

void classInitmydsp(int sample_rate) {
}

void instanceResetUserInterfacemydsp(mydsp* dsp) {
	dsp->fHslider0 = (FAUSTFLOAT)(0.5f);
	dsp->fHslider1 = (FAUSTFLOAT)(0.01f);
	dsp->fHslider2 = (FAUSTFLOAT)(0.1f);
	dsp->fHslider3 = (FAUSTFLOAT)(0.62184876f);
	dsp->fHslider4 = (FAUSTFLOAT)(0.0f);
	dsp->fHslider5 = (FAUSTFLOAT)(1.0f);
	dsp->fHslider6 = (FAUSTFLOAT)(0.8333333f);
	dsp->fHslider7 = (FAUSTFLOAT)(0.5f);
	dsp->fHslider8 = (FAUSTFLOAT)(0.789916f);
	dsp->fHslider9 = (FAUSTFLOAT)(0.0f);
	dsp->fHslider10 = (FAUSTFLOAT)(0.0f);
	dsp->fHslider11 = (FAUSTFLOAT)(1.0f);
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
			dsp->fRec18[l1] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l2;
		for (l2 = 0; l2 < 2; l2 = l2 + 1) {
			dsp->fRec19[l2] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l3;
		for (l3 = 0; l3 < 2; l3 = l3 + 1) {
			dsp->fRec21[l3] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l4;
		for (l4 = 0; l4 < 2; l4 = l4 + 1) {
			dsp->fRec20[l4] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l5;
		for (l5 = 0; l5 < 2; l5 = l5 + 1) {
			dsp->fRec23[l5] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l6;
		for (l6 = 0; l6 < 2; l6 = l6 + 1) {
			dsp->fRec25[l6] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l7;
		for (l7 = 0; l7 < 2; l7 = l7 + 1) {
			dsp->fRec24[l7] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l8;
		for (l8 = 0; l8 < 2; l8 = l8 + 1) {
			dsp->fRec26[l8] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l9;
		for (l9 = 0; l9 < 2; l9 = l9 + 1) {
			dsp->fRec27[l9] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l10;
		for (l10 = 0; l10 < 2; l10 = l10 + 1) {
			dsp->fVec1[l10] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l11;
		for (l11 = 0; l11 < 2; l11 = l11 + 1) {
			dsp->fRec22[l11] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l12;
		for (l12 = 0; l12 < 2; l12 = l12 + 1) {
			dsp->fRec28[l12] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l13;
		for (l13 = 0; l13 < 2; l13 = l13 + 1) {
			dsp->fRec29[l13] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l14;
		for (l14 = 0; l14 < 2; l14 = l14 + 1) {
			dsp->fRec30[l14] = 0.0f;
		}
	}
	dsp->IOTA0 = 0;
	/* C99 loop */
	{
		int l15;
		for (l15 = 0; l15 < 131072; l15 = l15 + 1) {
			dsp->fVec2[l15] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l16;
		for (l16 = 0; l16 < 2; l16 = l16 + 1) {
			dsp->fRec32[l16] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l17;
		for (l17 = 0; l17 < 2; l17 = l17 + 1) {
			dsp->fRec34[l17] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l18;
		for (l18 = 0; l18 < 2; l18 = l18 + 1) {
			dsp->fRec35[l18] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l19;
		for (l19 = 0; l19 < 65536; l19 = l19 + 1) {
			dsp->fVec3[l19] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l20;
		for (l20 = 0; l20 < 2; l20 = l20 + 1) {
			dsp->fVec4[l20] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l21;
		for (l21 = 0; l21 < 2; l21 = l21 + 1) {
			dsp->fRec33[l21] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l22;
		for (l22 = 0; l22 < 2; l22 = l22 + 1) {
			dsp->fRec31[l22] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l23;
		for (l23 = 0; l23 < 1024; l23 = l23 + 1) {
			dsp->fVec5[l23] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l24;
		for (l24 = 0; l24 < 2; l24 = l24 + 1) {
			dsp->fRec16[l24] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l25;
		for (l25 = 0; l25 < 2; l25 = l25 + 1) {
			dsp->fRec38[l25] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l26;
		for (l26 = 0; l26 < 2; l26 = l26 + 1) {
			dsp->fRec40[l26] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l27;
		for (l27 = 0; l27 < 2; l27 = l27 + 1) {
			dsp->fRec41[l27] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l28;
		for (l28 = 0; l28 < 2; l28 = l28 + 1) {
			dsp->fVec6[l28] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l29;
		for (l29 = 0; l29 < 2; l29 = l29 + 1) {
			dsp->fRec39[l29] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l30;
		for (l30 = 0; l30 < 2; l30 = l30 + 1) {
			dsp->fRec42[l30] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l31;
		for (l31 = 0; l31 < 131072; l31 = l31 + 1) {
			dsp->fVec7[l31] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l32;
		for (l32 = 0; l32 < 65536; l32 = l32 + 1) {
			dsp->fVec8[l32] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l33;
		for (l33 = 0; l33 < 2; l33 = l33 + 1) {
			dsp->fVec9[l33] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l34;
		for (l34 = 0; l34 < 2; l34 = l34 + 1) {
			dsp->fRec44[l34] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l35;
		for (l35 = 0; l35 < 2; l35 = l35 + 1) {
			dsp->fRec43[l35] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l36;
		for (l36 = 0; l36 < 1024; l36 = l36 + 1) {
			dsp->fVec10[l36] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l37;
		for (l37 = 0; l37 < 2; l37 = l37 + 1) {
			dsp->fRec36[l37] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l38;
		for (l38 = 0; l38 < 2; l38 = l38 + 1) {
			dsp->fRec48[l38] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l39;
		for (l39 = 0; l39 < 2; l39 = l39 + 1) {
			dsp->fRec49[l39] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l40;
		for (l40 = 0; l40 < 2; l40 = l40 + 1) {
			dsp->fVec11[l40] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l41;
		for (l41 = 0; l41 < 2; l41 = l41 + 1) {
			dsp->fRec47[l41] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l42;
		for (l42 = 0; l42 < 2; l42 = l42 + 1) {
			dsp->fRec50[l42] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l43;
		for (l43 = 0; l43 < 131072; l43 = l43 + 1) {
			dsp->fVec12[l43] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l44;
		for (l44 = 0; l44 < 1024; l44 = l44 + 1) {
			dsp->fVec13[l44] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l45;
		for (l45 = 0; l45 < 2; l45 = l45 + 1) {
			dsp->fRec45[l45] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l46;
		for (l46 = 0; l46 < 2; l46 = l46 + 1) {
			dsp->fRec53[l46] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l47;
		for (l47 = 0; l47 < 2; l47 = l47 + 1) {
			dsp->fRec55[l47] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l48;
		for (l48 = 0; l48 < 2; l48 = l48 + 1) {
			dsp->fRec56[l48] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l49;
		for (l49 = 0; l49 < 2; l49 = l49 + 1) {
			dsp->fVec14[l49] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l50;
		for (l50 = 0; l50 < 2; l50 = l50 + 1) {
			dsp->fRec54[l50] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l51;
		for (l51 = 0; l51 < 2; l51 = l51 + 1) {
			dsp->fRec57[l51] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l52;
		for (l52 = 0; l52 < 131072; l52 = l52 + 1) {
			dsp->fVec15[l52] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l53;
		for (l53 = 0; l53 < 1024; l53 = l53 + 1) {
			dsp->fVec16[l53] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l54;
		for (l54 = 0; l54 < 2; l54 = l54 + 1) {
			dsp->fRec51[l54] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l55;
		for (l55 = 0; l55 < 2; l55 = l55 + 1) {
			dsp->fRec61[l55] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l56;
		for (l56 = 0; l56 < 2; l56 = l56 + 1) {
			dsp->fRec62[l56] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l57;
		for (l57 = 0; l57 < 2; l57 = l57 + 1) {
			dsp->fVec17[l57] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l58;
		for (l58 = 0; l58 < 2; l58 = l58 + 1) {
			dsp->fRec60[l58] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l59;
		for (l59 = 0; l59 < 2; l59 = l59 + 1) {
			dsp->fRec63[l59] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l60;
		for (l60 = 0; l60 < 131072; l60 = l60 + 1) {
			dsp->fVec18[l60] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l61;
		for (l61 = 0; l61 < 1024; l61 = l61 + 1) {
			dsp->fVec19[l61] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l62;
		for (l62 = 0; l62 < 2; l62 = l62 + 1) {
			dsp->fRec58[l62] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l63;
		for (l63 = 0; l63 < 2; l63 = l63 + 1) {
			dsp->fRec66[l63] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l64;
		for (l64 = 0; l64 < 2; l64 = l64 + 1) {
			dsp->fRec68[l64] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l65;
		for (l65 = 0; l65 < 2; l65 = l65 + 1) {
			dsp->fRec69[l65] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l66;
		for (l66 = 0; l66 < 2; l66 = l66 + 1) {
			dsp->fVec20[l66] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l67;
		for (l67 = 0; l67 < 2; l67 = l67 + 1) {
			dsp->fRec67[l67] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l68;
		for (l68 = 0; l68 < 2; l68 = l68 + 1) {
			dsp->fRec70[l68] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l69;
		for (l69 = 0; l69 < 131072; l69 = l69 + 1) {
			dsp->fVec21[l69] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l70;
		for (l70 = 0; l70 < 1024; l70 = l70 + 1) {
			dsp->fVec22[l70] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l71;
		for (l71 = 0; l71 < 2; l71 = l71 + 1) {
			dsp->fRec64[l71] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l72;
		for (l72 = 0; l72 < 2; l72 = l72 + 1) {
			dsp->fRec74[l72] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l73;
		for (l73 = 0; l73 < 2; l73 = l73 + 1) {
			dsp->fRec75[l73] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l74;
		for (l74 = 0; l74 < 2; l74 = l74 + 1) {
			dsp->fVec23[l74] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l75;
		for (l75 = 0; l75 < 2; l75 = l75 + 1) {
			dsp->fRec73[l75] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l76;
		for (l76 = 0; l76 < 2; l76 = l76 + 1) {
			dsp->fRec76[l76] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l77;
		for (l77 = 0; l77 < 65536; l77 = l77 + 1) {
			dsp->fVec24[l77] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l78;
		for (l78 = 0; l78 < 1024; l78 = l78 + 1) {
			dsp->fVec25[l78] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l79;
		for (l79 = 0; l79 < 2; l79 = l79 + 1) {
			dsp->fRec71[l79] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l80;
		for (l80 = 0; l80 < 2; l80 = l80 + 1) {
			dsp->fRec79[l80] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l81;
		for (l81 = 0; l81 < 2; l81 = l81 + 1) {
			dsp->fRec81[l81] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l82;
		for (l82 = 0; l82 < 2; l82 = l82 + 1) {
			dsp->fRec82[l82] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l83;
		for (l83 = 0; l83 < 2; l83 = l83 + 1) {
			dsp->fVec26[l83] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l84;
		for (l84 = 0; l84 < 2; l84 = l84 + 1) {
			dsp->fRec80[l84] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l85;
		for (l85 = 0; l85 < 2; l85 = l85 + 1) {
			dsp->fRec83[l85] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l86;
		for (l86 = 0; l86 < 131072; l86 = l86 + 1) {
			dsp->fVec27[l86] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l87;
		for (l87 = 0; l87 < 1024; l87 = l87 + 1) {
			dsp->fVec28[l87] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l88;
		for (l88 = 0; l88 < 2; l88 = l88 + 1) {
			dsp->fRec77[l88] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l89;
		for (l89 = 0; l89 < 2; l89 = l89 + 1) {
			dsp->fRec87[l89] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l90;
		for (l90 = 0; l90 < 2; l90 = l90 + 1) {
			dsp->fRec88[l90] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l91;
		for (l91 = 0; l91 < 2; l91 = l91 + 1) {
			dsp->fVec29[l91] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l92;
		for (l92 = 0; l92 < 2; l92 = l92 + 1) {
			dsp->fRec86[l92] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l93;
		for (l93 = 0; l93 < 2; l93 = l93 + 1) {
			dsp->fRec89[l93] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l94;
		for (l94 = 0; l94 < 131072; l94 = l94 + 1) {
			dsp->fVec30[l94] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l95;
		for (l95 = 0; l95 < 1024; l95 = l95 + 1) {
			dsp->fVec31[l95] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l96;
		for (l96 = 0; l96 < 2; l96 = l96 + 1) {
			dsp->fRec84[l96] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l97;
		for (l97 = 0; l97 < 2; l97 = l97 + 1) {
			dsp->fRec92[l97] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l98;
		for (l98 = 0; l98 < 2; l98 = l98 + 1) {
			dsp->fRec94[l98] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l99;
		for (l99 = 0; l99 < 2; l99 = l99 + 1) {
			dsp->fRec95[l99] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l100;
		for (l100 = 0; l100 < 2; l100 = l100 + 1) {
			dsp->fVec32[l100] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l101;
		for (l101 = 0; l101 < 2; l101 = l101 + 1) {
			dsp->fRec93[l101] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l102;
		for (l102 = 0; l102 < 2; l102 = l102 + 1) {
			dsp->fRec96[l102] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l103;
		for (l103 = 0; l103 < 131072; l103 = l103 + 1) {
			dsp->fVec33[l103] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l104;
		for (l104 = 0; l104 < 1024; l104 = l104 + 1) {
			dsp->fVec34[l104] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l105;
		for (l105 = 0; l105 < 2; l105 = l105 + 1) {
			dsp->fRec90[l105] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l106;
		for (l106 = 0; l106 < 2; l106 = l106 + 1) {
			dsp->fRec100[l106] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l107;
		for (l107 = 0; l107 < 2; l107 = l107 + 1) {
			dsp->fRec101[l107] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l108;
		for (l108 = 0; l108 < 2; l108 = l108 + 1) {
			dsp->fVec35[l108] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l109;
		for (l109 = 0; l109 < 2; l109 = l109 + 1) {
			dsp->fRec99[l109] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l110;
		for (l110 = 0; l110 < 2; l110 = l110 + 1) {
			dsp->fRec102[l110] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l111;
		for (l111 = 0; l111 < 65536; l111 = l111 + 1) {
			dsp->fVec36[l111] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l112;
		for (l112 = 0; l112 < 1024; l112 = l112 + 1) {
			dsp->fVec37[l112] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l113;
		for (l113 = 0; l113 < 2; l113 = l113 + 1) {
			dsp->fRec97[l113] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l114;
		for (l114 = 0; l114 < 2; l114 = l114 + 1) {
			dsp->fRec105[l114] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l115;
		for (l115 = 0; l115 < 2; l115 = l115 + 1) {
			dsp->fRec107[l115] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l116;
		for (l116 = 0; l116 < 2; l116 = l116 + 1) {
			dsp->fRec108[l116] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l117;
		for (l117 = 0; l117 < 2; l117 = l117 + 1) {
			dsp->fVec38[l117] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l118;
		for (l118 = 0; l118 < 2; l118 = l118 + 1) {
			dsp->fRec106[l118] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l119;
		for (l119 = 0; l119 < 2; l119 = l119 + 1) {
			dsp->fRec109[l119] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l120;
		for (l120 = 0; l120 < 131072; l120 = l120 + 1) {
			dsp->fVec39[l120] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l121;
		for (l121 = 0; l121 < 1024; l121 = l121 + 1) {
			dsp->fVec40[l121] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l122;
		for (l122 = 0; l122 < 2; l122 = l122 + 1) {
			dsp->fRec103[l122] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l123;
		for (l123 = 0; l123 < 2; l123 = l123 + 1) {
			dsp->fRec113[l123] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l124;
		for (l124 = 0; l124 < 2; l124 = l124 + 1) {
			dsp->fRec114[l124] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l125;
		for (l125 = 0; l125 < 2; l125 = l125 + 1) {
			dsp->fVec41[l125] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l126;
		for (l126 = 0; l126 < 2; l126 = l126 + 1) {
			dsp->fRec112[l126] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l127;
		for (l127 = 0; l127 < 2; l127 = l127 + 1) {
			dsp->fRec115[l127] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l128;
		for (l128 = 0; l128 < 131072; l128 = l128 + 1) {
			dsp->fVec42[l128] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l129;
		for (l129 = 0; l129 < 1024; l129 = l129 + 1) {
			dsp->fVec43[l129] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l130;
		for (l130 = 0; l130 < 2; l130 = l130 + 1) {
			dsp->fRec110[l130] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l131;
		for (l131 = 0; l131 < 2; l131 = l131 + 1) {
			dsp->fRec118[l131] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l132;
		for (l132 = 0; l132 < 2; l132 = l132 + 1) {
			dsp->fRec120[l132] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l133;
		for (l133 = 0; l133 < 2; l133 = l133 + 1) {
			dsp->fRec121[l133] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l134;
		for (l134 = 0; l134 < 2; l134 = l134 + 1) {
			dsp->fVec44[l134] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l135;
		for (l135 = 0; l135 < 2; l135 = l135 + 1) {
			dsp->fRec119[l135] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l136;
		for (l136 = 0; l136 < 2; l136 = l136 + 1) {
			dsp->fRec122[l136] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l137;
		for (l137 = 0; l137 < 65536; l137 = l137 + 1) {
			dsp->fVec45[l137] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l138;
		for (l138 = 0; l138 < 1024; l138 = l138 + 1) {
			dsp->fVec46[l138] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l139;
		for (l139 = 0; l139 < 2; l139 = l139 + 1) {
			dsp->fRec116[l139] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l140;
		for (l140 = 0; l140 < 2; l140 = l140 + 1) {
			dsp->fRec126[l140] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l141;
		for (l141 = 0; l141 < 2; l141 = l141 + 1) {
			dsp->fRec127[l141] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l142;
		for (l142 = 0; l142 < 2; l142 = l142 + 1) {
			dsp->fVec47[l142] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l143;
		for (l143 = 0; l143 < 2; l143 = l143 + 1) {
			dsp->fRec125[l143] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l144;
		for (l144 = 0; l144 < 2; l144 = l144 + 1) {
			dsp->fRec128[l144] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l145;
		for (l145 = 0; l145 < 65536; l145 = l145 + 1) {
			dsp->fVec48[l145] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l146;
		for (l146 = 0; l146 < 1024; l146 = l146 + 1) {
			dsp->fVec49[l146] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l147;
		for (l147 = 0; l147 < 2; l147 = l147 + 1) {
			dsp->fRec123[l147] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l148;
		for (l148 = 0; l148 < 2; l148 = l148 + 1) {
			dsp->fRec132[l148] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l149;
		for (l149 = 0; l149 < 2; l149 = l149 + 1) {
			dsp->fRec133[l149] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l150;
		for (l150 = 0; l150 < 2; l150 = l150 + 1) {
			dsp->fVec50[l150] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l151;
		for (l151 = 0; l151 < 2; l151 = l151 + 1) {
			dsp->fRec131[l151] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l152;
		for (l152 = 0; l152 < 2; l152 = l152 + 1) {
			dsp->fRec134[l152] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l153;
		for (l153 = 0; l153 < 131072; l153 = l153 + 1) {
			dsp->fVec51[l153] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l154;
		for (l154 = 0; l154 < 1024; l154 = l154 + 1) {
			dsp->fVec52[l154] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l155;
		for (l155 = 0; l155 < 2; l155 = l155 + 1) {
			dsp->fRec129[l155] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l156;
		for (l156 = 0; l156 < 3; l156 = l156 + 1) {
			dsp->fRec0[l156] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l157;
		for (l157 = 0; l157 < 3; l157 = l157 + 1) {
			dsp->fRec1[l157] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l158;
		for (l158 = 0; l158 < 3; l158 = l158 + 1) {
			dsp->fRec2[l158] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l159;
		for (l159 = 0; l159 < 3; l159 = l159 + 1) {
			dsp->fRec3[l159] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l160;
		for (l160 = 0; l160 < 3; l160 = l160 + 1) {
			dsp->fRec4[l160] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l161;
		for (l161 = 0; l161 < 3; l161 = l161 + 1) {
			dsp->fRec5[l161] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l162;
		for (l162 = 0; l162 < 3; l162 = l162 + 1) {
			dsp->fRec6[l162] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l163;
		for (l163 = 0; l163 < 3; l163 = l163 + 1) {
			dsp->fRec7[l163] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l164;
		for (l164 = 0; l164 < 3; l164 = l164 + 1) {
			dsp->fRec8[l164] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l165;
		for (l165 = 0; l165 < 3; l165 = l165 + 1) {
			dsp->fRec9[l165] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l166;
		for (l166 = 0; l166 < 3; l166 = l166 + 1) {
			dsp->fRec10[l166] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l167;
		for (l167 = 0; l167 < 3; l167 = l167 + 1) {
			dsp->fRec11[l167] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l168;
		for (l168 = 0; l168 < 3; l168 = l168 + 1) {
			dsp->fRec12[l168] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l169;
		for (l169 = 0; l169 < 3; l169 = l169 + 1) {
			dsp->fRec13[l169] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l170;
		for (l170 = 0; l170 < 3; l170 = l170 + 1) {
			dsp->fRec14[l170] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l171;
		for (l171 = 0; l171 < 3; l171 = l171 + 1) {
			dsp->fRec15[l171] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l172;
		for (l172 = 0; l172 < 2; l172 = l172 + 1) {
			dsp->fRec135[l172] = 0.0f;
		}
	}
}

void instanceConstantsmydsp(mydsp* dsp, int sample_rate) {
	dsp->fSampleRate = sample_rate;
	dsp->fConst0 = fminf(1.92e+05f, fmaxf(1.0f, (float)(dsp->fSampleRate)));
	dsp->fConst1 = 44.1f / dsp->fConst0;
	dsp->fConst2 = 1.0f - dsp->fConst1;
	dsp->fConst3 = 1.0f / dsp->fConst0;
	dsp->fConst4 = 1382.3008f / dsp->fConst0;
	dsp->fConst5 = 0.62716556f * dsp->fConst0;
	dsp->fConst6 = 0.3f * dsp->fConst0;
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
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Reverb");
	ui_interface->declare(ui_interface->uiInterface, 0, "0", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Pre-filter");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider9, "0", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider9, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Low Cutoff", &dsp->fHslider9, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider8, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider8, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "High Cutoff", &dsp->fHslider8, (FAUSTFLOAT)0.789916f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, 0, "1", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Filter");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider4, "0", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider4, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Low Shelf", &dsp->fHslider4, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider5, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider5, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Low Gain", &dsp->fHslider5, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider3, "2", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider3, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "High Shelf", &dsp->fHslider3, (FAUSTFLOAT)0.62184876f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider6, "3", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider6, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "High Gain", &dsp->fHslider6, (FAUSTFLOAT)0.8333333f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, 0, "2", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Chorus");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider1, "0", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider1, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Amount", &dsp->fHslider1, (FAUSTFLOAT)0.01f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider2, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider2, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Rate", &dsp->fHslider2, (FAUSTFLOAT)0.1f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, 0, "3", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Space");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider10, "0", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider10, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Pre-Delay", &dsp->fHslider10, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider7, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider7, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Decay Time", &dsp->fHslider7, (FAUSTFLOAT)0.5f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider0, "2", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider0, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Size", &dsp->fHslider0, (FAUSTFLOAT)0.5f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider11, "4", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fHslider11, "style", "knob");
	ui_interface->addHorizontalSlider(ui_interface->uiInterface, "Mix", &dsp->fHslider11, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->closeBox(ui_interface->uiInterface);
}

void computemydsp(mydsp* dsp, int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
	FAUSTFLOAT* input0 = inputs[0];
	FAUSTFLOAT* input1 = inputs[1];
	FAUSTFLOAT* output0 = outputs[0];
	FAUSTFLOAT* output1 = outputs[1];
	float fSlow0 = dsp->fConst1 * (float)(dsp->fHslider0);
	float fSlow1 = dsp->fConst1 * (float)(dsp->fHslider1);
	float fSlow2 = dsp->fConst1 * (float)(dsp->fHslider2);
	float fSlow3 = dsp->fConst1 * (float)(dsp->fHslider3);
	float fSlow4 = dsp->fConst1 * (float)(dsp->fHslider4);
	float fSlow5 = dsp->fConst1 * (float)(dsp->fHslider5);
	float fSlow6 = dsp->fConst1 * (float)(dsp->fHslider6);
	float fSlow7 = dsp->fConst1 * (float)(dsp->fHslider7);
	float fSlow8 = dsp->fConst1 * (float)(dsp->fHslider8);
	float fSlow9 = dsp->fConst1 * (float)(dsp->fHslider9);
	float fSlow10 = dsp->fConst1 * (float)(dsp->fHslider10);
	float fSlow11 = dsp->fConst1 * (float)(dsp->fHslider11);
	/* C99 loop */
	{
		int i0;
		for (i0 = 0; i0 < count; i0 = i0 + 1) {
			dsp->iVec0[0] = 1;
			dsp->fRec18[0] = fSlow0 + dsp->fConst2 * dsp->fRec18[1];
			float fTemp0 = powf(2.0f, 4.0f * fmaxf(0.0f, fminf(1.0f, fmaxf(0.0f, fminf(1.0f, dsp->fRec18[0])))) + -3.0f);
			dsp->fRec19[0] = fSlow1 + dsp->fConst2 * dsp->fRec19[1];
			float fTemp1 = mydsp_faustpower2_f(fmaxf(0.0f, fminf(1.0f, dsp->fRec19[0])));
			int iTemp2 = 1 - dsp->iVec0[1];
			dsp->fRec21[0] = fSlow2 + dsp->fConst2 * dsp->fRec21[1];
			float fTemp3 = dsp->fConst3 * fminf(16.0f, expf(11.0f * dsp->fRec21[0] + -8.0f));
			float fTemp4 = ((iTemp2) ? 0.25f : dsp->fRec20[1] + fTemp3);
			dsp->fRec20[0] = fTemp4 - floorf(fTemp4);
			float fTemp5 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec20[0]);
			float fTemp6 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp5 + 0.15313378f));
			float fTemp7 = fTemp6 + -0.999995f;
			float fTemp8 = floorf(fTemp7);
			float fTemp9 = fTemp6 - fTemp8;
			float fTemp10 = fTemp6 + (-1.0f - fTemp8);
			float fTemp11 = fTemp9 * fTemp10;
			float fTemp12 = fTemp6 + (-2.0f - fTemp8);
			dsp->fRec23[0] = fSlow3 + dsp->fConst2 * dsp->fRec23[1];
			float fTemp13 = tanf(dsp->fConst4 * powf(2.0f, 0.083333336f * (119.0f * fmaxf(0.0f, fminf(1.0f, dsp->fRec23[0])) + -53.0f)));
			float fTemp14 = 1.0f / fTemp13;
			float fTemp15 = 1.0f - fTemp14;
			dsp->fRec25[0] = fSlow4 + dsp->fConst2 * dsp->fRec25[1];
			float fTemp16 = tanf(dsp->fConst4 * powf(2.0f, 0.083333336f * (119.0f * fmaxf(0.0f, fminf(1.0f, dsp->fRec25[0])) + -53.0f)));
			float fTemp17 = 1.0f / fTemp16;
			float fTemp18 = 1.0f - fTemp17;
			float fTemp19 = fTemp17 + 1.0f;
			dsp->fRec24[0] = -((dsp->fRec24[1] * fTemp18 - (dsp->fRec0[1] - dsp->fRec0[2]) / fTemp16) / fTemp19);
			dsp->fRec26[0] = -((fTemp18 * dsp->fRec26[1] - (dsp->fRec0[1] + dsp->fRec0[2])) / fTemp19);
			dsp->fRec27[0] = fSlow5 + dsp->fConst2 * dsp->fRec27[1];
			float fTemp20 = powf(1e+01f, -(1.2f * (1.0f - fmaxf(0.0f, fminf(1.0f, dsp->fRec27[0])))));
			float fTemp21 = dsp->fRec24[0] + dsp->fRec26[0] * fTemp20;
			dsp->fVec1[0] = fTemp21;
			float fTemp22 = fTemp14 + 1.0f;
			dsp->fRec22[0] = -((fTemp15 * dsp->fRec22[1] - (fTemp21 + dsp->fVec1[1])) / fTemp22);
			dsp->fRec28[0] = -((dsp->fRec28[1] * fTemp15 - (fTemp21 - dsp->fVec1[1]) / fTemp13) / fTemp22);
			dsp->fRec29[0] = fSlow6 + dsp->fConst2 * dsp->fRec29[1];
			float fTemp23 = powf(1e+01f, -(1.2f * (1.0f - fmaxf(0.0f, fminf(1.0f, dsp->fRec29[0])))));
			dsp->fRec30[0] = fSlow7 + dsp->fConst2 * dsp->fRec30[1];
			float fTemp24 = fTemp0 / fmaxf(0.1f, fminf(1e+02f, expf(12.0f * dsp->fRec30[0] + -6.0f)));
			float fTemp25 = (dsp->fRec22[0] + dsp->fRec28[0] * fTemp23) * powf(0.001f, 0.15313378f * fTemp24);
			dsp->fVec2[dsp->IOTA0 & 131071] = fTemp25;
			int iTemp26 = (int)(fTemp7);
			dsp->fRec32[0] = fSlow8 + dsp->fConst2 * dsp->fRec32[1];
			float fTemp27 = 1.0f / tanf(dsp->fConst4 * powf(2.0f, 0.083333336f * (119.0f * fmaxf(0.0f, fminf(1.0f, dsp->fRec32[0])) + -53.0f)));
			float fTemp28 = 1.0f - fTemp27;
			dsp->fRec34[0] = fSlow9 + dsp->fConst2 * dsp->fRec34[1];
			float fTemp29 = tanf(dsp->fConst4 * powf(2.0f, 0.083333336f * (119.0f * fmaxf(0.0f, fminf(1.0f, dsp->fRec34[0])) + -53.0f)));
			float fTemp30 = 1.0f / fTemp29;
			float fTemp31 = 1.0f - fTemp30;
			dsp->fRec35[0] = fSlow10 + dsp->fConst2 * dsp->fRec35[1];
			float fTemp32 = fmaxf(1.0f, dsp->fConst6 * fmaxf(0.0f, fminf(1.0f, dsp->fRec35[0])));
			float fTemp33 = fTemp32 + -0.999995f;
			float fTemp34 = floorf(fTemp33);
			float fTemp35 = fTemp32 + (-3.0f - fTemp34);
			float fTemp36 = fTemp32 + (-2.0f - fTemp34);
			float fTemp37 = fTemp32 - fTemp34;
			float fTemp38 = (float)(input0[i0]);
			dsp->fVec3[dsp->IOTA0 & 65535] = fTemp38;
			int iTemp39 = (int)(fTemp33);
			int iTemp40 = (int)(fminf(dsp->fConst6, (float)(max(0, iTemp39 + 1))));
			int iTemp41 = (int)(fminf(dsp->fConst6, (float)(max(0, iTemp39))));
			float fTemp42 = fTemp32 + (-1.0f - fTemp34);
			float fTemp43 = fTemp37 * fTemp42;
			int iTemp44 = (int)(fminf(dsp->fConst6, (float)(max(0, iTemp39 + 2))));
			float fTemp45 = fTemp43 * fTemp36;
			int iTemp46 = (int)(fminf(dsp->fConst6, (float)(max(0, iTemp39 + 3))));
			float fTemp47 = fTemp35 * (fTemp36 * (0.5f * fTemp37 * dsp->fVec3[(dsp->IOTA0 - iTemp40) & 65535] - 0.16666667f * dsp->fVec3[(dsp->IOTA0 - iTemp41) & 65535] * fTemp42) - 0.5f * fTemp43 * dsp->fVec3[(dsp->IOTA0 - iTemp44) & 65535]) + 0.16666667f * fTemp45 * dsp->fVec3[(dsp->IOTA0 - iTemp46) & 65535];
			dsp->fVec4[0] = fTemp47;
			float fTemp48 = fTemp30 + 1.0f;
			dsp->fRec33[0] = -((dsp->fRec33[1] * fTemp31 - (fTemp47 - dsp->fVec4[1]) / fTemp29) / fTemp48);
			float fTemp49 = fTemp27 + 1.0f;
			dsp->fRec31[0] = -((dsp->fRec31[1] * fTemp28 - (dsp->fRec33[0] + dsp->fRec33[1])) / fTemp49);
			float fTemp50 = 0.25f * dsp->fRec31[0];
			float fTemp51 = 0.16666667f * fTemp11 * fTemp12 * dsp->fVec2[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp26 + 3))))) & 131071] + (fTemp6 + (-3.0f - fTemp8)) * (fTemp12 * (0.5f * fTemp9 * dsp->fVec2[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp26 + 1))))) & 131071] - 0.16666667f * dsp->fVec2[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp26))))) & 131071] * fTemp10) - 0.5f * fTemp11 * dsp->fVec2[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp26 + 2))))) & 131071]) + fTemp50 - 0.6f * dsp->fRec16[1];
			dsp->fVec5[dsp->IOTA0 & 1023] = fTemp51;
			dsp->fRec16[0] = dsp->fVec5[(dsp->IOTA0 - 1000) & 1023];
			float fRec17 = 0.6f * fTemp51;
			float fTemp52 = ((iTemp2) ? 0.1875f : fTemp3 + dsp->fRec38[1]);
			dsp->fRec38[0] = fTemp52 - floorf(fTemp52);
			float fTemp53 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec38[0]);
			float fTemp54 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.12767006f - fTemp53));
			float fTemp55 = fTemp54 + -0.999995f;
			float fTemp56 = floorf(fTemp55);
			float fTemp57 = fTemp54 - fTemp56;
			float fTemp58 = fTemp54 + (-1.0f - fTemp56);
			float fTemp59 = fTemp57 * fTemp58;
			float fTemp60 = fTemp54 + (-2.0f - fTemp56);
			dsp->fRec40[0] = -((fTemp18 * dsp->fRec40[1] - (dsp->fRec15[1] - dsp->fRec15[2]) / fTemp16) / fTemp19);
			dsp->fRec41[0] = -((fTemp18 * dsp->fRec41[1] - (dsp->fRec15[1] + dsp->fRec15[2])) / fTemp19);
			float fTemp61 = dsp->fRec40[0] + dsp->fRec41[0] * fTemp20;
			dsp->fVec6[0] = fTemp61;
			dsp->fRec39[0] = -((fTemp15 * dsp->fRec39[1] - (fTemp61 + dsp->fVec6[1])) / fTemp22);
			dsp->fRec42[0] = -((fTemp15 * dsp->fRec42[1] - (fTemp61 - dsp->fVec6[1]) / fTemp13) / fTemp22);
			float fTemp62 = (dsp->fRec39[0] + dsp->fRec42[0] * fTemp23) * powf(0.001f, 0.12767006f * fTemp24);
			dsp->fVec7[dsp->IOTA0 & 131071] = fTemp62;
			int iTemp63 = (int)(fTemp55);
			float fTemp64 = (float)(input1[i0]);
			dsp->fVec8[dsp->IOTA0 & 65535] = fTemp64;
			float fTemp65 = fTemp35 * (fTemp36 * (0.5f * fTemp37 * dsp->fVec8[(dsp->IOTA0 - iTemp40) & 65535] - 0.16666667f * fTemp42 * dsp->fVec8[(dsp->IOTA0 - iTemp41) & 65535]) - 0.5f * fTemp43 * dsp->fVec8[(dsp->IOTA0 - iTemp44) & 65535]) + 0.16666667f * fTemp45 * dsp->fVec8[(dsp->IOTA0 - iTemp46) & 65535];
			dsp->fVec9[0] = fTemp65;
			dsp->fRec44[0] = -((fTemp31 * dsp->fRec44[1] - (fTemp65 - dsp->fVec9[1]) / fTemp29) / fTemp48);
			dsp->fRec43[0] = -((fTemp28 * dsp->fRec43[1] - (dsp->fRec44[0] + dsp->fRec44[1])) / fTemp49);
			float fTemp66 = 0.25f * dsp->fRec43[0];
			float fTemp67 = 0.16666667f * fTemp59 * fTemp60 * dsp->fVec7[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp63 + 3))))) & 131071] + fTemp66 + (fTemp54 + (-3.0f - fTemp56)) * (fTemp60 * (0.5f * fTemp57 * dsp->fVec7[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp63 + 1))))) & 131071] - 0.16666667f * dsp->fVec7[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp63))))) & 131071] * fTemp58) - 0.5f * fTemp59 * dsp->fVec7[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp63 + 2))))) & 131071]) - 0.6f * dsp->fRec36[1];
			dsp->fVec10[dsp->IOTA0 & 1023] = fTemp67;
			dsp->fRec36[0] = dsp->fVec10[(dsp->IOTA0 - 996) & 1023];
			float fRec37 = 0.6f * fTemp67;
			float fTemp68 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp53 + 0.17490976f));
			float fTemp69 = fTemp68 + -0.999995f;
			float fTemp70 = floorf(fTemp69);
			float fTemp71 = fTemp68 - fTemp70;
			float fTemp72 = fTemp68 + (-1.0f - fTemp70);
			float fTemp73 = fTemp71 * fTemp72;
			float fTemp74 = fTemp68 + (-2.0f - fTemp70);
			dsp->fRec48[0] = -((fTemp18 * dsp->fRec48[1] - (dsp->fRec11[1] - dsp->fRec11[2]) / fTemp16) / fTemp19);
			dsp->fRec49[0] = -((fTemp18 * dsp->fRec49[1] - (dsp->fRec11[1] + dsp->fRec11[2])) / fTemp19);
			float fTemp75 = dsp->fRec48[0] + dsp->fRec49[0] * fTemp20;
			dsp->fVec11[0] = fTemp75;
			dsp->fRec47[0] = -((fTemp15 * dsp->fRec47[1] - (fTemp75 + dsp->fVec11[1])) / fTemp22);
			dsp->fRec50[0] = -((fTemp15 * dsp->fRec50[1] - (fTemp75 - dsp->fVec11[1]) / fTemp13) / fTemp22);
			float fTemp76 = (dsp->fRec47[0] + dsp->fRec50[0] * fTemp23) * powf(0.001f, 0.17490976f * fTemp24);
			dsp->fVec12[dsp->IOTA0 & 131071] = fTemp76;
			int iTemp77 = (int)(fTemp69);
			float fTemp78 = 0.16666667f * fTemp73 * fTemp74 * dsp->fVec12[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp77 + 3))))) & 131071] + fTemp66 + (fTemp68 + (-3.0f - fTemp70)) * (fTemp74 * (0.5f * fTemp71 * dsp->fVec12[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp77 + 1))))) & 131071] - 0.16666667f * dsp->fVec12[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp77))))) & 131071] * fTemp72) - 0.5f * fTemp73 * dsp->fVec12[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp77 + 2))))) & 131071]) - 0.6f * dsp->fRec45[1];
			dsp->fVec13[dsp->IOTA0 & 1023] = fTemp78;
			dsp->fRec45[0] = dsp->fVec13[(dsp->IOTA0 - 566) & 1023];
			float fRec46 = 0.6f * fTemp78;
			float fTemp79 = ((iTemp2) ? 0.4375f : fTemp3 + dsp->fRec53[1]);
			dsp->fRec53[0] = fTemp79 - floorf(fTemp79);
			float fTemp80 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec53[0]);
			float fTemp81 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.12786055f - fTemp80));
			float fTemp82 = fTemp81 + -0.999995f;
			float fTemp83 = floorf(fTemp82);
			float fTemp84 = fTemp81 - fTemp83;
			float fTemp85 = fTemp81 + (-1.0f - fTemp83);
			float fTemp86 = fTemp84 * fTemp85;
			float fTemp87 = fTemp81 + (-2.0f - fTemp83);
			dsp->fRec55[0] = -((fTemp18 * dsp->fRec55[1] - (dsp->fRec7[1] - dsp->fRec7[2]) / fTemp16) / fTemp19);
			dsp->fRec56[0] = -((fTemp18 * dsp->fRec56[1] - (dsp->fRec7[1] + dsp->fRec7[2])) / fTemp19);
			float fTemp88 = dsp->fRec55[0] + dsp->fRec56[0] * fTemp20;
			dsp->fVec14[0] = fTemp88;
			dsp->fRec54[0] = -((fTemp15 * dsp->fRec54[1] - (fTemp88 + dsp->fVec14[1])) / fTemp22);
			dsp->fRec57[0] = -((fTemp15 * dsp->fRec57[1] - (fTemp88 - dsp->fVec14[1]) / fTemp13) / fTemp22);
			float fTemp89 = (dsp->fRec54[0] + dsp->fRec57[0] * fTemp23) * powf(0.001f, 0.12786055f * fTemp24);
			dsp->fVec15[dsp->IOTA0 & 131071] = fTemp89;
			int iTemp90 = (int)(fTemp82);
			float fTemp91 = 0.16666667f * fTemp86 * fTemp87 * dsp->fVec15[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp90 + 3))))) & 131071] + fTemp66 + (fTemp81 + (-3.0f - fTemp83)) * (fTemp87 * (0.5f * fTemp84 * dsp->fVec15[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp90 + 1))))) & 131071] - 0.16666667f * dsp->fVec15[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp90))))) & 131071] * fTemp85) - 0.5f * fTemp86 * dsp->fVec15[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp90 + 2))))) & 131071]) - 0.6f * dsp->fRec51[1];
			dsp->fVec16[dsp->IOTA0 & 1023] = fTemp91;
			dsp->fRec51[0] = dsp->fVec16[(dsp->IOTA0 - 852) & 1023];
			float fRec52 = 0.6f * fTemp91;
			float fTemp92 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp80 + 0.25688207f));
			float fTemp93 = fTemp92 + -0.999995f;
			float fTemp94 = floorf(fTemp93);
			float fTemp95 = fTemp92 - fTemp94;
			float fTemp96 = fTemp92 + (-1.0f - fTemp94);
			float fTemp97 = fTemp95 * fTemp96;
			float fTemp98 = fTemp92 + (-2.0f - fTemp94);
			dsp->fRec61[0] = -((fTemp18 * dsp->fRec61[1] - (dsp->fRec3[1] - dsp->fRec3[2]) / fTemp16) / fTemp19);
			dsp->fRec62[0] = -((fTemp18 * dsp->fRec62[1] - (dsp->fRec3[1] + dsp->fRec3[2])) / fTemp19);
			float fTemp99 = dsp->fRec61[0] + dsp->fRec62[0] * fTemp20;
			dsp->fVec17[0] = fTemp99;
			dsp->fRec60[0] = -((fTemp15 * dsp->fRec60[1] - (fTemp99 + dsp->fVec17[1])) / fTemp22);
			dsp->fRec63[0] = -((fTemp15 * dsp->fRec63[1] - (fTemp99 - dsp->fVec17[1]) / fTemp13) / fTemp22);
			float fTemp100 = (dsp->fRec60[0] + dsp->fRec63[0] * fTemp23) * powf(0.001f, 0.25688207f * fTemp24);
			dsp->fVec18[dsp->IOTA0 & 131071] = fTemp100;
			int iTemp101 = (int)(fTemp93);
			float fTemp102 = 0.16666667f * fTemp97 * fTemp98 * dsp->fVec18[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp101 + 3))))) & 131071] + fTemp66 + (fTemp92 + (-3.0f - fTemp94)) * (fTemp98 * (0.5f * fTemp95 * dsp->fVec18[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp101 + 1))))) & 131071] - 0.16666667f * dsp->fVec18[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp101))))) & 131071] * fTemp96) - 0.5f * fTemp97 * dsp->fVec18[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp101 + 2))))) & 131071]) - 0.6f * dsp->fRec58[1];
			dsp->fVec19[dsp->IOTA0 & 1023] = fTemp102;
			dsp->fRec58[0] = dsp->fVec19[(dsp->IOTA0 - 875) & 1023];
			float fRec59 = 0.6f * fTemp102;
			float fTemp103 = ((iTemp2) ? 0.125f : fTemp3 + dsp->fRec66[1]);
			dsp->fRec66[0] = fTemp103 - floorf(fTemp103);
			float fTemp104 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec66[0]);
			float fTemp105 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.11940046f - fTemp104));
			float fTemp106 = fTemp105 + -0.999995f;
			float fTemp107 = floorf(fTemp106);
			float fTemp108 = fTemp105 - fTemp107;
			float fTemp109 = fTemp105 + (-1.0f - fTemp107);
			float fTemp110 = fTemp108 * fTemp109;
			float fTemp111 = fTemp105 + (-2.0f - fTemp107);
			dsp->fRec68[0] = -((fTemp18 * dsp->fRec68[1] - (dsp->fRec14[1] - dsp->fRec14[2]) / fTemp16) / fTemp19);
			dsp->fRec69[0] = -((fTemp18 * dsp->fRec69[1] - (dsp->fRec14[1] + dsp->fRec14[2])) / fTemp19);
			float fTemp112 = dsp->fRec68[0] + dsp->fRec69[0] * fTemp20;
			dsp->fVec20[0] = fTemp112;
			dsp->fRec67[0] = -((fTemp15 * dsp->fRec67[1] - (fTemp112 + dsp->fVec20[1])) / fTemp22);
			dsp->fRec70[0] = -((fTemp15 * dsp->fRec70[1] - (fTemp112 - dsp->fVec20[1]) / fTemp13) / fTemp22);
			float fTemp113 = (dsp->fRec67[0] + dsp->fRec70[0] * fTemp23) * powf(0.001f, 0.11940046f * fTemp24);
			dsp->fVec21[dsp->IOTA0 & 131071] = fTemp113;
			int iTemp114 = (int)(fTemp106);
			float fTemp115 = 0.16666667f * fTemp110 * fTemp111 * dsp->fVec21[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp114 + 3))))) & 131071] + fTemp50 + (fTemp105 + (-3.0f - fTemp107)) * (fTemp111 * (0.5f * fTemp108 * dsp->fVec21[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp114 + 1))))) & 131071] - 0.16666667f * dsp->fVec21[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp114))))) & 131071] * fTemp109) - 0.5f * fTemp110 * dsp->fVec21[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp114 + 2))))) & 131071]) - 0.6f * dsp->fRec64[1];
			dsp->fVec22[dsp->IOTA0 & 1023] = fTemp115;
			dsp->fRec64[0] = dsp->fVec22[(dsp->IOTA0 - 662) & 1023];
			float fRec65 = 0.6f * fTemp115;
			float fTemp116 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp104 + 0.08223061f));
			float fTemp117 = fTemp116 + -0.999995f;
			float fTemp118 = floorf(fTemp117);
			float fTemp119 = fTemp116 - fTemp118;
			float fTemp120 = fTemp116 + (-1.0f - fTemp118);
			float fTemp121 = fTemp119 * fTemp120;
			float fTemp122 = fTemp116 + (-2.0f - fTemp118);
			dsp->fRec74[0] = -((fTemp18 * dsp->fRec74[1] - (dsp->fRec10[1] - dsp->fRec10[2]) / fTemp16) / fTemp19);
			dsp->fRec75[0] = -((fTemp18 * dsp->fRec75[1] - (dsp->fRec10[1] + dsp->fRec10[2])) / fTemp19);
			float fTemp123 = dsp->fRec74[0] + dsp->fRec75[0] * fTemp20;
			dsp->fVec23[0] = fTemp123;
			dsp->fRec73[0] = -((fTemp15 * dsp->fRec73[1] - (fTemp123 + dsp->fVec23[1])) / fTemp22);
			dsp->fRec76[0] = -((fTemp15 * dsp->fRec76[1] - (fTemp123 - dsp->fVec23[1]) / fTemp13) / fTemp22);
			float fTemp124 = (dsp->fRec73[0] + dsp->fRec76[0] * fTemp23) * powf(0.001f, 0.08223061f * fTemp24);
			dsp->fVec24[dsp->IOTA0 & 65535] = fTemp124;
			int iTemp125 = (int)(fTemp117);
			float fTemp126 = 0.16666667f * fTemp121 * fTemp122 * dsp->fVec24[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp125 + 3))))) & 65535] + fTemp50 + (fTemp116 + (-3.0f - fTemp118)) * (fTemp122 * (0.5f * fTemp119 * dsp->fVec24[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp125 + 1))))) & 65535] - 0.16666667f * dsp->fVec24[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp125))))) & 65535] * fTemp120) - 0.5f * fTemp121 * dsp->fVec24[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp125 + 2))))) & 65535]) - 0.6f * dsp->fRec71[1];
			dsp->fVec25[dsp->IOTA0 & 1023] = fTemp126;
			dsp->fRec71[0] = dsp->fVec25[(dsp->IOTA0 - 710) & 1023];
			float fRec72 = 0.6f * fTemp126;
			float fTemp127 = ((iTemp2) ? 0.375f : fTemp3 + dsp->fRec79[1]);
			dsp->fRec79[0] = fTemp127 - floorf(fTemp127);
			float fTemp128 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec79[0]);
			float fTemp129 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.19230045f - fTemp128));
			float fTemp130 = fTemp129 + -0.999995f;
			float fTemp131 = floorf(fTemp130);
			float fTemp132 = fTemp129 - fTemp131;
			float fTemp133 = fTemp129 + (-1.0f - fTemp131);
			float fTemp134 = fTemp132 * fTemp133;
			float fTemp135 = fTemp129 + (-2.0f - fTemp131);
			dsp->fRec81[0] = -((fTemp18 * dsp->fRec81[1] - (dsp->fRec6[1] - dsp->fRec6[2]) / fTemp16) / fTemp19);
			dsp->fRec82[0] = -((fTemp18 * dsp->fRec82[1] - (dsp->fRec6[1] + dsp->fRec6[2])) / fTemp19);
			float fTemp136 = dsp->fRec81[0] + dsp->fRec82[0] * fTemp20;
			dsp->fVec26[0] = fTemp136;
			dsp->fRec80[0] = -((fTemp15 * dsp->fRec80[1] - (fTemp136 + dsp->fVec26[1])) / fTemp22);
			dsp->fRec83[0] = -((fTemp15 * dsp->fRec83[1] - (fTemp136 - dsp->fVec26[1]) / fTemp13) / fTemp22);
			float fTemp137 = (dsp->fRec80[0] + dsp->fRec83[0] * fTemp23) * powf(0.001f, 0.19230045f * fTemp24);
			dsp->fVec27[dsp->IOTA0 & 131071] = fTemp137;
			int iTemp138 = (int)(fTemp130);
			float fTemp139 = 0.16666667f * fTemp134 * fTemp135 * dsp->fVec27[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp138 + 3))))) & 131071] + fTemp50 + (fTemp129 + (-3.0f - fTemp131)) * (fTemp135 * (0.5f * fTemp132 * dsp->fVec27[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp138 + 1))))) & 131071] - 0.16666667f * dsp->fVec27[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp138))))) & 131071] * fTemp133) - 0.5f * fTemp134 * dsp->fVec27[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp138 + 2))))) & 131071]) - 0.6f * dsp->fRec77[1];
			dsp->fVec28[dsp->IOTA0 & 1023] = fTemp139;
			dsp->fRec77[0] = dsp->fVec28[(dsp->IOTA0 - 906) & 1023];
			float fRec78 = 0.6f * fTemp139;
			float fTemp140 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp128 + 0.17470522f));
			float fTemp141 = fTemp140 + -0.999995f;
			float fTemp142 = floorf(fTemp141);
			float fTemp143 = fTemp140 - fTemp142;
			float fTemp144 = fTemp140 + (-1.0f - fTemp142);
			float fTemp145 = fTemp143 * fTemp144;
			float fTemp146 = fTemp140 + (-2.0f - fTemp142);
			dsp->fRec87[0] = -((fTemp18 * dsp->fRec87[1] - (dsp->fRec2[1] - dsp->fRec2[2]) / fTemp16) / fTemp19);
			dsp->fRec88[0] = -((fTemp18 * dsp->fRec88[1] - (dsp->fRec2[1] + dsp->fRec2[2])) / fTemp19);
			float fTemp147 = dsp->fRec87[0] + dsp->fRec88[0] * fTemp20;
			dsp->fVec29[0] = fTemp147;
			dsp->fRec86[0] = -((fTemp15 * dsp->fRec86[1] - (fTemp147 + dsp->fVec29[1])) / fTemp22);
			dsp->fRec89[0] = -((fTemp15 * dsp->fRec89[1] - (fTemp147 - dsp->fVec29[1]) / fTemp13) / fTemp22);
			float fTemp148 = (dsp->fRec86[0] + dsp->fRec89[0] * fTemp23) * powf(0.001f, 0.17470522f * fTemp24);
			dsp->fVec30[dsp->IOTA0 & 131071] = fTemp148;
			int iTemp149 = (int)(fTemp141);
			float fTemp150 = 0.16666667f * fTemp145 * fTemp146 * dsp->fVec30[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp149 + 3))))) & 131071] + fTemp50 + (fTemp140 + (-3.0f - fTemp142)) * (fTemp146 * (0.5f * fTemp143 * dsp->fVec30[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp149 + 1))))) & 131071] - 0.16666667f * dsp->fVec30[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp149))))) & 131071] * fTemp144) - 0.5f * fTemp145 * dsp->fVec30[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp149 + 2))))) & 131071]) - 0.6f * dsp->fRec84[1];
			dsp->fVec31[dsp->IOTA0 & 1023] = fTemp150;
			dsp->fRec84[0] = dsp->fVec31[(dsp->IOTA0 - 932) & 1023];
			float fRec85 = 0.6f * fTemp150;
			float fTemp151 = ((iTemp2) ? 0.0625f : fTemp3 + dsp->fRec92[1]);
			dsp->fRec92[0] = fTemp151 - floorf(fTemp151);
			float fTemp152 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec92[0]);
			float fTemp153 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.14782245f - fTemp152));
			float fTemp154 = fTemp153 + -0.999995f;
			float fTemp155 = floorf(fTemp154);
			float fTemp156 = fTemp153 - fTemp155;
			float fTemp157 = fTemp153 + (-1.0f - fTemp155);
			float fTemp158 = fTemp156 * fTemp157;
			float fTemp159 = fTemp153 + (-2.0f - fTemp155);
			dsp->fRec94[0] = -((fTemp18 * dsp->fRec94[1] - (dsp->fRec13[1] - dsp->fRec13[2]) / fTemp16) / fTemp19);
			dsp->fRec95[0] = -((fTemp18 * dsp->fRec95[1] - (dsp->fRec13[1] + dsp->fRec13[2])) / fTemp19);
			float fTemp160 = dsp->fRec94[0] + dsp->fRec95[0] * fTemp20;
			dsp->fVec32[0] = fTemp160;
			dsp->fRec93[0] = -((fTemp15 * dsp->fRec93[1] - (fTemp160 + dsp->fVec32[1])) / fTemp22);
			dsp->fRec96[0] = -((fTemp15 * dsp->fRec96[1] - (fTemp160 - dsp->fVec32[1]) / fTemp13) / fTemp22);
			float fTemp161 = (dsp->fRec93[0] + dsp->fRec96[0] * fTemp23) * powf(0.001f, 0.14782245f * fTemp24);
			dsp->fVec33[dsp->IOTA0 & 131071] = fTemp161;
			int iTemp162 = (int)(fTemp154);
			float fTemp163 = 0.16666667f * fTemp158 * fTemp159 * dsp->fVec33[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp162 + 3))))) & 131071] + fTemp66 + (fTemp153 + (-3.0f - fTemp155)) * (fTemp159 * (0.5f * fTemp156 * dsp->fVec33[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp162 + 1))))) & 131071] - 0.16666667f * dsp->fVec33[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp162))))) & 131071] * fTemp157) - 0.5f * fTemp158 * dsp->fVec33[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp162 + 2))))) & 131071]) - 0.6f * dsp->fRec90[1];
			dsp->fVec34[dsp->IOTA0 & 1023] = fTemp163;
			dsp->fRec90[0] = dsp->fVec34[(dsp->IOTA0 - 778) & 1023];
			float fRec91 = 0.6f * fTemp163;
			float fTemp164 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp152 + 0.07776644f));
			float fTemp165 = fTemp164 + -0.999995f;
			float fTemp166 = floorf(fTemp165);
			float fTemp167 = fTemp164 - fTemp166;
			float fTemp168 = fTemp164 + (-1.0f - fTemp166);
			float fTemp169 = fTemp167 * fTemp168;
			float fTemp170 = fTemp164 + (-2.0f - fTemp166);
			dsp->fRec100[0] = -((fTemp18 * dsp->fRec100[1] - (dsp->fRec9[1] - dsp->fRec9[2]) / fTemp16) / fTemp19);
			dsp->fRec101[0] = -((fTemp18 * dsp->fRec101[1] - (dsp->fRec9[1] + dsp->fRec9[2])) / fTemp19);
			float fTemp171 = dsp->fRec100[0] + dsp->fRec101[0] * fTemp20;
			dsp->fVec35[0] = fTemp171;
			dsp->fRec99[0] = -((fTemp15 * dsp->fRec99[1] - (fTemp171 + dsp->fVec35[1])) / fTemp22);
			dsp->fRec102[0] = -((fTemp15 * dsp->fRec102[1] - (fTemp171 - dsp->fVec35[1]) / fTemp13) / fTemp22);
			float fTemp172 = (dsp->fRec99[0] + dsp->fRec102[0] * fTemp23) * powf(0.001f, 0.07776644f * fTemp24);
			dsp->fVec36[dsp->IOTA0 & 65535] = fTemp172;
			int iTemp173 = (int)(fTemp165);
			float fTemp174 = 0.16666667f * fTemp169 * fTemp170 * dsp->fVec36[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp173 + 3))))) & 65535] + fTemp66 + (fTemp164 + (-3.0f - fTemp166)) * (fTemp170 * (0.5f * fTemp167 * dsp->fVec36[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp173 + 1))))) & 65535] - 0.16666667f * dsp->fVec36[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp173))))) & 65535] * fTemp168) - 0.5f * fTemp169 * dsp->fVec36[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp173 + 2))))) & 65535]) - 0.6f * dsp->fRec97[1];
			dsp->fVec37[dsp->IOTA0 & 1023] = fTemp174;
			dsp->fRec97[0] = dsp->fVec37[(dsp->IOTA0 - 1001) & 1023];
			float fRec98 = 0.6f * fTemp174;
			float fTemp175 = ((iTemp2) ? 0.3125f : fTemp3 + dsp->fRec105[1]);
			dsp->fRec105[0] = fTemp175 - floorf(fTemp175);
			float fTemp176 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec105[0]);
			float fTemp177 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.125f - fTemp176));
			float fTemp178 = fTemp177 + -0.999995f;
			float fTemp179 = floorf(fTemp178);
			float fTemp180 = fTemp177 - fTemp179;
			float fTemp181 = fTemp177 + (-1.0f - fTemp179);
			float fTemp182 = fTemp180 * fTemp181;
			float fTemp183 = fTemp177 + (-2.0f - fTemp179);
			dsp->fRec107[0] = -((fTemp18 * dsp->fRec107[1] - (dsp->fRec5[1] - dsp->fRec5[2]) / fTemp16) / fTemp19);
			dsp->fRec108[0] = -((fTemp18 * dsp->fRec108[1] - (dsp->fRec5[1] + dsp->fRec5[2])) / fTemp19);
			float fTemp184 = dsp->fRec107[0] + dsp->fRec108[0] * fTemp20;
			dsp->fVec38[0] = fTemp184;
			dsp->fRec106[0] = -((fTemp15 * dsp->fRec106[1] - (fTemp184 + dsp->fVec38[1])) / fTemp22);
			dsp->fRec109[0] = -((fTemp15 * dsp->fRec109[1] - (fTemp184 - dsp->fVec38[1]) / fTemp13) / fTemp22);
			float fTemp185 = (dsp->fRec106[0] + dsp->fRec109[0] * fTemp23) * powf(0.001f, 0.125f * fTemp24);
			dsp->fVec39[dsp->IOTA0 & 131071] = fTemp185;
			int iTemp186 = (int)(fTemp178);
			float fTemp187 = 0.16666667f * fTemp182 * fTemp183 * dsp->fVec39[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp186 + 3))))) & 131071] + fTemp66 + (fTemp177 + (-3.0f - fTemp179)) * (fTemp183 * (0.5f * fTemp180 * dsp->fVec39[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp186 + 1))))) & 131071] - 0.16666667f * dsp->fVec39[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp186))))) & 131071] * fTemp181) - 0.5f * fTemp182 * dsp->fVec39[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp186 + 2))))) & 131071]) - 0.6f * dsp->fRec103[1];
			dsp->fVec40[dsp->IOTA0 & 1023] = fTemp187;
			dsp->fRec103[0] = dsp->fVec40[(dsp->IOTA0 - 806) & 1023];
			float fRec104 = 0.6f * fTemp187;
			float fTemp188 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp176 + 0.21039456f));
			float fTemp189 = fTemp188 + -0.999995f;
			float fTemp190 = floorf(fTemp189);
			float fTemp191 = fTemp188 - fTemp190;
			float fTemp192 = fTemp188 + (-1.0f - fTemp190);
			float fTemp193 = fTemp191 * fTemp192;
			float fTemp194 = fTemp188 + (-2.0f - fTemp190);
			dsp->fRec113[0] = -((fTemp18 * dsp->fRec113[1] - (dsp->fRec1[1] - dsp->fRec1[2]) / fTemp16) / fTemp19);
			dsp->fRec114[0] = -((fTemp18 * dsp->fRec114[1] - (dsp->fRec1[1] + dsp->fRec1[2])) / fTemp19);
			float fTemp195 = dsp->fRec113[0] + dsp->fRec114[0] * fTemp20;
			dsp->fVec41[0] = fTemp195;
			dsp->fRec112[0] = -((fTemp15 * dsp->fRec112[1] - (fTemp195 + dsp->fVec41[1])) / fTemp22);
			dsp->fRec115[0] = -((fTemp15 * dsp->fRec115[1] - (fTemp195 - dsp->fVec41[1]) / fTemp13) / fTemp22);
			float fTemp196 = (dsp->fRec112[0] + dsp->fRec115[0] * fTemp23) * powf(0.001f, 0.21039456f * fTemp24);
			dsp->fVec42[dsp->IOTA0 & 131071] = fTemp196;
			int iTemp197 = (int)(fTemp189);
			float fTemp198 = 0.16666667f * fTemp193 * fTemp194 * dsp->fVec42[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp197 + 3))))) & 131071] + (fTemp188 + (-3.0f - fTemp190)) * (fTemp194 * (0.5f * fTemp191 * dsp->fVec42[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp197 + 1))))) & 131071] - 0.16666667f * dsp->fVec42[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp197))))) & 131071] * fTemp192) - 0.5f * fTemp193 * dsp->fVec42[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp197 + 2))))) & 131071]) + fTemp66 - 0.6f * dsp->fRec110[1];
			dsp->fVec43[dsp->IOTA0 & 1023] = fTemp198;
			dsp->fRec110[0] = dsp->fVec43[(dsp->IOTA0 - 798) & 1023];
			float fRec111 = 0.6f * fTemp198;
			float fTemp199 = ((iTemp2) ? 0.0f : fTemp3 + dsp->fRec118[1]);
			dsp->fRec118[0] = fTemp199 - floorf(fTemp199);
			float fTemp200 = 0.05668934f * fTemp1 * sinf(6.2831855f * dsp->fRec118[0]);
			float fTemp201 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.10252925f - fTemp200));
			float fTemp202 = fTemp201 + -0.999995f;
			float fTemp203 = floorf(fTemp202);
			float fTemp204 = fTemp201 - fTemp203;
			float fTemp205 = fTemp201 + (-1.0f - fTemp203);
			float fTemp206 = fTemp204 * fTemp205;
			float fTemp207 = fTemp201 + (-2.0f - fTemp203);
			dsp->fRec120[0] = -((fTemp18 * dsp->fRec120[1] - (dsp->fRec12[1] - dsp->fRec12[2]) / fTemp16) / fTemp19);
			dsp->fRec121[0] = -((fTemp18 * dsp->fRec121[1] - (dsp->fRec12[1] + dsp->fRec12[2])) / fTemp19);
			float fTemp208 = dsp->fRec120[0] + dsp->fRec121[0] * fTemp20;
			dsp->fVec44[0] = fTemp208;
			dsp->fRec119[0] = -((fTemp15 * dsp->fRec119[1] - (fTemp208 + dsp->fVec44[1])) / fTemp22);
			dsp->fRec122[0] = -((fTemp15 * dsp->fRec122[1] - (fTemp208 - dsp->fVec44[1]) / fTemp13) / fTemp22);
			float fTemp209 = (dsp->fRec119[0] + dsp->fRec122[0] * fTemp23) * powf(0.001f, 0.10252925f * fTemp24);
			dsp->fVec45[dsp->IOTA0 & 65535] = fTemp209;
			int iTemp210 = (int)(fTemp202);
			float fTemp211 = 0.16666667f * fTemp206 * fTemp207 * dsp->fVec45[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp210 + 3))))) & 65535] + fTemp50 + (fTemp201 + (-3.0f - fTemp203)) * (fTemp207 * (0.5f * fTemp204 * dsp->fVec45[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp210 + 1))))) & 65535] - 0.16666667f * dsp->fVec45[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp210))))) & 65535] * fTemp205) - 0.5f * fTemp206 * dsp->fVec45[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp210 + 2))))) & 65535]) - 0.6f * dsp->fRec116[1];
			dsp->fVec46[dsp->IOTA0 & 1023] = fTemp211;
			dsp->fRec116[0] = dsp->fVec46[(dsp->IOTA0 - 832) & 1023];
			float fRec117 = 0.6f * fTemp211;
			float fTemp212 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (fTemp200 + 0.070764855f));
			float fTemp213 = fTemp212 + -0.999995f;
			float fTemp214 = floorf(fTemp213);
			float fTemp215 = fTemp212 - fTemp214;
			float fTemp216 = fTemp212 + (-1.0f - fTemp214);
			float fTemp217 = fTemp215 * fTemp216;
			float fTemp218 = fTemp212 + (-2.0f - fTemp214);
			dsp->fRec126[0] = -((fTemp18 * dsp->fRec126[1] - (dsp->fRec8[1] - dsp->fRec8[2]) / fTemp16) / fTemp19);
			dsp->fRec127[0] = -((fTemp18 * dsp->fRec127[1] - (dsp->fRec8[1] + dsp->fRec8[2])) / fTemp19);
			float fTemp219 = dsp->fRec126[0] + dsp->fRec127[0] * fTemp20;
			dsp->fVec47[0] = fTemp219;
			dsp->fRec125[0] = -((fTemp15 * dsp->fRec125[1] - (fTemp219 + dsp->fVec47[1])) / fTemp22);
			dsp->fRec128[0] = -((fTemp15 * dsp->fRec128[1] - (fTemp219 - dsp->fVec47[1]) / fTemp13) / fTemp22);
			float fTemp220 = (dsp->fRec125[0] + dsp->fRec128[0] * fTemp23) * powf(0.001f, 0.070764855f * fTemp24);
			dsp->fVec48[dsp->IOTA0 & 65535] = fTemp220;
			int iTemp221 = (int)(fTemp213);
			float fTemp222 = 0.16666667f * fTemp217 * fTemp218 * dsp->fVec48[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp221 + 3))))) & 65535] + fTemp50 + (fTemp212 + (-3.0f - fTemp214)) * (fTemp218 * (0.5f * fTemp215 * dsp->fVec48[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp221 + 1))))) & 65535] - 0.16666667f * dsp->fVec48[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp221))))) & 65535] * fTemp216) - 0.5f * fTemp217 * dsp->fVec48[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp221 + 2))))) & 65535]) - 0.6f * dsp->fRec123[1];
			dsp->fVec49[dsp->IOTA0 & 1023] = fTemp222;
			dsp->fRec123[0] = dsp->fVec49[(dsp->IOTA0 - 956) & 1023];
			float fRec124 = 0.6f * fTemp222;
			float fTemp223 = fmaxf(1.0f, dsp->fConst0 * fTemp0 * (0.21998005f - fTemp5));
			float fTemp224 = fTemp223 + -0.999995f;
			float fTemp225 = floorf(fTemp224);
			float fTemp226 = fTemp223 - fTemp225;
			float fTemp227 = fTemp223 + (-1.0f - fTemp225);
			float fTemp228 = fTemp226 * fTemp227;
			float fTemp229 = fTemp223 + (-2.0f - fTemp225);
			dsp->fRec132[0] = -((fTemp18 * dsp->fRec132[1] - (dsp->fRec4[1] - dsp->fRec4[2]) / fTemp16) / fTemp19);
			dsp->fRec133[0] = -((fTemp18 * dsp->fRec133[1] - (dsp->fRec4[1] + dsp->fRec4[2])) / fTemp19);
			float fTemp230 = dsp->fRec132[0] + dsp->fRec133[0] * fTemp20;
			dsp->fVec50[0] = fTemp230;
			dsp->fRec131[0] = -((fTemp15 * dsp->fRec131[1] - (fTemp230 + dsp->fVec50[1])) / fTemp22);
			dsp->fRec134[0] = -((fTemp15 * dsp->fRec134[1] - (fTemp230 - dsp->fVec50[1]) / fTemp13) / fTemp22);
			float fTemp231 = (dsp->fRec131[0] + dsp->fRec134[0] * fTemp23) * powf(0.001f, 0.21998005f * fTemp24);
			dsp->fVec51[dsp->IOTA0 & 131071] = fTemp231;
			int iTemp232 = (int)(fTemp224);
			float fTemp233 = 0.16666667f * fTemp228 * fTemp229 * dsp->fVec51[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp232 + 3))))) & 131071] + fTemp50 + (fTemp223 + (-3.0f - fTemp225)) * (fTemp229 * (0.5f * fTemp226 * dsp->fVec51[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp232 + 1))))) & 131071] - 0.16666667f * dsp->fVec51[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp232))))) & 131071] * fTemp227) - 0.5f * fTemp228 * dsp->fVec51[(dsp->IOTA0 - (int)(fminf(dsp->fConst5, (float)(max(0, iTemp232 + 2))))) & 131071]) - 0.6f * dsp->fRec129[1];
			dsp->fVec52[dsp->IOTA0 & 1023] = fTemp233;
			dsp->fRec129[0] = dsp->fVec52[(dsp->IOTA0 - 894) & 1023];
			float fRec130 = 0.6f * fTemp233;
			float fTemp234 = fRec117 + fRec124 + fRec17 + fRec130;
			float fTemp235 = 0.25f * (dsp->fRec36[1] + dsp->fRec45[1] + dsp->fRec51[1] + dsp->fRec58[1] + dsp->fRec64[1] + dsp->fRec71[1] + dsp->fRec77[1] + dsp->fRec84[1] + dsp->fRec90[1] + dsp->fRec97[1] + dsp->fRec103[1] + dsp->fRec110[1] + dsp->fRec116[1] + dsp->fRec123[1] + dsp->fRec129[1] + dsp->fRec16[1] + fRec37 + fRec46 + fRec52 + fRec59 + fRec65 + fRec72 + fRec78 + fRec85 + fRec91 + fRec98 + fRec104 + fRec111 + fTemp234);
			float fTemp236 = dsp->fRec116[1] + dsp->fRec123[1] + dsp->fRec129[1] + dsp->fRec16[1] + fTemp234;
			float fTemp237 = dsp->fRec58[1] + dsp->fRec84[1] + dsp->fRec110[1] + dsp->fRec16[1] + fRec59 + fRec85 + fRec17 + fRec111;
			dsp->fRec0[0] = fRec17 + dsp->fRec16[1] + fTemp235 - 0.5f * (fTemp236 + fTemp237);
			float fTemp238 = dsp->fRec90[1] + dsp->fRec97[1] + dsp->fRec103[1] + dsp->fRec110[1] + fRec91 + fRec98 + fRec111 + fRec104;
			dsp->fRec1[0] = fRec111 + dsp->fRec110[1] + fTemp235 - 0.5f * (fTemp238 + fTemp237);
			float fTemp239 = dsp->fRec64[1] + dsp->fRec71[1] + dsp->fRec77[1] + dsp->fRec84[1] + fRec65 + fRec72 + fRec85 + fRec78;
			dsp->fRec2[0] = fRec85 + dsp->fRec84[1] + fTemp235 - 0.5f * (fTemp239 + fTemp237);
			float fTemp240 = dsp->fRec36[1] + dsp->fRec45[1] + dsp->fRec51[1] + dsp->fRec58[1] + fRec37 + fRec46 + fRec59 + fRec52;
			dsp->fRec3[0] = fRec59 + dsp->fRec58[1] + fTemp235 - 0.5f * (fTemp240 + fTemp237);
			float fTemp241 = dsp->fRec51[1] + dsp->fRec77[1] + dsp->fRec103[1] + dsp->fRec129[1] + fRec52 + fRec78 + fRec130 + fRec104;
			dsp->fRec4[0] = fRec130 + dsp->fRec129[1] + fTemp235 - 0.5f * (fTemp236 + fTemp241);
			dsp->fRec5[0] = fRec104 + dsp->fRec103[1] + fTemp235 - 0.5f * (fTemp238 + fTemp241);
			dsp->fRec6[0] = fRec78 + dsp->fRec77[1] + fTemp235 - 0.5f * (fTemp239 + fTemp241);
			dsp->fRec7[0] = fRec52 + dsp->fRec51[1] + fTemp235 - 0.5f * (fTemp240 + fTemp241);
			float fTemp242 = dsp->fRec45[1] + dsp->fRec71[1] + dsp->fRec97[1] + dsp->fRec123[1] + fRec46 + fRec72 + fRec124 + fRec98;
			dsp->fRec8[0] = fRec124 + dsp->fRec123[1] + fTemp235 - 0.5f * (fTemp236 + fTemp242);
			dsp->fRec9[0] = fRec98 + dsp->fRec97[1] + fTemp235 - 0.5f * (fTemp238 + fTemp242);
			dsp->fRec10[0] = fRec72 + dsp->fRec71[1] + fTemp235 - 0.5f * (fTemp239 + fTemp242);
			dsp->fRec11[0] = fRec46 + dsp->fRec45[1] + fTemp235 - 0.5f * (fTemp240 + fTemp242);
			float fTemp243 = dsp->fRec36[1] + dsp->fRec64[1] + dsp->fRec90[1] + dsp->fRec116[1] + fRec37 + fRec65 + fRec117 + fRec91;
			dsp->fRec12[0] = fRec117 + dsp->fRec116[1] + fTemp235 - 0.5f * (fTemp236 + fTemp243);
			dsp->fRec13[0] = fRec91 + dsp->fRec90[1] + fTemp235 - 0.5f * (fTemp238 + fTemp243);
			dsp->fRec14[0] = fRec65 + dsp->fRec64[1] + fTemp235 - 0.5f * (fTemp239 + fTemp243);
			dsp->fRec15[0] = fRec37 + dsp->fRec36[1] + fTemp235 - 0.5f * (fTemp240 + fTemp243);
			dsp->fRec135[0] = fSlow11 + dsp->fConst2 * dsp->fRec135[1];
			float fTemp244 = 1.5707964f * fmaxf(0.0f, fminf(1.0f, fmaxf(0.0f, fminf(1.0f, dsp->fRec135[0]))));
			float fTemp245 = sinf(fTemp244);
			float fTemp246 = cosf(fTemp244);
			output0[i0] = (FAUSTFLOAT)(1.4142135f * (0.35355338f * (dsp->fRec1[0] + dsp->fRec3[0] + dsp->fRec5[0] + dsp->fRec7[0] + dsp->fRec9[0] + dsp->fRec11[0] + dsp->fRec13[0] + dsp->fRec15[0]) * fTemp245 + 0.70710677f * fTemp38 * fTemp246));
			output1[i0] = (FAUSTFLOAT)(1.4142135f * (0.35355338f * (dsp->fRec0[0] + dsp->fRec2[0] + dsp->fRec4[0] + dsp->fRec6[0] + dsp->fRec8[0] + dsp->fRec10[0] + dsp->fRec12[0] + dsp->fRec14[0]) * fTemp245 + 0.70710677f * fTemp64 * fTemp246));
			dsp->iVec0[1] = dsp->iVec0[0];
			dsp->fRec18[1] = dsp->fRec18[0];
			dsp->fRec19[1] = dsp->fRec19[0];
			dsp->fRec21[1] = dsp->fRec21[0];
			dsp->fRec20[1] = dsp->fRec20[0];
			dsp->fRec23[1] = dsp->fRec23[0];
			dsp->fRec25[1] = dsp->fRec25[0];
			dsp->fRec24[1] = dsp->fRec24[0];
			dsp->fRec26[1] = dsp->fRec26[0];
			dsp->fRec27[1] = dsp->fRec27[0];
			dsp->fVec1[1] = dsp->fVec1[0];
			dsp->fRec22[1] = dsp->fRec22[0];
			dsp->fRec28[1] = dsp->fRec28[0];
			dsp->fRec29[1] = dsp->fRec29[0];
			dsp->fRec30[1] = dsp->fRec30[0];
			dsp->IOTA0 = dsp->IOTA0 + 1;
			dsp->fRec32[1] = dsp->fRec32[0];
			dsp->fRec34[1] = dsp->fRec34[0];
			dsp->fRec35[1] = dsp->fRec35[0];
			dsp->fVec4[1] = dsp->fVec4[0];
			dsp->fRec33[1] = dsp->fRec33[0];
			dsp->fRec31[1] = dsp->fRec31[0];
			dsp->fRec16[1] = dsp->fRec16[0];
			dsp->fRec38[1] = dsp->fRec38[0];
			dsp->fRec40[1] = dsp->fRec40[0];
			dsp->fRec41[1] = dsp->fRec41[0];
			dsp->fVec6[1] = dsp->fVec6[0];
			dsp->fRec39[1] = dsp->fRec39[0];
			dsp->fRec42[1] = dsp->fRec42[0];
			dsp->fVec9[1] = dsp->fVec9[0];
			dsp->fRec44[1] = dsp->fRec44[0];
			dsp->fRec43[1] = dsp->fRec43[0];
			dsp->fRec36[1] = dsp->fRec36[0];
			dsp->fRec48[1] = dsp->fRec48[0];
			dsp->fRec49[1] = dsp->fRec49[0];
			dsp->fVec11[1] = dsp->fVec11[0];
			dsp->fRec47[1] = dsp->fRec47[0];
			dsp->fRec50[1] = dsp->fRec50[0];
			dsp->fRec45[1] = dsp->fRec45[0];
			dsp->fRec53[1] = dsp->fRec53[0];
			dsp->fRec55[1] = dsp->fRec55[0];
			dsp->fRec56[1] = dsp->fRec56[0];
			dsp->fVec14[1] = dsp->fVec14[0];
			dsp->fRec54[1] = dsp->fRec54[0];
			dsp->fRec57[1] = dsp->fRec57[0];
			dsp->fRec51[1] = dsp->fRec51[0];
			dsp->fRec61[1] = dsp->fRec61[0];
			dsp->fRec62[1] = dsp->fRec62[0];
			dsp->fVec17[1] = dsp->fVec17[0];
			dsp->fRec60[1] = dsp->fRec60[0];
			dsp->fRec63[1] = dsp->fRec63[0];
			dsp->fRec58[1] = dsp->fRec58[0];
			dsp->fRec66[1] = dsp->fRec66[0];
			dsp->fRec68[1] = dsp->fRec68[0];
			dsp->fRec69[1] = dsp->fRec69[0];
			dsp->fVec20[1] = dsp->fVec20[0];
			dsp->fRec67[1] = dsp->fRec67[0];
			dsp->fRec70[1] = dsp->fRec70[0];
			dsp->fRec64[1] = dsp->fRec64[0];
			dsp->fRec74[1] = dsp->fRec74[0];
			dsp->fRec75[1] = dsp->fRec75[0];
			dsp->fVec23[1] = dsp->fVec23[0];
			dsp->fRec73[1] = dsp->fRec73[0];
			dsp->fRec76[1] = dsp->fRec76[0];
			dsp->fRec71[1] = dsp->fRec71[0];
			dsp->fRec79[1] = dsp->fRec79[0];
			dsp->fRec81[1] = dsp->fRec81[0];
			dsp->fRec82[1] = dsp->fRec82[0];
			dsp->fVec26[1] = dsp->fVec26[0];
			dsp->fRec80[1] = dsp->fRec80[0];
			dsp->fRec83[1] = dsp->fRec83[0];
			dsp->fRec77[1] = dsp->fRec77[0];
			dsp->fRec87[1] = dsp->fRec87[0];
			dsp->fRec88[1] = dsp->fRec88[0];
			dsp->fVec29[1] = dsp->fVec29[0];
			dsp->fRec86[1] = dsp->fRec86[0];
			dsp->fRec89[1] = dsp->fRec89[0];
			dsp->fRec84[1] = dsp->fRec84[0];
			dsp->fRec92[1] = dsp->fRec92[0];
			dsp->fRec94[1] = dsp->fRec94[0];
			dsp->fRec95[1] = dsp->fRec95[0];
			dsp->fVec32[1] = dsp->fVec32[0];
			dsp->fRec93[1] = dsp->fRec93[0];
			dsp->fRec96[1] = dsp->fRec96[0];
			dsp->fRec90[1] = dsp->fRec90[0];
			dsp->fRec100[1] = dsp->fRec100[0];
			dsp->fRec101[1] = dsp->fRec101[0];
			dsp->fVec35[1] = dsp->fVec35[0];
			dsp->fRec99[1] = dsp->fRec99[0];
			dsp->fRec102[1] = dsp->fRec102[0];
			dsp->fRec97[1] = dsp->fRec97[0];
			dsp->fRec105[1] = dsp->fRec105[0];
			dsp->fRec107[1] = dsp->fRec107[0];
			dsp->fRec108[1] = dsp->fRec108[0];
			dsp->fVec38[1] = dsp->fVec38[0];
			dsp->fRec106[1] = dsp->fRec106[0];
			dsp->fRec109[1] = dsp->fRec109[0];
			dsp->fRec103[1] = dsp->fRec103[0];
			dsp->fRec113[1] = dsp->fRec113[0];
			dsp->fRec114[1] = dsp->fRec114[0];
			dsp->fVec41[1] = dsp->fVec41[0];
			dsp->fRec112[1] = dsp->fRec112[0];
			dsp->fRec115[1] = dsp->fRec115[0];
			dsp->fRec110[1] = dsp->fRec110[0];
			dsp->fRec118[1] = dsp->fRec118[0];
			dsp->fRec120[1] = dsp->fRec120[0];
			dsp->fRec121[1] = dsp->fRec121[0];
			dsp->fVec44[1] = dsp->fVec44[0];
			dsp->fRec119[1] = dsp->fRec119[0];
			dsp->fRec122[1] = dsp->fRec122[0];
			dsp->fRec116[1] = dsp->fRec116[0];
			dsp->fRec126[1] = dsp->fRec126[0];
			dsp->fRec127[1] = dsp->fRec127[0];
			dsp->fVec47[1] = dsp->fVec47[0];
			dsp->fRec125[1] = dsp->fRec125[0];
			dsp->fRec128[1] = dsp->fRec128[0];
			dsp->fRec123[1] = dsp->fRec123[0];
			dsp->fRec132[1] = dsp->fRec132[0];
			dsp->fRec133[1] = dsp->fRec133[0];
			dsp->fVec50[1] = dsp->fVec50[0];
			dsp->fRec131[1] = dsp->fRec131[0];
			dsp->fRec134[1] = dsp->fRec134[0];
			dsp->fRec129[1] = dsp->fRec129[0];
			dsp->fRec0[2] = dsp->fRec0[1];
			dsp->fRec0[1] = dsp->fRec0[0];
			dsp->fRec1[2] = dsp->fRec1[1];
			dsp->fRec1[1] = dsp->fRec1[0];
			dsp->fRec2[2] = dsp->fRec2[1];
			dsp->fRec2[1] = dsp->fRec2[0];
			dsp->fRec3[2] = dsp->fRec3[1];
			dsp->fRec3[1] = dsp->fRec3[0];
			dsp->fRec4[2] = dsp->fRec4[1];
			dsp->fRec4[1] = dsp->fRec4[0];
			dsp->fRec5[2] = dsp->fRec5[1];
			dsp->fRec5[1] = dsp->fRec5[0];
			dsp->fRec6[2] = dsp->fRec6[1];
			dsp->fRec6[1] = dsp->fRec6[0];
			dsp->fRec7[2] = dsp->fRec7[1];
			dsp->fRec7[1] = dsp->fRec7[0];
			dsp->fRec8[2] = dsp->fRec8[1];
			dsp->fRec8[1] = dsp->fRec8[0];
			dsp->fRec9[2] = dsp->fRec9[1];
			dsp->fRec9[1] = dsp->fRec9[0];
			dsp->fRec10[2] = dsp->fRec10[1];
			dsp->fRec10[1] = dsp->fRec10[0];
			dsp->fRec11[2] = dsp->fRec11[1];
			dsp->fRec11[1] = dsp->fRec11[0];
			dsp->fRec12[2] = dsp->fRec12[1];
			dsp->fRec12[1] = dsp->fRec12[0];
			dsp->fRec13[2] = dsp->fRec13[1];
			dsp->fRec13[1] = dsp->fRec13[0];
			dsp->fRec14[2] = dsp->fRec14[1];
			dsp->fRec14[1] = dsp->fRec14[0];
			dsp->fRec15[2] = dsp->fRec15[1];
			dsp->fRec15[1] = dsp->fRec15[0];
			dsp->fRec135[1] = dsp->fRec135[0];
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif

typedef struct {
  mydsp dsp;
  
  // Pre-filter parameters
  double low_cutoff;   // 0.0 to 1.0
  double high_cutoff;  // 0.0 to 1.0
  
  // Filter parameters
  double low_shelf;   // 0.0 to 1.0
  double low_gain;    // 0.0 to 1.0
  double high_shelf;  // 0.0 to 1.0
  double high_gain;   // 0.0 to 1.0
  
  // Chorus parameters
  double amount;     // 0.0 to 1.0
  double rate;       // 0.0 to 1.0
  
  // Space parameters
  double pre_delay;  // 0.0 to 1.0
  double decay_time; // 0.0 to 1.0
  double size;       // 0.0 to 1.0
  
  // Global parameters
  double mix;        // 0.0 to 1.0
} vital_rev_state;

// Update the DSP parameters based on node state
static void update_vital_rev_parameters(vital_rev_state *state) {
  // Pre-filter parameters
  state->dsp.fHslider9 = (FAUSTFLOAT)state->low_cutoff;    // Low Cutoff
  state->dsp.fHslider8 = (FAUSTFLOAT)state->high_cutoff;   // High Cutoff
  
  // Filter parameters
  state->dsp.fHslider4 = (FAUSTFLOAT)state->low_shelf;     // Low Shelf
  state->dsp.fHslider5 = (FAUSTFLOAT)state->low_gain;      // Low Gain
  state->dsp.fHslider3 = (FAUSTFLOAT)state->high_shelf;    // High Shelf
  state->dsp.fHslider6 = (FAUSTFLOAT)state->high_gain;     // High Gain
  
  // Chorus parameters
  state->dsp.fHslider1 = (FAUSTFLOAT)state->amount;        // Amount
  state->dsp.fHslider2 = (FAUSTFLOAT)state->rate;          // Rate
  
  // Space parameters
  state->dsp.fHslider10 = (FAUSTFLOAT)state->pre_delay;    // Pre-Delay
  state->dsp.fHslider7 = (FAUSTFLOAT)state->decay_time;    // Decay Time
  state->dsp.fHslider0 = (FAUSTFLOAT)state->size;          // Size
  
  // Global parameters
  state->dsp.fHslider11 = (FAUSTFLOAT)state->mix;          // Mix
}

// The vital_rev_perform function that will be called by the audio graph
void *__vital_rev_perform(Node *node, vital_rev_state *state, Node *inputs[],
                        int nframes, double spf) {
  double *out = node->output.buf;

  int is_stereo_input = inputs[0]->output.layout > 1;
  double *in = inputs[0]->output.buf;

  // Setup temporary buffers for FAUST processing
  FAUSTFLOAT *faust_input[2];
  FAUSTFLOAT *faust_output[2];

  FAUSTFLOAT in_buffer_left[nframes];
  FAUSTFLOAT in_buffer_right[nframes];
  FAUSTFLOAT out_buffer_left[nframes];
  FAUSTFLOAT out_buffer_right[nframes];

  // Copy input to FAUST format buffer and convert to float
  if (is_stereo_input) {
    for (int i = 0; i < nframes; i++) {
      in_buffer_left[i] = (FAUSTFLOAT)in[2 * i];
      in_buffer_right[i] = (FAUSTFLOAT)in[2 * i + 1];
    }
  } else {
    for (int i = 0; i < nframes; i++) {
      in_buffer_left[i] = (FAUSTFLOAT)in[i];
      in_buffer_right[i] = (FAUSTFLOAT)in[i];
    }
  }

  faust_input[0] = in_buffer_left;
  faust_input[1] = in_buffer_right;
  faust_output[0] = out_buffer_left;
  faust_output[1] = out_buffer_right;

  // Update parameters
  update_vital_rev_parameters(state);

  // Process the audio
  computemydsp(&state->dsp, nframes, faust_input, faust_output);

  for (int i = 0; i < nframes; i++) {
    out[2 * i] = (double)out_buffer_left[i];
    out[2 * i + 1] = (double)out_buffer_right[i];
  }

  return node->output.buf;
}

// The vital_rev_perform function that will be called by the audio graph
void *vital_rev_perform(Node *node, vital_rev_state *state, Node *inputs[],
                        int nframes, double spf) {
  double *out = node->output.buf;
  int is_stereo_input = inputs[0]->output.layout > 1;
  double *in = inputs[0]->output.buf;

  // Setup temporary buffers for FAUST processing
  FAUSTFLOAT *faust_input[2];
  FAUSTFLOAT *faust_output[2];

  FAUSTFLOAT in_buffer_left[nframes];
  FAUSTFLOAT in_buffer_right[nframes];
  FAUSTFLOAT out_buffer_left[nframes];
  FAUSTFLOAT out_buffer_right[nframes];

  // Copy input to FAUST format buffer and convert to float
  if (is_stereo_input) {
    // Extract left and right channels from interleaved stereo
    for (int i = 0; i < nframes; i++) {
      in_buffer_left[i] = (FAUSTFLOAT)in[2 * i];
      in_buffer_right[i] = (FAUSTFLOAT)in[2 * i + 1];
    }
  } else {

    // Duplicate mono input to both channels
    for (int i = 0; i < nframes; i++) {
      in_buffer_left[i] = (FAUSTFLOAT)in[i];
      in_buffer_right[i] = (FAUSTFLOAT)in[i];
    }
  }

  faust_input[0] = in_buffer_left;
  faust_input[1] = in_buffer_right;
  faust_output[0] = out_buffer_left;
  faust_output[1] = out_buffer_right;

  // Update parameters
  update_vital_rev_parameters(state);

  // Process the audio
  computemydsp(&state->dsp, nframes, faust_input, faust_output);

  // Write back to interleaved stereo output
  for (int i = 0; i < nframes; i++) {
    out[2 * i] = (double)out_buffer_left[i];
    out[2 * i + 1] = (double)out_buffer_right[i];
  }

  return node->output.buf;
}
// Create a static (fixed parameter) vital_rev node
NodeRef ___vital_rev_node(double size, double amount, double rate, double high_shelf, 
                      double low_shelf, double high_gain, double low_gain, 
                      double decay_time, double high_cutoff, double low_cutoff, 
                      double pre_delay, double mix, NodeRef input) {
  AudioGraph *graph = _graph;
  NodeRef node = allocate_node_in_graph(graph, sizeof(vital_rev_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)vital_rev_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(vital_rev_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(vital_rev_state)),
      
      // Determine if output should be stereo based on input
      .output = (Signal){.layout = 2, // Always stereo output
                         .size = BUF_SIZE * 2,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE * 2)},
      .meta = "vital_rev",
  };

  // Initialize state
  vital_rev_state *state =
      (vital_rev_state *)(graph->nodes_state_memory + node->state_offset);

  // Set initial parameter values
  state->size = size;                  // 0.0 to 1.0
  state->amount = amount;              // 0.0 to 1.0
  state->rate = rate;                  // 0.0 to 1.0
  state->high_shelf = high_shelf;      // 0.0 to 1.0
  state->low_shelf = low_shelf;        // 0.0 to 1.0
  state->high_gain = high_gain;        // 0.0 to 1.0
  state->low_gain = low_gain;          // 0.0 to 1.0
  state->decay_time = decay_time;      // 0.0 to 1.0
  state->high_cutoff = high_cutoff;    // 0.0 to 1.0
  state->low_cutoff = low_cutoff;      // 0.0 to 1.0
  state->pre_delay = pre_delay;        // 0.0 to 1.0
  state->mix = mix;                    // 0.0 to 1.0

  double spf = ctx_spf();
  // Initialize the Faust DSP
  instanceConstantsmydsp(&state->dsp, (int)(1.0 / spf));
  instanceResetUserInterfacemydsp(&state->dsp);
  instanceClearmydsp(&state->dsp);
  // update_vital_rev_parameters(state);

  // Connect input
  node->connections[0].source_node_index = input->node_index;

  return node;
}
// Create a modified vital_rev node with proper initialization
NodeRef vital_rev_node(double size, double amount, double rate, double high_shelf, 
                      double low_shelf, double high_gain, double low_gain, 
                      double decay_time, double high_cutoff, double low_cutoff, 
                      double pre_delay, double mix, NodeRef input) {
  AudioGraph *graph = _graph;
  NodeRef node = allocate_node_in_graph(graph, sizeof(vital_rev_state));
  printf("need %lu memory for state %d\n", sizeof(vital_rev_state), 1 << 16);

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)vital_rev_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(vital_rev_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(vital_rev_state)),
      
      // Always stereo output
      .output = (Signal){.layout = 2,
                         .size = BUF_SIZE * 2,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE * 2)},
      .meta = "vital_rev",
  };

  // Initialize state
  vital_rev_state *state =
      (vital_rev_state *)(graph->nodes_state_memory + node->state_offset);
  
  // Initialize all memory to zero before setting parameters
  memset(state, 0, sizeof(vital_rev_state));

  // Set initial parameter values
  state->size = size;                  // 0.0 to 1.0
  state->amount = amount;              // 0.0 to 1.0
  state->rate = rate;                  // 0.0 to 1.0
  state->high_shelf = high_shelf;      // 0.0 to 1.0
  state->low_shelf = low_shelf;        // 0.0 to 1.0
  state->high_gain = high_gain;        // 0.0 to 1.0
  state->low_gain = low_gain;          // 0.0 to 1.0
  state->decay_time = decay_time;      // 0.0 to 1.0
  state->high_cutoff = high_cutoff;    // 0.0 to 1.0
  state->low_cutoff = low_cutoff;      // 0.0 to 1.0
  state->pre_delay = pre_delay;        // 0.0 to 1.0
  state->mix = mix;                    // 0.0 to 1.0

  int sample_rate = ctx_sample_rate();
  
  // Initialize the Faust DSP properly
  instanceConstantsmydsp(&state->dsp, sample_rate);
  instanceResetUserInterfacemydsp(&state->dsp);
  instanceClearmydsp(&state->dsp);
  
  // Apply parameters right after initialization
  update_vital_rev_parameters(state);

  // Connect input
  node->connections[0].source_node_index = input->node_index;

  return node;
}
