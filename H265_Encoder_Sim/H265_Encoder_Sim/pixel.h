/*
* pixel.h
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#ifndef PIXEL_H_
#define PIXEL_H_

#include "common.h"
#include "x265.h"

int sse_pixel(const pixel* pix1, int stride_pix1, const pixel* pix2, int stride_pix2, int lx, int ly);

void blockcopy_pp_c(pixel* a, intptr_t stridea, pixel* b, intptr_t strideb, int bx, int by);

void blockcopy_pp(pixel* a, intptr_t stridea, const pixel* b, intptr_t strideb, int bx, int by);

int sad(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2, int lx, int ly);

void sad_x3(const pixel* pix1, const pixel* pix2, const pixel* pix3, const pixel* pix4, intptr_t frefstride, int* res, int lx, int ly);
void sad_x4(const pixel* pix1, const pixel* pix2, const pixel* pix3, const pixel* pix4, const pixel* pix5, intptr_t frefstride, int* res, int lx, int ly);

#endif /* PIXEL_H_ */
