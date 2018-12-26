/*
* scalinglist.c
*
*  Created on: 2015-11-6
*      Author: adminster
*/

#include "scalingList.h"
#include "x265.h"
#include "primitives.h"
#include <stdio.h>

int32_t quantTSDefault4x4[16] =
{
	16, 16, 16, 16,
	16, 16, 16, 16,
	16, 16, 16, 16,
	16, 16, 16, 16
};

int32_t quantIntraDefault8x8[64] =
{
	16, 16, 16, 16, 17, 18, 21, 24,
	16, 16, 16, 16, 17, 19, 22, 25,
	16, 16, 17, 18, 20, 22, 25, 29,
	16, 16, 18, 21, 24, 27, 31, 36,
	17, 17, 20, 24, 30, 35, 41, 47,
	18, 19, 22, 27, 35, 44, 54, 65,
	21, 22, 25, 31, 41, 54, 70, 88,
	24, 25, 29, 36, 47, 65, 88, 115
};

int32_t quantInterDefault8x8[64] =
{
	16, 16, 16, 16, 17, 18, 20, 24,
	16, 16, 16, 17, 18, 20, 24, 25,
	16, 16, 17, 18, 20, 24, 25, 28,
	16, 17, 18, 20, 24, 25, 28, 33,
	17, 18, 20, 24, 25, 28, 33, 41,
	18, 20, 24, 25, 28, 33, 41, 54,
	20, 24, 25, 28, 33, 41, 54, 71,
	24, 25, 28, 33, 41, 54, 71, 91
};
const int     const_s_numCoefPerSize[NUM_SIZES1] = { 16, 64, 256, 1024 };
const int32_t const_s_quantScales[NUM_REM] = { 26214, 23302, 20560, 18396, 16384, 14564 };
const int32_t const_s_invQuantScales[NUM_REM] = { 40, 45, 51, 57, 64, 72 };

void ScalingList_ScalingList(ScalingList* list)
{
	memset(list->m_quantCoef, 0, sizeof(list->m_quantCoef));
	memset(list->m_dequantCoef, 0, sizeof(list->m_dequantCoef));
	memset(list->m_scalingListCoef, 0, sizeof(list->m_scalingListCoef));
}
bool ScalingList_init(ScalingList* scalingList)
{
	ScalingList_ScalingList(scalingList);

	bool ok = TRUE;
	for (int sizeId = 0; sizeId < NUM_SIZES1; sizeId++)
	{
		for (int listId = 0; listId < NUM_LISTS; listId++)
		{
			scalingList->m_scalingListCoef[sizeId][listId] = X265_MALLOC(int32_t, X265_MIN(MAX_MATRIX_COEF_NUM, const_s_numCoefPerSize[sizeId]));
			ok &= !!scalingList->m_scalingListCoef[sizeId][listId];
			scalingList->s_quantScales[listId] = const_s_quantScales[listId];
			for (int rem = 0; rem < NUM_REM; rem++)
			{
				scalingList->m_quantCoef[sizeId][listId][rem] = X265_MALLOC(int32_t, const_s_numCoefPerSize[sizeId]);
				scalingList->m_dequantCoef[sizeId][listId][rem] = X265_MALLOC(int32_t, const_s_numCoefPerSize[sizeId]);
				ok &= scalingList->m_quantCoef[sizeId][listId][rem] && scalingList->m_dequantCoef[sizeId][listId][rem];
				scalingList->s_invQuantScales[rem] = const_s_invQuantScales[rem];
			}
		}
		scalingList->s_numCoefPerSize[sizeId] = const_s_numCoefPerSize[sizeId];
	}

	scalingList->m_bEnabled = FALSE;
	scalingList->m_bDataPresent = TRUE;
	return ok;
}


const int checkPredMode(struct ScalingList* scalingList, int size, int list)
{
	int predList;
	for (predList = list; predList >= 0; predList--)
	{
		// check DC value
		if (size < BLOCK_16x16 && scalingList->m_scalingListDC[size][list] != scalingList->m_scalingListDC[size][predList])
			continue;

		// check value of matrix
		if (!memcmp(scalingList->m_scalingListCoef[size][list],
			list == predList ? getScalingListDefaultAddress(size, predList) : scalingList->m_scalingListCoef[size][predList],
			sizeof(int32_t) * X265_MIN(MAX_MATRIX_COEF_NUM, scalingList->s_numCoefPerSize[size])))
			return predList;
	}

	return -1;
}

const int32_t* getScalingListDefaultAddress(int sizeId, int listId)
{
	switch (sizeId)
	{
	case BLOCK_4x4:
		return quantTSDefault4x4;
	case BLOCK_8x8:
		return (listId < 3) ? quantIntraDefault8x8 : quantInterDefault8x8;
	case BLOCK_16x16:
		return (listId < 3) ? quantIntraDefault8x8 : quantInterDefault8x8;
	case BLOCK_32x32:
		return (listId < 1) ? quantIntraDefault8x8 : quantInterDefault8x8;
	default:
		break;
	}

	if (!0)
	{
		printf("invalid scaling list size\n");
		return NULL;
	}

}


/** set quantized matrix coefficient for encode */
void ScalingList_setupQuantMatrices(ScalingList *scal)
{
	for (int size = 0; size < NUM_SIZES1; size++)
	{
		int width = 1 << (size + 2);
		int ratio = width / X265_MIN(MAX_MATRIX_SIZE_NUM, width);
		int stride = X265_MIN(MAX_MATRIX_SIZE_NUM, width);
		int count = const_s_numCoefPerSize[size];

		for (int list = 0; list < NUM_LISTS; list++)
		{
			int32_t *coeff = scal->m_scalingListCoef[size][list];
			int32_t dc = scal->m_scalingListDC[size][list];

			for (int rem = 0; rem < NUM_REM; rem++)
			{
				int32_t *quantCoeff = scal->m_quantCoef[size][list][rem];
				int32_t *dequantCoeff = scal->m_dequantCoef[size][list][rem];

				if (scal->m_bEnabled)
				{
					processScalingListEnc(coeff, quantCoeff, const_s_quantScales[rem] << 4, width, width, ratio, stride, dc);
					processScalingListDec(coeff, dequantCoeff, const_s_invQuantScales[rem], width, width, ratio, stride, dc);
				}
				else
				{
					/* flat quant and dequant coefficients */
					for (int i = 0; i < count; i++)
					{
						quantCoeff[i] = const_s_quantScales[rem];
						dequantCoeff[i] = const_s_invQuantScales[rem];
					}
				}
			}
		}
	}
}

void processScalingListEnc(int32_t *coeff, int32_t *quantcoeff, int32_t quantScales, int height, int width,
	int ratio, int stride, int32_t dc)
{
	for (int j = 0; j < height; j++)
		for (int i = 0; i < width; i++)
			quantcoeff[j * width + i] = quantScales / coeff[stride * (j / ratio) + i / ratio];

	if (ratio > 1)
		quantcoeff[0] = quantScales / dc;
}

void processScalingListDec(int32_t *coeff, int32_t *dequantcoeff, int32_t invQuantScales, int height, int width,
	int ratio, int stride, int32_t dc)
{
	for (int j = 0; j < height; j++)
		for (int i = 0; i < width; i++)
			dequantcoeff[j * width + i] = invQuantScales * coeff[stride * (j / ratio) + i / ratio];

	if (ratio > 1)
		dequantcoeff[0] = invQuantScales * dc;
}

