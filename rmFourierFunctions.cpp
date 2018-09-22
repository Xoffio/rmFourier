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
		PF_PixelFloat *pixelPointerAt = NULL;

		pixelPointerAt = (PF_PixelFloat*)((char*)siP->input_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));

		if (siP->colorComputations[3]) {
			double grayPixel = (pixelPointerAt->red + pixelPointerAt->green + pixelPointerAt->blue) / 3;
			siP->imgVectorGS[currentIndex].real(grayPixel);
		}
		else {
			if (siP->colorComputations[0]) siP->imgVectorR[currentIndex].real(pixelPointerAt->red);
			if (siP->colorComputations[1]) siP->imgVectorG[currentIndex].real(pixelPointerAt->green);
			if (siP->colorComputations[2]) siP->imgVectorB[currentIndex].real(pixelPointerAt->blue);
		}
		
	}

	/*siP->currentProcess += 1;
	if (siP->currentProcess >= numOfIterations) {
		siP->currentProcess = 0;
		ERR(PF_PROGRESS(&siP->in_data, 1, 10));
	}*/

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

	PF_FpShort finalR, finalG, finalB, finalGS;

	if (!siP->inverseCB) {
		if (!siP->fftPhase) {

			if (siP->colorComputations[3]) {
				finalGS = log(1 + abs(siP->imgVectorGS[currentIndex]));
				finalR = finalGS;
				finalG = finalGS;
				finalB = finalGS;
			}
			else {
				if (siP->colorComputations[0]) finalR = log(1 + abs(siP->imgVectorR[currentIndex]));
				else finalR = 0;

				if (siP->colorComputations[1]) finalG = log(1 + abs(siP->imgVectorG[currentIndex]));
				else finalG = 0;

				if (siP->colorComputations[2]) finalB = log(1 + abs(siP->imgVectorB[currentIndex]));
				else finalB = 0;
			}

			if (finalR > siP->rMax) siP->rMax = finalR;
			if (finalR > siP->gMax) siP->gMax = finalG;
			if (finalR > siP->bMax) siP->bMax = finalB;
		}
		else {
			if (siP->colorComputations[3]) {
				finalGS = atan2(siP->imgVectorGS[currentIndex].imag(), siP->imgVectorGS[currentIndex].real());
				finalR = finalGS;
				finalG = finalGS;
				finalB = finalGS;
			}
			else {
				if (siP->colorComputations[0]) finalR = atan2(siP->imgVectorR[currentIndex].imag(), siP->imgVectorR[currentIndex].real());
				else finalR = 0;

				if (siP->colorComputations[1]) finalG = atan2(siP->imgVectorG[currentIndex].imag(), siP->imgVectorG[currentIndex].real());
				else finalG = 0;

				if (siP->colorComputations[2]) finalB = atan2(siP->imgVectorB[currentIndex].imag(), siP->imgVectorB[currentIndex].real());
				else finalB = 0;
			}
			
		}
		
	}
	else {
		if (siP->colorComputations[3]) {
			finalGS = finalR = abs(siP->imgVectorGS[currentIndex]);
			finalR = finalGS;
			finalG = finalGS;
			finalB = finalGS;
		}
		else {
			if (siP->colorComputations[0]) finalR = abs(siP->imgVectorR[currentIndex]);
			else finalR = 0;

			if (siP->colorComputations[1]) finalG = abs(siP->imgVectorG[currentIndex]);
			else finalG = 0;

			if (siP->colorComputations[2]) finalB = abs(siP->imgVectorB[currentIndex]);
			else finalB = 0;
		}

		if (finalR > siP->rMax) siP->rMax = finalR;
		if (finalG > siP->gMax) siP->gMax = finalG;
		if (finalB > siP->bMax) siP->bMax = finalB;
	}

	outP->alpha = inP->alpha;
	outP->red = finalR;
	outP->green = finalG;
	outP->blue = finalB;

	return err;
}

PF_Err
vectorToPixelTmp(
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

	outP->alpha = inP->alpha;
	outP->red = log(1 + sqrt(pow(siP->out[currentIndex][0], 2) + pow(siP->out[currentIndex][1], 2)) );
	outP->green = 0;
	outP->blue = 0;

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

	std::vector<std::complex<double> > currentRowVecR, currentRowVecG, currentRowVecB, currentRowVecGS;

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;

		if (siP->colorComputations[3]) currentRowVecGS.push_back(siP->imgVectorGS[currentIndex]);
		else {
			if (siP->colorComputations[0]) currentRowVecR.push_back(siP->imgVectorR[currentIndex]);
			if (siP->colorComputations[1]) currentRowVecG.push_back(siP->imgVectorG[currentIndex]);
			if (siP->colorComputations[2]) currentRowVecB.push_back(siP->imgVectorB[currentIndex]);
		}
	}

	if (siP->colorComputations[3]) fft::transform(currentRowVecGS, siP);
	else {
		if (siP->colorComputations[0]) fft::transform(currentRowVecR, siP);
		if (siP->colorComputations[1]) fft::transform(currentRowVecG, siP);
		if (siP->colorComputations[2]) fft::transform(currentRowVecB, siP);
	}

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;

		if (siP->colorComputations[3]) siP->imgVectorGS.operator[](currentIndex) = currentRowVecGS[i];
		else {
			if (siP->colorComputations[0]) siP->imgVectorR.operator[](currentIndex) = currentRowVecR[i];
			if (siP->colorComputations[1]) siP->imgVectorG.operator[](currentIndex) = currentRowVecG[i];
			if (siP->colorComputations[2]) siP->imgVectorB.operator[](currentIndex) = currentRowVecB[i];
		}
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

	std::vector<std::complex<double> > currentColVecR, currentColVecG, currentColVecB, currentColVecGS;

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;

		if (siP->colorComputations[3]) currentColVecGS.push_back(siP->imgVectorGS[currentIndex]);
		else {
			if (siP->colorComputations[0]) currentColVecR.push_back(siP->imgVectorR[currentIndex]);
			if (siP->colorComputations[1]) currentColVecG.push_back(siP->imgVectorG[currentIndex]);
			if (siP->colorComputations[2]) currentColVecB.push_back(siP->imgVectorB[currentIndex]);
		}
	}

	if (siP->colorComputations[3]) fft::transform(currentColVecGS, siP);
	else {
		if (siP->colorComputations[0]) fft::transform(currentColVecR, siP);
		if (siP->colorComputations[1]) fft::transform(currentColVecG, siP);
		if (siP->colorComputations[2]) fft::transform(currentColVecB, siP);
	}

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;

		if (siP->colorComputations[3]) siP->imgVectorGS.operator[](currentIndex) = currentColVecGS[i];
		else {
			if (siP->colorComputations[0]) siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
			if (siP->colorComputations[1]) siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
			if (siP->colorComputations[2]) siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
		}
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

	std::vector<std::complex<double> > currentRowVecR, currentRowVecG, currentRowVecB, currentRowVecGS;

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;

		PF_PixelFloat *pixelPointerAt = (PF_PixelFloat*)((char*)siP->tmp_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
		std::complex<double> invR, invG, invB, invGS;

		if (siP->colorComputations[3]) {
			double grayPixel = (pixelPointerAt->red + pixelPointerAt->green + pixelPointerAt->blue) / 3;
			invGS = exp(imaginaryI * grayPixel);

			pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
			grayPixel = ((exp(pixelPointerAt->red) - 1) + (exp(pixelPointerAt->green) - 1) + (exp(pixelPointerAt->blue) - 1)) / 3;
			invGS = std::complex<double>(grayPixel) * invGS;
			currentRowVecGS.push_back(invGS);
		}
		else {
			if (siP->colorComputations[0]) invR = exp(imaginaryI * double(pixelPointerAt->red));
			if (siP->colorComputations[1]) invG = exp(imaginaryI * double(pixelPointerAt->green));
			if (siP->colorComputations[2]) invB = exp(imaginaryI * double(pixelPointerAt->blue));

			pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));

			if (siP->colorComputations[0]) {
				invR = std::complex<double>(exp(pixelPointerAt->red) - 1) * invR;
				currentRowVecR.push_back(invR);
			}
			if (siP->colorComputations[1]) {
				invG = std::complex<double>(exp(pixelPointerAt->green) - 1) * invG;
				currentRowVecG.push_back(invG);
			}
			if (siP->colorComputations[2]) {
				invB = std::complex<double>(exp(pixelPointerAt->blue) - 1) * invB;
				currentRowVecB.push_back(invB);
			}
		}
		
	}

	if (siP->colorComputations[3]) fft::inverseTransform(currentRowVecGS, siP);
	else {
		if (siP->colorComputations[0]) fft::inverseTransform(currentRowVecR, siP);
		if (siP->colorComputations[1]) fft::inverseTransform(currentRowVecG, siP);
		if (siP->colorComputations[2]) fft::inverseTransform(currentRowVecB, siP);
	}

	for (A_long i = 0; i < siP->inWidth; i++) {
		A_long currentIndex = (siP->inWidth*iterationCount) + i;

		if (siP->colorComputations[3]) siP->imgVectorGS.operator[](currentIndex) = currentRowVecGS[i];
		else {
			if (siP->colorComputations[0]) siP->imgVectorR.operator[](currentIndex) = currentRowVecR[i];
			if (siP->colorComputations[1]) siP->imgVectorG.operator[](currentIndex) = currentRowVecG[i];
			if (siP->colorComputations[2]) siP->imgVectorB.operator[](currentIndex) = currentRowVecB[i];
		}
		
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

	std::vector<std::complex<double> > currentColVecR, currentColVecG, currentColVecB, currentColVecGS;

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;

		if (siP->colorComputations[3]) currentColVecGS.push_back(siP->imgVectorGS[currentIndex]);
		else {
			if (siP->colorComputations[0]) currentColVecR.push_back(siP->imgVectorR[currentIndex]);
			if (siP->colorComputations[1]) currentColVecG.push_back(siP->imgVectorG[currentIndex]);
			if (siP->colorComputations[2]) currentColVecB.push_back(siP->imgVectorB[currentIndex]);
		}
	}

	if (siP->colorComputations[3]) fft::inverseTransform(currentColVecGS, siP);
	else {
		if (siP->colorComputations[0]) fft::inverseTransform(currentColVecR, siP);
		if (siP->colorComputations[1]) fft::inverseTransform(currentColVecG, siP);
		if (siP->colorComputations[2]) fft::inverseTransform(currentColVecB, siP);
	}

	for (A_long i = 0; i < siP->inHeight; i++) {
		A_long currentIndex = (siP->inWidth*i) + iterationCount;

		if (siP->colorComputations[3]) siP->imgVectorGS.operator[](currentIndex) = currentColVecGS[i];
		else {
			if (siP->colorComputations[0]) siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
			if (siP->colorComputations[1]) siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
			if (siP->colorComputations[2]) siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
		}
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

void preTransform(size_t vSize, void *refcon) {
	const double M_PI = 3.14159265358979323846;
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	// Length variables
	size_t n = vSize;//vec.size();

	if ((n & (n - 1)) == 0) {  // If is power of 2
		if (n != siP->expTable.size()) {
			siP->expTable.clear();
			siP->transformType = 1;
			siP->levels = 0;  // Compute levels = floor(log2(n))
			for (size_t temp = n; temp > 1U; temp >>= 1)
				siP->levels++;
			if (static_cast<size_t>(1U) << siP->levels != n)
				throw "Length is not a power of 2";

			// Trignometric table
			for (size_t i = 0; i < n / 2; i++)
				siP->expTable.push_back(std::exp(std::complex<double>(0, -2 * M_PI * i / n)));
		}
	}
	else{ 
		if (n != siP->expTable.size()) {
			siP->transformType = 2;
			siP->m = 1;
			while (siP->m / 2 <= n) {
				if (siP->m > SIZE_MAX / 2)
					throw "Vector too large";
				siP->m *= 2;
			}

			siP->expTable.clear();

			// Trignometric table
			for (size_t i = 0; i < n; i++) {
				unsigned long long temp = static_cast<unsigned long long>(i) * i;
				temp %= static_cast<unsigned long long>(n) * 2;
				double angle = M_PI * temp / n;
				// Less accurate alternative if long long is unavailable: double angle = M_PI * i * i / n;
				siP->expTable.push_back(std::exp(std::complex<double>(0, -angle)));
			}

			siP->bv.resize(siP->m);
			siP->bv[0] = siP->expTable[0];
			for (size_t i = 1; i < n; i++)
				siP->bv[i] = siP->bv[siP->m - i] = std::conj(siP->expTable[i]);

			siP->preBluestein = false;

			// ------------ Trignometric table of convolution ------------
			siP->convExpTable.clear();
			siP->convLevels = 0;  // Compute levels = floor(log2(n))
			for (size_t temp = siP->m; temp > 1U; temp >>= 1)
				siP->convLevels++;
			if (static_cast<size_t>(1U) << siP->convLevels != siP->m)
				throw "Length is not a power of 2";

			// Trignometric table
			for (size_t i = 0; i < siP->m / 2; i++)
				siP->convExpTable.push_back(std::exp(std::complex<double>(0, -2 * M_PI * i / siP->m)));
		}
	}
}