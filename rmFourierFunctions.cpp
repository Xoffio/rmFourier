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
pushPixelToVector(
	void			*refcon,
	A_long 			xL,
	A_long 			yL,
	PF_PixelFloat 	*inP,
	PF_PixelFloat 	*outP)
{
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	PF_Err				err = PF_Err_NONE;

	AEGP_SuiteHandler suites(siP->in_data.pica_basicP);


	if (siP->fftState == 0) {
		siP->tmpVectorR.operator[](xL) = inP->red;
	}
	if (siP->fftState == 1) {
		A_long currentIndex = (yL*siP->in_data.width) + xL;
		siP->tmpVectorR.operator[](yL) = siP->imgRedDataVector->operator[](currentIndex);
	}
	if (siP->fftState == 2) {

		outP->alpha = inP->alpha;
		outP->red = log(1 + abs(siP->tmpVectorR.operator[](yL)));
		outP->green = inP->green;
		outP->blue = inP->blue;

		if (outP->red > siP->rMax) siP->rMax = outP->red;
	}


	return err;
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

	outP->red = outP->red / siP->rMax;
	outP->green = outP->green / siP->rMax;
	outP->blue = outP->blue / siP->rMax;


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

	if ((xL < wHalf) && (yL < hHalf)) {
		xL2 = xL + wHalf;
		yL2 = yL + hHalf;
	}
	if ((xL < wHalf) && (yL >= hHalf)) {
		xL2 = xL + wHalf;
		yL2 = yL - hHalf;
	}

	unsigned long dstPointAt = (yL2 * siP->imgWidth) + xL2;

	PF_PixelFloat *pixelPointerAt = (PF_PixelFloat*)((char*)siP->tmpOutput->data + (dstPointAt * sizeof(PF_PixelFloat)));
	PF_PixelFloat tmpPixel = *outP;
	
	*outP = *pixelPointerAt;
	*pixelPointerAt = tmpPixel;

	return err;
}

PF_Err
pixelToVector(
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
		finalR = log(1 + abs(siP->imgVectorR[currentIndex]));
		finalG = log(1 + abs(siP->imgVectorG[currentIndex]));
		finalB = log(1 + abs(siP->imgVectorB[currentIndex]));

		if (finalR > siP->rMax) siP->rMax = finalR;
		if (finalR > siP->gMax) siP->gMax = finalG;
		if (finalR > siP->bMax) siP->bMax = finalB;
	}
	else {
		finalR = abs(siP->imgVectorR[currentIndex]);
		finalG = abs(siP->imgVectorG[currentIndex]);
		finalB = abs(siP->imgVectorB[currentIndex]);
	}

	outP->alpha = 1;
	outP->red = finalR;
	outP->green = finalG;
	outP->blue = finalB;

	return err;
}

void transformRow(
	std::vector<std::complex<double>> *imgDataVecR, 
	std::vector<std::complex<double>> *imgDataVecG,
	std::vector<std::complex<double>> *imgDataVecB,
	A_long row, 
	A_long imgWidth,
	bool inv) {

	std::vector<std::complex<double>> currentRowVecR, currentRowVecG, currentRowVecB;

	for (A_long i = 0; i < imgWidth; i++) {
		A_long currentIndex = (imgWidth*row) + i;
		currentRowVecR.push_back((*imgDataVecR)[currentIndex]);
		currentRowVecG.push_back((*imgDataVecG)[currentIndex]);
		currentRowVecB.push_back((*imgDataVecB)[currentIndex]);
	}

	if (!inv) {
		fft::transform(currentRowVecR);
		fft::transform(currentRowVecG);
		fft::transform(currentRowVecB);
	}
	else {
		fft::inverseTransform(currentRowVecR);
		fft::inverseTransform(currentRowVecG);
		fft::inverseTransform(currentRowVecB);
	}

	for (A_long i = 0; i < imgWidth; i++) {
		A_long currentIndex = (imgWidth*row) + i;
		imgDataVecR->operator[](currentIndex) = currentRowVecR[i];
		imgDataVecG->operator[](currentIndex) = currentRowVecG[i];
		imgDataVecB->operator[](currentIndex) = currentRowVecB[i];
	}
}

void transformColumn(
	std::vector<std::complex<double>> *imgDataVecR,
	std::vector<std::complex<double>> *imgDataVecG,
	std::vector<std::complex<double>> *imgDataVecB, 
	A_long col, 
	A_long imgWidth, 
	A_long imgHeight,
	bool inv) {

	std::vector<std::complex<double>> currentColVecR, currentColVecG, currentColVecB;

	for (A_long i = 0; i < imgHeight; i++) {
		A_long currentIndex = (imgWidth*i) + col;
		currentColVecR.push_back((*imgDataVecR)[currentIndex]);
		currentColVecG.push_back((*imgDataVecG)[currentIndex]);
		currentColVecB.push_back((*imgDataVecB)[currentIndex]);
	}

	if (!inv) {
		fft::transform(currentColVecR);
		fft::transform(currentColVecG);
		fft::transform(currentColVecB);
	}
	else {
		fft::inverseTransform(currentColVecR);
		fft::inverseTransform(currentColVecG);
		fft::inverseTransform(currentColVecB);
	}

	for (A_long i = 0; i < imgHeight; i++) {
		A_long currentIndex = (imgWidth*i) + col;
		imgDataVecR->operator[](currentIndex) = currentColVecR[i];
		imgDataVecG->operator[](currentIndex) = currentColVecG[i];
		imgDataVecB->operator[](currentIndex) = currentColVecB[i];
	}
}
