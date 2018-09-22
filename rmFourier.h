#pragma once

#ifndef RMFOURIER_H
#define RMFOURIER_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include "Smart_Utils.h"
#include "AEFX_SuiteHelper.h"
#include "FftComplex.hpp"

#include <string>
#include <vector>
#include <complex>
#include <ratio>
#include <thread>
#include <fftw3.h>

const std::string   strName = "rmFourier",
					strDescription = "RM skeleton for ctrl plugind";
const   std::complex<double> imaginaryI(0.0, 1.0);

/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


// RM-NOTE: parameter UI order
enum {
	RMFOURIER_INPUT = 0,
	RMFOURIER_PHASE_LAYER,
	RMFOURIER_INVERSE_FFT,
	RMFOURIER_FFT_PHASE,
	RMFOURIER_COLOR_MODE,
	RMFOURIER_NUM_PARAMS
};

// RM-NOTE: parameter disk order
enum {
	PHASE_LAYER_DISK_ID = 1,
	INVERSE_FFT_DISK_ID,
	FFT_PHASE_DISK_ID,
	COLOR_MODE_DISK_ID
};

typedef struct {
	PF_ProgPtr							ref;
	PF_SampPB							samp_pb;
	PF_InData							in_data, *inD;
	PF_Boolean							no_opB;
	PF_EffectWorld						*output_worldP,
										*input_worldP,
										*tmp_worldP,
										*copy_worldP;

	bool								inverseCB,
										fftPhase;
	bool								colorComputations[4] = {false}; // R, G, B, GS
	
	std::vector<std::complex<double> >	imgVectorR, imgVectorG, imgVectorB, imgVectorGS,
										phaseVectorR, phaseVectorG, phaseVectorB, phaseVectorGS;
	fftw_complex *in, *out;
	double								rMax, gMax, bMax;
	A_long								inWidth, inHeight,
										outWidth, outHeight;
	A_long								nMaxThreads;
	int tmpCount, tmpMax;

	bool								preBluestein;
	int									levels,
										convLevels,
										transformType;
	size_t								m;
	std::vector<std::complex<double> >	expTable,
										convExpTable,
										bv;
	A_long								currentProcess,
										totalProcess;
} rmFourierInfo;


extern "C" {
	DllExport	
	PF_Err 
	EntryPointFunc(	
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra) ;
}

static PF_PixelFloat
*getXY32(PF_EffectWorld &def, int x, int y);

PF_Err
vectorToPixelTmp(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

PF_Err
normalizeImg(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

PF_Err
pixelToVector(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations);

PF_Err
vectorToPixel(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP);

PF_Err
fftRowsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations);

PF_Err
fftColumnsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations);

PF_Err
ifftRowsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations);

PF_Err
ifftColumnsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations);

PF_Err
fftShift(
	void *refcon,
	A_long threadNum,
	A_long yL,
	A_long numOfIterations);

PF_Err
ifftShift(
	void *refcon,
	A_long threadNum,
	A_long yL,
	A_long numOfIterations);

PF_Err
tmpRender16(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_Pixel16 	*inP,
	PF_Pixel16 	*outP);

PF_Err
tmpRender8(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_Pixel8 	*inP,
	PF_Pixel8 	*outP);

PF_Err
pixelToVector(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations);

void preTransform(size_t vSize, void *refcon);

#endif // RMFOURIER_H