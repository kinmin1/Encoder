/*
* quant.c
*
*  Created on: 2016-5-15
*      Author: Administrator
*/
#include "common.h"
#include "math.h"
#include "quant.h"
#include "primitives.h"
#include "cudata.h"
#include "dct.h"
#include "constants.h"
#include "framedata.h"

extern EncoderPrimitives primitives;
int MF[6] = { 26214, 23302, 20560, 18396, 16384, 14564 };//量化缩放因子

int V[6] = { 40, 45, 51, 57, 64, 72 };//反量化缩放因子

void setQpParam(QpParam *qpParam, int qpScaled)
{
	if (qpParam->qp != qpScaled)
	{
		qpParam->rem = qpScaled % 6;
		qpParam->per = qpScaled / 6;
		qpParam->qp = qpScaled;
		qpParam->lambda2 = (int64_t)(x265_lambda2_tab[qpParam->qp - QP_BD_OFFSET] * 256. + 0.5);
		qpParam->lambda = (int32_t)(x265_lambda_tab[qpParam->qp - QP_BD_OFFSET] * 256. + 0.5);
		X265_CHECK((x265_lambda_tab[qpParam->qp - QP_BD_OFFSET] * 256. + 0.5) < (double)MAX_INT, "x265_lambda_tab[] value too large\n");
	}
}
void Quant_setChromaQP(Quant *quant, int qpin, TextType ttype, int chFmt)
{
	int qp = x265_clip3(-QP_BD_OFFSET, 57, qpin);
	if (qp >= 30)
	{
		if (chFmt == X265_CSP_I420)
			qp = g_chromaScale[qp];
		else
			qp = X265_MIN(qp, QP_MAX_SPEC);
	}
	setQpParam(&quant->m_qpParam[ttype], qp + QP_BD_OFFSET);
}

void Quant_setQPforQuant(Quant *quant, CUData* ctu, int qp)
{
	quant->m_tqBypass = !!ctu->m_tqBypass[0];
	if (quant->m_tqBypass)
		return;
	quant->m_nr = quant->m_frameNr ? &quant->m_frameNr[ctu->m_encData->m_frameEncoderID] : NULL;
	setQpParam(&quant->m_qpParam[TEXT_LUMA], qp + QP_BD_OFFSET);
	Quant_setChromaQP(quant, qp + ctu->m_slice->m_pps->chromaQpOffset[0], TEXT_CHROMA_U, ctu->m_chromaFormat);
	Quant_setChromaQP(quant, qp + ctu->m_slice->m_pps->chromaQpOffset[1], TEXT_CHROMA_V, ctu->m_chromaFormat);
}

/* Context derivation process of coeff_abs_significant_flag */
uint32_t getSigCoeffGroupCtxInc(uint64_t cgGroupMask, uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG)
{
	X265_CHECK(cgBlkPos < 64, "cgBlkPos is too large\n");
	// NOTE: unsafe shift operator, see NOTE in calcPatternSigCtx
	const uint32_t sigPos = (uint32_t)(cgGroupMask >> (cgBlkPos + 1)); // just need lowest 8-bits valid
	const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & sigPos;//右相邻CG的CSBF的值。
	const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 1));//左相邻CG的CSBF的值。

	return (sigRight | sigLower) & 1;//ctx=min(1,Sr+Sl)。
}
/* Pattern decision for context derivation process of significant_coeff_flag */
uint32_t calcPatternSigCtx(uint64_t sigCoeffGroupFlag64, uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG)
{
	if (trSizeCG == 1)
		return 0;

	X265_CHECK(trSizeCG <= 8, "transform CG is too large\n");
	X265_CHECK(cgBlkPos < 64, "cgBlkPos is too large\n");
	// NOTE: cgBlkPos+1 may more than 63, it is invalid for shift,
	//       but in this case, both cgPosX and cgPosY equal to (trSizeCG - 1),
	//       the sigRight and sigLower will clear value to zero, the final result will be correct
	const uint32_t sigPos = (uint32_t)(sigCoeffGroupFlag64 >> (cgBlkPos + 1)); // just need lowest 7-bits valid

	// TODO: instruction BT is faster, but _bittest64 still generate instruction 'BT m, r' in VS2012
	const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & (sigPos & 1);
	const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 2)) & 2;
	return sigRight + sigLower;
}
/* Context derivation process of coeff_abs_significant_flag */
uint32_t getSigCtxInc(uint32_t patternSigCtx, uint32_t log2TrSize, uint32_t trSize, uint32_t blkPos, char bIsLuma, uint32_t firstSignificanceMapContext)
{
	static const uint8_t ctxIndMap[16] =
	{
		0, 1, 4, 5,
		2, 3, 4, 5,
		6, 6, 8, 8,
		7, 7, 8, 8
	};

	if (!blkPos) // special case for the DC context variable
		return 0;

	if (log2TrSize == 2) // 4x4
		return ctxIndMap[blkPos];

	const uint32_t posY = blkPos >> log2TrSize;
	const uint32_t posX = blkPos & (trSize - 1);
	X265_CHECK((blkPos - (posY << log2TrSize)) == posX, "block pos check failed\n");

	int posXinSubset = blkPos & 3;
	X265_CHECK((posX & 3) == (blkPos & 3), "pos alignment fail\n");
	int posYinSubset = posY & 3;

	// NOTE: [patternSigCtx][posXinSubset][posYinSubset]
	static const uint8_t table_cnt[4][4][4] =
	{
		// patternSigCtx = 0
		{
			{ 2, 1, 1, 0 },
			{ 1, 1, 0, 0 },
			{ 1, 0, 0, 0 },
			{ 0, 0, 0, 0 },
		},
		// patternSigCtx = 1
		{
			{ 2, 1, 0, 0 },
			{ 2, 1, 0, 0 },
			{ 2, 1, 0, 0 },
			{ 2, 1, 0, 0 },
		},
		// patternSigCtx = 2
		{
			{ 2, 2, 2, 2 },
			{ 1, 1, 1, 1 },
			{ 0, 0, 0, 0 },
			{ 0, 0, 0, 0 },
		},
		// patternSigCtx = 3
		{
			{ 2, 2, 2, 2 },
			{ 2, 2, 2, 2 },
			{ 2, 2, 2, 2 },
			{ 2, 2, 2, 2 },
		}
	};

	int cnt = table_cnt[patternSigCtx][posXinSubset][posYinSubset];
	int offset = firstSignificanceMapContext;

	offset += cnt;

	return (bIsLuma && (posX | posY) >= 4) ? 3 + offset : offset;
}

bool Quant_init(Quant *quant, int rdoqLevel, double psyScale, ScalingList* scalingList, Entropy* entropy)
{
	quant->m_entropyCoder = entropy;
	quant->m_rdoqLevel = rdoqLevel;
	quant->m_psyRdoqScale = (int32_t)(psyScale * 256.0);
	X265_CHECK((psyScale * 256.0) < (double)MAX_INT, "psyScale value too large\n");
	quant->m_scalingList = scalingList;
	quant->m_resiDctCoeff = X265_MALLOC(int16_t, MAX_TR_SIZE * MAX_TR_SIZE * 2);//已释放
	if (quant->m_resiDctCoeff == NULL)
		printf("malloc m_resiDctCoeff failed!\n");

	quant->m_fencDctCoeff = quant->m_resiDctCoeff + (MAX_TR_SIZE * MAX_TR_SIZE);
	quant->m_fencShortBuf = X265_MALLOC(int16_t, MAX_TR_SIZE * MAX_TR_SIZE);//已释放
	quant->m_tqBypass = FALSE;

	return quant->m_resiDctCoeff && quant->m_fencShortBuf;
}
/* To minimize the distortion only. No rate is considered */
uint32_t Quant_signBitHidingHDQ(Quant *quant, int16_t* coeff, int32_t* deltaU, uint32_t numSig, struct TUEntropyCodingParameters *codeParams)
{
	const uint32_t log2TrSizeCG = codeParams->log2TrSizeCG;
	const uint16_t* scan = codeParams->scan;
	bool lastCG = TRUE;

	for (int cg = (1 << (log2TrSizeCG * 2)) - 1; cg >= 0; cg--)
	{
		int cgStartPos = cg << LOG2_SCAN_SET_SIZE;
		int n;

		for (n = SCAN_SET_SIZE - 1; n >= 0; --n)
			if (coeff[scan[n + cgStartPos]])
				break;
		if (n < 0)
			continue;

		int lastNZPosInCG = n;

		for (n = 0;; n++)
			if (coeff[scan[n + cgStartPos]])
				break;

		int firstNZPosInCG = n;

		if (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD)
		{
			uint32_t signbit = coeff[scan[cgStartPos + firstNZPosInCG]] > 0 ? 0 : 1;
			uint32_t absSum = 0;

			for (n = firstNZPosInCG; n <= lastNZPosInCG; n++)
				absSum += coeff[scan[n + cgStartPos]];

			if (signbit != (absSum & 0x1)) // compare signbit with sum_parity
			{
				int minCostInc = MAX_INT, minPos = -1, curCost = MAX_INT;
				int16_t finalChange = 0, curChange = 0;

				for (n = (lastCG ? lastNZPosInCG : SCAN_SET_SIZE - 1); n >= 0; --n)
				{
					uint32_t blkPos = scan[n + cgStartPos];
					if (coeff[blkPos])
					{
						if (deltaU[blkPos] > 0)
						{
							curCost = -deltaU[blkPos];
							curChange = 1;
						}
						else
						{
							if (n == firstNZPosInCG && abs(coeff[blkPos]) == 1)
								curCost = MAX_INT;
							else
							{
								curCost = deltaU[blkPos];
								curChange = -1;
							}
						}
					}
					else
					{
						if (n < firstNZPosInCG)
						{
							uint32_t thisSignBit = quant->m_resiDctCoeff[blkPos] >= 0 ? 0 : 1;
							if (thisSignBit != signbit)
								curCost = MAX_INT;
							else
							{
								curCost = -deltaU[blkPos];
								curChange = 1;
							}
						}
						else
						{
							curCost = -deltaU[blkPos];
							curChange = 1;
						}
					}

					if (curCost < minCostInc)
					{
						minCostInc = curCost;
						finalChange = curChange;
						minPos = blkPos;
					}
				}

				/* do not allow change to violate coeff clamp */
				if (coeff[minPos] == 32767 || coeff[minPos] == -32768)
					finalChange = -1;

				if (!coeff[minPos])
					numSig++;
				else if (finalChange == -1 && abs(coeff[minPos]) == 1)
					numSig--;

				if (quant->m_resiDctCoeff[minPos] >= 0)
					coeff[minPos] += finalChange;
				else
					coeff[minPos] -= finalChange;
			}
		}

		lastCG = FALSE;
	}

	return numSig;
}

uint32_t Quant_transformNxN(Quant *quant, struct CUData* cu, const pixel* fenc, uint32_t fencStride, int16_t* residual, uint32_t resiStride, coeff_t* coeff, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx, bool useTransformSkip)
{
	/*
	uint32_t sizeIdx = log2TrSize - 2;
	if (quant->m_tqBypass)
	{
		X265_CHECK(log2TrSize >= 2 && log2TrSize <= 5, "Block size mistake!\n");
		return primitives.cu[sizeIdx].copy_cnt(coeff, residual, resiStride, pow(2, double(log2TrSize)));
	}

	bool isLuma = ttype == TEXT_LUMA;
	bool usePsy = quant->m_psyRdoqScale && isLuma && !useTransformSkip;
	int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize; // Represents scaling through forward transform

	X265_CHECK((cu->m_slice->m_sps->quadtreeTULog2MaxSize >= log2TrSize), "transform size too large\n");
	if (useTransformSkip)
	{
#if X265_DEPTH <= 10
		X265_CHECK(transformShift >= 0, "invalid transformShift\n");
		primitives.cu[sizeIdx].cpy2Dto1D_shl(quant->m_resiDctCoeff, residual, resiStride, transformShift, pow(2, double(log2TrSize)));
#else
		if (transformShift >= 0)
			primitives.cu[sizeIdx].cpy2Dto1D_shl(quant->m_resiDctCoeff, residual, resiStride, transformShift, pow(2, log2TrSize));
		else
			primitives.cu[sizeIdx].cpy2Dto1D_shr(quant->m_resiDctCoeff, residual, resiStride, -transformShift, pow(2, log2TrSize));
#endif
	}
	else
	{
		bool isIntra = isIntra_cudata(cu, absPartIdx);

		if (!sizeIdx && isLuma && isIntra)
			primitives.dst4x4(residual, quant->m_resiDctCoeff, resiStride);
		else
			primitives.cu[sizeIdx].dct(residual, quant->m_resiDctCoeff, resiStride);
		// NOTE: if RDOQ is disabled globally, psy-rdoq is also disabled, so
		// there is no risk of performing this DCT unnecessarily //
		if (usePsy)
		{
			int trSize = 1 << log2TrSize;
			// perform DCT on source pixels for psy-rdoq //
			primitives.cu[sizeIdx].copy_ps(quant->m_fencShortBuf, trSize, fenc, fencStride, pow(2, double(log2TrSize)), pow(2, double(log2TrSize)));
			primitives.cu[sizeIdx].dct(quant->m_fencShortBuf, quant->m_fencDctCoeff, trSize);
		}

		if (quant->m_nr)
		{
			// denoise is not applied to intra residual, so DST can be ignored //
			int cat = sizeIdx + 4 * !isLuma + 8 * !isIntra;
			int numCoeff = 1 << (log2TrSize * 2);
			primitives.denoiseDct(quant->m_resiDctCoeff, quant->m_nr->residualSum[cat], quant->m_nr->offsetDenoise[cat], numCoeff);
			quant->m_nr->count[cat]++;
		}
	}

   {
	   int deltaU[32 * 32];

	   int scalingListType = (isIntra_cudata(cu, absPartIdx) ? 0 : 3) + ttype;
	   int rem = quant->m_qpParam[ttype].rem;
	   int per = quant->m_qpParam[ttype].per;
	   //const int32_t* quantCoeff=malloc(4*6*6);
	   const int32_t* quantCoeff = quant->m_scalingList->m_quantCoef[log2TrSize - 2][scalingListType][rem];

	   int qbits = QUANT_SHIFT + per + transformShift;
	   int add = (cu->m_slice->m_sliceType == I_SLICE ? 171 : 85) << (qbits - 9);
	   int numCoeff = 1 << (log2TrSize * 2);

	   uint32_t numSig = primitives.quant(quant->m_resiDctCoeff, quantCoeff, deltaU, coeff, qbits, add, numCoeff);

	   if (numSig >= 2 && cu->m_slice->m_pps->bSignHideEnabled)
	   {
		   TUEntropyCodingParameters *codeParams;
		   getTUEntropyCodingParameters(cu, codeParams, absPartIdx, log2TrSize, isLuma);
		   return Quant_signBitHidingHDQ(quant, coeff, deltaU, numSig, codeParams);
	   }
	   else
		   return numSig;
   }*/
	return 0;
}

void Quant_invtransformNxN(Quant *quant, int16_t* residual, uint32_t resiStride, coeff_t* coeff,
	uint32_t log2TrSize, TextType ttype, bool bIntra, bool useTransformSkip, uint32_t numSig)
{
	const uint32_t sizeIdx = log2TrSize - 2;
	if (quant->m_tqBypass)
	{
		primitives.cu[sizeIdx].cpy1Dto2D_shl(residual, coeff, resiStride, 0, pow(2, double(log2TrSize)));
		return;
	}

	// Values need to pass as input parameter in dequant
	int rem = quant->m_qpParam[ttype].rem;
	int per = quant->m_qpParam[ttype].per;
	int transformShift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH - log2TrSize;
	int shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - transformShift;
	int numCoeff = 1 << (log2TrSize * 2);

	if (quant->m_scalingList->m_bEnabled)
	{
		int scalingListType = (bIntra ? 0 : 3) + ttype;
		const int32_t* dequantCoef = quant->m_scalingList->m_dequantCoef[sizeIdx][scalingListType][rem];
		primitives.dequant_scaling(coeff, dequantCoef, quant->m_resiDctCoeff, numCoeff, per, shift);
	}
	else
	{
		int scale = quant->m_scalingList->s_invQuantScales[rem] << per;
		primitives.dequant_normal(coeff, quant->m_resiDctCoeff, numCoeff, scale, shift);
	}

	if (useTransformSkip)
	{
#if X265_DEPTH <= 10
		X265_CHECK(transformShift > 0, "invalid transformShift\n");
		primitives.cu[sizeIdx].cpy1Dto2D_shr(residual, quant->m_resiDctCoeff, resiStride, transformShift, pow(2, double(log2TrSize)));
#else
		if (transformShift > 0)
			primitives.cu[sizeIdx].cpy1Dto2D_shr(residual, m_resiDctCoeff, resiStride, transformShift);
		else
			primitives.cu[sizeIdx].cpy1Dto2D_shl(residual, m_resiDctCoeff, resiStride, -transformShift);
#endif
	}
	else
	{
		int useDST = !sizeIdx && ttype == TEXT_LUMA && bIntra;
		X265_CHECK((int)numSig == primitives.cu[log2TrSize - 2].count_nonzero(pow(2, double(log2TrSize)), coeff), "numSig differ\n");
		// DC only
		if (numSig == 1 && coeff[0] != 0 && !useDST)
		{
			const int shift_1st = 7 - 6;
			const int add_1st = 1 << (shift_1st - 1);
			const int shift_2nd = 12 - (X265_DEPTH - 8) - 3;
			const int add_2nd = 1 << (shift_2nd - 1);

			int dc_val = (((quant->m_resiDctCoeff[0] * (64 >> 6) + add_1st) >> shift_1st) * (64 >> 3) + add_2nd) >> shift_2nd;
			primitives.cu[sizeIdx].blockfill_s(residual, resiStride, (int16_t)dc_val, pow(2, double(log2TrSize)));
			return;
		}

		if (useDST)
			primitives.idst4x4(quant->m_resiDctCoeff, residual, resiStride);
		else
			primitives.cu[sizeIdx].idct(quant->m_resiDctCoeff, residual, resiStride);
	}
}