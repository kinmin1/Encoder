/*
* scalinglist.h
*
*  Created on: 2015-11-6
*      Author: adminster
*/
#ifndef X265_SCALINGLIST_H
#define X265_SCALINGLIST_H

#include "common.h"

enum { NUM_SIZES1 = 4 };            // 4x4, 8x8, 16x16, 32x32
enum { NUM_LISTS = 6 };            // number of quantization matrix lists (YUV * inter/intra)
enum { NUM_REM = 6 };              // number of remainders of QP/6
enum { MAX_MATRIX_COEF_NUM = 64 }; // max coefficient number per quantization matrix
enum { MAX_MATRIX_SIZE_NUM = 8 };  // max size number for quantization matrix

typedef struct ScalingList
{

	int     s_numCoefPerSize[NUM_SIZES1];
	int32_t s_invQuantScales[NUM_REM];
	int32_t s_quantScales[NUM_REM];
	int32_t  m_scalingListDC[NUM_SIZES1][NUM_LISTS];   // the DC value of the matrix coefficient for 16x16
	int32_t* m_scalingListCoef[NUM_SIZES1][NUM_LISTS]; // quantization matrix

	int32_t* m_quantCoef[NUM_SIZES1][NUM_LISTS][NUM_REM];   // array of quantization matrix coefficient 4x4
	int32_t* m_dequantCoef[NUM_SIZES1][NUM_LISTS][NUM_REM]; // array of dequantization matrix coefficient 4x4

	bool     m_bEnabled;
	bool     m_bDataPresent; // non-default scaling lists must be signaled

}ScalingList;

bool ScalingList_init(ScalingList* scalingList);
const int checkPredMode(struct ScalingList* scalingList, int size, int list);
const int32_t* getScalingListDefaultAddress(int sizeId, int listId);
void ScalingList_setupQuantMatrices(struct ScalingList *scal);
void processScalingListEnc(int32_t *coeff, int32_t *quantcoeff, int32_t quantScales, int height, int width,
	int ratio, int stride, int32_t dc);
void processScalingListDec(int32_t *coeff, int32_t *dequantcoeff, int32_t invQuantScales, int height, int width,
	int ratio, int stride, int32_t dc);
#endif // ifndef X265_SCALINGLIST_H


