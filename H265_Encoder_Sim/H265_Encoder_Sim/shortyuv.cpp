/*
* shortyuv.c
*
*  Created on: 2015-10-26
*      Author: adminster
*/
#include "shortyuv.h"
#include "constants.h"
#include "x265.h"
#include <math.h>
#include "yuv.h"

int16_t shortyuv0[1536] = { 1 };
int16_t shortyuv1[1536] = { 1 };
int16_t resiQtYuv_0[1536] = { 1 };
int16_t resiQtYuv_1[1536] = { 1 };
int16_t resiQtYuv_2[1536] = { 1 };
int16_t resiQtYuv_3[1536] = { 1 };
int16_t tmpResiYuv_0[1536] = { 1 };
int16_t tmpResiYuv_1[1536] = { 1 };
int16_t tmpResiYuv_2[1536] = { 1 };

bool ShortYuv_create(ShortYuv* shyuv, uint32_t size, int csp)
{
	shyuv->m_csp = csp;
	shyuv->m_hChromaShift = CHROMA_H_SHIFT(csp);
	shyuv->m_vChromaShift = CHROMA_V_SHIFT(csp);

	shyuv->m_size = size;
	shyuv->m_csize = size >> shyuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (shyuv->m_hChromaShift + shyuv->m_vChromaShift);
	X265_CHECK((sizeC & 15) == 0, "invalid size");

	shyuv->m_buf[0] = shortyuv0;
	//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
	shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
	shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	return TRUE;

fail:
	return FALSE;
}

bool ShortYuv_create1(ShortYuv* shyuv, uint32_t size, int csp)
{
	shyuv->m_csp = csp;
	shyuv->m_hChromaShift = CHROMA_H_SHIFT(csp);
	shyuv->m_vChromaShift = CHROMA_V_SHIFT(csp);

	shyuv->m_size = size;
	shyuv->m_csize = size >> shyuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (shyuv->m_hChromaShift + shyuv->m_vChromaShift);
	X265_CHECK((sizeC & 15) == 0, "invalid size");

	shyuv->m_buf[0] = shortyuv1;
	//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
	shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
	shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	return TRUE;

fail:
	return FALSE;
}

bool ShortYuv_create_search(ShortYuv* shyuv, uint32_t size, int csp, int num)
{
	shyuv->m_csp = csp;
	shyuv->m_hChromaShift = CHROMA_H_SHIFT(csp);
	shyuv->m_vChromaShift = CHROMA_V_SHIFT(csp);

	shyuv->m_size = size;
	shyuv->m_csize = size >> shyuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (shyuv->m_hChromaShift + shyuv->m_vChromaShift);
	X265_CHECK((sizeC & 15) == 0, "invalid size");

	if (num == 0)
	{
		shyuv->m_buf[0] = resiQtYuv_0;
		//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
		shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
		shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 1)
	{
		shyuv->m_buf[0] = resiQtYuv_1;
		//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
		shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
		shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 2)
	{
		shyuv->m_buf[0] = resiQtYuv_2;
		//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
		shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
		shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 3)
	{
		shyuv->m_buf[0] = resiQtYuv_3;
		//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
		shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
		shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	}

	return TRUE;

fail:
	return FALSE;
}

bool ShortYuv_create_search_1(ShortYuv* shyuv, uint32_t size, int csp, int num)
{
	shyuv->m_csp = csp;
	shyuv->m_hChromaShift = CHROMA_H_SHIFT(csp);
	shyuv->m_vChromaShift = CHROMA_V_SHIFT(csp);

	shyuv->m_size = size;
	shyuv->m_csize = size >> shyuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (shyuv->m_hChromaShift + shyuv->m_vChromaShift);
	X265_CHECK((sizeC & 15) == 0, "invalid size");

	if (num == 0)
	{
		shyuv->m_buf[0] = tmpResiYuv_0;
		//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
		shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
		shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 1)
	{
		shyuv->m_buf[0] = tmpResiYuv_1;
		//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
		shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
		shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	}


	if (num == 2)
	{
		shyuv->m_buf[0] = tmpResiYuv_2;
		//CHECKED_MALLOC(shyuv->m_buf[0], int16_t, sizeL + sizeC * 2);
		shyuv->m_buf[1] = shyuv->m_buf[0] + sizeL;
		shyuv->m_buf[2] = shyuv->m_buf[0] + sizeL + sizeC;
	}

	return TRUE;

fail:
	return FALSE;
}


int16_t* ShortYuv_getLumaAddr(ShortYuv* shortyuv, uint32_t absPartIdx)                      { return shortyuv->m_buf[0] + ShortYuv_getAddrOffset(absPartIdx, shortyuv->m_size); }
int16_t* ShortYuv_getCbAddr(ShortYuv* shortyuv, uint32_t absPartIdx)                        { return shortyuv->m_buf[1] + ShortYuv_getChromaAddrOffset(shortyuv, absPartIdx); }
int16_t* ShortYuv_getCrAddr(ShortYuv* shortyuv, uint32_t absPartIdx)                        { return shortyuv->m_buf[2] + ShortYuv_getChromaAddrOffset(shortyuv, absPartIdx); }
int16_t* ShortYuv_getChromaAddr(ShortYuv* shortyuv, uint32_t chromaId, uint32_t absPartIdx) { return shortyuv->m_buf[chromaId] + ShortYuv_getChromaAddrOffset(shortyuv, absPartIdx); }

const int16_t* ShortYuv_getLumaAddr_const(ShortYuv* shortyuv, uint32_t absPartIdx)                       { return shortyuv->m_buf[0] + ShortYuv_getAddrOffset(absPartIdx, shortyuv->m_size); }
const int16_t* ShortYuv_getCbAddr_const(ShortYuv* shortyuv, uint32_t absPartIdx)                         { return shortyuv->m_buf[1] + ShortYuv_getChromaAddrOffset(shortyuv, absPartIdx); }
const int16_t* ShortYuv_getCrAddr_const(ShortYuv* shortyuv, uint32_t absPartIdx)                         { return shortyuv->m_buf[2] + ShortYuv_getChromaAddrOffset(shortyuv, absPartIdx); }
const int16_t* ShortYuv_getChromaAddr_const(ShortYuv* shortyuv, uint32_t chromaId, uint32_t absPartIdx)  { return shortyuv->m_buf[chromaId] + ShortYuv_getChromaAddrOffset(shortyuv, absPartIdx); }



int ShortYuv_getChromaAddrOffset(ShortYuv* shortYuv, uint32_t idx)
{
	int blkX = g_zscanToPelX[idx] >> shortYuv->m_hChromaShift;
	int blkY = g_zscanToPelY[idx] >> shortYuv->m_vChromaShift;

	return blkX + blkY * shortYuv->m_csize;
}

static int ShortYuv_getAddrOffset(uint32_t idx, uint32_t width)
{
	int blkX = g_zscanToPelX[idx];
	int blkY = g_zscanToPelY[idx];

	return blkX + blkY * width;
}

void ShortYuv_copyPartToPartLuma(ShortYuv* srcYuv, ShortYuv* dstYuv, uint32_t absPartIdx, uint32_t log2Size)
{
	const int16_t* src = ShortYuv_getLumaAddr_const(srcYuv, absPartIdx);
	int16_t* dst = ShortYuv_getLumaAddr(dstYuv, absPartIdx);

	primitives.cu[log2Size - 2].copy_ss(dst, dstYuv->m_size, src, srcYuv->m_size, pow(2, double(log2Size)), pow(2, double(log2Size)));
}

void StoYuv_copyPartToPartLuma(ShortYuv* srcYuv, Yuv* dstYuv, uint32_t absPartIdx, uint32_t log2Size)
{
	const int16_t* src = ShortYuv_getLumaAddr_const(srcYuv, absPartIdx);
	pixel* dst = Yuv_getLumaAddr(dstYuv, absPartIdx);

	primitives.cu[log2Size - 2].copy_sp(dst, dstYuv->m_size, src, srcYuv->m_size, pow(2, double(log2Size)), pow(2, double(log2Size)));
}

void ShortYuv_copyPartToPartChroma(ShortYuv* srcYuv, ShortYuv* dstYuv, uint32_t absPartIdx, uint32_t log2SizeL)
{
	int part = partitionFromLog2Size(log2SizeL);
	const int16_t* srcU = ShortYuv_getCbAddr_const(srcYuv, absPartIdx);
	const int16_t* srcV = ShortYuv_getCrAddr_const(srcYuv, absPartIdx);
	int16_t* dstU = ShortYuv_getCbAddr(dstYuv, absPartIdx);
	int16_t* dstV = ShortYuv_getCrAddr(dstYuv, absPartIdx);

	primitives.chroma[srcYuv->m_csp].cu[part].copy_ss(dstU, dstYuv->m_csize, srcU, srcYuv->m_csize, pow(2, double(part + 2)), pow(2, double(part + 2)));
	primitives.chroma[srcYuv->m_csp].cu[part].copy_ss(dstV, dstYuv->m_csize, srcV, srcYuv->m_csize, pow(2, double(part + 2)), pow(2, double(part + 2)));
}

void StoYuv_copyPartToPartChroma(ShortYuv* srcYuv, struct Yuv* dstYuv, uint32_t absPartIdx, uint32_t log2SizeL)
{
	int part = partitionFromLog2Size(log2SizeL);
	const int16_t* srcU = ShortYuv_getCbAddr_const(srcYuv, absPartIdx);
	const int16_t* srcV = ShortYuv_getCrAddr_const(srcYuv, absPartIdx);
	pixel* dstU = Yuv_getCbAddr(dstYuv, absPartIdx);
	pixel* dstV = Yuv_getCrAddr(dstYuv, absPartIdx);

	primitives.chroma[srcYuv->m_csp].cu[part].copy_sp(dstU, dstYuv->m_csize, srcU, srcYuv->m_csize, pow(2, double(part + 2)), pow(2, double(part + 2)));
	primitives.chroma[srcYuv->m_csp].cu[part].copy_sp(dstV, dstYuv->m_csize, srcV, srcYuv->m_csize, pow(2, double(part + 2)), pow(2, double(part + 2)));
}

void ShortYuv_subtract(ShortYuv* shortYuv, Yuv* srcYuv0, Yuv* srcYuv1, uint32_t log2Size)
{
	const int sizeIdx = log2Size - 2;
	primitives.cu[sizeIdx].sub_ps(shortYuv->m_buf[0], shortYuv->m_size, srcYuv0->m_buf[0], srcYuv1->m_buf[0], srcYuv0->m_size, srcYuv1->m_size, pow(2, double(2 + sizeIdx)), pow(2, double(2 + sizeIdx)));
	primitives.chroma[shortYuv->m_csp].cu[sizeIdx].sub_ps(shortYuv->m_buf[1], shortYuv->m_csize, srcYuv0->m_buf[1], srcYuv1->m_buf[1], srcYuv0->m_csize, srcYuv1->m_csize, pow(2, double(2 + sizeIdx)), pow(2, double(2 + sizeIdx)));
	primitives.chroma[shortYuv->m_csp].cu[sizeIdx].sub_ps(shortYuv->m_buf[2], shortYuv->m_csize, srcYuv0->m_buf[2], srcYuv1->m_buf[2], srcYuv0->m_csize, srcYuv1->m_csize, pow(2, double(2 + sizeIdx)), pow(2, double(2 + sizeIdx)));
}
