/*
* intrapred.h
*
*  Created on: 2015-12-11
*      Author: adminster
*/

#ifndef INTRAPRED_H_
#define INTRAPRED_H_

#include "common.h"
#include "primitives.h"
#include "constants.h"

void intraFilter(const pixel* samples, pixel* filtered, int tuSize);
void dcPredFilter(const pixel* above, const pixel* left, pixel* dst, intptr_t dststride, int size);
void intra_pred_dc_c(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter, int width);
void planar_pred_c(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter, int log2Size);
void intra_pred_ang_c(pixel* dst, intptr_t dstStride, const pixel *srcPix0, int dirMode, int bFilter, int width);
void all_angs_pred_c(pixel *dest, pixel *refPix, pixel *filtPix, int bLuma, int log2Size);
void setupIntraPrimitives_c(struct EncoderPrimitives* p);

#endif /* INTRAPRED_H_ */
