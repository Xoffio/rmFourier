#include "rmFourier.h"

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

	if (!(siP->rMax == 0)) outP->red = inP->red / siP->rMax;
	if (!(siP->gMax == 0)) outP->green = inP->green / siP->gMax;
	if (!(siP->bMax == 0)) outP->blue = inP->blue / siP->bMax;

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
ifftShift(
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
			yL2 = yL - hHalf;

	for (A_long xL = 0; xL < siP->inWidth; xL++) {
		A_long xL2 = xL - wHalf;

		if (xL2 < 0) xL2 = xL2 + siP->inWidth;
		if (yL2 < 0) yL2 = yL2 + siP->inHeight;

		unsigned long srcPPixel = (yL2 * siP->inWidth) + xL2;
		unsigned long dstPPixel = (yL * siP->inWidth) + xL;
		PF_PixelFloat *srcPixel = (PF_PixelFloat*)((char*)siP->input_worldP->data + (srcPPixel * sizeof(PF_PixelFloat)));
		PF_PixelFloat *dstPixel = (PF_PixelFloat*)((char*)siP->output_worldP->data + (dstPPixel * sizeof(PF_PixelFloat)));

		*dstPixel = *srcPixel;
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
		PF_PixelFloat *pixelPointerAt, *phasePixelPointAt;

		if (!siP->inverseCB) {
			pixelPointerAt = (PF_PixelFloat*)((char*)siP->input_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));

			if (siP->colorComputations[3]) {
				siP->inVectorGS[currentIndex][0] = (pixelPointerAt->red + pixelPointerAt->green + pixelPointerAt->blue)/3;
				siP->inVectorGS[currentIndex][1] = 0.0f;
			}
			else {
				if (siP->colorComputations[0]) {
					siP->inVectorR[currentIndex][0] = pixelPointerAt->red;
					siP->inVectorR[currentIndex][1] = 0.0f;
				}
				if (siP->colorComputations[1]) {
					siP->inVectorG[currentIndex][0] = pixelPointerAt->green;
					siP->inVectorG[currentIndex][1] = 0.0f;
				}
				if (siP->colorComputations[2]) {
					siP->inVectorB[currentIndex][0] = pixelPointerAt->blue;
					siP->inVectorB[currentIndex][1] = 0.0f;
				}
			}
			
		}
		else {
			pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
			phasePixelPointAt = (PF_PixelFloat*)((char*)siP->tmp_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));

			if (siP->colorComputations[3]) {
				double tmpMagGrayscale, tmpPhaGrayscale;
				tmpMagGrayscale = (pixelPointerAt->red + pixelPointerAt->green + pixelPointerAt->blue) / 3;
				tmpPhaGrayscale = (phasePixelPointAt->red + phasePixelPointAt->green + phasePixelPointAt->blue) / 3;

				siP->inVectorGS[currentIndex][0] = tmpMagGrayscale * cos(tmpPhaGrayscale);
				siP->inVectorGS[currentIndex][1] = tmpMagGrayscale * sin(tmpPhaGrayscale);
			}
			else {
				if (siP->colorComputations[0]) {
					siP->inVectorR[currentIndex][0] = pixelPointerAt->red * cos(phasePixelPointAt->red);
					siP->inVectorR[currentIndex][1] = pixelPointerAt->red * sin(phasePixelPointAt->red);
				}
				if (siP->colorComputations[1]) {
					siP->inVectorG[currentIndex][0] = pixelPointerAt->green * cos(phasePixelPointAt->green);
					siP->inVectorG[currentIndex][1] = pixelPointerAt->green * sin(phasePixelPointAt->green);
				}
				if (siP->colorComputations[2]) {
					siP->inVectorB[currentIndex][0] = (pixelPointerAt->blue) * cos(phasePixelPointAt->blue);
					siP->inVectorB[currentIndex][1] = (pixelPointerAt->blue) * sin(phasePixelPointAt->blue);
				}
			}
		}
	}

	return err;
}

PF_Err
preVectorToPixel(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err					err = PF_Err_NONE;
	PF_PixelFloat			tmpPixel;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long currentIndex = (yL * siP->inWidth) + xL;

	if (!siP->inverseCB) {
		if (!siP->fftPhase) { // Compute the magnitude

			if (siP->colorComputations[3]) {
				tmpPixel.red = sqrt(pow(siP->outVectorGS[currentIndex][0], 2) + pow(siP->outVectorGS[currentIndex][1], 2));
				tmpPixel.green = tmpPixel.red;
				tmpPixel.blue = tmpPixel.red;
			}
			else {
				if (siP->colorComputations[0]) tmpPixel.red = sqrt(pow(siP->outVectorR[currentIndex][0], 2) + pow(siP->outVectorR[currentIndex][1], 2));
				else tmpPixel.red = 0;

				if (siP->colorComputations[1]) tmpPixel.green = sqrt(pow(siP->outVectorG[currentIndex][0], 2) + pow(siP->outVectorG[currentIndex][1], 2));
				else tmpPixel.green = 0;

				if (siP->colorComputations[2]) tmpPixel.blue = sqrt(pow(siP->outVectorB[currentIndex][0], 2) + pow(siP->outVectorB[currentIndex][1], 2));
				else tmpPixel.blue = 0;
			}
		}
		else { // Compute the phase

			if (siP->colorComputations[3]) {
				tmpPixel.red = atan2(siP->outVectorGS[currentIndex][1], siP->outVectorGS[currentIndex][0]);
				tmpPixel.green = tmpPixel.red;
				tmpPixel.blue = tmpPixel.red;
			}
			else {
				if (siP->colorComputations[0]) tmpPixel.red = atan2(siP->outVectorR[currentIndex][1], siP->outVectorR[currentIndex][0]);
				else tmpPixel.red = 0;

				if (siP->colorComputations[1]) tmpPixel.green = atan2(siP->outVectorG[currentIndex][1], siP->outVectorG[currentIndex][0]);
				else tmpPixel.green = 0;

				if (siP->colorComputations[2]) tmpPixel.blue = atan2(siP->outVectorB[currentIndex][1], siP->outVectorB[currentIndex][0]);
				else tmpPixel.blue = 0;
			}
		}
	}
	else { // When inverse
		/*if (siP->colorComputations[3]) {
			tmpPixel.red = siP->outVectorGS[currentIndex][0];
			tmpPixel.green = siP->outVectorGS[currentIndex][0];
			tmpPixel.blue = siP->outVectorGS[currentIndex][0];
		}
		else {
			if (siP->colorComputations[0]) tmpPixel.red = siP->outVectorR[currentIndex][0];
			else tmpPixel.red = 0;

			if (siP->colorComputations[1]) tmpPixel.green = siP->outVectorG[currentIndex][0];
			else tmpPixel.green = 0;

			if (siP->colorComputations[2]) tmpPixel.blue = siP->outVectorB[currentIndex][0];
			else tmpPixel.blue = 0;
		}*/
	}

	if (tmpPixel.red > siP->rMax) siP->rMax = tmpPixel.red;
	if (tmpPixel.green > siP->gMax) siP->gMax = tmpPixel.green;
	if (tmpPixel.blue > siP->bMax) siP->bMax = tmpPixel.blue;

	/*outP->alpha = inP->alpha;
	outP->red = tmpPixel.red;
	outP->green = tmpPixel.green;
	outP->blue = tmpPixel.blue;*/
	
	siP->preR[currentIndex] = tmpPixel.red;
	siP->preG[currentIndex] = tmpPixel.green;
	siP->preB[currentIndex] = tmpPixel.blue;

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
	PF_Err					err = PF_Err_NONE;
	PF_PixelFloat			tmpPixel;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long currentIndex = (yL * siP->inWidth) + xL;

	if (!siP->inverseCB) {
		tmpPixel.red = siP->preR[currentIndex] / siP->rMax;
		tmpPixel.green = siP->preG[currentIndex] / siP->gMax;
		tmpPixel.blue = siP->preB[currentIndex] / siP->bMax;
	}
	else { // When inverse
		if (siP->colorComputations[3]) {
			tmpPixel.red = siP->outVectorGS[currentIndex][0];
			tmpPixel.green = siP->outVectorGS[currentIndex][0];
			tmpPixel.blue = siP->outVectorGS[currentIndex][0];
		}
		else {
			if (siP->colorComputations[0]) tmpPixel.red = siP->outVectorR[currentIndex][0];
			else tmpPixel.red = 0;

			if (siP->colorComputations[1]) tmpPixel.green = siP->outVectorG[currentIndex][0];
			else tmpPixel.green = 0;

			if (siP->colorComputations[2]) tmpPixel.blue = siP->outVectorB[currentIndex][0];
			else tmpPixel.blue = 0;
		}
	}

	outP->alpha = inP->alpha;
	outP->red	= tmpPixel.red;
	outP->green = tmpPixel.green;
	outP->blue	= tmpPixel.blue;

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