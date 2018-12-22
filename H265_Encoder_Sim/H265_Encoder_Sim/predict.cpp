/*
* predict.c
*
*  Created on: 2015-10-26
*      Author: adminster
*/
#include "picyuv.h"
#include "framedata.h"
#include "predict.h"
#include "primitives.h"
#include "slice.h"
#include "cudata.h"
#include "intrapred.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "pixel.h"
#include "ipfilter.h"

short int m_immedVals[4544] = { 1 };

//void fillReferenceSamples_chroma(pixel* adiOrigin, int picStride, struct IntraNeighbors* intraNeighbors, pixel dst[258]);
bool allocBuffers(Predict* pred, int csp)
{
	pred->m_csp = csp;
	pred->m_hChromaShift = CHROMA_H_SHIFT(csp);
	pred->m_vChromaShift = CHROMA_V_SHIFT(csp);

	pred->m_immedVals = m_immedVals;

	return ShortYuv_create(&(pred->m_predShortYuv[0]), MAX_CU_SIZE, csp) && ShortYuv_create1(&(pred->m_predShortYuv[1]), MAX_CU_SIZE, csp);

fail:
	return FALSE;
}

void predIntraLumaAng(Predict* pred, uint32_t dirMode, pixel* dst, int stride, uint32_t log2TrSize)
{
	int tuSize = 1 << log2TrSize;
	int sizeIdx = log2TrSize - 2;
	X265_CHECK(sizeIdx >= 0 && sizeIdx < 4, "intra block size is out of range\n");

	int filter = !!(g_intraFilterFlags[dirMode] & tuSize);
	bool bFilter = log2TrSize <= 4;
	if (dirMode == PLANAR_IDX)
		primitives.cu[sizeIdx].intra_pred[dirMode](dst, stride, pred->intraNeighbourBuf[filter], dirMode, bFilter, log2TrSize);
	else
		primitives.cu[sizeIdx].intra_pred[dirMode](dst, stride, pred->intraNeighbourBuf[filter], dirMode, bFilter, pow(2, double(log2TrSize)));
}

void predIntraChromaAng(Predict* pred, uint32_t dirMode, pixel* dst, int stride, uint32_t log2TrSizeC)
{
	int tuSize = 1 << log2TrSizeC;
	int sizeIdx = log2TrSizeC - 2;
	X265_CHECK(sizeIdx >= 0 && sizeIdx < 4, "intra block size is out of range\n");

	int filter = !!(pred->m_csp == X265_CSP_I444 && (g_intraFilterFlags[dirMode] & tuSize));
	//primitives.cu[sizeIdx].intra_pred[dirMode](dst, stride, intraNeighbourBuf[filter], dirMode, 0);
	if (dirMode == PLANAR_IDX)
		primitives.cu[sizeIdx].intra_pred[dirMode](dst, stride, pred->intraNeighbourBuf[filter], dirMode, 0, log2TrSizeC);
	else
		primitives.cu[sizeIdx].intra_pred[dirMode](dst, stride, pred->intraNeighbourBuf[filter], dirMode, 0, pow(2, double(log2TrSizeC)));

}

void initAdiPattern(Predict* predict, CUData* cu, CUGeom* cuGeom, uint32_t puAbsPartIdx, struct IntraNeighbors* intraNeighbors, int dirMode)
{
	int tuSize = 1 << intraNeighbors->log2TrSize;//log2TrSize=cu.m_log2CUSize[0] - tuDepth;
	int tuSize2 = tuSize << 1;

	pixel* adiOrigin = Picyuv_CUgetLumaAddr(cu->m_encData->m_reconPic, cu->m_cuAddr, cuGeom->absPartIdx + puAbsPartIdx);
	intptr_t picStride = cu->m_encData->m_reconPic->m_stride;

	fillReferenceSamples_t(adiOrigin, picStride, intraNeighbors, predict->intraNeighbourBuf[0]);//预测参考数据的获取

	pixel* refBuf = predict->intraNeighbourBuf[0];
	pixel* fltBuf = predict->intraNeighbourBuf[1];

	pixel topLeft = refBuf[0], topLast = refBuf[tuSize2], leftLast = refBuf[tuSize2 + tuSize2];

	if (dirMode == ALL_IDX ? (8 | 16 | 32) & tuSize : g_intraFilterFlags[dirMode] & tuSize)
	{
		// generate filtered intra prediction samples

		if (cu->m_slice->m_sps->bUseStrongIntraSmoothing && tuSize == 32)
		{
			const int threshold = 1 << (X265_DEPTH - 5);

			pixel topMiddle = refBuf[32], leftMiddle = refBuf[tuSize2 + 32];

			if (abs(topLeft + topLast - (topMiddle << 1)) < threshold &&
				abs(topLeft + leftLast - (leftMiddle << 1)) < threshold)
			{
				// "strong" bilinear interpolation
				const int shift = 5 + 1;
				int init = (topLeft << shift) + tuSize;
				int deltaL, deltaR;

				deltaL = leftLast - topLeft; deltaR = topLast - topLeft;

				fltBuf[0] = topLeft;
				for (int i = 1; i < tuSize2; i++)
				{
					fltBuf[i + tuSize2] = (pixel)((init + deltaL * i) >> shift); // Left Filtering
					fltBuf[i] = (pixel)((init + deltaR * i) >> shift);           // Above Filtering
				}
				fltBuf[tuSize2] = topLast;
				fltBuf[tuSize2 + tuSize2] = leftLast;
				return;
			}
		}

		intraFilter(refBuf, fltBuf, pow(2.0,double(intraNeighbors->log2TrSize)));
	}
}

void initAdiPatternChroma(Predict* pdata, CUData* cu, const CUGeom* cuGeom, uint32_t puAbsPartIdx, struct IntraNeighbors* intraNeighbors, uint32_t chromaId)
{/*
	pixel* adiOrigin = Picyuv_CUgetChromaAddr(cu->m_encData->m_reconPic, chromaId, cu->m_cuAddr, cuGeom->absPartIdx + puAbsPartIdx);
	intptr_t picStride = cu->m_encData->m_reconPic->m_strideC;

	intraNeighbors->log2TrSize = 4;
	fillReferenceSamples_t(adiOrigin, picStride, intraNeighbors, pdata->intraNeighbourBuf[0]);

	if (pdata->m_csp == X265_CSP_I444)
		intraFilter(pdata->intraNeighbourBuf[0], pdata->intraNeighbourBuf[1], pow(2, intraNeighbors->log2TrSize));*/
}

void initIntraNeighbors(CUData* cu, uint32_t absPartIdx, uint32_t tuDepth, bool isLuma, struct IntraNeighbors *intraNeighbors)
{
	uint32_t log2TrSize = cu->m_log2CUSize[0] - tuDepth;
	int log2UnitWidth = LOG2_UNIT_SIZE;
	int log2UnitHeight = LOG2_UNIT_SIZE;

	if (!isLuma)
	{
		log2TrSize -= cu->m_hChromaShift;
		log2UnitWidth -= cu->m_hChromaShift;
		log2UnitHeight -= cu->m_vChromaShift;
	}

	int numIntraNeighbor;
	bool* bNeighborFlags = intraNeighbors->bNeighborFlags;

	uint32_t numPartInWidth = 1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE - tuDepth);
	uint32_t partIdxLT = cu->m_absIdxInCTU + absPartIdx;
	uint32_t partIdxRT = g_rasterToZscan[g_zscanToRaster[partIdxLT] + numPartInWidth - 1];

	uint32_t tuSize = 1 << log2TrSize;
	int  tuWidthInUnits = tuSize >> log2UnitWidth;
	int  tuHeightInUnits = tuSize >> log2UnitHeight;
	int  aboveUnits = tuWidthInUnits << 1;
	int  leftUnits = tuHeightInUnits << 1;
	int  partIdxStride = cu->m_slice->m_sps->numPartInCUSize;
	uint32_t partIdxLB = g_rasterToZscan[g_zscanToRaster[partIdxLT] + ((tuHeightInUnits - 1) * partIdxStride)];

	if (Slice_isIntra(cu->m_slice) || !cu->m_slice->m_pps->bConstrainedIntraPred)
	{
		bNeighborFlags[leftUnits] = isAboveLeftAvailable(cu, partIdxLT, 0);
		numIntraNeighbor = (int)(bNeighborFlags[leftUnits]);
		numIntraNeighbor += isAboveAvailable(cu, partIdxLT, partIdxRT, bNeighborFlags + leftUnits + 1, 0);
		numIntraNeighbor += isAboveRightAvailable(cu, partIdxRT, bNeighborFlags + leftUnits + 1 + tuWidthInUnits, tuWidthInUnits, 0);
		numIntraNeighbor += isLeftAvailable(cu, partIdxLT, partIdxLB, bNeighborFlags + leftUnits - 1, 0);
		numIntraNeighbor += isBelowLeftAvailable(cu, partIdxLB, bNeighborFlags + tuHeightInUnits - 1, tuHeightInUnits, 0);
	}
	else
	{
		bNeighborFlags[leftUnits] = isAboveLeftAvailable(cu, partIdxLT, 1);
		numIntraNeighbor = (int)(bNeighborFlags[leftUnits]);
		numIntraNeighbor += isAboveAvailable(cu, partIdxLT, partIdxRT, bNeighborFlags + leftUnits + 1, 1);
		numIntraNeighbor += isAboveRightAvailable(cu, partIdxRT, bNeighborFlags + leftUnits + 1 + tuWidthInUnits, tuWidthInUnits, 1);
		numIntraNeighbor += isLeftAvailable(cu, partIdxLT, partIdxLB, bNeighborFlags + leftUnits - 1, 1);
		numIntraNeighbor += isBelowLeftAvailable(cu, partIdxLB, bNeighborFlags + tuHeightInUnits - 1, tuHeightInUnits, 1);
	}

	intraNeighbors->numIntraNeighbor = numIntraNeighbor;
	intraNeighbors->totalUnits = aboveUnits + leftUnits + 1;
	intraNeighbors->aboveUnits = aboveUnits;
	intraNeighbors->leftUnits = leftUnits;
	intraNeighbors->unitWidth = 1 << log2UnitWidth;
	intraNeighbors->unitHeight = 1 << log2UnitHeight;
	intraNeighbors->log2TrSize = log2TrSize;
	
}
//预测参考数据的获取和滤波处理
void fillReferenceSamples_t(pixel* adiOrigin, int picStride, struct IntraNeighbors* intraNeighbors, pixel dst[258])
{
	pixel dcValue = (pixel)(1 << (X265_DEPTH - 1));//X265_DEPTH=8、10
	int numIntraNeighbor = intraNeighbors->numIntraNeighbor;
	int totalUnits = intraNeighbors->totalUnits;
	uint32_t tuSize = 1 << intraNeighbors->log2TrSize;
	uint32_t refSize = tuSize * 2 + 1;

	// Nothing is available, perform DC prediction.若参考点均不可得，按照DC模式设置参考点
	if (numIntraNeighbor == 0)
	{
		// Fill top border with DC value
		for (uint32_t i = 0; i < refSize; i++)
			dst[i] = dcValue;

		// Fill left border with DC value
		for (uint32_t i = 0; i < refSize - 1; i++)
			dst[i + refSize] = dcValue;
	}
	else if (numIntraNeighbor == totalUnits)//所有参考点都可获得，直接设为当前CU的参考值
	{
		// Fill top border with rec. samples
		pixel* adiTemp = adiOrigin - picStride - 1;//左上角边界，其实就是CU左上角的一个点
		memcpy(dst, adiTemp, refSize * sizeof(pixel));

		// Fill left border with rec. samples
		adiTemp = adiOrigin - 1;//当前CU左上顶点的左边像素
		for (uint32_t i = 0; i < refSize - 1; i++)
		{
			dst[i + refSize] = adiTemp[0];
			adiTemp += picStride;
		}
	}
	else // reference samples are partially available 只有部分参考点可获得
	{
		bool *bNeighborFlags = intraNeighbors->bNeighborFlags;
		bool *pNeighborFlags;
		int aboveUnits = intraNeighbors->aboveUnits;
		int leftUnits = intraNeighbors->leftUnits;
		int unitWidth = intraNeighbors->unitWidth;
		int unitHeight = intraNeighbors->unitHeight;
		int totalSamples = (leftUnits * unitHeight) + ((aboveUnits + 1) * unitWidth);
		pixel adiLineBuffer[5 * MAX_CU_SIZE];
		pixel *adi;

		// Initialize
		for (int i = 0; i < totalSamples; i++)//用均值模式进行初始化
			adiLineBuffer[i] = dcValue;

		// Fill top-left sample
		pixel* adiTemp = adiOrigin - picStride - 1;//指向重建像素中当前CU的左上角位置
		adi = adiLineBuffer + (leftUnits * unitHeight);
		pNeighborFlags = bNeighborFlags + leftUnits;
		if (*pNeighborFlags)//如果左上方的参考数据可用
		{
			pixel topLeftVal = adiTemp[0];
			for (int i = 0; i < unitWidth; i++)
				adi[i] = topLeftVal;
		}

		// Fill left & below-left samples
		adiTemp += picStride;//从左上顶点的左上角移动到左方
		adi--;//缓存指针前移一位
		pNeighborFlags--;//可用性标记指针前移一位
		for (int j = 0; j < leftUnits; j++)
		{
			if (*pNeighborFlags)
				for (int i = 0; i < unitHeight; i++)//判断过程分组进行处理，如对于一个32×32的CU，左侧和左下侧共64个预测点，总共进行16×4次赋值
					adi[-i] = adiTemp[i * picStride];

			adiTemp += unitHeight * picStride;
			adi -= unitHeight;
			pNeighborFlags--;
		}

		// Fill above & above-right samples
		adiTemp = adiOrigin - picStride;//水平方向上的处理与垂直方向类似
		adi = adiLineBuffer + (leftUnits * unitHeight) + unitWidth;
		pNeighborFlags = bNeighborFlags + leftUnits + 1;
		for (int j = 0; j < aboveUnits; j++)
		{
			if (*pNeighborFlags)
				memcpy(adi, adiTemp, unitWidth * sizeof(*adiTemp));
			adiTemp += unitWidth;
			adi += unitWidth;
			pNeighborFlags++;
		}

		// Pad reference samples when necessary
		int curr = 0;
		int next = 1;
		adi = adiLineBuffer;//指向参考数组的起点
		int pAdiLineTopRowOffset = leftUnits * (unitHeight - unitWidth);
		if (!bNeighborFlags[0])
		{
			// very bottom unit of bottom-left; at least one unit will be valid.
			while (next < totalUnits && !bNeighborFlags[next])//找到第一个可以获得的点
				next++;

			pixel* pAdiLineNext = adiLineBuffer + ((next < leftUnits) ? (next * unitHeight) : (pAdiLineTopRowOffset + (next * unitWidth)));
			pixel refSample = *pAdiLineNext;
			// Pad unavailable samples with new value
			int nextOrTop = X265_MIN(next, leftUnits);

			// fill left column
#if HIGH_BIT_DEPTH
			while (curr < nextOrTop)
			{
				for (int i = 0; i < unitHeight; i++)
					adi[i] = refSample;

				adi += unitHeight;
				curr++;
			}

			// fill top row
			while (curr < next)
			{
				for (int i = 0; i < unitWidth; i++)
					adi[i] = refSample;

				adi += unitWidth;
				curr++;
			}
#else
			X265_CHECK(curr <= nextOrTop, "curr must be less than or equal to nextOrTop\n");
			if (curr < nextOrTop)
			{
				int fillSize = unitHeight * (nextOrTop - curr);
				memset(adi, refSample, fillSize * sizeof(pixel));
				curr = nextOrTop;
				adi += fillSize;
			}

			if (curr < next)
			{
				int fillSize = unitWidth * (next - curr);
				memset(adi, refSample, fillSize * sizeof(pixel));
				curr = next;
				adi += fillSize;
			}
#endif
		}

		// pad all other reference samples.
		while (curr < totalUnits)
		{
			if (!bNeighborFlags[curr]) // samples not available
			{
				int numSamplesInCurrUnit = (curr >= leftUnits) ? unitWidth : unitHeight;
				pixel refSample = *(adi - 1);
				for (int i = 0; i < numSamplesInCurrUnit; i++)
					adi[i] = refSample;

				adi += numSamplesInCurrUnit;
				curr++;
			}
			else
			{
				adi += (curr >= leftUnits) ? unitWidth : unitHeight;
				curr++;
			}
		}

		// Copy processed samples 输出前面所准备的数据
		adi = adiLineBuffer + refSize + unitWidth - 2;
		memcpy(dst, adi, refSize * sizeof(pixel));

		adi = adiLineBuffer + refSize - 1;
		for (int i = 0; i < (int)refSize - 1; i++)
			dst[i + refSize] = adi[-(i + 1)];
	}
}

bool isAboveLeftAvailable(CUData* cu, uint32_t partIdxLT, bool cip)
{
	uint32_t partAboveLeft;
	CUData* cuAboveLeft = CUData_getPUAboveLeft(cu, &partAboveLeft, partIdxLT);

	return cuAboveLeft && (!cip || isIntra_cudata(cuAboveLeft, partAboveLeft));
}

int isAboveAvailable(CUData* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags, bool cip)
{
	const uint32_t rasterPartBegin = g_zscanToRaster[partIdxLT];
	const uint32_t rasterPartEnd = g_zscanToRaster[partIdxRT];
	const uint32_t idxStep = 1;
	int numIntra = 0;

	for (uint32_t rasterPart = rasterPartBegin; rasterPart <= rasterPartEnd; rasterPart += idxStep, bValidFlags++)
	{
		uint32_t partAbove;
		CUData* cuAbove = CUData_getPUAbove(cu, &partAbove, g_rasterToZscan[rasterPart]);
		if (cuAbove && (!cip || isIntra_cudata(cuAbove, partAbove)))
		{
			numIntra++;
			*bValidFlags = TRUE;
		}
		else
			*bValidFlags = FALSE;
	}

	return numIntra;
}

int isLeftAvailable(CUData* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags, bool cip)
{
	const uint32_t rasterPartBegin = g_zscanToRaster[partIdxLT];
	const uint32_t rasterPartEnd = g_zscanToRaster[partIdxLB];
	const uint32_t idxStep = cu->m_slice->m_sps->numPartInCUSize;
	int numIntra = 0;

	for (uint32_t rasterPart = rasterPartBegin; rasterPart <= rasterPartEnd; rasterPart += idxStep, bValidFlags--) // opposite direction
	{
		uint32_t partLeft;
		CUData* cuLeft = CUData_getPULeft(cu, &partLeft, g_rasterToZscan[rasterPart]);
		if (cuLeft && (!cip || isIntra_cudata(cuLeft, partLeft)))
		{
			numIntra++;
			*bValidFlags = TRUE;
		}
		else
			*bValidFlags = FALSE;
	}
	return numIntra;
}

int isAboveRightAvailable(CUData* cu, uint32_t partIdxRT, bool* bValidFlags, uint32_t numUnits, bool cip)
{
	int numIntra = 0;

	for (uint32_t offset = 1; offset <= numUnits; offset++, bValidFlags++)
	{
		uint32_t partAboveRight;
		CUData* cuAboveRight = CUData_getPUAboveRightAdi(cu, &partAboveRight, partIdxRT, offset);
		if (cuAboveRight && (!cip || isIntra_cudata(cuAboveRight, partAboveRight)))
		{
			numIntra++;
			*bValidFlags = TRUE;
		}
		else
			*bValidFlags = FALSE;
	}

	return numIntra;
}

int isBelowLeftAvailable(CUData* cu, uint32_t partIdxLB, bool* bValidFlags, uint32_t numUnits, bool cip)
{
	int numIntra = 0;

	for (uint32_t offset = 1; offset <= numUnits; offset++, bValidFlags--) // opposite direction
	{
		uint32_t partBelowLeft;
		CUData* cuBelowLeft = CUData_getPUBelowLeftAdi(cu, &partBelowLeft, partIdxLB, offset);
		if (cuBelowLeft && (!cip || isIntra_cudata(cuBelowLeft, partBelowLeft)))
		{
			numIntra++;
			*bValidFlags = TRUE;
		}
		else
			*bValidFlags = FALSE;
	}

	return numIntra;
}

void PredictionUnit_PredictionUnit(struct PredictionUnit *predictionUnit, struct CUData* cu, const CUGeom* cuGeom, int puIdx)
{
	/* address of CTU */
	predictionUnit->ctuAddr = cu->m_cuAddr;

	/* offset of CU */
	predictionUnit->cuAbsPartIdx = cuGeom->absPartIdx;

	/* offset and dimensions of PU */
	CUData_getPartIndexAndSize(cu, puIdx, &predictionUnit->puAbsPartIdx, &predictionUnit->width, &predictionUnit->height);
}

void Predict_motionCompensation(struct Predict *predict, CUData* cu, struct PredictionUnit* pu, Yuv* predYuv, bool bLuma, bool bChroma)
{/*
	int refIdx0 = cu->m_refIdx[0][pu->puAbsPartIdx];
	int refIdx1 = cu->m_refIdx[1][pu->puAbsPartIdx];

	if (isInterP(cu->m_slice))
	{
		// P Slice //
		struct WeightValues wv0[3];

		const WeightParam *wp0 = cu->m_slice->m_weightPredTable[0][refIdx0];

		MV mv0 = cu->m_mv[0][pu->puAbsPartIdx];
		CUData_clipMv(cu, &mv0);

		if (cu->m_slice->m_pps->bUseWeightPred && wp0->bPresentFlag)
		{
			for (int plane = 0; plane < 3; plane++)
			{
				wv0[plane].w = wp0[plane].inputWeight;
				wv0[plane].offset = wp0[plane].inputOffset * (1 << (X265_DEPTH - 8));
				wv0[plane].shift = wp0[plane].log2WeightDenom;
				wv0[plane].round = wp0[plane].log2WeightDenom >= 1 ? 1 << (wp0[plane].log2WeightDenom - 1) : 0;
			}

			ShortYuv* shortYuv = &predict->m_predShortYuv[0];

			if (bLuma)
				Predict_predInterLumaShort(pu, shortYuv, cu->m_slice->m_refPicList[0][refIdx0]->m_reconPic, &mv0);
			if (bChroma)
				Predict_predInterChromaShort(predict, pu, shortYuv, cu->m_slice->m_refPicList[0][refIdx0]->m_reconPic, &mv0);

			Predict_addWeightUni(pu, predYuv, shortYuv, wv0, bLuma, bChroma);
		}
		else
		{
			if (bLuma)
				Predict_predInterLumaPixel(pu, predYuv, cu->m_slice->m_refPicList[0][refIdx0]->m_reconPic, &mv0);
			if (bChroma)
				Predict_predInterChromaPixel(predict, pu, predYuv, cu->m_slice->m_refPicList[0][refIdx0]->m_reconPic, &mv0);
		}
	}*/
}
void Predict_predInterLumaShort(struct PredictionUnit* pu, ShortYuv* dstSYuv, PicYuv* refPic, MV *mv)
{/*
	int16_t* dst = ShortYuv_getLumaAddr(dstSYuv, pu->puAbsPartIdx);
	intptr_t dstStride = dstSYuv->m_size;

	intptr_t srcStride = refPic->m_stride;
	intptr_t srcOffset = (mv->x >> 2) + (mv->y >> 2) * srcStride;
	pixel* src = Picyuv_CUgetLumaAddr(refPic, pu->ctuAddr, pu->cuAbsPartIdx + pu->puAbsPartIdx) + srcOffset;

	int xFrac = mv->x & 0x3;
	int yFrac = mv->y & 0x3;

	int partEnum = partitionFromSizes(pu->width, pu->height);

	if (!(yFrac | xFrac))
		filterPixelToShort_c(src, srcStride, dst, dstStride, pu->width, pu->height);
	else if (!yFrac)
		interp_horiz_ps_c(src, srcStride, dst, dstStride, xFrac, 0, pu->width, pu->height, 8);
	else if (!xFrac)
		interp_vert_ps_c(src, srcStride, dst, dstStride, yFrac, pu->width, pu->height, 8);
	else
	{
		int tmpStride = pu->width;
		int filterSize = NTAPS_LUMA;
		int halfFilterSize = (filterSize >> 1);

		interp_horiz_ps_c(src, srcStride, m_immedVals, tmpStride, xFrac, 1, pu->width, pu->height, 8);
		interp_vert_ss_c(m_immedVals + (halfFilterSize - 1) * tmpStride, tmpStride, dst, dstStride, yFrac, pu->width, pu->height, 8);

	}*/
}

void Predict_predInterChromaShort(struct Predict *predict, struct PredictionUnit* pu, ShortYuv* dstSYuv, PicYuv* refPic, MV *mv)
{/*
	intptr_t refStride = refPic->m_strideC;
	intptr_t dstStride = dstSYuv->m_csize;

	int shiftHor = (2 + predict->m_hChromaShift);
	int shiftVer = (2 + predict->m_vChromaShift);

	intptr_t refOffset = (mv->x >> shiftHor) + (mv->y >> shiftVer) * refStride;

	pixel* refCb = Picyuv_CUgetCbAddr(refPic, pu->ctuAddr, pu->cuAbsPartIdx + pu->puAbsPartIdx) + refOffset;
	pixel* refCr = Picyuv_CUgetCrAddr(refPic, pu->ctuAddr, pu->cuAbsPartIdx + pu->puAbsPartIdx) + refOffset;

	int16_t* dstCb = ShortYuv_getCbAddr(dstSYuv, pu->puAbsPartIdx);
	int16_t* dstCr = ShortYuv_getCrAddr(dstSYuv, pu->puAbsPartIdx);

	int xFrac = mv->x & ((1 << shiftHor) - 1);
	int yFrac = mv->y & ((1 << shiftVer) - 1);

	int partEnum = partitionFromSizes(pu->width, pu->height);

	uint32_t cxWidth = pu->width >> predict->m_hChromaShift;

	if (!(yFrac | xFrac))
	{
		filterPixelToShort_c(refCb, refStride, dstCb, dstStride, pu->width, pu->height);
		filterPixelToShort_c(refCr, refStride, dstCr, dstStride, pu->width, pu->height);
	}
	else if (!yFrac)
	{
		interp_horiz_ps_c(refCb, refStride, dstCb, dstStride, xFrac << (1 - predict->m_hChromaShift), 0, pu->width, pu->height, 4);
		interp_horiz_ps_c(refCr, refStride, dstCr, dstStride, xFrac << (1 - predict->m_hChromaShift), 0, pu->width, pu->height, 4);
	}
	else if (!xFrac)
	{
		interp_vert_ps_c(refCb, refStride, dstCb, dstStride, yFrac << (1 - predict->m_vChromaShift), pu->width, pu->height, 4);
		interp_vert_ps_c(refCr, refStride, dstCr, dstStride, yFrac << (1 - predict->m_vChromaShift), pu->width, pu->height, 4);
	}
	else
	{
		int extStride = cxWidth;
		int filterSize = NTAPS_CHROMA;
		int halfFilterSize = (filterSize >> 1);
		interp_horiz_ps_c(refCb, refStride, predict->m_immedVals, extStride, xFrac << (1 - predict->m_hChromaShift), 1, pu->width, pu->height, 4);
		interp_vert_ss_c(m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, yFrac << (1 - predict->m_vChromaShift), pu->width, pu->height, 4);
		interp_horiz_ps_c(refCr, refStride, predict->m_immedVals, extStride, xFrac << (1 - predict->m_hChromaShift), 1, pu->width, pu->height, 4);
		interp_vert_ss_c(m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, yFrac << (1 - predict->m_vChromaShift), pu->width, pu->height, 4);
	}*/
}

/* weighted averaging for uni-pred */
void Predict_addWeightUni(struct PredictionUnit* pu, Yuv* predYuv, ShortYuv* srcYuv, struct WeightValues wp[3], bool bLuma, bool bChroma)
{/*
	int w0, offset, shiftNum, shift, round;
	uint32_t srcStride, dstStride;

	if (bLuma)
	{
		pixel* dstY = Yuv_getLumaAddr(predYuv, pu->puAbsPartIdx);
		const int16_t* srcY0 = ShortYuv_getLumaAddr(srcYuv, pu->puAbsPartIdx);

		// Luma
		w0 = wp[0].w;
		offset = wp[0].offset;
		shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
		shift = wp[0].shift + shiftNum;
		round = shift ? (1 << (shift - 1)) : 0;
		srcStride = srcYuv->m_size;
		dstStride = predYuv->m_size;

		primitives.weight_sp(srcY0, dstY, srcStride, dstStride, pu->width, pu->height, w0, round, shift, offset);
	}

	if (bChroma)
	{
		pixel* dstU = Yuv_getCbAddr(predYuv, pu->puAbsPartIdx);
		pixel* dstV = Yuv_getCrAddr(predYuv, pu->puAbsPartIdx);
		const int16_t* srcU0 = ShortYuv_getCbAddr(srcYuv, pu->puAbsPartIdx);
		const int16_t* srcV0 = ShortYuv_getCbAddr(srcYuv, pu->puAbsPartIdx);

		// Chroma U
		w0 = wp[1].w;
		offset = wp[1].offset;
		shiftNum = IF_INTERNAL_PREC - X265_DEPTH;
		shift = wp[1].shift + shiftNum;
		round = shift ? (1 << (shift - 1)) : 0;

		srcStride = srcYuv->m_csize;
		dstStride = predYuv->m_csize;

		uint32_t cwidth = pu->width >> srcYuv->m_hChromaShift;
		uint32_t cheight = pu->height >> srcYuv->m_vChromaShift;

		primitives.weight_sp(srcU0, dstU, srcStride, dstStride, cwidth, cheight, w0, round, shift, offset);

		// Chroma V
		w0 = wp[2].w;
		offset = wp[2].offset;
		shift = wp[2].shift + shiftNum;
		round = shift ? (1 << (shift - 1)) : 0;

		primitives.weight_sp(srcV0, dstV, srcStride, dstStride, cwidth, cheight, w0, round, shift, offset);
	}*/
}

void Predict_predInterLumaPixel(struct PredictionUnit* pu, Yuv* dstYuv, PicYuv* refPic, MV *mv)
{/*
	pixel* dst = Yuv_getLumaAddr(dstYuv, pu->puAbsPartIdx);
	intptr_t dstStride = dstYuv->m_size;

	intptr_t srcStride = refPic->m_stride;
	intptr_t srcOffset = (mv->x >> 2) + (mv->y >> 2) * srcStride;
	int partEnum = partitionFromSizes(pu->width, pu->height);
	pixel* src = Picyuv_CUgetLumaAddr(refPic, pu->ctuAddr, pu->cuAbsPartIdx + pu->puAbsPartIdx) + srcOffset;

	int xFrac = mv->x & 0x3;
	int yFrac = mv->y & 0x3;

	if (!(yFrac | xFrac))
		primitives.pu[partEnum].copy_pp(dst, dstStride, src, srcStride, pu->width, pu->height);//copy_pp = blockcopy_pp_c
	else if (!yFrac)
		interp_horiz_pp_c(src, srcStride, dst, dstStride, xFrac, pu->width, pu->height, 8);
	else if (!xFrac)
		interp_vert_pp_c(src, srcStride, dst, dstStride, yFrac, pu->width, pu->height, 8);
	else
		interp_hv_pp_c(src, srcStride, dst, dstStride, xFrac, yFrac, pu->width, pu->height, 8);*/
}
void Predict_predInterChromaPixel(struct Predict *predict, struct PredictionUnit* pu, Yuv* dstYuv, PicYuv* refPic, MV *mv)
{/*
	intptr_t dstStride = dstYuv->m_csize;
	intptr_t refStride = refPic->m_strideC;

	int shiftHor = (2 + predict->m_hChromaShift);
	int shiftVer = (2 + predict->m_vChromaShift);

	intptr_t refOffset = (mv->x >> shiftHor) + (mv->y >> shiftVer) * refStride;

	pixel* refCb = Picyuv_CUgetCbAddr(refPic, pu->ctuAddr, pu->cuAbsPartIdx + pu->puAbsPartIdx) + refOffset;
	pixel* refCr = Picyuv_CUgetCrAddr(refPic, pu->ctuAddr, pu->cuAbsPartIdx + pu->puAbsPartIdx) + refOffset;

	pixel* dstCb = Yuv_getCbAddr(dstYuv, pu->puAbsPartIdx);
	pixel* dstCr = Yuv_getCrAddr(dstYuv, pu->puAbsPartIdx);

	int xFrac = mv->x & ((1 << shiftHor) - 1);
	int yFrac = mv->y & ((1 << shiftVer) - 1);

	int partEnum = partitionFromSizes(pu->width, pu->height);

	if (!(yFrac | xFrac))
	{
		primitives.chroma[predict->m_csp].pu[partEnum].copy_pp(dstCb, dstStride, refCb, refStride, 16, 16);
		primitives.chroma[predict->m_csp].pu[partEnum].copy_pp(dstCr, dstStride, refCr, refStride, 16, 16);
	}
	else if (!yFrac)
	{
		interp_horiz_pp_c(refCb, refStride, dstCb, dstStride, xFrac << (1 - predict->m_hChromaShift), 16, 16, 4);
		interp_horiz_pp_c(refCr, refStride, dstCr, dstStride, xFrac << (1 - predict->m_hChromaShift), 16, 16, 4);
	}
	else if (!xFrac)
	{
		interp_vert_pp_c(refCb, refStride, dstCb, dstStride, yFrac << (1 - predict->m_vChromaShift), 16, 16, 4);
		interp_vert_pp_c(refCr, refStride, dstCr, dstStride, yFrac << (1 - predict->m_vChromaShift), 16, 16, 4);
	}
	else
	{
		int extStride = pu->width >> predict->m_hChromaShift;
		int filterSize = NTAPS_CHROMA;
		int halfFilterSize = (filterSize >> 1);

		interp_horiz_ps_c(refCb, refStride, m_immedVals, extStride, xFrac << (1 - predict->m_hChromaShift), 1, 16, 16, 4);
		interp_vert_sp_c(m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCb, dstStride, yFrac << (1 - predict->m_vChromaShift), 16, 16, 4);

		interp_horiz_ps_c(refCr, refStride, m_immedVals, extStride, xFrac << (1 - predict->m_hChromaShift), 1, 16, 16, 4);
		interp_vert_sp_c(m_immedVals + (halfFilterSize - 1) * extStride, extStride, dstCr, dstStride, yFrac << (1 - predict->m_vChromaShift), 16, 16, 4);
	}*/
}
