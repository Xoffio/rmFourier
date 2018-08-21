#include "rmFourier.h"

bool checkWorldSizes(PF_EffectWorld *in, PF_EffectWorld *out) {
	bool same = false;

	if ((in->width == out->width) && (in->height == out->height)) same = true;

	return (same);
}

static PF_PixelFloat
*getXY32(PF_EffectWorld &def, int x, int y) {
	return (PF_PixelFloat*)((char*)def.data +
		(y * def.rowbytes) +
		(x * sizeof(PF_PixelFloat)));

}

PF_Err
normalizeImg(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	outP->red = inP->red / siP->rMax;
	outP->green = inP->green / siP->rMax;
	outP->blue = inP->blue / siP->rMax;

	return err;
}

PF_Err
fftShift(
	void *refcon,
	A_long threadNum,
	A_long yL,
	A_long numOfIterations)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long	hHalf = siP->inHeight / 2,
		wHalf = siP->inWidth / 2,
		yL2 = yL + hHalf;

	for (A_long xL = 0; xL < siP->inWidth; xL++) {
		A_long xL2 = xL + wHalf;

		if (xL2 >= siP->inWidth) xL2 = xL2 - siP->inWidth;
		if (yL2 >= siP->inHeight) yL2 = yL2 - siP->inHeight;

		unsigned long srcPPixel = (yL2 * siP->inWidth) + xL2;
		unsigned long dstPPixel = (yL * siP->inWidth) + xL;
		PF_PixelFloat *srcPixel = (PF_PixelFloat*)((char*)siP->copy_worldP->data + (srcPPixel * sizeof(PF_PixelFloat)));
		PF_PixelFloat *dstPixel = (PF_PixelFloat*)((char*)siP->output_worldP->data + (dstPPixel * sizeof(PF_PixelFloat)));
	
		*dstPixel = *srcPixel;
	}

	return err;
}

PF_Err
circularShift(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	// TODO: 
	// ODD numbeers.
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long xL2 = xL; 
	A_long yL2 = yL;
	A_long wHalf = siP->inWidth / 2;
	A_long hHalf = siP->inHeight / 2;

	if (yL < hHalf) yL2 = yL + hHalf;
	else yL2 = yL - hHalf;

	if (xL < wHalf) {
		xL2 = xL + wHalf;

		if (siP->inWidth % 2 != 0) xL2++;
		if (siP->inHeight % 2 != 0) yL2++;

		unsigned long dstPointAt = (yL2 * siP->inWidth) + xL2;

		PF_PixelFloat *pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (dstPointAt * sizeof(PF_PixelFloat)));
		PF_PixelFloat tmpPixel = *inP;

		if (!siP->inverseCB) *outP = *pixelPointerAt;
		else {
			*outP = *(PF_PixelFloat*)((char*)siP->input_worldP->data + (dstPointAt * sizeof(PF_PixelFloat)));
		}

		*pixelPointerAt = tmpPixel;
	}
	
	return err;
}

PF_Err
pixelToVector(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	for (A_long xL = 0; xL < siP->inWidth; xL++) {
		A_long currentIndex = (iterationCount * siP->inWidth) + xL;
		PF_PixelFloat *pixelPointerAt = NULL;

		pixelPointerAt = (PF_PixelFloat*)((char*)siP->input_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
		siP->imgVectorR[currentIndex].real(pixelPointerAt->red);
		siP->imgVectorG[currentIndex].real(pixelPointerAt->green);
		siP->imgVectorB[currentIndex].real(pixelPointerAt->blue);
	}

	return err;
}

PF_Err
vectorToPixel(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long currentIndex = (yL * siP->inWidth) + xL;

	PF_FpShort finalR, finalG, finalB;

	if (!siP->inverseCB) {
		if (!siP->fftPhase) {
			finalR = log(1 + abs(siP->imgVectorR[currentIndex]));
			finalG = log(1 + abs(siP->imgVectorG[currentIndex]));
			finalB = log(1 + abs(siP->imgVectorB[currentIndex]));

			if (finalR > siP->rMax) siP->rMax = finalR;
			if (finalR > siP->gMax) siP->gMax = finalG;
			if (finalR > siP->bMax) siP->bMax = finalB;
		}
		else {
			finalR = atan2(siP->imgVectorR[currentIndex].imag(), siP->imgVectorR[currentIndex].real());
			finalG = atan2(siP->imgVectorG[currentIndex].imag(), siP->imgVectorG[currentIndex].real());
			finalB = atan2(siP->imgVectorB[currentIndex].imag(), siP->imgVectorB[currentIndex].real());
		}
		
	}
	else {
		finalR = abs(siP->imgVectorR[currentIndex]);
		finalG = abs(siP->imgVectorG[currentIndex]);
		finalB = abs(siP->imgVectorB[currentIndex]);

		if (finalR > siP->rMax) siP->rMax = finalR;
		if (finalR > siP->gMax) siP->gMax = finalG;
		if (finalR > siP->bMax) siP->bMax = finalB;
	}

	outP->alpha = 1;
	outP->red = finalR;
	outP->green = finalG;
	outP->blue = finalB;

	return err;
}

PF_Err
fftRowsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	siP->tmpCount = siP->tmpCount + 1;
	if (threadNum > siP->tmpMax) siP->tmpMax = threadNum;

	std::vector<std::complex<double>> currentRowVecR, currentRowVecG, currentRowVecB;

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;
		currentRowVecR.push_back(siP->imgVectorR[currentIndex]);
		currentRowVecG.push_back(siP->imgVectorG[currentIndex]);
		currentRowVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::transform(currentRowVecR);
	fft::transform(currentRowVecG);
	fft::transform(currentRowVecB);

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;
		siP->imgVectorR.operator[](currentIndex) = currentRowVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentRowVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentRowVecB[i];
	}

	return err;
}

PF_Err
fftColumnsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	std::vector<std::complex<double>> currentColVecR, currentColVecG, currentColVecB;

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		currentColVecR.push_back(siP->imgVectorR[currentIndex]);
		currentColVecG.push_back(siP->imgVectorG[currentIndex]);
		currentColVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::transform(currentColVecR);
	fft::transform(currentColVecG);
	fft::transform(currentColVecB);

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
	}

	return err;
}

PF_Err
ifftRowsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	siP->tmpCount = siP->tmpCount + 1;
	if (threadNum > siP->tmpMax) siP->tmpMax = threadNum;

	std::vector<std::complex<double>> currentRowVecR, currentRowVecG, currentRowVecB;

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;

		PF_PixelFloat *pixelPointerAt = (PF_PixelFloat*)((char*)siP->tmp_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
		std::complex<double>	invR = exp(imaginaryI * double(pixelPointerAt->red)),   
								invG = exp(imaginaryI * double(pixelPointerAt->green)),
								invB = exp(imaginaryI * double(pixelPointerAt->blue));  

		pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
		std::complex<double>	tmpB = exp(pixelPointerAt->blue) - 1;

		invR = std::complex<double>(exp(pixelPointerAt->red) - 1) * invR;
		invG = std::complex<double>(exp(pixelPointerAt->green) - 1) * invG;
		invB = tmpB * invB;

		currentRowVecR.push_back(invR);
		currentRowVecG.push_back(invG);
		currentRowVecB.push_back(invB);
	}

	fft::inverseTransform(currentRowVecR);
	fft::inverseTransform(currentRowVecG);
	fft::inverseTransform(currentRowVecB);

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;
		siP->imgVectorR.operator[](currentIndex) = currentRowVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentRowVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentRowVecB[i];
	}

	return err;
}

PF_Err
ifftColumnsTh(
	void *refcon,
	A_long threadNum,
	A_long iterationCount,
	A_long numOfIterations)
{
	PF_Err err = PF_Err_NONE;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	std::vector<std::complex<double>> currentColVecR, currentColVecG, currentColVecB;

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		currentColVecR.push_back(siP->imgVectorR[currentIndex]);
		currentColVecG.push_back(siP->imgVectorG[currentIndex]);
		currentColVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::inverseTransform(currentColVecR);
	fft::inverseTransform(currentColVecG);
	fft::inverseTransform(currentColVecB);

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;
		siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
	}

	return err;
}

PF_Err
tmpRender16(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_Pixel16 	*inP,
	PF_Pixel16 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	*outP = *inP;

	return err;
}

PF_Err
tmpRender8(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_Pixel8 	*inP,
	PF_Pixel8 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	*outP = *inP;

	return err;
}