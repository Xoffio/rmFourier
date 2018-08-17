#include "rmFourier.h"


static PF_PixelFloat
*getXY32(PF_EffectWorld &def, int x, int y) {
	return (PF_PixelFloat*)((char*)def.data +
		(y * def.rowbytes) +
		(x * sizeof(PF_PixelFloat)));

}

PF_EffectWorld tmpFourier(PF_EffectWorld inWorld) {
	std::vector<std::complex<double>>	imgRedDataVector, imgGreenDataVector, imgBlueDataVector, finalImgGreenDataVector;
	std::vector<double>					copyImgRedDataVector, copyImgGreenDataVector, copyImgBlueDataVector;
	int imgArraySize = inWorld.height * inWorld.width;

	// Convert the raw images to vectors
	PF_PixelFloat* pixelPointerAtIndex;
	for (unsigned long index = 0; index < imgArraySize; index++) {
		pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (index * sizeof(PF_PixelFloat)));
		imgRedDataVector.push_back((std::complex<double>) pixelPointerAtIndex->red);
		//imgGreenDataVector.push_back((std::complex<double>) pixelPointerAtIndex->green);
		imgBlueDataVector.push_back((std::complex<double>) pixelPointerAtIndex->blue);
	}

	// Transform rows
	for (int row = 0; row < inWorld.height; row++) {
		std::vector<std::complex<double>> tmpRedVector, tmpGreenVector, tmpBlueVector;
		tmpRedVector.clear();
		tmpGreenVector.clear();
		tmpBlueVector.clear();
		for (int col = 0; col < inWorld.width; col++) {
			unsigned long pointAt = (row * inWorld.width) + col;
			pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (pointAt * sizeof(PF_PixelFloat)));
			//tmpRedVector.push_back((std::complex<double>) pixelPointerAtIndex->red);
			tmpGreenVector.push_back((std::complex<double>) pixelPointerAtIndex->green);
			//tmpBlueVector.push_back((std::complex<double>) pixelPointerAtIndex->blue);
			//finalImgRedDataVector.push_back(0);
			finalImgGreenDataVector.push_back(0);
			//finalImgBlueDataVector.push_back(0);
		}

		// Fourier Transform Row
		//fft::transform(tmpRedVector);
		fft::transform(tmpGreenVector);
		//fft::transform(tmpBlueVector);

		//imgRedDataVector.insert(std::end(imgRedDataVector), std::begin(tmpRedVector), std::end(tmpRedVector));
		imgGreenDataVector.insert(std::end(imgGreenDataVector), std::begin(tmpGreenVector), std::end(tmpGreenVector));
		//imgBlueDataVector.insert(std::end(imgBlueDataVector), std::begin(tmpBlueVector), std::end(tmpBlueVector));
	}

	// Transform Columns
	for (int col = 0; col < inWorld.width; col++) {
		std::vector<std::complex<double>> tmpRedVector, tmpGreenVector, tmpBlueVector;
		tmpRedVector.clear();
		tmpGreenVector.clear();
		tmpBlueVector.clear();
		for (int row = 0; row < inWorld.height; row++) {
			unsigned long pointAt = (row * inWorld.width) + col;
			//tmpRedVector.push_back(imgRedDataVector[pointAt]);
			tmpGreenVector.push_back(imgGreenDataVector[pointAt]);
			//tmpBlueVector.push_back(imgBlueDataVector[pointAt]);
		}
		// Fourier Transform Row
		//fft::transform(tmpRedVector);
		fft::transform(tmpGreenVector);
		//fft::transform(tmpBlueVector);
		for (int row = 0; row < inWorld.height; row++) {
			unsigned long pointAt = (row * inWorld.width) + col;
			//finalImgRedDataVector[pointAt] = tmpRedVector[row];
			finalImgGreenDataVector[pointAt] = tmpGreenVector[row];
			//finalImgBlueDataVector[pointAt] = tmpBlueVector[row];
		}
	}

	// Fourier Transform vectors
	fft::transform(imgRedDataVector);
	fft::transform(imgGreenDataVector);
	fft::transform(imgBlueDataVector);

	// Copy the Fourier data from the vectors back to the image
	double rMaxVal = 0;
	double gMaxVal = 0;
	double bMaxVal = 0;
	for (unsigned long index = 0; index < imgArraySize; index++) {
		pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (index * sizeof(PF_PixelFloat)));
		pixelPointerAtIndex->red = log(1.0 + abs(imgRedDataVector[index]));
		pixelPointerAtIndex->green = log(1.0 + abs(finalImgGreenDataVector[index]));
		pixelPointerAtIndex->blue = log(1.0 + abs(imgBlueDataVector[index]));

		// Get max value
		if (pixelPointerAtIndex->red > rMaxVal) rMaxVal = pixelPointerAtIndex->red;
		if (pixelPointerAtIndex->green > gMaxVal) gMaxVal = pixelPointerAtIndex->green;
		if (pixelPointerAtIndex->blue > bMaxVal) bMaxVal = pixelPointerAtIndex->blue;
	}

	// Normalize values
	for (unsigned long index = 0; index < imgArraySize; index++) {
		pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (index * sizeof(PF_PixelFloat)));
		pixelPointerAtIndex->red = pixelPointerAtIndex->red / rMaxVal;
		pixelPointerAtIndex->green = pixelPointerAtIndex->green / gMaxVal;
		pixelPointerAtIndex->blue = pixelPointerAtIndex->blue / bMaxVal;

		// Make a copy
		copyImgRedDataVector.push_back(pixelPointerAtIndex->red);
		copyImgGreenDataVector.push_back(pixelPointerAtIndex->green);
		copyImgBlueDataVector.push_back(pixelPointerAtIndex->blue);
	}

	// Circular shift
	for (int row = 0; row < inWorld.height; row++) {
		for (int col = 0; col < inWorld.width; col++) {
			PF_PixelFloat currentP;
			PF_Point srcP = { 0, (inWorld.height - 1 - row) };

			if (col < (inWorld.width / 2)) {
				srcP.h = col + (inWorld.width / 2);
			}
			else {
				srcP.h = col - (inWorld.width / 2);
			}

			if (row < (inWorld.height / 2)) {
				srcP.v = row + (inWorld.height / 2);
			}
			else {
				srcP.v = row - (inWorld.width / 2);
			}

			unsigned long dstPointAt = (row * inWorld.width) + col;
			unsigned long srcPointAt = (srcP.v * inWorld.width) + srcP.h;

			pixelPointerAtIndex = (PF_PixelFloat*)((char*)inWorld.data + (dstPointAt * sizeof(PF_PixelFloat)));

			pixelPointerAtIndex->red = copyImgRedDataVector[srcPointAt];
			pixelPointerAtIndex->green = copyImgGreenDataVector[srcPointAt];
			//pixelPointerAtIndex->blue = copyImgBlueDataVector[srcPointAt];
		}
	}

	return(inWorld);
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
circularShift(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long xL2 = xL; 
	A_long yL2 = yL;
	A_long wHalf = siP->imgWidth / 2;
	A_long hHalf = siP->imgHeight / 2;

	if (yL < hHalf) yL2 = yL + hHalf;
	else yL2 = yL - hHalf;

	if (xL < wHalf) {
		xL2 = xL + wHalf;

		unsigned long dstPointAt = (yL2 * siP->imgWidth) + xL2;

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

/*PF_Err
pixelToVectorOld(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);

	A_long currentIndex = (yL * siP->in_data.width) + xL;
	
	siP->imgVectorR[currentIndex].real(inP->red);
	siP->imgVectorG[currentIndex].real(inP->green);
	siP->imgVectorB[currentIndex].real(inP->blue);
	
	return err;
}*/

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

	for (A_long xL = 0; xL < siP->imgWidth; xL++) {
		A_long currentIndex = (iterationCount * siP->in_data.width) + xL;
		PF_PixelFloat *pixelPointerAt = NULL;

		/*if (siP->inverseCB) {
			pixelPointerAt = (PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat))); //(PF_PixelFloat*)((char*)siP->output_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
			siP->imgVectorR[currentIndex] = exp(pixelPointerAt->red) - 1;
			siP->imgVectorG[currentIndex] = exp(pixelPointerAt->green) - 1;
			siP->imgVectorB[currentIndex] = exp(pixelPointerAt->blue) - 1;
		}
		else {*/
			pixelPointerAt = (PF_PixelFloat*)((char*)siP->input_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
			siP->imgVectorR[currentIndex].real(pixelPointerAt->red);
			siP->imgVectorG[currentIndex].real(pixelPointerAt->green);
			siP->imgVectorB[currentIndex].real(pixelPointerAt->blue);
		//}

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

	A_long currentIndex = (yL * siP->in_data.width) + xL;

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
			finalG = atan2(siP->imgVectorG[currentIndex].imag(), siP->imgVectorR[currentIndex].real());
			finalB = atan2(siP->imgVectorB[currentIndex].imag(), siP->imgVectorR[currentIndex].real());
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

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*iterationCount) + i;
		currentRowVecR.push_back(siP->imgVectorR[currentIndex]);
		currentRowVecG.push_back(siP->imgVectorG[currentIndex]);
		currentRowVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::transform(currentRowVecR);
	fft::transform(currentRowVecG);
	fft::transform(currentRowVecB);

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*iterationCount) + i;
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

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*i) + iterationCount;
		currentColVecR.push_back(siP->imgVectorR[currentIndex]);
		currentColVecG.push_back(siP->imgVectorG[currentIndex]);
		currentColVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::transform(currentColVecR);
	fft::transform(currentColVecG);
	fft::transform(currentColVecB);

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*i) + iterationCount;
		siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
	}

	return err;
}

//---------------

/*std::complex<double> multiply(std::complex<double> x, std::complex<double> y){
	x.real = (x.real * y.real) - (x.imag * y.imag);
	x.imag = (x.imag * y.real) + (x.real * y.imag);

	return (x);
}*/

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

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*iterationCount) + i;

		PF_PixelFloat *pixelPointerAt = (PF_PixelFloat*)((char*)siP->tmp_worldP->data + (currentIndex * sizeof(PF_PixelFloat)));
		std::complex<double>	invR = exp(imaginaryI * double(pixelPointerAt->red)),   //atan2(siP->imgVectorR[currentIndex].imag(), siP->imgVectorR[currentIndex].real())),
								invG = exp(imaginaryI * double(pixelPointerAt->green)), //atan2(siP->imgVectorG[currentIndex].imag(), siP->imgVectorR[currentIndex].real())),
								invB = exp(imaginaryI * double(pixelPointerAt->blue));  //atan2(siP->imgVectorB[currentIndex].imag(), siP->imgVectorR[currentIndex].real()));

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

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*iterationCount) + i;
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

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*i) + iterationCount;
		currentColVecR.push_back(siP->imgVectorR[currentIndex]);
		currentColVecG.push_back(siP->imgVectorG[currentIndex]);
		currentColVecB.push_back(siP->imgVectorB[currentIndex]);
	}

	fft::inverseTransform(currentColVecR);
	fft::inverseTransform(currentColVecG);
	fft::inverseTransform(currentColVecB);

	for (A_long i = 0; i < siP->imgWidth; i++) {
		A_long currentIndex = (siP->imgWidth*i) + iterationCount;
		siP->imgVectorR.operator[](currentIndex) = currentColVecR[i];
		siP->imgVectorG.operator[](currentIndex) = currentColVecG[i];
		siP->imgVectorB.operator[](currentIndex) = currentColVecB[i];
	}

	return err;
}