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
pixelToVectorTmp(
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
		else {
			pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
			phasePixelPointAt = (PF_PixelFloat*)((char*)siP->tmp_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));

			if (siP->colorComputations[0]) {
				siP->inVectorR[currentIndex][0] = pixelPointerAt->red * cos(phasePixelPointAt->red);
				siP->inVectorR[currentIndex][1] = pixelPointerAt->red * sin(phasePixelPointAt->red);
			}
			if (siP->colorComputations[1]) {
				siP->inVectorG[currentIndex][0] = pixelPointerAt->green * cos(phasePixelPointAt->green);
				siP->inVectorG[currentIndex][1] = pixelPointerAt->green * sin(phasePixelPointAt->green);
			}
			if (siP->colorComputations[2]) {
				siP->inVectorB[currentIndex][0] = (1/pixelPointerAt->blue) * cos(phasePixelPointAt->blue);
				siP->inVectorB[currentIndex][1] = (1/pixelPointerAt->blue) * sin(phasePixelPointAt->blue);
			}
		}
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
	PF_Err					err = PF_Err_NONE;
	PF_PixelFloat			tmpPixel;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long currentIndex = (yL * siP->inWidth) + xL;

	if (!siP->inverseCB) {
		if (!siP->fftPhase) { // Compute the magnitude

			if (siP->colorComputations[3]) {

			}
			else {
				if (siP->colorComputations[0]) tmpPixel.red = sqrt(pow(siP->outVectorR[currentIndex][0], 2) + pow(siP->outVectorR[currentIndex][1], 2));
				else tmpPixel.red = 0;

				if (siP->colorComputations[1]) tmpPixel.green = sqrt(pow(siP->outVectorG[currentIndex][0], 2) + pow(siP->outVectorG[currentIndex][1], 2));
				else tmpPixel.green = 0;

				if (siP->colorComputations[2]) tmpPixel.blue = 1 / sqrt(pow(siP->outVectorB[currentIndex][0], 2) + pow(siP->outVectorB[currentIndex][1], 2));
				else tmpPixel.blue = 0;
			}
		}
		else { // Compute the phase

			if (siP->colorComputations[3]) {

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
		if (siP->colorComputations[3]) {

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

	if (tmpPixel.red > siP->rMax) siP->rMax = tmpPixel.red;
	if (tmpPixel.green > siP->gMax) siP->gMax = tmpPixel.green;
	if (tmpPixel.blue > siP->bMax) siP->bMax = tmpPixel.blue;

	outP->alpha = inP->alpha;
	outP->red	= tmpPixel.red;
	outP->green = tmpPixel.green;
	outP->blue	= tmpPixel.blue;

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