#include "./zita_rev.h"
#include "../audio_graph.h"
#include "../ctx.h"
#include <string.h>
/* ------------------------------------------------------------
author: "JOS, Revised by RM"
name: "zitaRevFDN"
version: "0.0"
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
	int fSampleRate;
	float fConst0;
	float fConst1;
	FAUSTFLOAT fVslider0;
	float fConst2;
	float fRec0[2];
	int IOTA0;
	float fVec0[16384];
	FAUSTFLOAT fVslider1;
	float fRec1[2];
	float fConst3;
	FAUSTFLOAT fVslider2;
	FAUSTFLOAT fVslider3;
	FAUSTFLOAT fVslider4;
	FAUSTFLOAT fVslider5;
	float fConst4;
	float fConst5;
	FAUSTFLOAT fVslider6;
	FAUSTFLOAT fVslider7;
	FAUSTFLOAT fVslider8;
	float fConst6;
	FAUSTFLOAT fVslider9;
	float fRec15[2];
	float fRec14[2];
	float fVec1[32768];
	float fConst7;
	int iConst8;
	float fVec2[16384];
	float fConst9;
	FAUSTFLOAT fVslider10;
	float fVec3[2048];
	int iConst10;
	float fRec12[2];
	float fConst11;
	float fConst12;
	float fRec19[2];
	float fRec18[2];
	float fVec4[32768];
	float fConst13;
	int iConst14;
	float fVec5[4096];
	int iConst15;
	float fRec16[2];
	float fConst16;
	float fConst17;
	float fRec23[2];
	float fRec22[2];
	float fVec6[16384];
	float fConst18;
	int iConst19;
	float fVec7[4096];
	int iConst20;
	float fRec20[2];
	float fConst21;
	float fConst22;
	float fRec27[2];
	float fRec26[2];
	float fVec8[32768];
	float fConst23;
	int iConst24;
	float fVec9[4096];
	int iConst25;
	float fRec24[2];
	float fConst26;
	float fConst27;
	float fRec31[2];
	float fRec30[2];
	float fVec10[16384];
	float fConst28;
	int iConst29;
	float fVec11[2048];
	int iConst30;
	float fRec28[2];
	float fConst31;
	float fConst32;
	float fRec35[2];
	float fRec34[2];
	float fVec12[16384];
	float fConst33;
	int iConst34;
	float fVec13[4096];
	int iConst35;
	float fRec32[2];
	float fConst36;
	float fConst37;
	float fRec39[2];
	float fRec38[2];
	float fVec14[16384];
	float fConst38;
	int iConst39;
	float fVec15[4096];
	int iConst40;
	float fRec36[2];
	float fConst41;
	float fConst42;
	float fRec43[2];
	float fRec42[2];
	float fVec16[16384];
	float fConst43;
	int iConst44;
	float fVec17[2048];
	int iConst45;
	float fRec40[2];
	float fRec4[3];
	float fRec5[3];
	float fRec6[3];
	float fRec7[3];
	float fRec8[3];
	float fRec9[3];
	float fRec10[3];
	float fRec11[3];
	float fRec3[3];
	float fRec2[3];
	float fRec45[3];
	float fRec44[3];
} mydsp;

mydsp* newmydsp() { 
	mydsp* dsp = (mydsp*)calloc(1, sizeof(mydsp));
	return dsp;
}

void deletemydsp(mydsp* dsp) { 
	free(dsp);
}

void metadatamydsp(MetaGlue* m) { 
	m->declare(m->metaInterface, "author", "JOS, Revised by RM");
	m->declare(m->metaInterface, "basics.lib/name", "Faust Basic Element Library");
	m->declare(m->metaInterface, "basics.lib/version", "1.21.0");
	m->declare(m->metaInterface, "compile_options", "-a pure.c -lang c -ct 1 -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
	m->declare(m->metaInterface, "delays.lib/name", "Faust Delay Library");
	m->declare(m->metaInterface, "delays.lib/version", "1.1.0");
	m->declare(m->metaInterface, "demos.lib/name", "Faust Demos Library");
	m->declare(m->metaInterface, "demos.lib/version", "1.2.0");
	m->declare(m->metaInterface, "demos.lib/zita_rev1:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "demos.lib/zita_rev1:licence", "MIT");
	m->declare(m->metaInterface, "description", "Reverb demo application based on `zita_rev_fdn`.");
	m->declare(m->metaInterface, "filename", "zita_rev.dsp");
	m->declare(m->metaInterface, "filters.lib/allpass_comb:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/allpass_comb:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/allpass_comb:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/fir:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/fir:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/iir:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/iir:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/lowpass:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/lowpass:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/name", "Faust Filters Library");
	m->declare(m->metaInterface, "filters.lib/peak_eq_rm:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/peak_eq_rm:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/peak_eq_rm:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/tf1:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/tf1:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/tf1s:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/tf1s:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/tf2:author", "Julius O. Smith III");
	m->declare(m->metaInterface, "filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
	m->declare(m->metaInterface, "filters.lib/tf2:license", "MIT-style STK-4.3 license");
	m->declare(m->metaInterface, "filters.lib/version", "1.7.1");
	m->declare(m->metaInterface, "maths.lib/author", "GRAME");
	m->declare(m->metaInterface, "maths.lib/copyright", "GRAME");
	m->declare(m->metaInterface, "maths.lib/license", "LGPL with exception");
	m->declare(m->metaInterface, "maths.lib/name", "Faust Math Library");
	m->declare(m->metaInterface, "maths.lib/version", "2.8.1");
	m->declare(m->metaInterface, "name", "zitaRevFDN");
	m->declare(m->metaInterface, "platform.lib/name", "Generic Platform Library");
	m->declare(m->metaInterface, "platform.lib/version", "1.3.0");
	m->declare(m->metaInterface, "reverbs.lib/name", "Faust Reverb Library");
	m->declare(m->metaInterface, "reverbs.lib/version", "1.4.0");
	m->declare(m->metaInterface, "routes.lib/hadamard:author", "Remy Muller, revised by Romain Michon");
	m->declare(m->metaInterface, "routes.lib/name", "Faust Signal Routing Library");
	m->declare(m->metaInterface, "routes.lib/version", "1.2.0");
	m->declare(m->metaInterface, "signals.lib/name", "Faust Signal Routing Library");
	m->declare(m->metaInterface, "signals.lib/version", "1.6.0");
	m->declare(m->metaInterface, "version", "0.0");
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
	dsp->fVslider0 = (FAUSTFLOAT)(-2e+01f);
	dsp->fVslider1 = (FAUSTFLOAT)(0.0f);
	dsp->fVslider2 = (FAUSTFLOAT)(1.5e+03f);
	dsp->fVslider3 = (FAUSTFLOAT)(0.0f);
	dsp->fVslider4 = (FAUSTFLOAT)(315.0f);
	dsp->fVslider5 = (FAUSTFLOAT)(0.0f);
	dsp->fVslider6 = (FAUSTFLOAT)(2.0f);
	dsp->fVslider7 = (FAUSTFLOAT)(6e+03f);
	dsp->fVslider8 = (FAUSTFLOAT)(3.0f);
	dsp->fVslider9 = (FAUSTFLOAT)(2e+02f);
	dsp->fVslider10 = (FAUSTFLOAT)(6e+01f);
}

void instanceClearmydsp(mydsp* dsp) {
	/* C99 loop */
	{
		int l0;
		for (l0 = 0; l0 < 2; l0 = l0 + 1) {
			dsp->fRec0[l0] = 0.0f;
		}
	}
	dsp->IOTA0 = 0;
	/* C99 loop */
	{
		int l1;
		for (l1 = 0; l1 < 16384; l1 = l1 + 1) {
			dsp->fVec0[l1] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l2;
		for (l2 = 0; l2 < 2; l2 = l2 + 1) {
			dsp->fRec1[l2] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l3;
		for (l3 = 0; l3 < 2; l3 = l3 + 1) {
			dsp->fRec15[l3] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l4;
		for (l4 = 0; l4 < 2; l4 = l4 + 1) {
			dsp->fRec14[l4] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l5;
		for (l5 = 0; l5 < 32768; l5 = l5 + 1) {
			dsp->fVec1[l5] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l6;
		for (l6 = 0; l6 < 16384; l6 = l6 + 1) {
			dsp->fVec2[l6] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l7;
		for (l7 = 0; l7 < 2048; l7 = l7 + 1) {
			dsp->fVec3[l7] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l8;
		for (l8 = 0; l8 < 2; l8 = l8 + 1) {
			dsp->fRec12[l8] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l9;
		for (l9 = 0; l9 < 2; l9 = l9 + 1) {
			dsp->fRec19[l9] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l10;
		for (l10 = 0; l10 < 2; l10 = l10 + 1) {
			dsp->fRec18[l10] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l11;
		for (l11 = 0; l11 < 32768; l11 = l11 + 1) {
			dsp->fVec4[l11] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l12;
		for (l12 = 0; l12 < 4096; l12 = l12 + 1) {
			dsp->fVec5[l12] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l13;
		for (l13 = 0; l13 < 2; l13 = l13 + 1) {
			dsp->fRec16[l13] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l14;
		for (l14 = 0; l14 < 2; l14 = l14 + 1) {
			dsp->fRec23[l14] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l15;
		for (l15 = 0; l15 < 2; l15 = l15 + 1) {
			dsp->fRec22[l15] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l16;
		for (l16 = 0; l16 < 16384; l16 = l16 + 1) {
			dsp->fVec6[l16] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l17;
		for (l17 = 0; l17 < 4096; l17 = l17 + 1) {
			dsp->fVec7[l17] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l18;
		for (l18 = 0; l18 < 2; l18 = l18 + 1) {
			dsp->fRec20[l18] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l19;
		for (l19 = 0; l19 < 2; l19 = l19 + 1) {
			dsp->fRec27[l19] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l20;
		for (l20 = 0; l20 < 2; l20 = l20 + 1) {
			dsp->fRec26[l20] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l21;
		for (l21 = 0; l21 < 32768; l21 = l21 + 1) {
			dsp->fVec8[l21] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l22;
		for (l22 = 0; l22 < 4096; l22 = l22 + 1) {
			dsp->fVec9[l22] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l23;
		for (l23 = 0; l23 < 2; l23 = l23 + 1) {
			dsp->fRec24[l23] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l24;
		for (l24 = 0; l24 < 2; l24 = l24 + 1) {
			dsp->fRec31[l24] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l25;
		for (l25 = 0; l25 < 2; l25 = l25 + 1) {
			dsp->fRec30[l25] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l26;
		for (l26 = 0; l26 < 16384; l26 = l26 + 1) {
			dsp->fVec10[l26] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l27;
		for (l27 = 0; l27 < 2048; l27 = l27 + 1) {
			dsp->fVec11[l27] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l28;
		for (l28 = 0; l28 < 2; l28 = l28 + 1) {
			dsp->fRec28[l28] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l29;
		for (l29 = 0; l29 < 2; l29 = l29 + 1) {
			dsp->fRec35[l29] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l30;
		for (l30 = 0; l30 < 2; l30 = l30 + 1) {
			dsp->fRec34[l30] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l31;
		for (l31 = 0; l31 < 16384; l31 = l31 + 1) {
			dsp->fVec12[l31] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l32;
		for (l32 = 0; l32 < 4096; l32 = l32 + 1) {
			dsp->fVec13[l32] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l33;
		for (l33 = 0; l33 < 2; l33 = l33 + 1) {
			dsp->fRec32[l33] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l34;
		for (l34 = 0; l34 < 2; l34 = l34 + 1) {
			dsp->fRec39[l34] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l35;
		for (l35 = 0; l35 < 2; l35 = l35 + 1) {
			dsp->fRec38[l35] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l36;
		for (l36 = 0; l36 < 16384; l36 = l36 + 1) {
			dsp->fVec14[l36] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l37;
		for (l37 = 0; l37 < 4096; l37 = l37 + 1) {
			dsp->fVec15[l37] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l38;
		for (l38 = 0; l38 < 2; l38 = l38 + 1) {
			dsp->fRec36[l38] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l39;
		for (l39 = 0; l39 < 2; l39 = l39 + 1) {
			dsp->fRec43[l39] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l40;
		for (l40 = 0; l40 < 2; l40 = l40 + 1) {
			dsp->fRec42[l40] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l41;
		for (l41 = 0; l41 < 16384; l41 = l41 + 1) {
			dsp->fVec16[l41] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l42;
		for (l42 = 0; l42 < 2048; l42 = l42 + 1) {
			dsp->fVec17[l42] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l43;
		for (l43 = 0; l43 < 2; l43 = l43 + 1) {
			dsp->fRec40[l43] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l44;
		for (l44 = 0; l44 < 3; l44 = l44 + 1) {
			dsp->fRec4[l44] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l45;
		for (l45 = 0; l45 < 3; l45 = l45 + 1) {
			dsp->fRec5[l45] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l46;
		for (l46 = 0; l46 < 3; l46 = l46 + 1) {
			dsp->fRec6[l46] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l47;
		for (l47 = 0; l47 < 3; l47 = l47 + 1) {
			dsp->fRec7[l47] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l48;
		for (l48 = 0; l48 < 3; l48 = l48 + 1) {
			dsp->fRec8[l48] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l49;
		for (l49 = 0; l49 < 3; l49 = l49 + 1) {
			dsp->fRec9[l49] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l50;
		for (l50 = 0; l50 < 3; l50 = l50 + 1) {
			dsp->fRec10[l50] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l51;
		for (l51 = 0; l51 < 3; l51 = l51 + 1) {
			dsp->fRec11[l51] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l52;
		for (l52 = 0; l52 < 3; l52 = l52 + 1) {
			dsp->fRec3[l52] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l53;
		for (l53 = 0; l53 < 3; l53 = l53 + 1) {
			dsp->fRec2[l53] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l54;
		for (l54 = 0; l54 < 3; l54 = l54 + 1) {
			dsp->fRec45[l54] = 0.0f;
		}
	}
	/* C99 loop */
	{
		int l55;
		for (l55 = 0; l55 < 3; l55 = l55 + 1) {
			dsp->fRec44[l55] = 0.0f;
		}
	}
}

void instanceConstantsmydsp(mydsp* dsp, int sample_rate) {
	dsp->fSampleRate = sample_rate;
	dsp->fConst0 = fminf(1.92e+05f, fmaxf(1.0f, (float)(dsp->fSampleRate)));
	dsp->fConst1 = 44.1f / dsp->fConst0;
	dsp->fConst2 = 1.0f - dsp->fConst1;
	dsp->fConst3 = 6.2831855f / dsp->fConst0;
	dsp->fConst4 = floorf(0.219991f * dsp->fConst0 + 0.5f);
	dsp->fConst5 = 6.9077554f * (dsp->fConst4 / dsp->fConst0);
	dsp->fConst6 = 3.1415927f / dsp->fConst0;
	dsp->fConst7 = floorf(0.019123f * dsp->fConst0 + 0.5f);
	dsp->iConst8 = (int)(fminf(16384.0f, fmaxf(0.0f, dsp->fConst4 - dsp->fConst7)));
	dsp->fConst9 = 0.001f * dsp->fConst0;
	dsp->iConst10 = (int)(fminf(1024.0f, fmaxf(0.0f, dsp->fConst7 + -1.0f)));
	dsp->fConst11 = floorf(0.256891f * dsp->fConst0 + 0.5f);
	dsp->fConst12 = 6.9077554f * (dsp->fConst11 / dsp->fConst0);
	dsp->fConst13 = floorf(0.027333f * dsp->fConst0 + 0.5f);
	dsp->iConst14 = (int)(fminf(16384.0f, fmaxf(0.0f, dsp->fConst11 - dsp->fConst13)));
	dsp->iConst15 = (int)(fminf(2048.0f, fmaxf(0.0f, dsp->fConst13 + -1.0f)));
	dsp->fConst16 = floorf(0.192303f * dsp->fConst0 + 0.5f);
	dsp->fConst17 = 6.9077554f * (dsp->fConst16 / dsp->fConst0);
	dsp->fConst18 = floorf(0.029291f * dsp->fConst0 + 0.5f);
	dsp->iConst19 = (int)(fminf(8192.0f, fmaxf(0.0f, dsp->fConst16 - dsp->fConst18)));
	dsp->iConst20 = (int)(fminf(2048.0f, fmaxf(0.0f, dsp->fConst18 + -1.0f)));
	dsp->fConst21 = floorf(0.210389f * dsp->fConst0 + 0.5f);
	dsp->fConst22 = 6.9077554f * (dsp->fConst21 / dsp->fConst0);
	dsp->fConst23 = floorf(0.024421f * dsp->fConst0 + 0.5f);
	dsp->iConst24 = (int)(fminf(16384.0f, fmaxf(0.0f, dsp->fConst21 - dsp->fConst23)));
	dsp->iConst25 = (int)(fminf(2048.0f, fmaxf(0.0f, dsp->fConst23 + -1.0f)));
	dsp->fConst26 = floorf(0.125f * dsp->fConst0 + 0.5f);
	dsp->fConst27 = 6.9077554f * (dsp->fConst26 / dsp->fConst0);
	dsp->fConst28 = floorf(0.013458f * dsp->fConst0 + 0.5f);
	dsp->iConst29 = (int)(fminf(8192.0f, fmaxf(0.0f, dsp->fConst26 - dsp->fConst28)));
	dsp->iConst30 = (int)(fminf(1024.0f, fmaxf(0.0f, dsp->fConst28 + -1.0f)));
	dsp->fConst31 = floorf(0.127837f * dsp->fConst0 + 0.5f);
	dsp->fConst32 = 6.9077554f * (dsp->fConst31 / dsp->fConst0);
	dsp->fConst33 = floorf(0.031604f * dsp->fConst0 + 0.5f);
	dsp->iConst34 = (int)(fminf(8192.0f, fmaxf(0.0f, dsp->fConst31 - dsp->fConst33)));
	dsp->iConst35 = (int)(fminf(2048.0f, fmaxf(0.0f, dsp->fConst33 + -1.0f)));
	dsp->fConst36 = floorf(0.174713f * dsp->fConst0 + 0.5f);
	dsp->fConst37 = 6.9077554f * (dsp->fConst36 / dsp->fConst0);
	dsp->fConst38 = floorf(0.022904f * dsp->fConst0 + 0.5f);
	dsp->iConst39 = (int)(fminf(8192.0f, fmaxf(0.0f, dsp->fConst36 - dsp->fConst38)));
	dsp->iConst40 = (int)(fminf(2048.0f, fmaxf(0.0f, dsp->fConst38 + -1.0f)));
	dsp->fConst41 = floorf(0.153129f * dsp->fConst0 + 0.5f);
	dsp->fConst42 = 6.9077554f * (dsp->fConst41 / dsp->fConst0);
	dsp->fConst43 = floorf(0.020346f * dsp->fConst0 + 0.5f);
	dsp->iConst44 = (int)(fminf(8192.0f, fmaxf(0.0f, dsp->fConst41 - dsp->fConst43)));
	dsp->iConst45 = (int)(fminf(1024.0f, fmaxf(0.0f, dsp->fConst43 + -1.0f)));
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
	ui_interface->declare(ui_interface->uiInterface, 0, "0", "");
	ui_interface->declare(ui_interface->uiInterface, 0, "tooltip", "~ ZITA REV1 FEEDBACK DELAY NETWORK (FDN) & SCHROEDER     ALLPASS-COMB REVERBERATOR (8x8). See Faust's reverbs.lib for documentation and     references");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Zita_Rev1");
	ui_interface->declare(ui_interface->uiInterface, 0, "1", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Input");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider10, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider10, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider10, "tooltip", "Delay in ms         before reverberation begins");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider10, "unit", "ms");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "In Delay", &dsp->fVslider10, (FAUSTFLOAT)6e+01f, (FAUSTFLOAT)2e+01f, (FAUSTFLOAT)1e+02f, (FAUSTFLOAT)1.0f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, 0, "2", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Decay Times in Bands (see tooltips)");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider9, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider9, "scale", "log");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider9, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider9, "tooltip", "Crossover frequency (Hz) separating low and middle frequencies");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider9, "unit", "Hz");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "LF X", &dsp->fVslider9, (FAUSTFLOAT)2e+02f, (FAUSTFLOAT)5e+01f, (FAUSTFLOAT)1e+03f, (FAUSTFLOAT)1.0f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider8, "2", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider8, "scale", "log");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider8, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider8, "tooltip", "T60 = time (in seconds) to decay 60dB in low-frequency band");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider8, "unit", "s");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Low RT60", &dsp->fVslider8, (FAUSTFLOAT)3.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)8.0f, (FAUSTFLOAT)0.1f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider6, "3", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider6, "scale", "log");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider6, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider6, "tooltip", "T60 = time (in seconds) to decay 60dB in middle band");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider6, "unit", "s");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Mid RT60", &dsp->fVslider6, (FAUSTFLOAT)2.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)8.0f, (FAUSTFLOAT)0.1f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider7, "4", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider7, "scale", "log");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider7, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider7, "tooltip", "Frequency (Hz) at which the high-frequency T60 is half the middle-band's T60");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider7, "unit", "Hz");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "HF Damping", &dsp->fVslider7, (FAUSTFLOAT)6e+03f, (FAUSTFLOAT)1.5e+03f, (FAUSTFLOAT)2.352e+04f, (FAUSTFLOAT)1.0f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, 0, "3", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "RM Peaking Equalizer 1");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider4, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider4, "scale", "log");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider4, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider4, "tooltip", "Center-frequency of second-order Regalia-Mitra peaking equalizer section 1");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider4, "unit", "Hz");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Eq1 Freq", &dsp->fVslider4, (FAUSTFLOAT)315.0f, (FAUSTFLOAT)4e+01f, (FAUSTFLOAT)2.5e+03f, (FAUSTFLOAT)1.0f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider5, "2", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider5, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider5, "tooltip", "Peak level         in dB of second-order Regalia-Mitra peaking equalizer section 1");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider5, "unit", "dB");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Eq1 Level", &dsp->fVslider5, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)-15.0f, (FAUSTFLOAT)15.0f, (FAUSTFLOAT)0.1f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, 0, "4", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "RM Peaking Equalizer 2");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider2, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider2, "scale", "log");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider2, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider2, "tooltip", "Center-frequency of second-order Regalia-Mitra peaking equalizer section 2");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider2, "unit", "Hz");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Eq2 Freq", &dsp->fVslider2, (FAUSTFLOAT)1.5e+03f, (FAUSTFLOAT)1.6e+02f, (FAUSTFLOAT)1e+04f, (FAUSTFLOAT)1.0f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider3, "2", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider3, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider3, "tooltip", "Peak level         in dB of second-order Regalia-Mitra peaking equalizer section 2");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider3, "unit", "dB");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Eq2 Level", &dsp->fVslider3, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)-15.0f, (FAUSTFLOAT)15.0f, (FAUSTFLOAT)0.1f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->declare(ui_interface->uiInterface, 0, "5", "");
	ui_interface->openHorizontalBox(ui_interface->uiInterface, "Output");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider1, "1", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider1, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider1, "tooltip", "Ratio of dry and wet signal. -1 = fully wet, +1 = fully dry");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Wet/Dry Mix", &dsp->fVslider1, (FAUSTFLOAT)0.0f, (FAUSTFLOAT)-1.0f, (FAUSTFLOAT)1.0f, (FAUSTFLOAT)0.01f);
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider0, "2", "");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider0, "style", "knob");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider0, "tooltip", "Output scale         factor");
	ui_interface->declare(ui_interface->uiInterface, &dsp->fVslider0, "unit", "dB");
	ui_interface->addVerticalSlider(ui_interface->uiInterface, "Level", &dsp->fVslider0, (FAUSTFLOAT)-2e+01f, (FAUSTFLOAT)-7e+01f, (FAUSTFLOAT)4e+01f, (FAUSTFLOAT)0.1f);
	ui_interface->closeBox(ui_interface->uiInterface);
	ui_interface->closeBox(ui_interface->uiInterface);
}

void computemydsp(mydsp* dsp, int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
	FAUSTFLOAT* input0 = inputs[0];
	FAUSTFLOAT* input1 = inputs[1];
	FAUSTFLOAT* output0 = outputs[0];
	FAUSTFLOAT* output1 = outputs[1];
	float fSlow0 = dsp->fConst1 * powf(1e+01f, 0.05f * (float)(dsp->fVslider0));
	float fSlow1 = dsp->fConst1 * (float)(dsp->fVslider1);
	float fSlow2 = (float)(dsp->fVslider2);
	float fSlow3 = powf(1e+01f, 0.05f * (float)(dsp->fVslider3));
	float fSlow4 = dsp->fConst3 * (fSlow2 / sqrtf(fmaxf(0.0f, fSlow3)));
	float fSlow5 = (1.0f - fSlow4) / (fSlow4 + 1.0f);
	float fSlow6 = (float)(dsp->fVslider4);
	float fSlow7 = powf(1e+01f, 0.05f * (float)(dsp->fVslider5));
	float fSlow8 = dsp->fConst3 * (fSlow6 / sqrtf(fmaxf(0.0f, fSlow7)));
	float fSlow9 = (1.0f - fSlow8) / (fSlow8 + 1.0f);
	float fSlow10 = (float)(dsp->fVslider6);
	float fSlow11 = expf(-(dsp->fConst5 / fSlow10));
	float fSlow12 = cosf(dsp->fConst3 * (float)(dsp->fVslider7));
	float fSlow13 = mydsp_faustpower2_f(fSlow11);
	float fSlow14 = 1.0f - fSlow12 * fSlow13;
	float fSlow15 = 1.0f - fSlow13;
	float fSlow16 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow14) / mydsp_faustpower2_f(fSlow15) + -1.0f));
	float fSlow17 = fSlow14 / fSlow15;
	float fSlow18 = fSlow11 * (fSlow16 + (1.0f - fSlow17));
	float fSlow19 = (float)(dsp->fVslider8);
	float fSlow20 = expf(-(dsp->fConst5 / fSlow19)) / fSlow11 + -1.0f;
	float fSlow21 = 1.0f / tanf(dsp->fConst6 * (float)(dsp->fVslider9));
	float fSlow22 = 1.0f / (fSlow21 + 1.0f);
	float fSlow23 = 1.0f - fSlow21;
	float fSlow24 = fSlow17 - fSlow16;
	int iSlow25 = (int)(fminf(8192.0f, fmaxf(0.0f, dsp->fConst9 * (float)(dsp->fVslider10))));
	float fSlow26 = expf(-(dsp->fConst12 / fSlow10));
	float fSlow27 = mydsp_faustpower2_f(fSlow26);
	float fSlow28 = 1.0f - fSlow12 * fSlow27;
	float fSlow29 = 1.0f - fSlow27;
	float fSlow30 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow28) / mydsp_faustpower2_f(fSlow29) + -1.0f));
	float fSlow31 = fSlow28 / fSlow29;
	float fSlow32 = fSlow26 * (fSlow30 + (1.0f - fSlow31));
	float fSlow33 = expf(-(dsp->fConst12 / fSlow19)) / fSlow26 + -1.0f;
	float fSlow34 = fSlow31 - fSlow30;
	float fSlow35 = expf(-(dsp->fConst17 / fSlow10));
	float fSlow36 = mydsp_faustpower2_f(fSlow35);
	float fSlow37 = 1.0f - fSlow12 * fSlow36;
	float fSlow38 = 1.0f - fSlow36;
	float fSlow39 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow37) / mydsp_faustpower2_f(fSlow38) + -1.0f));
	float fSlow40 = fSlow37 / fSlow38;
	float fSlow41 = fSlow35 * (fSlow39 + (1.0f - fSlow40));
	float fSlow42 = expf(-(dsp->fConst17 / fSlow19)) / fSlow35 + -1.0f;
	float fSlow43 = fSlow40 - fSlow39;
	float fSlow44 = expf(-(dsp->fConst22 / fSlow10));
	float fSlow45 = mydsp_faustpower2_f(fSlow44);
	float fSlow46 = 1.0f - fSlow12 * fSlow45;
	float fSlow47 = 1.0f - fSlow45;
	float fSlow48 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow46) / mydsp_faustpower2_f(fSlow47) + -1.0f));
	float fSlow49 = fSlow46 / fSlow47;
	float fSlow50 = fSlow44 * (fSlow48 + (1.0f - fSlow49));
	float fSlow51 = expf(-(dsp->fConst22 / fSlow19)) / fSlow44 + -1.0f;
	float fSlow52 = fSlow49 - fSlow48;
	float fSlow53 = expf(-(dsp->fConst27 / fSlow10));
	float fSlow54 = mydsp_faustpower2_f(fSlow53);
	float fSlow55 = 1.0f - fSlow12 * fSlow54;
	float fSlow56 = 1.0f - fSlow54;
	float fSlow57 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow55) / mydsp_faustpower2_f(fSlow56) + -1.0f));
	float fSlow58 = fSlow55 / fSlow56;
	float fSlow59 = fSlow53 * (fSlow57 + (1.0f - fSlow58));
	float fSlow60 = expf(-(dsp->fConst27 / fSlow19)) / fSlow53 + -1.0f;
	float fSlow61 = fSlow58 - fSlow57;
	float fSlow62 = expf(-(dsp->fConst32 / fSlow10));
	float fSlow63 = mydsp_faustpower2_f(fSlow62);
	float fSlow64 = 1.0f - fSlow12 * fSlow63;
	float fSlow65 = 1.0f - fSlow63;
	float fSlow66 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow64) / mydsp_faustpower2_f(fSlow65) + -1.0f));
	float fSlow67 = fSlow64 / fSlow65;
	float fSlow68 = fSlow62 * (fSlow66 + (1.0f - fSlow67));
	float fSlow69 = expf(-(dsp->fConst32 / fSlow19)) / fSlow62 + -1.0f;
	float fSlow70 = fSlow67 - fSlow66;
	float fSlow71 = expf(-(dsp->fConst37 / fSlow10));
	float fSlow72 = mydsp_faustpower2_f(fSlow71);
	float fSlow73 = 1.0f - fSlow12 * fSlow72;
	float fSlow74 = 1.0f - fSlow72;
	float fSlow75 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow73) / mydsp_faustpower2_f(fSlow74) + -1.0f));
	float fSlow76 = fSlow73 / fSlow74;
	float fSlow77 = fSlow71 * (fSlow75 + (1.0f - fSlow76));
	float fSlow78 = expf(-(dsp->fConst37 / fSlow19)) / fSlow71 + -1.0f;
	float fSlow79 = fSlow76 - fSlow75;
	float fSlow80 = expf(-(dsp->fConst42 / fSlow10));
	float fSlow81 = mydsp_faustpower2_f(fSlow80);
	float fSlow82 = 1.0f - fSlow81 * fSlow12;
	float fSlow83 = 1.0f - fSlow81;
	float fSlow84 = sqrtf(fmaxf(0.0f, mydsp_faustpower2_f(fSlow82) / mydsp_faustpower2_f(fSlow83) + -1.0f));
	float fSlow85 = fSlow82 / fSlow83;
	float fSlow86 = fSlow80 * (fSlow84 + (1.0f - fSlow85));
	float fSlow87 = expf(-(dsp->fConst42 / fSlow19)) / fSlow80 + -1.0f;
	float fSlow88 = fSlow85 - fSlow84;
	float fSlow89 = cosf(dsp->fConst3 * fSlow6) * (fSlow9 + 1.0f);
	float fSlow90 = cosf(dsp->fConst3 * fSlow2) * (fSlow5 + 1.0f);
	/* C99 loop */
	{
		int i0;
		for (i0 = 0; i0 < count; i0 = i0 + 1) {
			dsp->fRec0[0] = fSlow0 + dsp->fConst2 * dsp->fRec0[1];
			float fTemp0 = (float)(input0[i0]);
			dsp->fVec0[dsp->IOTA0 & 16383] = fTemp0;
			dsp->fRec1[0] = fSlow1 + dsp->fConst2 * dsp->fRec1[1];
			float fTemp1 = dsp->fRec1[0] + 1.0f;
			float fTemp2 = 1.0f - 0.5f * fTemp1;
			dsp->fRec15[0] = -(fSlow22 * (fSlow23 * dsp->fRec15[1] - (dsp->fRec11[1] + dsp->fRec11[2])));
			dsp->fRec14[0] = fSlow18 * (dsp->fRec11[1] + fSlow20 * dsp->fRec15[0]) + fSlow24 * dsp->fRec14[1];
			dsp->fVec1[dsp->IOTA0 & 32767] = 0.35355338f * dsp->fRec14[0] + 1e-20f;
			float fTemp3 = 0.6f * dsp->fRec12[1] + dsp->fVec1[(dsp->IOTA0 - dsp->iConst8) & 32767];
			float fTemp4 = (float)(input1[i0]);
			dsp->fVec2[dsp->IOTA0 & 16383] = fTemp4;
			float fTemp5 = 0.3f * dsp->fVec2[(dsp->IOTA0 - iSlow25) & 16383];
			dsp->fVec3[dsp->IOTA0 & 2047] = fTemp3 - fTemp5;
			dsp->fRec12[0] = dsp->fVec3[(dsp->IOTA0 - dsp->iConst10) & 2047];
			float fRec13 = 0.6f * (fTemp5 - fTemp3);
			dsp->fRec19[0] = -(fSlow22 * (fSlow23 * dsp->fRec19[1] - (dsp->fRec7[1] + dsp->fRec7[2])));
			dsp->fRec18[0] = fSlow32 * (dsp->fRec7[1] + fSlow33 * dsp->fRec19[0]) + fSlow34 * dsp->fRec18[1];
			dsp->fVec4[dsp->IOTA0 & 32767] = 0.35355338f * dsp->fRec18[0] + 1e-20f;
			float fTemp6 = 0.6f * dsp->fRec16[1] + dsp->fVec4[(dsp->IOTA0 - dsp->iConst14) & 32767];
			dsp->fVec5[dsp->IOTA0 & 4095] = fTemp6 - fTemp5;
			dsp->fRec16[0] = dsp->fVec5[(dsp->IOTA0 - dsp->iConst15) & 4095];
			float fRec17 = 0.6f * (fTemp5 - fTemp6);
			dsp->fRec23[0] = -(fSlow22 * (fSlow23 * dsp->fRec23[1] - (dsp->fRec9[1] + dsp->fRec9[2])));
			dsp->fRec22[0] = fSlow41 * (dsp->fRec9[1] + fSlow42 * dsp->fRec23[0]) + fSlow43 * dsp->fRec22[1];
			dsp->fVec6[dsp->IOTA0 & 16383] = 0.35355338f * dsp->fRec22[0] + 1e-20f;
			float fTemp7 = dsp->fVec6[(dsp->IOTA0 - dsp->iConst19) & 16383] + fTemp5 + 0.6f * dsp->fRec20[1];
			dsp->fVec7[dsp->IOTA0 & 4095] = fTemp7;
			dsp->fRec20[0] = dsp->fVec7[(dsp->IOTA0 - dsp->iConst20) & 4095];
			float fRec21 = -(0.6f * fTemp7);
			dsp->fRec27[0] = -(fSlow22 * (fSlow23 * dsp->fRec27[1] - (dsp->fRec5[1] + dsp->fRec5[2])));
			dsp->fRec26[0] = fSlow50 * (dsp->fRec5[1] + fSlow51 * dsp->fRec27[0]) + fSlow52 * dsp->fRec26[1];
			dsp->fVec8[dsp->IOTA0 & 32767] = 0.35355338f * dsp->fRec26[0] + 1e-20f;
			float fTemp8 = fTemp5 + 0.6f * dsp->fRec24[1] + dsp->fVec8[(dsp->IOTA0 - dsp->iConst24) & 32767];
			dsp->fVec9[dsp->IOTA0 & 4095] = fTemp8;
			dsp->fRec24[0] = dsp->fVec9[(dsp->IOTA0 - dsp->iConst25) & 4095];
			float fRec25 = -(0.6f * fTemp8);
			dsp->fRec31[0] = -(fSlow22 * (fSlow23 * dsp->fRec31[1] - (dsp->fRec10[1] + dsp->fRec10[2])));
			dsp->fRec30[0] = fSlow59 * (dsp->fRec10[1] + fSlow60 * dsp->fRec31[0]) + fSlow61 * dsp->fRec30[1];
			dsp->fVec10[dsp->IOTA0 & 16383] = 0.35355338f * dsp->fRec30[0] + 1e-20f;
			float fTemp9 = 0.3f * dsp->fVec0[(dsp->IOTA0 - iSlow25) & 16383];
			float fTemp10 = dsp->fVec10[(dsp->IOTA0 - dsp->iConst29) & 16383] - (fTemp9 + 0.6f * dsp->fRec28[1]);
			dsp->fVec11[dsp->IOTA0 & 2047] = fTemp10;
			dsp->fRec28[0] = dsp->fVec11[(dsp->IOTA0 - dsp->iConst30) & 2047];
			float fRec29 = 0.6f * fTemp10;
			dsp->fRec35[0] = -(fSlow22 * (fSlow23 * dsp->fRec35[1] - (dsp->fRec6[1] + dsp->fRec6[2])));
			dsp->fRec34[0] = fSlow68 * (dsp->fRec6[1] + fSlow69 * dsp->fRec35[0]) + fSlow70 * dsp->fRec34[1];
			dsp->fVec12[dsp->IOTA0 & 16383] = 0.35355338f * dsp->fRec34[0] + 1e-20f;
			float fTemp11 = dsp->fVec12[(dsp->IOTA0 - dsp->iConst34) & 16383] - (fTemp9 + 0.6f * dsp->fRec32[1]);
			dsp->fVec13[dsp->IOTA0 & 4095] = fTemp11;
			dsp->fRec32[0] = dsp->fVec13[(dsp->IOTA0 - dsp->iConst35) & 4095];
			float fRec33 = 0.6f * fTemp11;
			dsp->fRec39[0] = -(fSlow22 * (fSlow23 * dsp->fRec39[1] - (dsp->fRec8[1] + dsp->fRec8[2])));
			dsp->fRec38[0] = fSlow77 * (dsp->fRec8[1] + fSlow78 * dsp->fRec39[0]) + fSlow79 * dsp->fRec38[1];
			dsp->fVec14[dsp->IOTA0 & 16383] = 0.35355338f * dsp->fRec38[0] + 1e-20f;
			float fTemp12 = fTemp9 + dsp->fVec14[(dsp->IOTA0 - dsp->iConst39) & 16383] - 0.6f * dsp->fRec36[1];
			dsp->fVec15[dsp->IOTA0 & 4095] = fTemp12;
			dsp->fRec36[0] = dsp->fVec15[(dsp->IOTA0 - dsp->iConst40) & 4095];
			float fRec37 = 0.6f * fTemp12;
			dsp->fRec43[0] = -(fSlow22 * (fSlow23 * dsp->fRec43[1] - (dsp->fRec4[1] + dsp->fRec4[2])));
			dsp->fRec42[0] = fSlow86 * (dsp->fRec4[1] + fSlow87 * dsp->fRec43[0]) + fSlow88 * dsp->fRec42[1];
			dsp->fVec16[dsp->IOTA0 & 16383] = 0.35355338f * dsp->fRec42[0] + 1e-20f;
			float fTemp13 = dsp->fVec16[(dsp->IOTA0 - dsp->iConst44) & 16383] + fTemp9 - 0.6f * dsp->fRec40[1];
			dsp->fVec17[dsp->IOTA0 & 2047] = fTemp13;
			dsp->fRec40[0] = dsp->fVec17[(dsp->IOTA0 - dsp->iConst45) & 2047];
			float fRec41 = 0.6f * fTemp13;
			float fTemp14 = fRec41 + fRec37;
			float fTemp15 = fRec29 + fRec33 + fTemp14;
			dsp->fRec4[0] = dsp->fRec12[1] + dsp->fRec16[1] + dsp->fRec20[1] + dsp->fRec24[1] + dsp->fRec28[1] + dsp->fRec32[1] + dsp->fRec36[1] + dsp->fRec40[1] + fRec13 + fRec17 + fRec21 + fRec25 + fTemp15;
			dsp->fRec5[0] = dsp->fRec28[1] + dsp->fRec32[1] + dsp->fRec36[1] + dsp->fRec40[1] + fTemp15 - (dsp->fRec12[1] + dsp->fRec16[1] + dsp->fRec20[1] + dsp->fRec24[1] + fRec13 + fRec17 + fRec25 + fRec21);
			float fTemp16 = fRec33 + fRec29;
			dsp->fRec6[0] = dsp->fRec20[1] + dsp->fRec24[1] + dsp->fRec36[1] + dsp->fRec40[1] + fRec21 + fRec25 + fTemp14 - (dsp->fRec12[1] + dsp->fRec16[1] + dsp->fRec28[1] + dsp->fRec32[1] + fRec13 + fRec17 + fTemp16);
			dsp->fRec7[0] = dsp->fRec12[1] + dsp->fRec16[1] + dsp->fRec36[1] + dsp->fRec40[1] + fRec13 + fRec17 + fTemp14 - (dsp->fRec20[1] + dsp->fRec24[1] + dsp->fRec28[1] + dsp->fRec32[1] + fRec21 + fRec25 + fTemp16);
			float fTemp17 = fRec41 + fRec33;
			float fTemp18 = fRec37 + fRec29;
			dsp->fRec8[0] = dsp->fRec16[1] + dsp->fRec24[1] + dsp->fRec32[1] + dsp->fRec40[1] + fRec17 + fRec25 + fTemp17 - (dsp->fRec12[1] + dsp->fRec20[1] + dsp->fRec28[1] + dsp->fRec36[1] + fRec13 + fRec21 + fTemp18);
			dsp->fRec9[0] = dsp->fRec12[1] + dsp->fRec20[1] + dsp->fRec32[1] + dsp->fRec40[1] + fRec13 + fRec21 + fTemp17 - (dsp->fRec16[1] + dsp->fRec24[1] + dsp->fRec28[1] + dsp->fRec36[1] + fRec17 + fRec25 + fTemp18);
			float fTemp19 = fRec41 + fRec29;
			float fTemp20 = fRec37 + fRec33;
			dsp->fRec10[0] = dsp->fRec12[1] + dsp->fRec24[1] + dsp->fRec28[1] + dsp->fRec40[1] + fRec13 + fRec25 + fTemp19 - (dsp->fRec16[1] + dsp->fRec20[1] + dsp->fRec32[1] + dsp->fRec36[1] + fRec17 + fRec21 + fTemp20);
			dsp->fRec11[0] = dsp->fRec16[1] + dsp->fRec20[1] + dsp->fRec28[1] + dsp->fRec40[1] + fRec17 + fRec21 + fTemp19 - (dsp->fRec12[1] + dsp->fRec24[1] + dsp->fRec32[1] + dsp->fRec36[1] + fRec13 + fRec25 + fTemp20);
			float fTemp21 = 0.37f * (dsp->fRec5[0] + dsp->fRec6[0]);
			float fTemp22 = fSlow89 * dsp->fRec3[1];
			float fTemp23 = fTemp21 + fTemp22;
			dsp->fRec3[0] = fTemp23 - fSlow9 * dsp->fRec3[2];
			float fTemp24 = fSlow9 * dsp->fRec3[0];
			float fTemp25 = 0.5f * (fTemp24 + fTemp21 + dsp->fRec3[2] - fTemp22 + fSlow7 * (dsp->fRec3[2] + fTemp24 - fTemp23));
			float fTemp26 = fSlow90 * dsp->fRec2[1];
			float fTemp27 = fTemp25 + fTemp26;
			dsp->fRec2[0] = fTemp27 - fSlow5 * dsp->fRec2[2];
			float fTemp28 = fSlow5 * dsp->fRec2[0];
			output0[i0] = (FAUSTFLOAT)(0.5f * dsp->fRec0[0] * (fTemp0 * fTemp1 + fTemp2 * (fTemp28 + fTemp25 + dsp->fRec2[2] - fTemp26 + fSlow3 * (dsp->fRec2[2] + fTemp28 - fTemp27))));
			float fTemp29 = 0.37f * (dsp->fRec5[0] - dsp->fRec6[0]);
			float fTemp30 = fSlow89 * dsp->fRec45[1];
			float fTemp31 = fTemp29 + fTemp30;
			dsp->fRec45[0] = fTemp31 - fSlow9 * dsp->fRec45[2];
			float fTemp32 = fSlow9 * dsp->fRec45[0];
			float fTemp33 = 0.5f * (fTemp32 + fTemp29 + dsp->fRec45[2] - fTemp30 + fSlow7 * (dsp->fRec45[2] + fTemp32 - fTemp31));
			float fTemp34 = fSlow90 * dsp->fRec44[1];
			float fTemp35 = fTemp33 + fTemp34;
			dsp->fRec44[0] = fTemp35 - fSlow5 * dsp->fRec44[2];
			float fTemp36 = fSlow5 * dsp->fRec44[0];
			output1[i0] = (FAUSTFLOAT)(0.5f * dsp->fRec0[0] * (fTemp4 * fTemp1 + fTemp2 * (fTemp36 + fTemp33 + dsp->fRec44[2] - fTemp34 + fSlow3 * (dsp->fRec44[2] + fTemp36 - fTemp35))));
			dsp->fRec0[1] = dsp->fRec0[0];
			dsp->IOTA0 = dsp->IOTA0 + 1;
			dsp->fRec1[1] = dsp->fRec1[0];
			dsp->fRec15[1] = dsp->fRec15[0];
			dsp->fRec14[1] = dsp->fRec14[0];
			dsp->fRec12[1] = dsp->fRec12[0];
			dsp->fRec19[1] = dsp->fRec19[0];
			dsp->fRec18[1] = dsp->fRec18[0];
			dsp->fRec16[1] = dsp->fRec16[0];
			dsp->fRec23[1] = dsp->fRec23[0];
			dsp->fRec22[1] = dsp->fRec22[0];
			dsp->fRec20[1] = dsp->fRec20[0];
			dsp->fRec27[1] = dsp->fRec27[0];
			dsp->fRec26[1] = dsp->fRec26[0];
			dsp->fRec24[1] = dsp->fRec24[0];
			dsp->fRec31[1] = dsp->fRec31[0];
			dsp->fRec30[1] = dsp->fRec30[0];
			dsp->fRec28[1] = dsp->fRec28[0];
			dsp->fRec35[1] = dsp->fRec35[0];
			dsp->fRec34[1] = dsp->fRec34[0];
			dsp->fRec32[1] = dsp->fRec32[0];
			dsp->fRec39[1] = dsp->fRec39[0];
			dsp->fRec38[1] = dsp->fRec38[0];
			dsp->fRec36[1] = dsp->fRec36[0];
			dsp->fRec43[1] = dsp->fRec43[0];
			dsp->fRec42[1] = dsp->fRec42[0];
			dsp->fRec40[1] = dsp->fRec40[0];
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
			dsp->fRec3[2] = dsp->fRec3[1];
			dsp->fRec3[1] = dsp->fRec3[0];
			dsp->fRec2[2] = dsp->fRec2[1];
			dsp->fRec2[1] = dsp->fRec2[0];
			dsp->fRec45[2] = dsp->fRec45[1];
			dsp->fRec45[1] = dsp->fRec45[0];
			dsp->fRec44[2] = dsp->fRec44[1];
			dsp->fRec44[1] = dsp->fRec44[0];
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif
typedef struct {
  mydsp dsp;
  
  // Input parameters
  double in_delay;      // 20 to 100 ms
  
  // Decay Time parameters
  double lf_x;          // 50 to 1000 Hz - crossover frequency
  double low_rt60;      // 1 to 8 s - low freq decay time
  double mid_rt60;      // 1 to 8 s - mid freq decay time
  double hf_damping;    // 1500 to 23520 Hz - high freq damping
  
  // Equalizer 1
  double eq1_freq;      // 40 to 2500 Hz
  double eq1_level;     // -15 to +15 dB
  
  // Equalizer 2
  double eq2_freq;      // 160 to 10000 Hz
  double eq2_level;     // -15 to +15 dB
  
  // Output parameters
  double wet_dry_mix;   // -1 to 1 (-1 = fully wet, 1 = fully dry)
  double level;         // -70 to 40 dB
} zita_rev_state;

// Update the DSP parameters based on node state
static void update_zita_rev_parameters(zita_rev_state *state) {
  // Input parameters
  state->dsp.fVslider10 = (FAUSTFLOAT)state->in_delay;     // In Delay
  
  // Decay Time parameters
  state->dsp.fVslider9 = (FAUSTFLOAT)state->lf_x;          // LF X
  state->dsp.fVslider8 = (FAUSTFLOAT)state->low_rt60;      // Low RT60
  state->dsp.fVslider6 = (FAUSTFLOAT)state->mid_rt60;      // Mid RT60
  state->dsp.fVslider7 = (FAUSTFLOAT)state->hf_damping;    // HF Damping
  
  // Equalizer 1
  state->dsp.fVslider4 = (FAUSTFLOAT)state->eq1_freq;      // Eq1 Freq
  state->dsp.fVslider5 = (FAUSTFLOAT)state->eq1_level;     // Eq1 Level
  
  // Equalizer 2
  state->dsp.fVslider2 = (FAUSTFLOAT)state->eq2_freq;      // Eq2 Freq
  state->dsp.fVslider3 = (FAUSTFLOAT)state->eq2_level;     // Eq2 Level
  
  // Output parameters
  state->dsp.fVslider1 = (FAUSTFLOAT)state->wet_dry_mix;   // Wet/Dry Mix
  state->dsp.fVslider0 = (FAUSTFLOAT)state->level;         // Level
}

// The zita_rev_perform function that will be called by the audio graph
void *zita_rev_perform(Node *node, zita_rev_state *state, Node *inputs[],
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
  update_zita_rev_parameters(state);

  // Process the audio
  computemydsp(&state->dsp, nframes, faust_input, faust_output);

  // Write back to interleaved stereo output
  for (int i = 0; i < nframes; i++) {
    out[2 * i] = (double)out_buffer_left[i];
    out[2 * i + 1] = (double)out_buffer_right[i];
  }

  return node->output.buf;
}

// Create a zita_rev node with proper initialization
NodeRef zita_rev_node(double in_delay, double lf_x, double low_rt60, double mid_rt60, 
                     double hf_damping, double eq1_freq, double eq1_level, 
                     double eq2_freq, double eq2_level, double wet_dry_mix, 
                     double level, NodeRef input) {
  AudioGraph *graph = _graph;
  NodeRef node = allocate_node_in_graph(graph, sizeof(zita_rev_state));

  // Initialize node
  *node = (Node){
      .perform = (perform_func_t)zita_rev_perform,
      .node_index = node->node_index,
      .num_inputs = 1,
      .state_size = sizeof(zita_rev_state),
      .state_offset = state_offset_ptr_in_graph(graph, sizeof(zita_rev_state)),
      
      // Always stereo output
      .output = (Signal){.layout = 2,
                         .size = BUF_SIZE * 2,
                         .buf = allocate_buffer_from_pool(graph, BUF_SIZE * 2)},
      .meta = "zita_rev",
  };

  // Initialize state
  zita_rev_state *state =
      (zita_rev_state *)(graph->nodes_state_memory + node->state_offset);
  
  // Initialize all memory to zero before setting parameters
  memset(state, 0, sizeof(zita_rev_state));

  // Set initial parameter values
  state->in_delay = in_delay;        // 20 to 100 ms
  state->lf_x = lf_x;                // 50 to 1000 Hz
  state->low_rt60 = low_rt60;        // 1 to 8 s
  state->mid_rt60 = mid_rt60;        // 1 to 8 s
  state->hf_damping = hf_damping;    // 1500 to 23520 Hz
  state->eq1_freq = eq1_freq;        // 40 to 2500 Hz
  state->eq1_level = eq1_level;      // -15 to +15 dB
  state->eq2_freq = eq2_freq;        // 160 to 10000 Hz
  state->eq2_level = eq2_level;      // -15 to +15 dB
  state->wet_dry_mix = wet_dry_mix;  // -1 to 1
  state->level = level;              // -70 to 40 dB

  int sample_rate = ctx_sample_rate();
  
  // Initialize the Faust DSP properly
  instanceConstantsmydsp(&state->dsp, sample_rate);
  instanceResetUserInterfacemydsp(&state->dsp);
  instanceClearmydsp(&state->dsp);
  
  // Apply parameters right after initialization
  update_zita_rev_parameters(state);

  // Connect input
  // node->connections[0].source_node_index = input->node_index;
  plug_input_in_graph(0, node, input);

  return node;
}

// Helper function to create a reverb with recommended preset values
NodeRef zita_rev_preset(NodeRef input) {
  return zita_rev_node(
    60.0,     // in_delay - 60ms
    200.0,    // lf_x - 200Hz crossover
    4.0,      // low_rt60 - 4s decay
    3.0,      // mid_rt60 - 3s decay
    6000.0,   // hf_damping - 6kHz
    315.0,    // eq1_freq - 315Hz
    0.0,      // eq1_level - 0dB (flat)
    1500.0,   // eq2_freq - 1.5kHz
    0.0,      // eq2_level - 0dB (flat)
    -0.5,     // wet_dry_mix - more wet than dry
    0.0,    // level - moderate output level
    input
  );
}

// Create a zita_rev with "plate" like characteristics
NodeRef zita_rev_plate(NodeRef input) {
  return zita_rev_node(
    40.0,     // in_delay - 40ms (shorter)
    180.0,    // lf_x - lower crossover
    3.0,      // low_rt60 - 3s decay
    1.8,      // mid_rt60 - shorter decay
    8000.0,   // hf_damping - 8kHz (more dampening)
    250.0,    // eq1_freq - 250Hz
    2.0,      // eq1_level - slight low boost
    2500.0,   // eq2_freq - 2.5kHz
    -1.5,     // eq2_level - small high cut
    -0.75,    // wet_dry_mix - mostly wet
    0.0,    // level
    input
  );
}

// Create a zita_rev with "hall" like characteristics
NodeRef zita_rev_hall(NodeRef input) {
  return zita_rev_node(
    80.0,     // in_delay - 80ms (longer)
    120.0,    // lf_x - low crossover for full bass
    5.0,      // low_rt60 - 5s (longer bass decay)
    3.5,      // mid_rt60 - 3.5s
    5000.0,   // hf_damping - 5kHz
    315.0,    // eq1_freq
    -2.0,     // eq1_level - cut some low-mids
    1800.0,   // eq2_freq
    -1.0,     // eq2_level - slight high cut
    -0.6,     // wet_dry_mix - mostly wet
    0.0,    // level
    input
  );
}
