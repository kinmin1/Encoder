/*
* dct.h
*
*  Created on: 2017-4-21
*      Author: Administrator
*/
#ifndef DCT_H_
#define DCT_H_
#include"common.h"

extern int g_t4[4][4];
extern int g_t8[8][8];
extern int g_t16[16][16];
extern int g_t32[32][32];

extern int ig_t4[4][4];
extern int ig_t8[8][8];
extern int ig_t16[16][16];
extern int ig_t32[32][32];

void dequant_normal_c(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift);
void dequant_scaling_c(const int16_t* quantCoef, const int32_t* deQuantCoef, int16_t* coef, int num, int per, int shift);
uint32_t quant_c(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff);
uint32_t nquant_c(const int16_t* coef, const int32_t* quantCoeff, int16_t* qCoef, int qBits, int add, int numCoeff);
int  count_nonzero_c(uint32_t trSize, const int16_t* quantCoeff);
uint32_t copy_count(int16_t* coeff, int16_t* residual, intptr_t resiStride, int trSize);

void fastForwardDst(int16_t* block, int16_t* coeff, int shift);  // input block, output coeff
void dst4_c(int16_t* src, int16_t* dst, intptr_t srcStride);
void dct4_c(int16_t* src, int16_t* dst, intptr_t srcStride);
void dct8_c(int16_t* src, int16_t* dst, intptr_t srcStride);
void dct16_c(int16_t* src, int16_t* dst, intptr_t srcStride);
void dct32_c(int16_t* src, int16_t* dst, intptr_t srcStride);

void idst4_c(int16_t* src, int16_t* dst, intptr_t dstStride);
void idct4_c(int16_t* src, int16_t* dst, intptr_t dstStride);
void idct8_c(int16_t* src, int16_t* dst, intptr_t dstStride);
void idct16_c(int16_t* src, int16_t* dst, intptr_t dstStride);
void idct32_c(int16_t* src, int16_t* dst, intptr_t dstStride);

#endif /* DCT_H_ */
