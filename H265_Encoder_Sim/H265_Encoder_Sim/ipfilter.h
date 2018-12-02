/*
* ipfilter.h
*
*  Created on: 2017-7-10
*      Author: Administrator
*/

#ifndef IPFILTER_H_
#define IPFILTER_H

#include "common.h"

void extendCURowColBorder(pixel* txt, intptr_t stride, int width, int height, int marginX);
void interp_horiz_pp_c(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx, int width, int height, int N);
void interp_horiz_ps_c(pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt, int width, int height, int N);
void interp_vert_pp_c(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx, int width, int height, int N);
void interp_vert_ps_c(pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int width, int height, int N);
void interp_vert_sp_c(int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx, int width, int height, int N);
void interp_vert_sp_c(int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx, int width, int height, int N);
void interp_vert_ss_c(int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int width, int height, int N);
void filterVertical_sp_c(int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width, int height, int coeffIdx, int N);
void interp_hv_pp_c(pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int idxX, int idxY, int width, int height, int N);
void filterPixelToShort_c(pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int width, int height);

#endif /* IPFILTER_H_ */
