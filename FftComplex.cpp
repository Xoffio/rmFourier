/*
* Free FFT and convolution (C++)
*
* Copyright (c) 2017 Project Nayuki. (MIT License)
* https://www.nayuki.io/page/free-small-fft-in-multiple-languages
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
* - The above copyright notice and this permission notice shall be included in
*   all copies or substantial portions of the Software.
* - The Software is provided "as is", without warranty of any kind, express or
*   implied, including but not limited to the warranties of merchantability,
*   fitness for a particular purpose and noninfringement. In no event shall the
*   authors or copyright holders be liable for any claim, damages or other
*   liability, whether in an action of contract, tort or otherwise, arising from,
*   out of or in connection with the Software or the use or other dealings in the
*   Software.
*/

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "FftComplex.hpp"

using std::complex;
using std::size_t;
using std::vector;

// Private function prototypes
static size_t reverseBits(size_t x, int n);

void fft::transform(vector<complex<double> > &vec, void *refcon) {
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	size_t n = vec.size();
	if (n == 0)
		return;

	//else if ((n & (n - 1)) == 0)  // Is power of 2
	else if(siP->transformType == 1)
		transformRadix2(vec, siP);
	else if (siP->transformType == 2)  // More complicated algorithm for arbitrary sizes
		transformBluestein(vec, siP);
}

void fft::inverseTransform(vector<complex<double> > &vec, void *refcon) {
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	std::transform(vec.cbegin(), vec.cend(), vec.begin(),
		static_cast<complex<double>(*)(const complex<double> &)>(std::conj));

	if (siP->transformType == 1) transform(vec, refcon);
	if (siP->transformType == 2) convTransformRadix2(vec, refcon);
	
	std::transform(vec.cbegin(), vec.cend(), vec.begin(),
		static_cast<complex<double>(*)(const complex<double> &)>(std::conj));
}

void fft::transformRadix2(vector<complex<double> > &vec, void *refcon) {
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	size_t n = vec.size();

	// Bit-reversed addressing permutation
	for (size_t i = 0; i < n; i++) {
		size_t j = reverseBits(i, siP->levels);
		if (j > i)
			std::swap(vec[i], vec[j]);
	}

	// Cooley-Tukey decimation-in-time radix-2 FFT
	for (size_t size = 2; size <= n; size *= 2) {
		size_t halfsize = size / 2;
		size_t tablestep = n / size;
		for (size_t i = 0; i < n; i += size) {
			for (size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep) {
				complex<double> temp = vec[j + halfsize] * siP->expTable[k];
				vec[j + halfsize] = vec[j] - temp;
				vec[j] += temp;
			}
		}
		if (size == n)  // Prevent overflow in 'size *= 2'
			break;
	}
}

void fft::convTransformRadix2(vector<complex<double> > &vec, void *refcon) {
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;
	size_t n = vec.size();

	// Bit-reversed addressing permutation
	for (size_t i = 0; i < n; i++) {
		size_t j = reverseBits(i, siP->convLevels);
		if (j > i)
			std::swap(vec[i], vec[j]);
	}

	// Cooley-Tukey decimation-in-time radix-2 FFT
	for (size_t size = 2; size <= n; size *= 2) {
		size_t halfsize = size / 2;
		size_t tablestep = n / size;
		for (size_t i = 0; i < n; i += size) {
			for (size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep) {
				complex<double> temp = vec[j + halfsize] * siP->convExpTable[k];
				vec[j + halfsize] = vec[j] - temp;
				vec[j] += temp;
			}
		}
		if (size == n)  // Prevent overflow in 'size *= 2'
			break;
	}
}

void fft::transformBluestein(vector<complex<double> > &vec, void *refcon) {
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	// Find a power-of-2 convolution length m such that m >= n * 2 + 1
	size_t n = vec.size();

	// Temporary vectors and preprocessing
	vector<complex<double> > av(siP->m);
	for (size_t i = 0; i < n; i++)
		av[i] = vec[i] * siP->expTable[i];

	// Convolution
	vector<complex<double> > cv(siP->m);
	convolve(av, siP->bv, cv, siP);

	// Postprocessing
	for (size_t i = 0; i < n; i++)
		vec[i] = cv[i] * siP->expTable[i];
}


void fft::convolve(
	const vector<complex<double> > &xvec,
	const vector<complex<double> > &yvec,
	vector<complex<double> > &outvec,
	void *refcon) {
	register rmFourierInfo	*siP = (rmFourierInfo*)refcon;

	size_t n = xvec.size();
	if (n != yvec.size() || n != outvec.size())
		throw "Mismatched lengths";
	vector<complex<double> > xv = xvec;
	vector<complex<double> > yv = yvec;

	convTransformRadix2(xv, siP);
	convTransformRadix2(yv, siP);
	
	for (size_t i = 0; i < n; i++)
		xv[i] *= yv[i];

	inverseTransform(xv, siP);
	for (size_t i = 0; i < n; i++)  // Scaling (because this FFT implementation omits it)
		outvec[i] = xv[i] / static_cast<double>(n);
}


static size_t reverseBits(size_t x, int n) {
	size_t result = 0;
	for (int i = 0; i < n; i++, x >>= 1)
		result = (result << 1) | (x & 1U);
	return result;
}
