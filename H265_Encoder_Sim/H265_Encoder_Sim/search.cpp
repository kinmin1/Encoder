/*
* search.c
*
*  Created on: 2016-5-15
*      Author: Administrator
*/
#include "constants.h"
#include "contexts.h"
#include "entropy.h"
#include "primitives.h"
#include "predict.h"
#include "rdcost.h"
#include "search.h"
#include "quant.h"
#include <math.h>
#include <string.h>
#include "pixel.h"
#include "shortyuv.h"

#define MVP_IDX_BITS 1

void init_intraNeighbors(struct IntraNeighbors *intraNeighbors)
{
	for (int i = 0; i < 65; i++)
		intraNeighbors->bNeighborFlags[i] = TRUE;
}
void init_Cost(struct Cost *cost)
{
	cost->rdcost = 0; 
	cost->bits = 0; 
	cost->distortion = 0; 
	cost->energy = 0;
}
void initCosts(Mode *mode)
{
	mode->rdCost = 0;
	mode->sa8dCost = 0;
	mode->sa8dBits = 0;
	mode->psyEnergy = 0;
	mode->distortion = 0;
	mode->totalBits = 0;
	mode->mvBits = 0;
	mode->coeffBits = 0;
}

void invalidate(Mode *mode)
{
	/* set costs to invalid data, catch uninitialized re-use */
	mode->rdCost = UINT32_MAX / 2;  //mode->rdCost = UINT64_MAX / 2;
	mode->sa8dCost = UINT32_MAX / 2;//mode->rdCost = UINT64_MAX / 2;
	mode->sa8dBits = MAX_UINT / 2;
	mode->psyEnergy = MAX_UINT / 2;
	mode->distortion = MAX_UINT / 2;
	mode->totalBits = MAX_UINT / 2;
	mode->mvBits = MAX_UINT / 2;
	mode->coeffBits = MAX_UINT / 2;
}

bool ok(const Mode *mode)
{
	return !(mode->rdCost >= UINT32_MAX / 2 ||  //mode->rdCost >= UINT64_MAX / 2
		mode->sa8dCost >= UINT32_MAX / 2 ||//mode->sa8dCost >= UINT64_MAX / 2
		mode->sa8dBits >= MAX_UINT / 2 ||
		mode->psyEnergy >= MAX_UINT / 2 ||
		mode->distortion >= MAX_UINT / 2 ||
		mode->totalBits >= MAX_UINT / 2 ||
		mode->mvBits >= MAX_UINT / 2 ||
		mode->coeffBits >= MAX_UINT / 2);
}
coeff_t m_rqt_0[1536] = { 1 };
coeff_t m_rqt_1[1536] = { 1 };
coeff_t m_rqt_2[1536] = { 1 };
coeff_t m_rqt_3[1536] = { 1 };

uint8_t m_qtTempCbf[192] = { 1 };
uint8_t m_qtTempTransformSkipFlag[192] = { 1 };
pixel m_intraPred[10240] = { 1 };

bool initSearch(Search* search, x265_param* param, ScalingList *scalingList)
{
	int mov = 0;
	uint32_t maxLog2CUSize = g_log2Size[param->maxCUSize];
	search->m_param = param;
	search->m_bEnableRDOQ = !!param->rdoqLevel;
	search->m_bFrameParallel = param->frameNumThreads > 1;
	search->m_numLayers = g_log2Size[param->maxCUSize] - 2;

	setPsyRdScale(&(search->m_rdCost), param->psyRd);
	//MotionEstimate_init(&search->m_me, param->searchMethod, param->subpelRefine, param->internalCsp);
	bool ok = Quant_init(&search->m_quant, param->rdoqLevel, param->psyRdoq, scalingList, &search->m_entropyCoder);

	ok &= allocBuffers(&search->predict, 1); // sets m_hChromaShift & m_vChromaShift
	
	// When frame parallelism is active, only 'refLagPixels' of reference frames will be guaranteed
	// available for motion reference.  See refLagRows in FrameEncoder::compressCTURows()
	search->m_refLagPixels = search->m_bFrameParallel ? param->searchRange : param->sourceHeight;

	uint32_t sizeL = 1 << (maxLog2CUSize * 2);
	uint32_t sizeC = sizeL >> 2;
	uint32_t numPartitions = 1 << (maxLog2CUSize - LOG2_UNIT_SIZE) * 2;

	// these are indexed by qtLayer (log2size - 2) so nominally 0=4x4, 1=8x8, 2=16x16, 3=32x32
	// the coeffRQT and reconQtYuv are allocated to the max CU size at every depth. The parts
	// which are reconstructed at each depth are valid. At the end, the transform depth table
	// is walked and the coeff and recon at the correct depths are collected

	search->m_rqt[0].coeffRQT[0] = m_rqt_0;
	search->m_rqt[0].coeffRQT[1] = search->m_rqt[0].coeffRQT[0] + sizeL;
	search->m_rqt[0].coeffRQT[2] = search->m_rqt[0].coeffRQT[0] + sizeL + sizeC;

	search->m_rqt[1].coeffRQT[0] = m_rqt_1;
	search->m_rqt[1].coeffRQT[1] = search->m_rqt[1].coeffRQT[0] + sizeL;
	search->m_rqt[1].coeffRQT[2] = search->m_rqt[1].coeffRQT[0] + sizeL + sizeC;

	search->m_rqt[2].coeffRQT[0] = m_rqt_2;
	search->m_rqt[2].coeffRQT[1] = search->m_rqt[2].coeffRQT[0] + sizeL;
	search->m_rqt[2].coeffRQT[2] = search->m_rqt[2].coeffRQT[0] + sizeL + sizeC;

	search->m_rqt[3].coeffRQT[0] = m_rqt_3;
	search->m_rqt[3].coeffRQT[1] = search->m_rqt[3].coeffRQT[0] + sizeL;
	search->m_rqt[3].coeffRQT[2] = search->m_rqt[3].coeffRQT[0] + sizeL + sizeC;
	for (uint32_t i = 0; i <= search->m_numLayers; i++)
	{
		ok = Yuv_create_search(&(search->m_rqt[i].reconQtYuv), g_maxCUSize, 1, i);
		ok = ShortYuv_create_search(&(search->m_rqt[i].resiQtYuv), g_maxCUSize, 1, i);
	}
	
	// the rest of these buffers are indexed per-depth
	for (uint32_t i = 0; i <= g_maxCUDepth; i++)
	{
		int cuSize = g_maxCUSize >> i;
		ok = ShortYuv_create_search_1(&(search->m_rqt[i].tmpResiYuv), cuSize, 1, i);
		ok = Yuv_create_search_1(&(search->m_rqt[i].tmpPredYuv), cuSize, 1, i);
		ok = Yuv_create_search_2(&(search->m_rqt[i].bidirPredYuv[0]), cuSize, 1, i);
		ok = Yuv_create_search_3(&(search->m_rqt[i].bidirPredYuv[1]), cuSize, 1, i);
	}
	
	//CHECKED_MALLOC(search->m_qtTempCbf[0], uint8_t, numPartitions * 3);
	search->m_qtTempCbf[0] = m_qtTempCbf;
	if (!search->m_qtTempCbf)
	{
		printf("malloc of size %d failed\n", sizeof(uint8_t) * (192));
		goto fail;
	}
	search->m_qtTempCbf[1] = search->m_qtTempCbf[0] + numPartitions;
	search->m_qtTempCbf[2] = search->m_qtTempCbf[0] + numPartitions * 2;

	search->m_qtTempTransformSkipFlag[0] = m_qtTempTransformSkipFlag;

	if (!search->m_qtTempTransformSkipFlag)
	{
		printf("malloc of size %d failed\n", sizeof(uint8_t) * (192));
		goto fail;
	}

	search->m_qtTempTransformSkipFlag[1] = search->m_qtTempTransformSkipFlag[0] + numPartitions;
	search->m_qtTempTransformSkipFlag[2] = search->m_qtTempTransformSkipFlag[0] + numPartitions * 2;

	search->m_intraPred = m_intraPred;
	if (!search->m_intraPred)
	{
		printf("malloc of size %d failed\n", sizeof(pixel) * (10240));
		goto fail;
	}
	search->m_fencScaled = search->m_intraPred + 32 * 32;
	search->m_fencTransposed = search->m_fencScaled + 32 * 32;
	search->m_intraPredAngs = search->m_fencTransposed + 32 * 32;

	return ok;

fail:
	return FALSE;
}


int setLambdaFromQP(Search* search, CUData* ctu, int qp)
{
	X265_CHECK(qp >= QP_MIN && qp <= QP_MAX_MAX, "QP used for lambda is out of range\n");

	BitCost_init(&search->m_me.bitcost, qp);
	BitCost_setQP(&search->m_me.bitcost, qp);
	setQP(&search->m_rdCost, search->m_slice, qp);

	int quantQP = x265_clip3_int(QP_MIN, QP_MAX_SPEC, qp);
	Quant_setQPforQuant(&search->m_quant, ctu, quantQP);
	return quantQP;
}

void addSubCosts(Mode* dstMode, const Mode* subMode)
{
	X265_CHECK(ok(subMode), "sub-mode not initialized");

	dstMode->rdCost += subMode->rdCost;
	dstMode->sa8dCost += subMode->sa8dCost;
	dstMode->sa8dBits += subMode->sa8dBits;
	dstMode->psyEnergy += subMode->psyEnergy;
	dstMode->distortion += subMode->distortion;
	dstMode->totalBits += subMode->totalBits;
	dstMode->mvBits += subMode->mvBits;
	dstMode->coeffBits += subMode->coeffBits;
	
}

void invalidateContexts(Search* sea, int fromDepth)
{
	// catch reads without previous writes //
	for (int d = fromDepth; d < NUM_FULL_DEPTH; d++)
	{
		markInvalid(&(sea->m_rqt[d].cur));
		markInvalid(&(sea->m_rqt[d].rqtTemp));
		markInvalid(&(sea->m_rqt[d].rqtRoot));
		markInvalid(&(sea->m_rqt[d].rqtTest));
	}
}


void codeSubdivCbfQTChroma(Search* search, CUData* cu, uint32_t tuDepth, uint32_t absPartIdx)
{
	uint32_t subdiv = tuDepth < cu->m_tuDepth[absPartIdx];
	uint32_t log2TrSize = cu->m_log2CUSize[0] - tuDepth;

	if (!(log2TrSize - search->predict.m_hChromaShift < 2))
	{
		if (!tuDepth || getCbf(cu, absPartIdx, TEXT_CHROMA_U, tuDepth - 1))
			Entropy_codeQtCbfChroma(&(search->m_entropyCoder), cu, absPartIdx, TEXT_CHROMA_U, tuDepth, !subdiv);
		if (!tuDepth || getCbf(cu, absPartIdx, TEXT_CHROMA_V, tuDepth - 1))
			Entropy_codeQtCbfChroma(&(search->m_entropyCoder), cu, absPartIdx, TEXT_CHROMA_V, tuDepth, !subdiv);
	}

	if (subdiv)
	{
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
			codeSubdivCbfQTChroma(search, cu, tuDepth + 1, absPartIdx);
	}
}

void codeCoeffQTChroma(Search* search, CUData* cu, uint32_t tuDepth, uint32_t absPartIdx, enum TextType ttype)
{
	if (!getCbf(cu, absPartIdx, ttype, tuDepth))
		return;

	uint32_t log2TrSize = cu->m_log2CUSize[0] - tuDepth;

	if (tuDepth < cu->m_tuDepth[absPartIdx])
	{
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
			codeCoeffQTChroma(search, cu, tuDepth + 1, absPartIdx, ttype);

		return;
	}

	uint32_t tuDepthC = tuDepth;
	uint32_t log2TrSizeC = log2TrSize - search->predict.m_hChromaShift;

	if (log2TrSizeC < 2)
	{
		X265_CHECK(log2TrSize == 2 && search->predict.m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
		if (absPartIdx & 3)
			return;
		log2TrSizeC = 2;
		tuDepthC--;
	}

	uint32_t qtLayer = log2TrSize - 2;

	if (search->predict.m_csp != X265_CSP_I422)
	{
		uint32_t shift = (search->predict.m_csp == X265_CSP_I420) ? 2 : 0;
		uint32_t coeffOffset = absPartIdx << (LOG2_UNIT_SIZE * 2 - shift);
		coeff_t* coeff = search->m_rqt[qtLayer].coeffRQT[ttype] + coeffOffset;
		//coeff_t* coeff = (coeff_t*)search->m_entropyCoder.coeffC;
		codeCoeffNxN(&(search->m_entropyCoder), cu, coeff, absPartIdx, log2TrSizeC, ttype);
	}
	else
	{
		uint32_t coeffOffset = absPartIdx << (LOG2_UNIT_SIZE * 2 - 1);
		coeff_t* coeff = search->m_rqt[qtLayer].coeffRQT[ttype] + coeffOffset;
		uint32_t subTUSize = 1 << (log2TrSizeC * 2);
		uint32_t tuNumParts = 2 << ((log2TrSizeC - LOG2_UNIT_SIZE) * 2);
		if (getCbf(cu, absPartIdx, ttype, tuDepth + 1))
			codeCoeffNxN(&(search->m_entropyCoder), cu, coeff, absPartIdx, log2TrSizeC, ttype);
		if (getCbf(cu, absPartIdx + tuNumParts, ttype, tuDepth + 1))
			codeCoeffNxN(&(search->m_entropyCoder), cu, coeff + subTUSize, absPartIdx + tuNumParts, log2TrSizeC, ttype);
	}
	search->m_entropyCoder.m_fracBits = 18965994;
}

//Quant_init(Quant *quant, int rdoqLevel, double psyScale, const ScalingList* scalingList, Entropy* entropy);
void codeIntraLumaQT(Search* search, Mode* mode, CUGeom* cuGeom, uint32_t tuDepth, uint32_t absPartIdx, bool bAllowSplit, struct Cost* outCost, uint32_t depthRange[2])//选取最终的编码模式
{
	//CUData& cu = mode.cu;
	CUData* cu = &mode->cu;
	uint32_t fullDepth = cuGeom->depth + tuDepth;
	uint32_t log2TrSize = cuGeom->log2CUSize - tuDepth;
	uint32_t qtLayer = log2TrSize - 2;
	uint32_t sizeIdx = log2TrSize - 2;
	uint32_t width = (uint32_t)pow(2.0, double(sizeIdx + 2));
	bool mightNotSplit = log2TrSize <= depthRange[1];
	bool mightSplit = (log2TrSize > depthRange[0]) && (bAllowSplit || !mightNotSplit);//通过比较当前的TU大小是否介于允许的最大值和最小值之间，来确定TU是否继续分割

	// If maximum RD penalty, force spits at TU size 32x32 if SPS allows TUs of 16x16
	if (search->m_param->rdPenalty == 2 && search->m_slice->m_sliceType != I_SLICE && log2TrSize == 5 && depthRange[0] <= 4)
	{
		mightNotSplit = FALSE;
		mightSplit = TRUE;
	}

	struct Cost fullCost;
	uint32_t bCBF = 0;

	pixel*   reconQt = Yuv_getLumaAddr(&(search->m_rqt[qtLayer].reconQtYuv), absPartIdx);
	uint32_t reconQtStride = search->m_rqt[qtLayer].reconQtYuv.m_size;

	if (mightNotSplit)//TU=PU，即PU未划分时，计算各种COST
	{
		if (mightSplit)
			store(&(search->m_entropyCoder), &(search->m_rqt[fullDepth].rqtRoot));

		pixel* fenc = Yuv_getLumaAddr_const(mode->fencYuv, absPartIdx);
		pixel*   pred = Yuv_getLumaAddr(&(mode->predYuv), absPartIdx);
		int16_t* residual = ShortYuv_getLumaAddr(&(search->m_rqt[cuGeom->depth].tmpResiYuv), absPartIdx);
		uint32_t stride = mode->fencYuv->m_size;

		// init availability pattern
		uint32_t lumaPredMode = cu->m_lumaIntraDir[absPartIdx];
		struct IntraNeighbors intraNeighbors;
		initIntraNeighbors(cu, absPartIdx, tuDepth, TRUE, &intraNeighbors);
		initAdiPattern(&(search->predict), cu, cuGeom, absPartIdx, &intraNeighbors, lumaPredMode);

		// get prediction signal
		predIntraLumaAng(&(search->predict), lumaPredMode, pred, stride, log2TrSize);

		setTransformSkipSubParts(cu, 0, TEXT_LUMA, absPartIdx, fullDepth);
		setTUDepthSubParts(cu, tuDepth, absPartIdx, fullDepth);

		uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
		coeff_t* coeffY = search->m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;

		// store original entropy coding status
		if (search->m_bEnableRDOQ)
			estBit(&(search->m_entropyCoder), &(search->m_entropyCoder.m_estBitsSbac), log2TrSize, TRUE);

		primitives.cu[sizeIdx].calcresidual(fenc, pred, residual, stride, width);//计算残差

		uint32_t numSig = Quant_transformNxN(&search->m_quant, cu, fenc, stride, residual, stride, coeffY, log2TrSize, TEXT_LUMA, absPartIdx, FALSE);//numsig为非零系数个数
		if (numSig)
		{
			Quant_invtransformNxN(&search->m_quant, residual, stride, coeffY, log2TrSize, TEXT_LUMA, TRUE, FALSE, numSig);//重建残差
			primitives.cu[sizeIdx].add_ps(reconQt, reconQtStride, pred, residual, stride, stride, (int)pow(2.0, double(log2TrSize)), (int)pow(2.0, double(log2TrSize)));//残差加上预测值，为重建像素值，pixel_add_ps_c<W, H>
		}
		else
			// no coded residual, recon = pred
			primitives.cu[sizeIdx].copy_pp(reconQt, reconQtStride, pred, stride, (int)pow(2.0, double(log2TrSize)), (int)pow(2.0, double(log2TrSize)));
		bCBF = !!numSig << tuDepth;

		setCbfSubParts(cu, bCBF, TEXT_LUMA, absPartIdx, fullDepth);
		//fullCost.distortion = primitives.cu[sizeIdx].sse_pp(reconQt, reconQtStride, fenc, stride, width, width);//sse计算，复杂度最高的失真计算
		fullCost.distortion = sse_pixel(reconQt, reconQtStride, fenc, stride, width, width);
		entropy_resetBits(&(search->m_entropyCoder));
		if (!absPartIdx)
		{
			if (!Slice_isIntra(cu->m_slice))
			{
				if (cu->m_slice->m_pps->bTransquantBypassEnabled)
					codeCUTransquantBypassFlag(&(search->m_entropyCoder), cu->m_tqBypass[0]);
				codeSkipFlag(&(search->m_entropyCoder), cu, 0);//对跳跃标识cu_skipe_flag进行常规编码
				codePredMode(&(search->m_entropyCoder), cu->m_predMode[0]);//编码CU的模式，帧内或帧间
			}
		}
		if (cu->m_partSize[0] == SIZE_2Nx2N)
		{
			if (!absPartIdx)
				codeIntraDirLumaAng(&(search->m_entropyCoder), cu, 0, FALSE);
		}
		else
		{
			uint32_t qNumParts = cuGeom->numPartitions >> 2;
			if (!tuDepth)
			{
				for (uint32_t qIdx = 0; qIdx < 4; ++qIdx)
					codeIntraDirLumaAng(&(search->m_entropyCoder), cu, qIdx * qNumParts, FALSE);
			}
			else if (!(absPartIdx & (qNumParts - 1)))
				codeIntraDirLumaAng(&(search->m_entropyCoder), cu, absPartIdx, FALSE);
		}
		if (log2TrSize != depthRange[0])
			codeTransformSubdivFlag(&(search->m_entropyCoder), 0, 5 - log2TrSize);

		codeQtCbfLuma(&(search->m_entropyCoder), !!1, tuDepth);

		if (getCbf(cu, absPartIdx, TEXT_LUMA, tuDepth))
			codeCoeffNxN(&(search->m_entropyCoder), cu, coeffY, absPartIdx, log2TrSize, TEXT_LUMA);

		fullCost.bits = entropy_getNumberOfWrittenBits(&(search->m_entropyCoder));

		if (search->m_rdCost.m_psyRd)
		{
			fullCost.energy = psyCost(sizeIdx, fenc, mode->fencYuv->m_size, reconQt, reconQtStride);
			fullCost.rdcost = calcPsyRdCost(&(search->m_rdCost), fullCost.distortion, fullCost.bits, fullCost.energy);
		}
		else
			fullCost.rdcost = calcRdCost(&(search->m_rdCost), fullCost.distortion, fullCost.bits);
	}
	else
		fullCost.rdcost = MAX_INT;//fullCost.rdcost = MAX_INT64;

	if (mightSplit)//将PU一分为四时，计算各种COST
	{
		if (mightNotSplit)
		{
			store(&(search->m_entropyCoder), &(search->m_rqt[fullDepth].rqtTest));  // save state after full TU encode
			load(&(search->m_entropyCoder), &(search->m_rqt[fullDepth].rqtRoot));   // prep state of split encode
		}

		// code split block
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;

		int checkTransformSkip = 0;//search->m_slice->m_pps->bTransformSkipEnabled && (log2TrSize - 1) <= MAX_LOG2_TS_SIZE && !cu->m_tqBypass[0];
		if (search->m_param->bEnableTSkipFast)
			checkTransformSkip &= cu->m_partSize[0] != SIZE_2Nx2N;

		struct Cost splitCost;
		uint32_t cbf = 0;
		for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
		{
			codeIntraLumaQT(search, mode, cuGeom, tuDepth + 1, qPartIdx, bAllowSplit, &splitCost, depthRange);//递归编码

			cbf |= getCbf(cu, qPartIdx, TEXT_LUMA, tuDepth + 1);
		}
		for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
			cu->m_cbf[0][absPartIdx + offs] |= (cbf << tuDepth);

		if (mightNotSplit && log2TrSize != depthRange[0])
		{
			// If we could have coded this TU depth, include cost of subdiv flag
			entropy_resetBits(&(search->m_entropyCoder));
			codeTransformSubdivFlag(&(search->m_entropyCoder), 1, 5 - log2TrSize);
			splitCost.bits += entropy_getNumberOfWrittenBits(&(search->m_entropyCoder));

			if (search->m_rdCost.m_psyRd)
				splitCost.rdcost = calcPsyRdCost(&(search->m_rdCost), splitCost.distortion, splitCost.bits, splitCost.energy);
			else
				splitCost.rdcost = calcRdCost(&(search->m_rdCost), splitCost.distortion, splitCost.bits);
		}

		if (splitCost.rdcost < fullCost.rdcost)
		{
			outCost->rdcost += splitCost.rdcost;
			outCost->distortion += splitCost.distortion;
			outCost->bits += splitCost.bits;
			outCost->energy += splitCost.energy;
			return;
		}
		else
		{
			// recover entropy state of full-size TU encode
			load(&(search->m_entropyCoder), &(search->m_rqt[fullDepth].rqtTest));

			// recover transform index and Cbf values
			setTUDepthSubParts(cu, tuDepth, absPartIdx, fullDepth);
			setCbfSubParts(cu, bCBF, TEXT_LUMA, absPartIdx, fullDepth);
			setTransformSkipSubParts(cu, 0, TEXT_LUMA, absPartIdx, fullDepth);
		}
	}

	// set reconstruction for next intra prediction blocks if full TU prediction won
	pixel*   picReconY = Picyuv_CUgetLumaAddr(search->m_frame->m_reconPic, cu->m_cuAddr, cuGeom->absPartIdx + absPartIdx);
	intptr_t picStride = search->m_frame->m_reconPic->m_stride;
	//primitives.cu[sizeIdx].copy_pp(picReconY, picStride, reconQt, reconQtStride, width, width);
	blockcopy_pp_c(picReconY, picStride, reconQt, reconQtStride, width, width);

	outCost->rdcost += fullCost.rdcost;
	outCost->distortion += fullCost.distortion;
	outCost->bits += fullCost.bits;
	outCost->energy += fullCost.energy;
}

/* returns distortion */
uint32_t codeIntraChromaQt(Search* search, Mode* mode, const CUGeom* cuGeom, uint32_t tuDepth, uint32_t absPartIdx, uint32_t* psyEnergy)
{/*
	CUData* cu = &mode->cu;
	uint32_t log2TrSize = cuGeom->log2CUSize - tuDepth;
	uint32_t outDist;

	if (tuDepth < cu->m_tuDepth[absPartIdx])
	{
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		uint32_t outDist = 0, splitCbfU = 0, splitCbfV = 0;
		for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
		{
			outDist += codeIntraChromaQt(search, mode, cuGeom, tuDepth + 1, qPartIdx, psyEnergy);
			splitCbfU |= getCbf(cu, qPartIdx, TEXT_CHROMA_U, tuDepth + 1);
			splitCbfV |= getCbf(cu, qPartIdx, TEXT_CHROMA_V, tuDepth + 1);
		}
		for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
		{
			cu->m_cbf[1][absPartIdx + offs] |= (splitCbfU << tuDepth);
			cu->m_cbf[2][absPartIdx + offs] |= (splitCbfV << tuDepth);
		}

		return outDist;
	}

	uint32_t log2TrSizeC = log2TrSize - search->predict.m_hChromaShift;
	uint32_t tuDepthC = tuDepth;
	if (log2TrSizeC < 2)
	{
		X265_CHECK(log2TrSize == 2 && search->predict.m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
		if (absPartIdx & 3)
			return 0;
		log2TrSizeC = 2;
		tuDepthC--;
	}

	if (search->m_bEnableRDOQ)
		estBit(&(search->m_entropyCoder), &(search->m_entropyCoder.m_estBitsSbac), log2TrSizeC, FALSE);

	bool checkTransformSkip = 0;//search->m_slice->m_pps->bTransformSkipEnabled && log2TrSizeC <= MAX_LOG2_TS_SIZE && !cu->m_tqBypass[0];
	checkTransformSkip &= !search->m_param->bEnableTSkipFast || (log2TrSize <= MAX_LOG2_TS_SIZE && cu->m_transformSkip[TEXT_LUMA][absPartIdx]);

	ShortYuv* resiYuv = &(search->m_rqt[cuGeom->depth].tmpResiYuv);
	uint32_t qtLayer = log2TrSize - 2;
	uint32_t stride = mode->fencYuv->m_csize;
	const uint32_t sizeIdxC = log2TrSizeC - 2;
	uint32_t width = pow(2, log2TrSizeC);

	uint32_t curPartNum = cuGeom->numPartitions >> tuDepthC * 2;
	const enum SplitType splitType = (search->predict.m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

	TURecurse tuIterator;
	TURecurse_init(&tuIterator, splitType, curPartNum, absPartIdx);
	do
	{
		uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

		struct IntraNeighbors intraNeighbors;
		initIntraNeighbors(cu, absPartIdxC, tuDepthC, FALSE, &intraNeighbors);

		for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
		{
			enum TextType ttype = (enum TextType)chromaId;

			const pixel* fenc = Yuv_getChromaAddr_const(mode->fencYuv, chromaId, absPartIdxC);
			pixel*   pred = Yuv_getChromaAddr(&(mode->predYuv), chromaId, absPartIdxC);
			int16_t* residual = ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC);
			uint32_t coeffOffsetC = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (search->predict.m_hChromaShift + search->predict.m_vChromaShift));
			coeff_t* coeffC = search->m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
			pixel*   reconQt = Yuv_getChromaAddr(&(search->m_rqt[qtLayer].reconQtYuv), chromaId, absPartIdxC);
			uint32_t reconQtStride = search->m_rqt[qtLayer].reconQtYuv.m_csize;
			pixel*   picReconC = Picyuv_CUgetChromaAddr(search->m_frame->m_reconPic, chromaId, cu->m_cuAddr, cuGeom->absPartIdx + absPartIdxC);
			intptr_t picStride = search->m_frame->m_reconPic->m_strideC;

			uint32_t chromaPredMode = cu->m_chromaIntraDir[absPartIdxC];
			if (chromaPredMode == DM_CHROMA_IDX)
				chromaPredMode = cu->m_lumaIntraDir[(search->predict.m_csp == X265_CSP_I444) ? absPartIdxC : 0];
			if (search->predict.m_csp == X265_CSP_I422)
				chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

			// init availability pattern
			initAdiPatternChroma(&(search->predict), cu, cuGeom, absPartIdxC, &intraNeighbors, chromaId);

			// get prediction signal
			predIntraChromaAng(&(search->predict), chromaPredMode, pred, stride, log2TrSizeC);
			setTransformSkipPartRange(cu, 1, ttype, absPartIdxC, tuIterator.absPartIdxStep);
			primitives.cu[sizeIdxC].calcresidual(fenc, pred, residual, stride, width);

			uint32_t numSig = Quant_transformNxN(&search->m_quant, cu, fenc, stride, residual, stride, coeffC, log2TrSizeC, ttype, absPartIdxC, FALSE);
			if (numSig)
			{
				Quant_invtransformNxN(&search->m_quant, residual, stride, coeffC, log2TrSizeC, ttype, TRUE, FALSE, numSig);
				primitives.cu[sizeIdxC].add_ps(reconQt, reconQtStride, pred, residual, stride, stride, pow(2, log2TrSize), pow(2, log2TrSize));
				setCbfPartRange(cu, 1 << tuDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
			}
			else
			{
				// no coded residual, recon = pred
				primitives.cu[sizeIdxC].copy_pp(reconQt, reconQtStride, pred, stride, pow(2, log2TrSize), pow(2, log2TrSize));
				setCbfPartRange(cu, 0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
			}

			uint32_t dist = sse_pixel(reconQt, reconQtStride, fenc, stride, width, width);
			//outDist += scaleChromaDist(&(search->m_rdCost), chromaId, primitives.cu[sizeIdxC].sse_pp(reconQt, reconQtStride, fenc, stride, width, width));
			outDist += scaleChromaDist(&(search->m_rdCost), chromaId, dist);

			if (search->m_rdCost.m_psyRd)
				psyEnergy += psyCost(sizeIdxC, fenc, stride, reconQt, reconQtStride);

			blockcopy_pp_c(picReconC, picStride, reconQt, reconQtStride, width, width);
			//primitives.cu[sizeIdxC].copy_pp(picReconC, picStride, reconQt, reconQtStride, width, width);
		}
	} while (isNextSection(&tuIterator));

	if (splitType == VERTICAL_SPLIT)
	{
		offsetSubTUCBFs(search, cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
		offsetSubTUCBFs(search, cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
	}

	return outDist; */
		return 0;
}

//接近于HM中的xCheckRDCostIntra
void checkIntra(Search* search, Mode* intraMode, CUGeom* cuGeom, PartSize partSize, uint8_t* sharedModes, uint8_t* sharedChromaModes)
{
	CUData* cu = &intraMode->cu;

	setPartSizeSubParts(cu, partSize);
	setPredModeSubParts(cu, MODE_INTRA);
	search->m_quant.m_tqBypass = 0;//m_quant.m_tqBypass = !!cu.m_tqBypass[0];
	
	uint32_t tuDepthRange[2];
	CUData_getIntraTUQtDepthRange(cu, tuDepthRange, 0);
	
	initCosts(intraMode);
	intraMode->distortion += estIntraPredQT(search, intraMode, cuGeom, tuDepthRange, sharedModes);
	intraMode->distortion += estIntraPredChromaQT(search, intraMode, cuGeom, sharedChromaModes);
	
	entropy_resetBits(&(search->m_entropyCoder));
	if (search->m_slice->m_pps->bTransquantBypassEnabled)
		codeCUTransquantBypassFlag(&(search->m_entropyCoder), cu->m_tqBypass[0]);//对语法元素cu_tranaquant_bypass_flag进行常规编码，cu_tranaquant_bypass_flag表示对CU是否进行伸缩、变换和环路滤波过程。

	if (!Slice_isIntra(search->m_slice))
	{
		codeSkipFlag(&(search->m_entropyCoder), cu, 0);//对跳跃标识cu_skipe_flag进行常规编码，cu_skipe_flag表示是否跳过当前CU。
		codePredMode(&(search->m_entropyCoder), cu->m_predMode[0]);//编码CU的模式，帧内或帧间。
	}

	codePartSize(&(search->m_entropyCoder), cu, 0, cuGeom->depth);//编码PU的分割类型，跟当前预测模式有关
	codePredInfo(&(search->m_entropyCoder), cu, 0);
	intraMode->mvBits = entropy_getNumberOfWrittenBits(&(search->m_entropyCoder));

	bool bCodeDQP = search->m_slice->m_pps->bUseDQP;
	//codeCoeff(&(search->m_entropyCoder), cu, 0, bCodeDQP, tuDepthRange);//对TU层的各种语法元素进行熵编码，包括量化后的变换系数的熵编码也在这个部分完成
	store(&(search->m_entropyCoder), &(intraMode->contexts));
	intraMode->totalBits = entropy_getNumberOfWrittenBits(&(search->m_entropyCoder));
	intraMode->coeffBits = intraMode->totalBits - intraMode->mvBits;
	if (search->m_rdCost.m_psyRd)
	{
		const struct Yuv* fencYuv = intraMode->fencYuv;
		intraMode->psyEnergy = psyCost(cuGeom->log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, intraMode->reconYuv.m_buf[0], intraMode->reconYuv.m_size);
	}
	updateModeCost(search, intraMode);
	checkDQP(search, intraMode, cuGeom);
}


void extractIntraResultQT(Search* search, CUData* cu, struct Yuv* reconYuv, uint32_t tuDepth, uint32_t absPartIdx)
{
	uint32_t log2TrSize = cu->m_log2CUSize[0] - tuDepth;

	if (tuDepth == cu->m_tuDepth[absPartIdx])
	{
		uint32_t qtLayer = log2TrSize - 2;

		// copy transform coefficients
		uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
		coeff_t* coeffSrcY = search->m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;//m_rqt[5],coeffRQT[3];
		coeff_t* coeffDestY = cu->m_trCoeff[0] + coeffOffsetY;
		memcpy(coeffDestY, coeffSrcY, sizeof(coeff_t) << (log2TrSize * 2));

		// copy reconstruction
		Yuv_copyPartToPartLuma(&(search->m_rqt[qtLayer].reconQtYuv), reconYuv, absPartIdx, log2TrSize);
	}
	else
	{
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
			extractIntraResultQT(search, cu, reconYuv, tuDepth + 1, absPartIdx);
	}
}

void offsetCBFs(uint8_t subTUCBF[2])
{/*
	uint8_t combinedCBF = subTUCBF[0] | subTUCBF[1];
	subTUCBF[0] = subTUCBF[0] << 1 | combinedCBF;
	subTUCBF[1] = subTUCBF[1] << 1 | combinedCBF;*/
}

/* 4:2:2 post-TU split processing */
void offsetSubTUCBFs(Search* search, CUData* cu, enum TextType ttype, uint32_t tuDepth, uint32_t absPartIdx)
{/*
	uint32_t log2TrSize = cu->m_log2CUSize[0] - tuDepth;

	if (log2TrSize == 2)
	{
		X265_CHECK(search->predict.m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
		++log2TrSize;
	}

	uint32_t tuNumParts = 1 << ((log2TrSize - LOG2_UNIT_SIZE) * 2 - 1);

	// move the CBFs down a level and set the parent CBF
	uint8_t subTUCBF[2];
	subTUCBF[0] = getCbf(cu, absPartIdx, ttype, tuDepth);
	subTUCBF[1] = getCbf(cu, absPartIdx + tuNumParts, ttype, tuDepth);
	offsetCBFs(subTUCBF);

	setCbfPartRange(cu, subTUCBF[0] << tuDepth, ttype, absPartIdx, tuNumParts);
	setCbfPartRange(cu, subTUCBF[1] << tuDepth, ttype, absPartIdx + tuNumParts, tuNumParts);
	*/
}

void extractIntraResultChromaQT(Search* search, CUData* cu, struct Yuv* reconYuv, uint32_t absPartIdx, uint32_t tuDepth)
{/*
	uint32_t tuDepthL = cu->m_tuDepth[absPartIdx];
	uint32_t log2TrSize = cu->m_log2CUSize[0] - tuDepth;
	uint32_t log2TrSizeC = log2TrSize - search->predict.m_hChromaShift;

	if (tuDepthL == tuDepth || log2TrSizeC == 2)
	{
		// copy transform coefficients
		uint32_t numCoeffC = 1 << (log2TrSizeC * 2 + (search->predict.m_csp == X265_CSP_I422));
		uint32_t coeffOffsetC = absPartIdx << (LOG2_UNIT_SIZE * 2 - (search->predict.m_hChromaShift + search->predict.m_vChromaShift));

		uint32_t qtLayer = log2TrSize - 2 - (tuDepthL - tuDepth);
		coeff_t* coeffSrcU = search->m_rqt[qtLayer].coeffRQT[1] + coeffOffsetC;
		coeff_t* coeffSrcV = search->m_rqt[qtLayer].coeffRQT[2] + coeffOffsetC;
		coeff_t* coeffDstU = cu->m_trCoeff[1] + coeffOffsetC;
		coeff_t* coeffDstV = cu->m_trCoeff[2] + coeffOffsetC;
		memcpy(coeffDstU, coeffSrcU, numCoeffC);
		memcpy(coeffDstV, coeffSrcV, numCoeffC);

		// copy reconstruction
		Yuv_copyPartToPartChroma(&(search->m_rqt[qtLayer].reconQtYuv), reconYuv, absPartIdx, log2TrSizeC + search->predict.m_hChromaShift);
	}
	else
	{
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
			extractIntraResultChromaQT(search, cu, reconYuv, absPartIdx, tuDepth + 1);
	}*/
}

uint32_t estIntraPredChromaQT(Search* search, Mode *intraMode, const CUGeom* cuGeom, uint8_t* sharedChromaModes)
{
	CUData* cu = &(intraMode->cu);
	struct Yuv* reconYuv = &(intraMode->reconYuv);

	uint32_t depth = cuGeom->depth;
	uint32_t initTuDepth = cu->m_partSize[0] != SIZE_2Nx2N && search->predict.m_csp == X265_CSP_I444;
	uint32_t log2TrSize = cuGeom->log2CUSize - initTuDepth;
	uint32_t absPartStep = cuGeom->numPartitions;
	uint32_t totalDistortion = 0;

	int size = partitionFromLog2Size(log2TrSize);

	TURecurse tuIterator;
	TURecurse_init(&tuIterator, (initTuDepth == 0) ? DONT_SPLIT : QUAD_SPLIT, absPartStep, 0);

	do
	{
		uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

		uint32_t bestMode = 0;
		uint32_t bestDist = 0;
		uint64_t bestCost = MAX_INT;

		// init mode list
		uint32_t minMode = 0;
		uint32_t maxMode = NUM_CHROMA_MODE;
		uint32_t modeList[NUM_CHROMA_MODE];

		if (sharedChromaModes && !initTuDepth)
		{
			for (uint32_t l = 0; l < NUM_CHROMA_MODE; l++)
				modeList[l] = sharedChromaModes[0];
			maxMode = 1;
		}
		else
			CUData_getAllowedChromaDir(cu, absPartIdxC, modeList);//获得可用的色度

		// check chroma modes
		for (uint32_t mode = minMode; mode < 1; mode++)
		{
			// restore context models
			load(&(search->m_entropyCoder), &(search->m_rqt[depth].cur));

			setChromIntraDirSubParts(cu, modeList[mode], absPartIdxC, depth + initTuDepth);
			uint32_t psyEnergy = 0;
			uint32_t numsig = 0;
			uint32_t dist = codeIntraChromaQt(search, intraMode, cuGeom, initTuDepth, absPartIdxC, &psyEnergy);

			entropy_resetBits(&(search->m_entropyCoder));
			// chroma prediction mode
			if (cu->m_partSize[0] == SIZE_2Nx2N || search->predict.m_csp != X265_CSP_I444)
			{
				if (!absPartIdxC)
					codeIntraDirChroma(&(search->m_entropyCoder), cu, absPartIdxC, modeList);
			}
			else
			{
				uint32_t qNumParts = cuGeom->numPartitions >> 2;
				if (!(absPartIdxC & (qNumParts - 1)))
					codeIntraDirChroma(&(search->m_entropyCoder), cu, absPartIdxC, modeList);
			}

			codeSubdivCbfQTChroma(search, cu, initTuDepth, absPartIdxC);

			uint32_t bits = entropy_getNumberOfWrittenBits(&(search->m_entropyCoder));
			uint64_t cost = search->m_rdCost.m_psyRd ? calcPsyRdCost(&(search->m_rdCost), dist, bits, psyEnergy) : calcRdCost(&(search->m_rdCost), dist, bits);

			if (cost < bestCost)
			{
				bestCost = cost;
				bestDist = dist;
				bestMode = modeList[mode];
				extractIntraResultChromaQT(search, cu, reconYuv, absPartIdxC, initTuDepth);
				memcpy(search->m_qtTempCbf[1], cu->m_cbf[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
				memcpy(search->m_qtTempCbf[2], cu->m_cbf[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
				memcpy(search->m_qtTempTransformSkipFlag[1], cu->m_transformSkip[1] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
				memcpy(search->m_qtTempTransformSkipFlag[2], cu->m_transformSkip[2] + absPartIdxC, tuIterator.absPartIdxStep * sizeof(uint8_t));
			}
		}

		if (!isLastSection(&tuIterator))
		{
			uint32_t zorder = cuGeom->absPartIdx + absPartIdxC;
			uint32_t dststride = search->m_frame->m_reconPic->m_strideC;
			pixel* src;
			pixel* dst;

			dst = Picyuv_CUgetCbAddr(search->m_frame->m_reconPic, cu->m_cuAddr, zorder);
			src = Yuv_getCbAddr(reconYuv, absPartIdxC);
			primitives.chroma[search->predict.m_csp].cu[size].copy_pp(dst, dststride, src, reconYuv->m_csize, reconYuv->m_csize, reconYuv->m_csize);

			dst = Picyuv_CUgetCrAddr(search->m_frame->m_reconPic, cu->m_cuAddr, zorder);
			src = Yuv_getCbAddr(reconYuv, absPartIdxC);
			primitives.chroma[search->predict.m_csp].cu[size].copy_pp(dst, dststride, src, reconYuv->m_csize, reconYuv->m_csize, reconYuv->m_csize);
		}

		memcpy(cu->m_cbf[1] + absPartIdxC, search->m_qtTempCbf[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
		memcpy(cu->m_cbf[2] + absPartIdxC, search->m_qtTempCbf[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
		memcpy(cu->m_transformSkip[1] + absPartIdxC, search->m_qtTempTransformSkipFlag[1], tuIterator.absPartIdxStep * sizeof(uint8_t));
		memcpy(cu->m_transformSkip[2] + absPartIdxC, search->m_qtTempTransformSkipFlag[2], tuIterator.absPartIdxStep * sizeof(uint8_t));
		setChromIntraDirSubParts(cu, bestMode, absPartIdxC, depth + initTuDepth);
		totalDistortion += bestDist;
	} while (isNextSection(&tuIterator));

	if (initTuDepth != 0)
	{
		uint32_t combCbfU = 0;
		uint32_t combCbfV = 0;
		uint32_t qNumParts = tuIterator.absPartIdxStep;
		for (uint32_t qIdx = 0, qPartIdx = 0; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
		{
			combCbfU |= getCbf(cu, qPartIdx, TEXT_CHROMA_U, 1);
			combCbfV |= getCbf(cu, qPartIdx, TEXT_CHROMA_V, 1);
		}

		for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
		{
			cu->m_cbf[1][offs] |= combCbfU;
			cu->m_cbf[2][offs] |= combCbfV;
		}
	}

	// TODO: remove this
	load(&(search->m_entropyCoder), &(search->m_rqt[depth].cur));
	return totalDistortion; 
}

uint32_t estIntraPredQT(Search* search, Mode* intraMode, CUGeom* cuGeom, uint32_t depthRange[2], uint8_t* sharedModes)
{
	CUData* cu = &intraMode->cu;
	struct Yuv* reconYuv = &intraMode->reconYuv;
	struct Yuv* predYuv = &intraMode->predYuv;
	struct Yuv* fencYuv = intraMode->fencYuv;//fenc指编码帧
	int mode;
	

	uint32_t depth = cuGeom->depth;
	uint32_t initTuDepth = cu->m_partSize[0] != SIZE_2Nx2N;
	uint32_t numPU = 1 << (2 * initTuDepth);
	uint32_t log2TrSize = cuGeom->log2CUSize - initTuDepth;
	uint32_t tuSize = 1 << log2TrSize;
	uint32_t qNumParts = cuGeom->numPartitions >> 2;//puIdx（CU中每个PU第一个4*4块的地址）的地址偏移量，当cu为64*64时，qNumParts = 64 = 256>>2
	uint32_t sizeIdx = log2TrSize - 2;
	uint32_t width = (uint32_t)pow(2.0, double(sizeIdx + 2));
	uint32_t absPartIdx = 0;
	uint32_t totalDistortion = 0;

	int checkTransformSkip = 0;//search->m_slice->m_pps->bTransformSkipEnabled && !cu->m_tqBypass[0] && cu->m_partSize[0] != SIZE_2Nx2N;
	
	// loop over partitions
	for (uint32_t puIdx = 0; puIdx < numPU; puIdx++, absPartIdx += qNumParts)
	{
		uint32_t bmode = 0;

		if (sharedModes)//四个子TU共享一个预测模式？
			bmode = sharedModes[puIdx];
		else
		{
			uint64_t candCostList[MAX_RD_INTRA_MODES];
			uint32_t rdModeList[MAX_RD_INTRA_MODES];
			uint64_t bcost;
			int maxCandCount = 2 + search->m_param->rdLevel + ((depth + initTuDepth) >> 1);
			
			{
				//ProfileCUScope(intraMode->cu, intraAnalysisElapsedTime, countIntraAnalysis);
				
				// Reference sample smoothing
				struct IntraNeighbors intraNeighbors;
				init_intraNeighbors(&intraNeighbors);
				initIntraNeighbors(cu, absPartIdx, initTuDepth, TRUE, &intraNeighbors);
				initAdiPattern(&(search->predict), cu, cuGeom, absPartIdx, &intraNeighbors, ALL_IDX);
				
				// determine set of modes to be tested (using prediction signal only)
				const pixel* fenc = Yuv_getLumaAddr_const(fencYuv, absPartIdx);
				uint32_t stride = predYuv->m_size;

				int scaleTuSize = tuSize;
				int scaleStride = stride;
				int costShift = 0;
				
				if (tuSize > 32)
				{
					// origin is 64x64, we scale to 32x32 and setup required parameters
					primitives.scale2D_64to32(search->m_fencScaled, fenc, stride);
					fenc = search->m_fencScaled;

					pixel nScale[129];
					search->predict.intraNeighbourBuf[1][0] = search->predict.intraNeighbourBuf[0][0]; // Unfiltered/filtered neighbours of the current partition.
					primitives.scale1D_128to64(nScale + 1, search->predict.intraNeighbourBuf[0] + 1);

					memcpy(&(search->predict.intraNeighbourBuf[0][1]), &nScale[1], 2 * 64 * sizeof(pixel));
					memcpy(&(search->predict.intraNeighbourBuf[1][1]), &nScale[1], 2 * 64 * sizeof(pixel));

					scaleTuSize = 32;
					scaleStride = 32;
					costShift = 2;
					sizeIdx = 5 - 2; // log2(scaleTuSize) - 2
				}
				
				loadIntraDirModeLuma(&(search->m_entropyCoder), &(search->m_rqt[depth].cur));
				
				// there are three cost tiers for intra modes:
				//  pred[0]          - mode probable, least cost
				//  pred[1], pred[2] - less probable, slightly more cost
				//  non-mpm modes    - all cost the same (rbits) //
				uint64_t mpms;
				uint32_t mpmModes[3];
				uint32_t rbits = getIntraRemModeBits(search, cu, absPartIdx, mpmModes, &mpms);

				pixelcmp_t sa8d = primitives.cu[sizeIdx].sa8d;
				uint64_t modeCosts[35];
				
				// DC
				primitives.cu[sizeIdx].intra_pred[DC_IDX](search->m_intraPred, scaleStride, search->predict.intraNeighbourBuf[0], 0, (scaleTuSize <= 16), width);
				uint32_t bits = (mpms & ((uint64_t)1 << DC_IDX)) ? bitsIntraModeMPM(&(search->m_entropyCoder), mpmModes, DC_IDX) : rbits;
				uint32_t sad = sa8d(fenc, scaleStride, search->m_intraPred, scaleStride, width, width) << costShift;
				modeCosts[DC_IDX] = bcost = calcRdSADCost(&(search->m_rdCost), sad, bits);
				
				// PLANAR
				pixel* planar = search->predict.intraNeighbourBuf[0];
				if (tuSize >= 8 && tuSize <= 32)
					planar = search->predict.intraNeighbourBuf[1];
				
				primitives.cu[sizeIdx].intra_pred[PLANAR_IDX](search->m_intraPred, scaleStride, planar, 0, 0, log2TrSize);
				bits = (mpms & ((uint64_t)1 << PLANAR_IDX)) ? bitsIntraModeMPM(&(search->m_entropyCoder), mpmModes, PLANAR_IDX) : rbits;
				sad = sa8d(fenc, scaleStride, search->m_intraPred, scaleStride, width, width) << costShift;
				modeCosts[PLANAR_IDX] = calcRdSADCost(&(search->m_rdCost), sad, bits);
				COPY1_IF_LT(bcost, modeCosts[PLANAR_IDX]);
				
				// angular predictions
				if (primitives.cu[sizeIdx].intra_pred_allangs)
				{
					primitives.cu[sizeIdx].transpose(search->m_fencTransposed, fenc, scaleStride, width);
					primitives.cu[sizeIdx].intra_pred_allangs(search->m_intraPredAngs, search->predict.intraNeighbourBuf[0], search->predict.intraNeighbourBuf[1], (scaleTuSize <= 16), sizeIdx + 2);
					for (int mode = 2; mode < 35; mode++)
					{
						bits = (mpms & ((uint64_t)1 << mode)) ? bitsIntraModeMPM(&(search->m_entropyCoder), mpmModes, mode) : rbits;
						if (mode < 18)
							sad = sa8d(search->m_fencTransposed, scaleTuSize, &(search->m_intraPredAngs[(mode - 2) * (scaleTuSize * scaleTuSize)]), scaleTuSize, width, width) << costShift;
						else
							sad = sa8d(fenc, scaleStride, &(search->m_intraPredAngs[(mode - 2) * (scaleTuSize * scaleTuSize)]), scaleTuSize, width, width) << costShift;
						modeCosts[mode] = calcRdSADCost(&(search->m_rdCost), sad, bits);
						COPY1_IF_LT(bcost, modeCosts[mode]);
					}
				}
				else
				{
					for (mode = 2; mode < 35; mode++)
					{
						bits = (mpms & ((uint64_t)1 << mode)) ? bitsIntraModeMPM(&(search->m_entropyCoder), mpmModes, mode) : rbits;
						int filter = !!(g_intraFilterFlags[mode] & scaleTuSize);
						primitives.cu[sizeIdx].intra_pred[mode](search->m_intraPred, scaleTuSize, search->predict.intraNeighbourBuf[filter], mode, scaleTuSize <= 16, width);
						sad = sa8d(fenc, scaleStride, search->m_intraPred, scaleTuSize, width, width) << costShift;
						modeCosts[mode] = calcRdSADCost(&(search->m_rdCost), sad, bits);
						COPY1_IF_LT(bcost, modeCosts[mode]);
					}
				}
				
				// Find the top maxCandCount candidate modes with cost within 25% of best
				// or among the most probable modes. maxCandCount is derived from the
				// rdLevel and depth. In general we want to try more modes at slower RD
				// levels and at higher depths //
				for (int i = 0; i < maxCandCount; i++)
					candCostList[i] = MAX_INT;

				uint64_t paddedBcost = bcost + (bcost >> 3); // 1.12%
				for (mode = 0; mode < 35; mode++)
					if (modeCosts[mode] < paddedBcost || (mpms & ((uint64_t)1 << mode)))//默认5个最小的
						updateCandList(mode, modeCosts[mode], maxCandCount, rdModeList, candCostList);
				
			}
			
			// measure best candidates using simple RDO (no TU splits) //
			//遍历maxCandCount种候选模式，选出最佳预测模式。进行完全的、复杂度高的率失真计算，包括对失真和bits的计算。递归熵编码Intra CU，包括变换、量化等最后最佳模式为bmode。
			bcost = MAX_INT;
			//for (int i = 0; i < maxCandCount; i++)
			for (int i = 0; i < 1; i++)
			{
				if (candCostList[i] == MAX_INT)
					break;

				//ProfileCUScope(intraMode.cu, intraRDOElapsedTime[cuGeom.depth], countIntraRDO[cuGeom.depth]);

				load(&(search->m_entropyCoder), &(search->m_rqt[depth].cur));
				setLumaIntraDirSubParts(cu, rdModeList[i], absPartIdx, depth + initTuDepth);//将亮度分量候选预测模式赋给待处理PU

				//struct Cost icosts;
				struct Cost icosts;
				init_Cost(&icosts);
				//if (checkTransformSkip)
				// codeIntraLumaTSkip(intraMode, cuGeom, initTuDepth, absPartIdx, icosts);
				//else
				codeIntraLumaQT(search, intraMode, cuGeom, initTuDepth, absPartIdx, FALSE, &icosts, depthRange);//递归地进行变换、量化、熵编码操作,从几种候选模式中选出最佳预测模式
				COPY2_IF_LT(bcost, icosts.rdcost, bmode, rdModeList[i]);
			}
		}
		
		//ProfileCUScope(intraMode.cu, intraRDOElapsedTime[cuGeom.depth], countIntraRDO[cuGeom.depth]);
		
		struct Cost icosts;
		init_Cost(&icosts);
		// remeasure best mode, allowing TU splits //
		setLumaIntraDirSubParts(cu, bmode, absPartIdx, depth + initTuDepth);
		load(&(search->m_entropyCoder), &(search->m_rqt[depth].cur));

		//if (checkTransformSkip)
		//	codeIntraLumaTSkip(intraMode, cuGeom, initTuDepth, absPartIdx, icosts);
		//else
		codeIntraLumaQT(search, intraMode, cuGeom, initTuDepth, absPartIdx, TRUE, &icosts, depthRange);

		totalDistortion += icosts.distortion;

		extractIntraResultQT(search, cu, reconYuv, initTuDepth, absPartIdx);

		// set reconstruction for next intra prediction blocks
		if (puIdx != numPU - 1)
		{
			// This has important implications for parallelism and RDO.  It is writing intermediate results into the
			// output recon picture, so it cannot proceed in parallel with anything else when doing INTRA_NXN. Also
			// it is not updating m_rdContexts[depth].cur for the later PUs which I suspect is slightly wrong. I think
			// that the contexts should be tracked through each PU
			pixel*   dst = Picyuv_CUgetLumaAddr(search->m_frame->m_reconPic, cu->m_cuAddr, cuGeom->absPartIdx + absPartIdx);
			uint32_t dststride = search->m_frame->m_reconPic->m_stride;
			pixel*   src = Yuv_getLumaAddr_const(reconYuv, absPartIdx);
			uint32_t srcstride = reconYuv->m_size;
			primitives.cu[log2TrSize - 2].copy_pp(dst, dststride, src, srcstride, (int)pow(2.0, double(log2TrSize)), (int)pow(2.0, double(log2TrSize)));
		}
	}

	if (numPU > 1)
	{
		uint32_t combCbfY = 0;
		for (uint32_t qIdx = 0, qPartIdx = 0; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
			combCbfY |= getCbf(cu, qPartIdx, TEXT_LUMA, 1);

		for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
			cu->m_cbf[0][offs] |= combCbfY;
	}

	// TODO: remove this
	load(&(search->m_entropyCoder), &(search->m_rqt[depth].cur));

	return totalDistortion; 
}


/* returns the number of bits required to signal a non-most-probable mode.
* on return mpms contains bitmap of most probable modes */
unsigned int getIntraRemModeBits(const Search* search, CUData* cu, uint32_t absPartIdx, uint32_t mpmModes[3], uint64_t* mpms)
{
	uint32_t temp = 0;
	CUData_getIntraDirLumaPredictor(cu, absPartIdx, mpmModes);

	*mpms = 0;
	for (int i = 0; i < 3; ++i)
		*mpms |= ((uint64_t)1 << mpmModes[i]);


	return bitsIntraModeNonMPM(&(search->m_entropyCoder));
	
		return 0;
}

void updateCandList(uint32_t mode, uint64_t cost, int maxCandCount, uint32_t* candModeList, uint64_t* candCostList)
{
	uint32_t maxIndex = 0;
	uint64_t maxValue = 0;

	for (int i = 0; i < maxCandCount; i++)
	{
		if (maxValue < candCostList[i])
		{
			maxValue = candCostList[i];
			maxIndex = i;
		}
	}

	if (cost < maxValue)
	{
		candCostList[maxIndex] = cost;
		candModeList[maxIndex] = mode;
	}
}

void checkDQP(Search* search, Mode* mode, const CUGeom* cuGeom)
{
	CUData* cu = &mode->cu;
	if (cu->m_slice->m_pps->bUseDQP && cuGeom->depth <= cu->m_slice->m_pps->maxCuDQPDepth)
	{
		if (getQtRootCbf(cu, 0))
		{
			if (search->m_param->rdLevel >= 3)
			{
				updateModeCost(search, mode);
			}
			else if (search->m_param->rdLevel <= 1)
			{
				mode->sa8dBits++;
				mode->sa8dCost = calcRdSADCost(&(search->m_rdCost), mode->distortion, mode->sa8dBits);
			}
			else
			{
				mode->mvBits++;
				mode->totalBits++;
				updateModeCost(search, mode);
			}
		}
		else
			setQPSubParts(cu, getRefQP(cu, 0), 0, cuGeom->depth);
	}
}

void updateModeCost(const Search* sch, Mode* m)
{
	m->rdCost = sch->m_rdCost.m_psyRd ? calcPsyRdCost(&(sch->m_rdCost), m->distortion, m->totalBits, m->psyEnergy) : calcRdCost(&(sch->m_rdCost), m->distortion, m->totalBits);
}

int getTUBits(int idx, int numIdx)
{
	return idx + (idx < numIdx - 1);
}

/* Note: this function overwrites the RD cost variables of interMode, but leaves the sa8d cost unharmed */
void Search_encodeResAndCalcRdSkipCU(Search *search, Predict *predict, Mode* interMode)
{/*
	CUData* cu = &interMode->cu;
	Yuv* reconYuv = &interMode->reconYuv;
	Yuv* fencYuv = interMode->fencYuv;

	uint32_t depth = cu->m_cuDepth[0];

	// No residual coding : SKIP mode

	setPredModeSubParts(cu, MODE_SKIP);
	clearCbf(cu);
	setTUDepthSubParts(cu, 0, 0, depth);

	Yuv_copyFromYuv(reconYuv, interMode->predYuv);

	// Luma
	int part = partitionFromLog2Size(cu->m_log2CUSize[0]);
	interMode->distortion = primitives.cu[part].sse_pp(fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size, pow(2, 2 + part), pow(2, 2 + part));
	// Chroma
	interMode->distortion += scaleChromaDist(&search->m_rdCost, 1, primitives.chroma[predict->m_csp].cu[part].sse_pp(fencYuv->m_buf[1], fencYuv->m_csize, reconYuv->m_buf[1], reconYuv->m_csize, reconYuv->m_csize, reconYuv->m_csize));
	interMode->distortion += scaleChromaDist(&search->m_rdCost, 2, primitives.chroma[predict->m_csp].cu[part].sse_pp(fencYuv->m_buf[2], fencYuv->m_csize, reconYuv->m_buf[2], reconYuv->m_csize, reconYuv->m_csize, reconYuv->m_csize));

	load(&search->m_entropyCoder, &search->m_rqt[depth].cur);
	entropy_resetBits(&search->m_entropyCoder);
	if (search->m_slice->m_pps->bTransquantBypassEnabled)
		codeCUTransquantBypassFlag(&search->m_entropyCoder, cu->m_tqBypass[0]);
	codeSkipFlag(&search->m_entropyCoder, cu, 0);
	codeMergeIndex(&search->m_entropyCoder, cu, 0);

	interMode->mvBits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);
	interMode->coeffBits = 0;
	interMode->totalBits = interMode->mvBits;
	if (search->m_rdCost.m_psyRd)
		interMode->psyEnergy = psyCost(part, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
	//  interMode.psyEnergy = m_rdCost.psyCost(part, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

	updateModeCost(search, interMode);
	store(&search->m_entropyCoder, &interMode->contexts);
	*/
}

/* encode residual and calculate rate-distortion for a CU block.
* Note: this function overwrites the RD cost variables of interMode, but leaves the sa8d cost unharmed */
void Search_encodeResAndCalcRdInterCU(Search *search, Mode* interMode, CUGeom* cuGeom)
{/*
	//ProfileCUScope(interMode.cu, interRDOElapsedTime[cuGeom.depth], countInterRDO[cuGeom.depth]);

	CUData* cu = &interMode->cu;
	Yuv* reconYuv = &interMode->reconYuv;//重建图像
	Yuv* predYuv = &interMode->predYuv;//预测图像
	uint32_t depth = cuGeom->depth;
	ShortYuv* resiYuv = &search->m_rqt[depth].tmpResiYuv;//残差
	Yuv* fencYuv = interMode->fencYuv;//原始像素

	X265_CHECK(!isIntra_cudata(cu, 0), "intra CU not expected\n");

	uint32_t log2CUSize = cuGeom->log2CUSize;
	int sizeIdx = log2CUSize - 2;

	ShortYuv_subtract(resiYuv, fencYuv, predYuv, log2CUSize);//resiYuv表示残差，fencYuv指原始像素，predYuv指预测像素

	uint32_t tuDepthRange[2];
	CUData_getInterTUQtDepthRange(cu, tuDepthRange, 0);

	load(&search->m_entropyCoder, &search->m_rqt[depth].cur);

	struct Cost costs;
	costs.bits = 0; costs.distortion = 0; costs.energy = 0; costs.rdcost = 0;//初始化costs
	Search_estimateResidualQT(search, &search->predict, interMode, cuGeom, 0, 0, resiYuv, &costs, tuDepthRange);//对残差紧张变换量化，熵编码，反量化反变换

	uint32_t tqBypass = cu->m_tqBypass[0];
	if (!tqBypass)
	{
		uint32_t cbf0Dist = primitives.cu[sizeIdx].sse_pp(fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size, pow(2, log2CUSize), pow(2, log2CUSize));
		cbf0Dist += scaleChromaDist(&search->m_rdCost, 1, primitives.chroma[search->predict.m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[1], predYuv->m_csize, predYuv->m_buf[1], predYuv->m_csize, predYuv->m_csize, predYuv->m_csize));
		cbf0Dist += scaleChromaDist(&search->m_rdCost, 2, primitives.chroma[search->predict.m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[2], predYuv->m_csize, predYuv->m_buf[2], predYuv->m_csize, predYuv->m_csize, predYuv->m_csize));

		// Consider the RD cost of not signaling any residual //
		load(&search->m_entropyCoder, &search->m_rqt[depth].cur);
		entropy_resetBits(&search->m_entropyCoder);
		codeQtRootCbfZero(&search->m_entropyCoder);
		uint32_t cbf0Bits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

		uint64_t cbf0Cost;
		uint32_t cbf0Energy;
		if (search->m_rdCost.m_psyRd)
		{
			cbf0Energy = psyCost(log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size);
			cbf0Cost = calcPsyRdCost(&search->m_rdCost, cbf0Dist, cbf0Bits, cbf0Energy);
		}
		else
			cbf0Cost = calcRdCost(&search->m_rdCost, cbf0Dist, cbf0Bits);

		if (cbf0Cost < costs.rdcost)
		{
			clearCbf(cu);
			setTUDepthSubParts(cu, 0, 0, depth);
		}
	}

	if (getQtRootCbf(cu, 0))
		Search_saveResidualQTData(search, cu, resiYuv, 0, 0);

	// calculate signal bits for inter/merge/skip coded CU //
	load(&search->m_entropyCoder, &search->m_rqt[depth].cur);

	entropy_resetBits(&search->m_entropyCoder);
	if (search->m_slice->m_pps->bTransquantBypassEnabled)
		codeCUTransquantBypassFlag(&search->m_entropyCoder, tqBypass);

	uint32_t coeffBits, bits;
	if (cu->m_mergeFlag[0] && cu->m_partSize[0] == SIZE_2Nx2N && !getQtRootCbf(cu, 0))
	{
		setPredModeSubParts(cu, MODE_SKIP);

		// Merge/Skip //
		codeSkipFlag(&search->m_entropyCoder, cu, 0);
		codeMergeIndex(&search->m_entropyCoder, cu, 0);
		coeffBits = 0;
		bits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);
	}
	else
	{
		codeSkipFlag(&search->m_entropyCoder, cu, 0);
		codePredMode(&search->m_entropyCoder, cu->m_predMode[0]);
		codePartSize(&search->m_entropyCoder, cu, 0, cuGeom->depth);
		codePredInfo(&search->m_entropyCoder, cu, 0);
		uint32_t mvBits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

		bool bCodeDQP = search->m_slice->m_pps->bUseDQP;
		codeCoeff(&search->m_entropyCoder, cu, 0, bCodeDQP, tuDepthRange);
		bits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

		coeffBits = bits - mvBits;
	}

	store(&search->m_entropyCoder, &interMode->contexts);

	if (getQtRootCbf(cu, 0))
		Yuv_addClip(reconYuv, predYuv, resiYuv, log2CUSize);
	else
		Yuv_copyFromYuv(reconYuv, *predYuv);

	// update with clipped distortion and cost (qp estimation loop uses unclipped values)
	uint32_t bestDist = primitives.cu[sizeIdx].sse_pp(fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size, pow(2, log2CUSize), pow(2, log2CUSize));
	bestDist += scaleChromaDist(&search->m_rdCost, 1, primitives.chroma[search->predict.m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[1], fencYuv->m_csize, reconYuv->m_buf[1], reconYuv->m_csize, reconYuv->m_csize, reconYuv->m_csize));
	bestDist += scaleChromaDist(&search->m_rdCost, 2, primitives.chroma[search->predict.m_csp].cu[sizeIdx].sse_pp(fencYuv->m_buf[2], fencYuv->m_csize, reconYuv->m_buf[2], reconYuv->m_csize, reconYuv->m_csize, reconYuv->m_csize));
	if (search->m_rdCost.m_psyRd)
		interMode->psyEnergy = psyCost(sizeIdx, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);

	interMode->totalBits = bits;
	interMode->distortion = bestDist;
	interMode->coeffBits = coeffBits;
	interMode->mvBits = bits - coeffBits;
	updateModeCost(search, interMode);
	checkDQP(search, interMode, cuGeom);
	*/
}
uint64_t Search_estimateNullCbfCost(Search *search, uint32_t dist, uint32_t psyEnergy, uint32_t tuDepth, TextType compId)
{/*
	uint32_t nullBits = estimateCbfBits(&search->m_entropyCoder, 0, compId, tuDepth);

	if (search->m_rdCost.m_psyRd)
		return calcPsyRdCost(&search->m_rdCost, dist, nullBits, psyEnergy);
	else
		return calcRdCost(&search->m_rdCost, dist, nullBits);
	*/
		return 0;
}
//对残差紧张变换量化，熵编码，反量化反变换
void Search_estimateResidualQT(Search *search, Predict *predict, Mode* mode, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth, ShortYuv* resiYuv, struct Cost* outCosts, uint32_t depthRange[2])
{/*
	CUData* cu = &mode->cu;
	uint32_t depth = cuGeom->depth + tuDepth;
	uint32_t log2TrSize = cuGeom->log2CUSize - tuDepth;

	bool bCheckSplit = log2TrSize > depthRange[0];
	bool bCheckFull = log2TrSize <= depthRange[1];
	bool bSplitPresentFlag = bCheckSplit && bCheckFull;

	if (cu->m_partSize[0] != SIZE_2Nx2N && !tuDepth && bCheckSplit)
		bCheckFull = FALSE;

	X265_CHECK(bCheckFull || bCheckSplit, "check-full or check-split must be set\n");

	uint32_t log2TrSizeC = log2TrSize - predict->m_hChromaShift;
	bool bCodeChroma = TRUE;
	uint32_t tuDepthC = tuDepth;
	if (log2TrSizeC < 2)
	{
		X265_CHECK(log2TrSize == 2 && predict->m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
		log2TrSizeC = 2;
		tuDepthC--;
		bCodeChroma = !(absPartIdx & 3);
	}

	// code full block
	struct Cost fullCost;
	fullCost.bits = 0; fullCost.distortion = 0; fullCost.energy = 0; fullCost.rdcost = 0;
	fullCost.rdcost = MAX_INT64;
	*/
	uint8_t  cbfFlag[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	uint32_t numSig[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	uint32_t singleBits[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	uint32_t singleDist[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	uint32_t singlePsyEnergy[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	uint32_t bestTransformMode[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	uint64_t minCost[MAX_NUM_COMPONENT][2 /*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = { { MAX_INT64, MAX_INT64 }, { MAX_INT64, MAX_INT64 }, { MAX_INT64, MAX_INT64 } };
	/*
	store(&search->m_entropyCoder, &search->m_rqt[depth].rqtRoot);

	uint32_t trSize = 1 << log2TrSize;
	const bool splitIntoSubTUs = (predict->m_csp == X265_CSP_I422);
	uint32_t absPartIdxStep = cuGeom->numPartitions >> tuDepthC * 2;
	Yuv* fencYuv = mode->fencYuv;

	// code full block
	if (bCheckFull)
	{
		uint32_t trSizeC = 1 << log2TrSizeC;
		int partSize = partitionFromLog2Size(log2TrSize);
		int partSizeC = partitionFromLog2Size(log2TrSizeC);
		const uint32_t qtLayer = log2TrSize - 2;
		uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
		coeff_t* coeffCurY = search->m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;

		bool checkTransformSkip = search->m_slice->m_pps->bTransformSkipEnabled && !cu->m_tqBypass[0];
		bool checkTransformSkipY = checkTransformSkip && log2TrSize <= MAX_LOG2_TS_SIZE;
		bool checkTransformSkipC = checkTransformSkip && log2TrSizeC <= MAX_LOG2_TS_SIZE;

		setTUDepthSubParts(cu, tuDepth, absPartIdx, depth);
		setTransformSkipSubParts(cu, 0, TEXT_LUMA, absPartIdx, depth);

		if (search->m_bEnableRDOQ)
			estBit(&search->m_entropyCoder, &search->m_entropyCoder.m_estBitsSbac, log2TrSize, TRUE);

		pixel* fenc = Yuv_getLumaAddr(fencYuv, absPartIdx);
		int16_t* resi = ShortYuv_getLumaAddr(resiYuv, absPartIdx);

		numSig[TEXT_LUMA][0] = Quant_transformNxN(&search->m_quant, cu, fenc, fencYuv->m_size, resi, resiYuv->m_size, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, FALSE);
		cbfFlag[TEXT_LUMA][0] = !!numSig[TEXT_LUMA][0];

		entropy_resetBits(&search->m_entropyCoder);
		if (bSplitPresentFlag && log2TrSize > depthRange[0])
			codeTransformSubdivFlag(&search->m_entropyCoder, 0, 5 - log2TrSize);
		fullCost.bits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

		// Coding luma cbf flag has been removed from here. The context for cbf flag is different for each depth.
		// So it is valid if we encode coefficients and then cbfs at least for analysis.
		if (cbfFlag[TEXT_LUMA][0])
			codeCoeffNxN(&search->m_entropyCoder, cu, coeffCurY, absPartIdx, log2TrSize, TEXT_LUMA);

		uint32_t singleBitsPrev = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);
		singleBits[TEXT_LUMA][0] = singleBitsPrev - fullCost.bits;

		X265_CHECK(log2TrSize <= 5, "log2TrSize is too large\n");
		uint32_t distY = primitives.cu[partSize].ssd_s(ShortYuv_getLumaAddr(resiYuv, absPartIdx), resiYuv->m_size, resiYuv->m_size);
		uint32_t psyEnergyY = 0;
		if (search->m_rdCost.m_psyRd)//m_rdCost.//================
			psyEnergyY = psyCost_int16_t(partSize, ShortYuv_getLumaAddr(resiYuv, absPartIdx), resiYuv->m_size, (int16_t*)search->zeroShort, 0);

		int16_t* curResiY = ShortYuv_getLumaAddr(&search->m_rqt[qtLayer].resiQtYuv, absPartIdx);
		uint32_t strideResiY = search->m_rqt[qtLayer].resiQtYuv.m_size;

		if (cbfFlag[TEXT_LUMA][0])
		{
			Quant_invtransformNxN(&search->m_quant, curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, FALSE, FALSE, numSig[TEXT_LUMA][0]); //this is for inter mode only

			// non-zero cost calculation for luma - This is an approximation
			// finally we have to encode correct cbf after comparing with null cost
			const uint32_t nonZeroDistY = primitives.cu[partSize].sse_ss(ShortYuv_getLumaAddr(resiYuv, absPartIdx), resiYuv->m_size, curResiY, strideResiY, pow(2, log2TrSize), pow(2, log2TrSize));
			uint32_t nzCbfBitsY = estimateCbfBits(&search->m_entropyCoder, cbfFlag[TEXT_LUMA][0], TEXT_LUMA, tuDepth);
			uint32_t nonZeroPsyEnergyY = 0; uint64_t singleCostY = 0;
			if (search->m_rdCost.m_psyRd)//================
			{
				nonZeroPsyEnergyY = psyCost_int16_t(partSize, ShortYuv_getLumaAddr(resiYuv, absPartIdx), resiYuv->m_size, curResiY, strideResiY);
				singleCostY = calcPsyRdCost(&search->m_rdCost, nonZeroDistY, nzCbfBitsY + singleBits[TEXT_LUMA][0], nonZeroPsyEnergyY);
			}
			else
				singleCostY = calcRdCost(&search->m_rdCost, nonZeroDistY, nzCbfBitsY + singleBits[TEXT_LUMA][0]);

			if (cu->m_tqBypass[0])
			{
				singleDist[TEXT_LUMA][0] = nonZeroDistY;
				singlePsyEnergy[TEXT_LUMA][0] = nonZeroPsyEnergyY;
			}
			else
			{
				// zero-cost calculation for luma. This is an approximation
				// Initial cost calculation was also an approximation. First resetting the bit counter and then encoding zero cbf.
				// Now encoding the zero cbf without writing into bitstream, keeping m_fracBits unchanged. The same is valid for chroma.
				uint64_t nullCostY = Search_estimateNullCbfCost(search, distY, psyEnergyY, tuDepth, TEXT_LUMA);

				if (nullCostY < singleCostY)
				{
					cbfFlag[TEXT_LUMA][0] = 0;
					singleBits[TEXT_LUMA][0] = 0;
					primitives.cu[partSize].blockfill_s(curResiY, strideResiY, 0, pow(2, log2TrSize));
#if CHECKED_BUILD || _DEBUG
					uint32_t numCoeffY = 1 << (log2TrSize << 1);
					memset(coeffCurY, 0, sizeof(coeff_t) * numCoeffY);
#endif
					if (checkTransformSkipY)
						minCost[TEXT_LUMA][0] = nullCostY;
					singleDist[TEXT_LUMA][0] = distY;
					singlePsyEnergy[TEXT_LUMA][0] = psyEnergyY;
				}
				else
				{
					if (checkTransformSkipY)
						minCost[TEXT_LUMA][0] = singleCostY;
					singleDist[TEXT_LUMA][0] = nonZeroDistY;
					singlePsyEnergy[TEXT_LUMA][0] = nonZeroPsyEnergyY;
				}
			}
		}
		else
		{
			if (checkTransformSkipY)
				minCost[TEXT_LUMA][0] = Search_estimateNullCbfCost(search, distY, psyEnergyY, tuDepth, TEXT_LUMA);
			primitives.cu[partSize].blockfill_s(curResiY, strideResiY, 0, pow(2, log2TrSize));
			singleDist[TEXT_LUMA][0] = distY;
			singlePsyEnergy[TEXT_LUMA][0] = psyEnergyY;
		}

		setCbfSubParts(cu, cbfFlag[TEXT_LUMA][0] << tuDepth, TEXT_LUMA, absPartIdx, depth);

		if (bCodeChroma)
		{
			uint32_t coeffOffsetC = coeffOffsetY >> (predict->m_hChromaShift + predict->m_vChromaShift);
			uint32_t strideResiC = search->m_rqt[qtLayer].resiQtYuv.m_csize;
			for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
			{
				uint32_t distC = 0, psyEnergyC = 0;
				coeff_t* coeffCurC = search->m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
				TURecurse tuIterator;
				TURecurse_init(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

				do
				{
					uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
					uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

					setTransformSkipPartRange(cu, 0, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

					if (search->m_bEnableRDOQ && (chromaId != TEXT_CHROMA_V))
						estBit(&search->m_entropyCoder, &search->m_entropyCoder.m_estBitsSbac, log2TrSizeC, FALSE);

					fenc = Yuv_getChromaAddr(fencYuv, chromaId, absPartIdxC);
					resi = ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC);
					numSig[chromaId][tuIterator.section] = Quant_transformNxN(&search->m_quant, cu, fenc, fencYuv->m_csize, resi, resiYuv->m_csize, coeffCurC + subTUOffset, log2TrSizeC, (TextType)chromaId, absPartIdxC, FALSE);
					cbfFlag[chromaId][tuIterator.section] = !!numSig[chromaId][tuIterator.section];

					if (cbfFlag[chromaId][tuIterator.section])
						codeCoeffNxN(&search->m_entropyCoder, cu, coeffCurC + subTUOffset, absPartIdxC, log2TrSizeC, (TextType)chromaId);
					uint32_t newBits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);
					singleBits[chromaId][tuIterator.section] = newBits - singleBitsPrev;
					singleBitsPrev = newBits;

					int16_t* curResiC = ShortYuv_getChromaAddr(&search->m_rqt[qtLayer].resiQtYuv, chromaId, absPartIdxC);
					distC = scaleChromaDist(&search->m_rdCost, chromaId, primitives.cu[log2TrSizeC - 2].ssd_s(ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC), resiYuv->m_csize, resiYuv->m_csize));

					if (cbfFlag[chromaId][tuIterator.section])
					{
						Quant_invtransformNxN(&search->m_quant, curResiC, strideResiC, coeffCurC + subTUOffset,
							log2TrSizeC, (TextType)chromaId, FALSE, FALSE, numSig[chromaId][tuIterator.section]);

						// non-zero cost calculation for luma, same as luma - This is an approximation
						// finally we have to encode correct cbf after comparing with null cost
						uint32_t dist = primitives.cu[partSizeC].sse_ss(ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC), resiYuv->m_csize, curResiC, strideResiC, strideResiC, strideResiC);
						uint32_t nzCbfBitsC = estimateCbfBits(&search->m_entropyCoder, cbfFlag[chromaId][tuIterator.section], (TextType)chromaId, tuDepth);
						uint32_t nonZeroDistC = scaleChromaDist(&search->m_rdCost, chromaId, dist);
						uint32_t nonZeroPsyEnergyC = 0; uint64_t singleCostC = 0;
						if (search->m_rdCost.m_psyRd)
						{
							nonZeroPsyEnergyC = psyCost_int16_t(partSizeC, ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC), resiYuv->m_csize, curResiC, strideResiC);
							singleCostC = calcPsyRdCost(&search->m_rdCost, nonZeroDistC, nzCbfBitsC + singleBits[chromaId][tuIterator.section], nonZeroPsyEnergyC);
						}
						else
							singleCostC = calcRdCost(&search->m_rdCost, nonZeroDistC, nzCbfBitsC + singleBits[chromaId][tuIterator.section]);

						if (cu->m_tqBypass[0])
						{
							singleDist[chromaId][tuIterator.section] = nonZeroDistC;
							singlePsyEnergy[chromaId][tuIterator.section] = nonZeroPsyEnergyC;
						}
						else
						{
							//zero-cost calculation for chroma. This is an approximation
							uint64_t nullCostC = Search_estimateNullCbfCost(search, distC, psyEnergyC, tuDepth, (TextType)chromaId);

							if (nullCostC < singleCostC)
							{
								cbfFlag[chromaId][tuIterator.section] = 0;
								singleBits[chromaId][tuIterator.section] = 0;
								primitives.cu[partSizeC].blockfill_s(curResiC, strideResiC, 0, pow(2, log2TrSize));
#if CHECKED_BUILD || _DEBUG
								uint32_t numCoeffC = 1 << (log2TrSizeC << 1);
								memset(coeffCurC + subTUOffset, 0, sizeof(coeff_t) * numCoeffC);
#endif
								if (checkTransformSkipC)
									minCost[chromaId][tuIterator.section] = nullCostC;
								singleDist[chromaId][tuIterator.section] = distC;
								singlePsyEnergy[chromaId][tuIterator.section] = psyEnergyC;
							}
							else
							{
								if (checkTransformSkipC)
									minCost[chromaId][tuIterator.section] = singleCostC;
								singleDist[chromaId][tuIterator.section] = nonZeroDistC;
								singlePsyEnergy[chromaId][tuIterator.section] = nonZeroPsyEnergyC;
							}
						}
					}
					else
					{
						if (checkTransformSkipC)
							minCost[chromaId][tuIterator.section] = Search_estimateNullCbfCost(search, distC, psyEnergyC, tuDepthC, (TextType)chromaId);
						primitives.cu[partSizeC].blockfill_s(curResiC, strideResiC, 0, pow(2, log2TrSize));
						singleDist[chromaId][tuIterator.section] = distC;
						singlePsyEnergy[chromaId][tuIterator.section] = psyEnergyC;
					}

					setCbfPartRange(cu, cbfFlag[chromaId][tuIterator.section] << tuDepth, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
				} while (isNextSection(&tuIterator));
			}
		}

		if (checkTransformSkipY)
		{
			uint32_t nonZeroDistY = 0;
			uint32_t nonZeroPsyEnergyY = 0;
			uint64_t singleCostY = MAX_INT64;

			load(&search->m_entropyCoder, &search->m_rqt[depth].rqtRoot);

			setTransformSkipSubParts(cu, 1, TEXT_LUMA, absPartIdx, depth);

			if (search->m_bEnableRDOQ)
				estBit(&search->m_entropyCoder, &search->m_entropyCoder.m_estBitsSbac, log2TrSize, TRUE);

			fenc = Yuv_getLumaAddr(fencYuv, absPartIdx);
			resi = ShortYuv_getLumaAddr(resiYuv, absPartIdx);
			uint32_t numSigTSkipY = Quant_transformNxN(&search->m_quant, cu, fenc, fencYuv->m_size, resi, resiYuv->m_size, search->m_tsCoeff, log2TrSize, TEXT_LUMA, absPartIdx, TRUE);

			if (numSigTSkipY)
			{
				entropy_resetBits(&search->m_entropyCoder);
				codeQtCbfLuma(&search->m_entropyCoder, !!numSigTSkipY, tuDepth);
				codeCoeffNxN(&search->m_entropyCoder, cu, search->m_tsCoeff, absPartIdx, log2TrSize, TEXT_LUMA);
				const uint32_t skipSingleBitsY = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

				Quant_invtransformNxN(&search->m_quant, search->m_tsResidual, trSize, search->m_tsCoeff, log2TrSize, TEXT_LUMA, FALSE, TRUE, numSigTSkipY);

				nonZeroDistY = primitives.cu[partSize].sse_ss(ShortYuv_getLumaAddr(resiYuv, absPartIdx), resiYuv->m_size, search->m_tsResidual, trSize, pow(2, log2TrSize), pow(2, log2TrSize));

				if (search->m_rdCost.m_psyRd)
				{
					nonZeroPsyEnergyY = psyCost_int16_t(partSize, ShortYuv_getLumaAddr(resiYuv, absPartIdx), resiYuv->m_size, search->m_tsResidual, trSize);
					singleCostY = calcPsyRdCost(&search->m_rdCost, nonZeroDistY, skipSingleBitsY, nonZeroPsyEnergyY);
				}
				else
					singleCostY = calcRdCost(&search->m_rdCost, nonZeroDistY, skipSingleBitsY);
			}

			if (!numSigTSkipY || minCost[TEXT_LUMA][0] < singleCostY)
				setTransformSkipSubParts(cu, 0, TEXT_LUMA, absPartIdx, depth);
			else
			{
				singleDist[TEXT_LUMA][0] = nonZeroDistY;
				singlePsyEnergy[TEXT_LUMA][0] = nonZeroPsyEnergyY;
				cbfFlag[TEXT_LUMA][0] = !!numSigTSkipY;
				bestTransformMode[TEXT_LUMA][0] = 1;
				uint32_t numCoeffY = 1 << (log2TrSize << 1);
				memcpy(coeffCurY, search->m_tsCoeff, sizeof(coeff_t) * numCoeffY);
				primitives.cu[partSize].copy_ss(curResiY, strideResiY, search->m_tsResidual, trSize, pow(2, log2TrSize), pow(2, log2TrSize));
			}

			setCbfSubParts(cu, cbfFlag[TEXT_LUMA][0] << tuDepth, TEXT_LUMA, absPartIdx, depth);
		}

		if (bCodeChroma && checkTransformSkipC)
		{
			uint32_t nonZeroDistC = 0, nonZeroPsyEnergyC = 0;
			uint64_t singleCostC = MAX_INT64;
			uint32_t strideResiC = search->m_rqt[qtLayer].resiQtYuv.m_csize;
			uint32_t coeffOffsetC = coeffOffsetY >> (predict->m_hChromaShift + predict->m_vChromaShift);

			load(&search->m_entropyCoder, &search->m_rqt[depth].rqtRoot);

			for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
			{
				coeff_t* coeffCurC = search->m_rqt[qtLayer].coeffRQT[chromaId] + coeffOffsetC;
				TURecurse tuIterator;
				TURecurse_init(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);

				do
				{
					uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
					uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

					int16_t* curResiC = ShortYuv_getChromaAddr(&search->m_rqt[qtLayer].resiQtYuv, chromaId, absPartIdxC);

					setTransformSkipPartRange(cu, 1, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);

					if (search->m_bEnableRDOQ && (chromaId != TEXT_CHROMA_V))
						estBit(&search->m_entropyCoder, &search->m_entropyCoder.m_estBitsSbac, log2TrSizeC, FALSE);

					fenc = Yuv_getChromaAddr(fencYuv, chromaId, absPartIdxC);
					resi = ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC);
					uint32_t numSigTSkipC = Quant_transformNxN(&search->m_quant, cu, fenc, fencYuv->m_csize, resi, resiYuv->m_csize, search->m_tsCoeff, log2TrSizeC, (TextType)chromaId, absPartIdxC, TRUE);

					entropy_resetBits(&search->m_entropyCoder);
					singleBits[chromaId][tuIterator.section] = 0;

					if (numSigTSkipC)
					{
						codeQtCbfChroma(&search->m_entropyCoder, !!numSigTSkipC, tuDepth);
						codeCoeffNxN(&search->m_entropyCoder, cu, search->m_tsCoeff, absPartIdxC, log2TrSizeC, (TextType)chromaId);
						singleBits[chromaId][tuIterator.section] = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

						Quant_invtransformNxN(&search->m_quant, search->m_tsResidual, trSizeC, search->m_tsCoeff,
							log2TrSizeC, (TextType)chromaId, FALSE, TRUE, numSigTSkipC);
						uint32_t dist = primitives.cu[partSizeC].sse_ss(ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC), resiYuv->m_csize, search->m_tsResidual, trSizeC, trSizeC, trSizeC);
						nonZeroDistC = scaleChromaDist(&search->m_rdCost, chromaId, dist);
						if (search->m_rdCost.m_psyRd)
						{
							nonZeroPsyEnergyC = psyCost_int16_t(partSizeC, ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC), resiYuv->m_csize, search->m_tsResidual, trSizeC);
							singleCostC = calcPsyRdCost(&search->m_rdCost, nonZeroDistC, singleBits[chromaId][tuIterator.section], nonZeroPsyEnergyC);
						}
						else
							singleCostC = calcRdCost(&search->m_rdCost, nonZeroDistC, singleBits[chromaId][tuIterator.section]);
					}

					if (!numSigTSkipC || minCost[chromaId][tuIterator.section] < singleCostC)
						setTransformSkipPartRange(cu, 0, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
					else
					{
						singleDist[chromaId][tuIterator.section] = nonZeroDistC;
						singlePsyEnergy[chromaId][tuIterator.section] = nonZeroPsyEnergyC;
						cbfFlag[chromaId][tuIterator.section] = !!numSigTSkipC;
						bestTransformMode[chromaId][tuIterator.section] = 1;
						uint32_t numCoeffC = 1 << (log2TrSizeC << 1);
						memcpy(coeffCurC + subTUOffset, search->m_tsCoeff, sizeof(coeff_t) * numCoeffC);
						primitives.cu[partSizeC].copy_ss(curResiC, strideResiC, search->m_tsResidual, trSizeC, pow(2, log2TrSize), pow(2, log2TrSize));
					}

					setCbfPartRange(cu, cbfFlag[chromaId][tuIterator.section] << tuDepth, (TextType)chromaId, absPartIdxC, tuIterator.absPartIdxStep);
				} while (isNextSection(&tuIterator));
			}
		}

		// Here we were encoding cbfs and coefficients, after calculating distortion above.
		// Now I am encoding only cbfs, since I have encoded coefficients above. I have just collected
		// bits required for coefficients and added with number of cbf bits. As I tested the order does not
		// make any difference. But bit confused whether I should load the original context as below.
		load(&search->m_entropyCoder, &search->m_rqt[depth].rqtRoot);
		entropy_resetBits(&search->m_entropyCoder);

		//Encode cbf flags
		if (bCodeChroma)
		{
			if (!splitIntoSubTUs)
			{
				codeQtCbfChroma(&search->m_entropyCoder, cbfFlag[TEXT_CHROMA_U][0], tuDepth);
				codeQtCbfChroma(&search->m_entropyCoder, cbfFlag[TEXT_CHROMA_V][0], tuDepth);
			}
			else
			{
				offsetSubTUCBFs(search, cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
				offsetSubTUCBFs(search, cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
				codeQtCbfChroma(&search->m_entropyCoder, cbfFlag[TEXT_CHROMA_U][0], tuDepth);
				codeQtCbfChroma(&search->m_entropyCoder, cbfFlag[TEXT_CHROMA_U][1], tuDepth);
				codeQtCbfChroma(&search->m_entropyCoder, cbfFlag[TEXT_CHROMA_V][0], tuDepth);
				codeQtCbfChroma(&search->m_entropyCoder, cbfFlag[TEXT_CHROMA_V][1], tuDepth);
			}
		}

		codeQtCbfLuma(&search->m_entropyCoder, cbfFlag[TEXT_LUMA][0], tuDepth);

		uint32_t cbfBits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

		uint32_t coeffBits = 0;
		coeffBits = singleBits[TEXT_LUMA][0];
		for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
		{
			coeffBits += singleBits[TEXT_CHROMA_U][subTUIndex];
			coeffBits += singleBits[TEXT_CHROMA_V][subTUIndex];
		}

		// In split mode, we need only coeffBits. The reason is encoding chroma cbfs is different from luma.
		// In case of chroma, if any one of the split block's cbf is 1, then we need to encode cbf 1, and then for
		// four split block's individual cbf value. This is not known before analysis of four split blocks.
		// For that reason, I am collecting individual coefficient bits only.
		fullCost.bits = bSplitPresentFlag ? cbfBits + coeffBits : coeffBits;

		fullCost.distortion += singleDist[TEXT_LUMA][0];
		fullCost.energy += singlePsyEnergy[TEXT_LUMA][0];// need to check we need to add chroma also
		for (uint32_t subTUIndex = 0; subTUIndex < 2; subTUIndex++)
		{
			fullCost.distortion += singleDist[TEXT_CHROMA_U][subTUIndex];
			fullCost.distortion += singleDist[TEXT_CHROMA_V][subTUIndex];
		}

		if (search->m_rdCost.m_psyRd)
			fullCost.rdcost = calcPsyRdCost(&search->m_rdCost, fullCost.distortion, fullCost.bits, fullCost.energy);
		else
			fullCost.rdcost = calcRdCost(&search->m_rdCost, fullCost.distortion, fullCost.bits);
	}

	outCosts->distortion += fullCost.distortion;
	outCosts->rdcost += fullCost.rdcost;
	outCosts->bits += fullCost.bits;
	outCosts->energy += fullCost.energy;
	*/
}
void Search_saveResidualQTData(Search *search, CUData* cu, ShortYuv* resiYuv, uint32_t absPartIdx, uint32_t tuDepth)
{/*
	const uint32_t log2TrSize = cu->m_log2CUSize[0] - tuDepth;

	if (tuDepth < cu->m_tuDepth[absPartIdx])
	{
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		for (uint32_t qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
			Search_saveResidualQTData(search, cu, resiYuv, absPartIdx, tuDepth + 1);
		return;
	}

	const uint32_t qtLayer = log2TrSize - 2;

	uint32_t log2TrSizeC = log2TrSize - search->predict.m_hChromaShift;
	bool bCodeChroma = TRUE;
	uint32_t tuDepthC = tuDepth;
	if (log2TrSizeC < 2)
	{
		X265_CHECK(log2TrSize == 2 && search->predict.m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
		log2TrSizeC = 2;
		tuDepthC--;
		bCodeChroma = !(absPartIdx & 3);
	}

	ShortYuv_copyPartToPartLuma(&search->m_rqt[qtLayer].resiQtYuv, resiYuv, absPartIdx, log2TrSize);

	uint32_t numCoeffY = 1 << (log2TrSize * 2);
	uint32_t coeffOffsetY = absPartIdx << LOG2_UNIT_SIZE * 2;
	coeff_t* coeffSrcY = search->m_rqt[qtLayer].coeffRQT[0] + coeffOffsetY;
	coeff_t* coeffDstY = cu->m_trCoeff[0] + coeffOffsetY;
	memcpy(coeffDstY, coeffSrcY, sizeof(coeff_t) * numCoeffY);

	if (bCodeChroma)
	{
		ShortYuv_copyPartToPartChroma(&search->m_rqt[qtLayer].resiQtYuv, resiYuv, absPartIdx, log2TrSizeC + search->predict.m_hChromaShift);

		uint32_t numCoeffC = 1 << (log2TrSizeC * 2 + (search->predict.m_csp == X265_CSP_I422));
		uint32_t coeffOffsetC = coeffOffsetY >> (search->predict.m_hChromaShift + search->predict.m_vChromaShift);

		coeff_t* coeffSrcU = search->m_rqt[qtLayer].coeffRQT[1] + coeffOffsetC;
		coeff_t* coeffSrcV = search->m_rqt[qtLayer].coeffRQT[2] + coeffOffsetC;
		coeff_t* coeffDstU = cu->m_trCoeff[1] + coeffOffsetC;
		coeff_t* coeffDstV = cu->m_trCoeff[2] + coeffOffsetC;
		memcpy(coeffDstU, coeffSrcU, sizeof(coeff_t) * numCoeffC);
		memcpy(coeffDstV, coeffSrcV, sizeof(coeff_t) * numCoeffC);
	}*/
}

/* estimation of best merge coding of an inter PU (2Nx2N merge PUs are evaluated as their own mode) */
uint32_t Search_mergeEstimation(Search *search, CUData* cu, const CUGeom* cuGeom, struct PredictionUnit* pu, int puIdx, struct MergeData* m, int width, int height)
{/*
	X265_CHECK(cu->m_partSize[0] != SIZE_2Nx2N, "mergeEstimation() called for 2Nx2N\n");

	struct MVField  candMvField[MRG_MAX_NUM_CANDS][2];
	uint8_t  candDir[MRG_MAX_NUM_CANDS];
	uint32_t numMergeCand = CUData_getInterMergeCandidates(cu, pu->puAbsPartIdx, puIdx, candMvField, candDir);

	if (isBipredRestriction(cu))
	{
		// do not allow bidir merge candidates if PU is smaller than 8x8, drop L1 reference //
		for (uint32_t mergeCand = 0; mergeCand < numMergeCand; ++mergeCand)
		{
			if (candDir[mergeCand] == 3)
			{
				candDir[mergeCand] = 1;
				candMvField[mergeCand][1].refIdx = REF_NOT_VALID;
			}
		}
	}

	Yuv* tempYuv = &search->m_rqt[cuGeom->depth].tmpPredYuv;

	uint32_t outCost = MAX_UINT;
	for (uint32_t mergeCand = 0; mergeCand < numMergeCand; ++mergeCand)
	{
		// Prevent TMVP candidates from using unavailable reference pixels //
		if (search->m_bFrameParallel &&
			(candMvField[mergeCand][0].mv.y >= (search->m_param->searchRange + 1) * 4 ||
			candMvField[mergeCand][1].mv.y >= (search->m_param->searchRange + 1) * 4))
			continue;

		cu->m_mv[0][pu->puAbsPartIdx] = candMvField[mergeCand][0].mv;
		cu->m_refIdx[0][pu->puAbsPartIdx] = (int8_t)candMvField[mergeCand][0].refIdx;
		cu->m_mv[1][pu->puAbsPartIdx] = candMvField[mergeCand][1].mv;
		cu->m_refIdx[1][pu->puAbsPartIdx] = (int8_t)candMvField[mergeCand][1].refIdx;

		Predict_motionCompensation(&search->predict, cu, pu, tempYuv, TRUE, search->m_me.bChromaSATD);

		uint32_t costCand = bufSATD(&search->m_me, Yuv_getLumaAddr(tempYuv, pu->puAbsPartIdx), tempYuv->m_size, width, height);
		if (&search->m_me.bChromaSATD)
			costCand += bufChromaSATD(&search->m_me, tempYuv, pu->puAbsPartIdx, width, height);

		uint32_t bitsCand = getTUBits(mergeCand, numMergeCand);
		costCand = costCand + getCost(&search->m_rdCost, bitsCand);
		if (costCand < outCost)
		{
			outCost = costCand;
			m->bits = bitsCand;
			m->index = mergeCand;
		}
	}

	m->mvField[0] = candMvField[m->index][0];
	m->mvField[1] = candMvField[m->index][1];
	m->dir = candDir[m->index];

	return outCost;*/
	return 0;
}
void Search_setSearchRange(Search *search, const CUData* cu, const MV* mvp, int merange, MV* mvmin, MV* mvmax)
{/*
	MV *dist = (MV *)malloc(sizeof(MV));//已释放
	dist->x = (int16_t)merange << 2;
	dist->y = (int16_t)merange << 2;
	mvmin->x = mvp->x - dist->x;
	mvmin->y = mvp->y - dist->y;
	mvmax->x = mvp->x + dist->x;
	mvmax->y = mvp->y + dist->y;
	CUData_clipMv(cu, mvmin);
	CUData_clipMv(cu, mvmax);

	// Clip search range to signaled maximum MV length.
	// We do not support this VUI field being changed from the default 
	const int maxMvLen = (1 << 15) - 1;
	mvmin->x = X265_MAX(mvmin->x, -maxMvLen);
	mvmin->y = X265_MAX(mvmin->y, -maxMvLen);
	mvmax->x = X265_MIN(mvmax->x, maxMvLen);
	mvmax->y = X265_MIN(mvmax->y, maxMvLen);

	mvmin->x >>= 2;
	mvmin->y >>= 2;
	mvmax->x >>= 2;
	mvmax->y >>= 2;
	// conditional clipping for frame parallelism //
	mvmin->y = X265_MIN(mvmin->y, (int16_t)search->m_refLagPixels);
	mvmax->y = X265_MIN(mvmax->y, (int16_t)search->m_refLagPixels);
	free(dist);
	dist = NULL; */
}
/* Check if using an alternative MVP would result in a smaller MVD + signal bits */
MV Search_checkBestMVP(Search *search, BitCost *bitcost, const MV* amvpCand, const MV* mv, int mvpIdx, uint32_t outBits, uint32_t outCost)
{/*
	int diffBits = bitcost_2(bitcost, mv, &amvpCand[!mvpIdx]) - bitcost_2(bitcost, mv, &amvpCand[mvpIdx]);
	if (diffBits < 0)
	{
		mvpIdx = !mvpIdx;
		uint32_t origOutBits = outBits;
		outBits = origOutBits + diffBits;
		outCost = (outCost - getCost(&search->m_rdCost, origOutBits)) + getCost(&search->m_rdCost, outBits);
	}*/
	return amvpCand[mvpIdx];
}
/* find the best inter prediction for each PU of specified mode */
void Search_predInterSearch(Search *search, Mode* interMode, const CUGeom* cuGeom, bool bChromaMC)
{/*
	//ProfileCUScope(interMode.cu, motionEstimationElapsedTime, countMotionEstimate);

	CUData* cu = &interMode->cu;
	Yuv* predYuv = &interMode->predYuv;

	MV mvc[(MD_ABOVE_LEFT + 1) * 2 + 1];

	const Slice *slice = search->m_slice;
	int numPart = getNumPartInter(cu);
	int numPredDir = isInterP(slice) ? 1 : 2;
	const int* numRefIdx = slice->m_numRefIdx;
	uint32_t lastMode = 0;
	int      totalmebits = 0;
	int      numME = numRefIdx[0] + numRefIdx[1];
	bool     bTryDistributed = search->m_param->bDistributeMotionEstimation && numME > 2;

	Yuv*     tmpPredYuv = &search->m_rqt[cuGeom->depth].tmpPredYuv;

	struct MergeData merge;
	memset(&merge, 0, sizeof(merge));

	for (int puIdx = 0; puIdx < numPart; puIdx++)
	{
		MotionData* bestME = interMode->bestME[puIdx];
		struct PredictionUnit pu;
		PredictionUnit_PredictionUnit(&pu, cu, cuGeom, puIdx);
		MotionEstimate_setSourcePU(&search->m_me, interMode->fencYuv, pu.ctuAddr, pu.cuAbsPartIdx, pu.puAbsPartIdx, pu.width, pu.height);

		// find best cost merge candidate. note: 2Nx2N merge and bidir are handled as separate modes //
		uint32_t mrgCost = MAX_UINT;//numPart == 1 ? MAX_UINT : Search_mergeEstimation(search, cu, cuGeom, pu, puIdx, &merge, pu.width, pu.height);

		bestME[0].cost = MAX_UINT;
		bestME[1].cost = MAX_UINT;

		Search_getBlkBits((PartSize)cu->m_partSize[0], isInterP(slice), puIdx, lastMode, search->m_listSelBits);
		bool bDoUnidir = TRUE;

		CUData_getNeighbourMV(cu, puIdx, pu.puAbsPartIdx, interMode->interNeighbours);

		if (bDoUnidir)
		{
			for (int list = 0; list < numPredDir; list++)
			{
				for (int ref = 0; ref < numRefIdx[list]; ref++)
				{
					uint32_t bits = search->m_listSelBits[list] + MVP_IDX_BITS;
					bits += getTUBits(ref, numRefIdx[list]);
					int numMvc = CUData_getPMV(cu, interMode->interNeighbours, list, ref, interMode->amvpCand[list][ref], mvc);

					MV* amvp = interMode->amvpCand[list][ref];
					int mvpIdx = Search_selectMVP(search, cu, &pu, amvp, list, ref, pu.width, pu.height);

					MV mvmin, mvmax, outmv, mvp = amvp[mvpIdx];
					mvmin.word = 0;//------------------
					mvmax.word = 0;//------------------
					outmv.word = 0;//------------------

					Search_setSearchRange(search, cu, &mvp, search->m_param->searchRange, &mvmin, &mvmax);
					int satdCost = MotionEstimate_motionEstimate(&search->m_me, slice->m_mref[list][ref].referenceplanes, &mvmin, &mvmax, &mvp, numMvc, mvc, search->m_param->searchRange, &outmv, pu.width, pu.height);

					// Get total cost of partition, but only include MV bit cost once //
					bits += bitcost_1(&search->m_me.bitcost, &outmv);
					uint32_t cost = (satdCost - mvcost(&search->m_me.bitcost, outmv.x, outmv.y)) + getCost(&search->m_rdCost, bits);

					// Refine MVP selection, updates: mvpIdx, bits, cost //
					mvp = Search_checkBestMVP(search, &search->m_me.bitcost, amvp, &outmv, mvpIdx, bits, cost);

					if (cost < bestME[list].cost)
					{
						bestME[list].mv = outmv;
						bestME[list].mvp = mvp;
						bestME[list].mvpIdx = mvpIdx;
						bestME[list].ref = ref;
						bestME[list].cost = cost;
						bestME[list].bits = bits;
					}
				}
			}
		}

		// select best option and store into CU //
		//if (mrgCost < bidirCost && mrgCost < bestME[0].cost && mrgCost < bestME[1].cost)
		if (mrgCost < bestME[0].cost && mrgCost < bestME[1].cost)
		{
			cu->m_mergeFlag[pu.puAbsPartIdx] = TRUE;
			cu->m_mvpIdx[0][pu.puAbsPartIdx] = merge.index; // merge candidate ID is stored in L0 MVP idx //
			CUData_setPUInterDir(cu, merge.dir, pu.puAbsPartIdx, puIdx);
			CUData_setPUMv(cu, 0, &merge.mvField[0].mv, pu.puAbsPartIdx, puIdx);
			CUData_setPURefIdx(cu, 0, merge.mvField[0].refIdx, pu.puAbsPartIdx, puIdx);
			CUData_setPUMv(cu, 1, &merge.mvField[1].mv, pu.puAbsPartIdx, puIdx);
			CUData_setPURefIdx(cu, 1, merge.mvField[1].refIdx, pu.puAbsPartIdx, puIdx);

			totalmebits += merge.bits;
		}
		else if (bestME[0].cost <= bestME[1].cost)
		{
			lastMode = 0;

			cu->m_mergeFlag[pu.puAbsPartIdx] = FALSE;
			CUData_setPUInterDir(cu, 1, pu.puAbsPartIdx, puIdx);
			CUData_setPUMv(cu, 0, &bestME[0].mv, pu.puAbsPartIdx, puIdx);
			CUData_setPURefIdx(cu, 0, bestME[0].ref, pu.puAbsPartIdx, puIdx);
			cu->m_mvd[0][pu.puAbsPartIdx].x = bestME[0].mv.x - bestME[0].mvp.x;
			cu->m_mvd[0][pu.puAbsPartIdx].y = bestME[0].mv.y - bestME[0].mvp.y;
			cu->m_mvpIdx[0][pu.puAbsPartIdx] = bestME[0].mvpIdx;

			CUData_setPURefIdx(cu, 1, REF_NOT_VALID, pu.puAbsPartIdx, puIdx);
			CUData_setPUMv(cu, 1, mvzero(&bestME[1].mv, 0, 0), pu.puAbsPartIdx, puIdx);

			totalmebits += bestME[0].bits;
		}
		else
		{
			lastMode = 1;

			cu->m_mergeFlag[pu.puAbsPartIdx] = FALSE;
			CUData_setPUInterDir(cu, 2, pu.puAbsPartIdx, puIdx);
			CUData_setPUMv(cu, 1, &bestME[1].mv, pu.puAbsPartIdx, puIdx);
			CUData_setPURefIdx(cu, 1, bestME[1].ref, pu.puAbsPartIdx, puIdx);
			cu->m_mvd[1][pu.puAbsPartIdx].x = bestME[1].mv.x - bestME[1].mvp.x;
			cu->m_mvd[1][pu.puAbsPartIdx].y = bestME[1].mv.y - bestME[1].mvp.y;
			cu->m_mvpIdx[1][pu.puAbsPartIdx] = bestME[1].mvpIdx;

			CUData_setPURefIdx(cu, 0, REF_NOT_VALID, pu.puAbsPartIdx, puIdx);
			CUData_setPUMv(cu, 0, mvzero(&bestME[0].mv, 0, 0), pu.puAbsPartIdx, puIdx);

			totalmebits += bestME[1].bits;
		}

		Predict_motionCompensation(&search->predict, cu, &pu, predYuv, TRUE, bChromaMC);
	}
	X265_CHECK(ok(interMode), "inter mode is not ok");
	interMode->sa8dBits += totalmebits;
	free(&search->m_me); search->m_me = 0;*/
}
void Search_getBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3])
{/*
	if (cuMode == SIZE_2Nx2N)
	{
		blockBit[0] = (!bPSlice) ? 3 : 1;
		blockBit[1] = 3;
		blockBit[2] = 5;
	}
	else if (cuMode == SIZE_2NxN || cuMode == SIZE_2NxnU || cuMode == SIZE_2NxnD)
	{
		static const uint32_t listBits[2][3][3] =
		{
			{ { 0, 0, 3 }, { 0, 0, 0 }, { 0, 0, 0 } },
			{ { 5, 7, 7 }, { 7, 5, 7 }, { 9 - 3, 9 - 3, 9 - 3 } }
		};
		if (bPSlice)
		{
			blockBit[0] = 3;
			blockBit[1] = 0;
			blockBit[2] = 0;
		}
		else
			memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
	}
	else if (cuMode == SIZE_Nx2N || cuMode == SIZE_nLx2N || cuMode == SIZE_nRx2N)
	{
		static const uint32_t listBits[2][3][3] =
		{
			{ { 0, 2, 3 }, { 0, 0, 0 }, { 0, 0, 0 } },
			{ { 5, 7, 7 }, { 7 - 2, 7 - 2, 9 - 2 }, { 9 - 3, 9 - 3, 9 - 3 } }
		};
		if (bPSlice)
		{
			blockBit[0] = 3;
			blockBit[1] = 0;
			blockBit[2] = 0;
		}
		else
			memcpy(blockBit, listBits[partIdx][lastMode], 3 * sizeof(uint32_t));
	}
	else if (cuMode == SIZE_NxN)
	{
		blockBit[0] = (!bPSlice) ? 3 : 1;
		blockBit[1] = 3;
		blockBit[2] = 5;
	}
	else
	{
		X265_CHECK(0, "getBlkBits: unknown cuMode\n");
	}*/
}
/* Pick between the two AMVP candidates which is the best one to use as
* MVP for the motion search, based on SAD cost */
int Search_selectMVP(Search *search, CUData* cu, struct PredictionUnit* pu, MV amvp[AMVP_NUM_CANDS], int list, int ref, int width, int height)
{/*
	if (amvp[0].x == amvp[1].x && amvp[0].y == amvp[1].y)
		return 0;

	Yuv* tmpPredYuv = &search->m_rqt[cu->m_cuDepth[0]].tmpPredYuv;
	uint32_t costs[AMVP_NUM_CANDS];

	for (int i = 0; i < AMVP_NUM_CANDS; i++)
	{
		MV mvCand = amvp[i];

		// NOTE: skip mvCand if Y is > merange and -FN>1
		if (search->m_bFrameParallel && (mvCand.y >= (search->m_param->searchRange + 1) * 4))
			costs[i] = COST_MAX;
		else
		{
			CUData_clipMv(cu, &mvCand);
			Predict_predInterLumaPixel(pu, tmpPredYuv, cu->m_slice->m_refPicList[list][ref]->m_reconPic, &mvCand);
			costs[i] = bufSAD(&search->m_me, Yuv_getLumaAddr(tmpPredYuv, pu->puAbsPartIdx), tmpPredYuv->m_size, width, height);
		}
	}

	return costs[0] <= costs[1] ? 0 : 1;
	*/
		return 0;
}


/* Note that this function does not save the best intra prediction, it must
* be generated later. It records the best mode in the cu */
void Search_checkIntraInInter(Search *search, Mode* intraMode, CUGeom* cuGeom)
{/*
	CUData* cu = &intraMode->cu;
	uint32_t depth = cuGeom->depth;

	setPartSizeSubParts(cu, SIZE_2Nx2N);
	setPredModeSubParts(cu, MODE_INTRA);

	const uint32_t initTuDepth = 0;
	uint32_t log2TrSize = cuGeom->log2CUSize - initTuDepth;
	uint32_t tuSize = 1 << log2TrSize;
	const uint32_t absPartIdx = 0;

	// Reference sample smoothing
	struct IntraNeighbors intraNeighbors;
	initIntraNeighbors(cu, absPartIdx, initTuDepth, TRUE, &intraNeighbors);
	initAdiPattern(&search->predict, cu, cuGeom, absPartIdx, &intraNeighbors, ALL_IDX);

	const pixel* fenc = intraMode->fencYuv->m_buf[0];
	uint32_t stride = intraMode->fencYuv->m_size;

	int sad, bsad;
	uint32_t bits, bbits, mode, bmode;
	uint64_t cost, bcost;

	// 33 Angle modes once
	int scaleTuSize = tuSize;
	int scaleStride = stride;
	int costShift = 0;
	int sizeIdx = log2TrSize - 2;

	if (tuSize > 32)
	{
		// CU is 64x64, we scale to 32x32 and adjust required parameters
		primitives.scale2D_64to32(search->m_fencScaled, fenc, stride);
		fenc = search->m_fencScaled;

		pixel nScale[129];
		search->predict.intraNeighbourBuf[1][0] = search->predict.intraNeighbourBuf[0][0];
		primitives.scale1D_128to64(nScale + 1, search->predict.intraNeighbourBuf[0] + 1);

		// we do not estimate filtering for downscaled samples
		memcpy(&search->predict.intraNeighbourBuf[0][1], &nScale[1], 2 * 64 * sizeof(pixel));   // Top & Left pixels
		memcpy(&search->predict.intraNeighbourBuf[1][1], &nScale[1], 2 * 64 * sizeof(pixel));

		scaleTuSize = 32;
		scaleStride = 32;
		costShift = 2;
		sizeIdx = 5 - 2; // log2(scaleTuSize) - 2
	}

	pixelcmp_t sa8d = primitives.cu[sizeIdx].sa8d;
	int predsize = scaleTuSize * scaleTuSize;

	loadIntraDirModeLuma(&search->m_entropyCoder, &search->m_rqt[depth].cur);

	// there are three cost tiers for intra modes:
	//  pred[0]          - mode probable, least cost
	//  pred[1], pred[2] - less probable, slightly more cost
	//  non-mpm modes    - all cost the same (rbits)
	uint64_t mpms;
	uint32_t mpmModes[3];
	uint32_t rbits = getIntraRemModeBits(search, cu, absPartIdx, mpmModes, &mpms);

	// DC(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter,int width);
	primitives.cu[sizeIdx].intra_pred[DC_IDX](search->m_intraPredAngs, scaleStride, search->predict.intraNeighbourBuf[0], 0, (scaleTuSize <= 16), 32);
	bsad = sa8d(fenc, scaleStride, search->m_intraPredAngs, scaleStride, pow(2, 2 + sizeIdx), pow(2, 2 + sizeIdx)) << costShift;
	bmode = mode = DC_IDX;
	bbits = (mpms & ((uint64_t)1 << mode)) ? bitsIntraModeMPM(&search->m_entropyCoder, mpmModes, mode) : rbits;
	bcost = calcRdSADCost(&search->m_rdCost, bsad, bbits);

	// PLANAR
	pixel* planar = search->predict.intraNeighbourBuf[0];
	if (tuSize & (8 | 16 | 32))
		planar = search->predict.intraNeighbourBuf[1];

	primitives.cu[sizeIdx].intra_pred[PLANAR_IDX](search->m_intraPredAngs, scaleStride, planar, 0, 0, 32);
	sad = sa8d(fenc, scaleStride, search->m_intraPredAngs, scaleStride, pow(2, 2 + sizeIdx), pow(2, 2 + sizeIdx)) << costShift;
	mode = PLANAR_IDX;
	bits = (mpms & ((uint64_t)1 << mode)) ? bitsIntraModeMPM(&search->m_entropyCoder, mpmModes, mode) : rbits;
	cost = calcRdSADCost(&search->m_rdCost, sad, bits);
	COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);

	bool allangs = TRUE;
	if (primitives.cu[sizeIdx].intra_pred_allangs)
	{
		primitives.cu[sizeIdx].transpose(search->m_fencTransposed, fenc, scaleStride, 32);
		primitives.cu[sizeIdx].intra_pred_allangs(search->m_intraPredAngs, search->predict.intraNeighbourBuf[0], search->predict.intraNeighbourBuf[1], (scaleTuSize <= 16), 32);
	}
	else
		allangs = FALSE;

#define TRY_ANGLE(angle)\
    if (allangs) {\
        if (angle < 18)\
            sad = sa8d(search->m_fencTransposed, scaleTuSize, &search->m_intraPredAngs[(angle - 2) * predsize], scaleTuSize,pow(2,2+sizeIdx),pow(2,2+sizeIdx)) << costShift;\
	        else\
            sad = sa8d(fenc, scaleStride, &search->m_intraPredAngs[(angle - 2) * predsize], scaleTuSize,pow(2,2+sizeIdx),pow(2,2+sizeIdx)) << costShift;\
        bits = (mpms & ((uint64_t)1 << angle)) ? bitsIntraModeMPM(&search->m_entropyCoder, mpmModes, angle) : rbits;\
        cost = calcRdSADCost(&search->m_rdCost, sad, bits);\
	    } else {\
        int filter = !!(g_intraFilterFlags[angle] & scaleTuSize);\
        primitives.cu[sizeIdx].intra_pred[angle](search->m_intraPredAngs, scaleTuSize, search->predict.intraNeighbourBuf[filter], angle, scaleTuSize <= 16,32);\
        sad = sa8d(fenc, scaleStride, search->m_intraPredAngs, scaleTuSize,pow(2,2+sizeIdx),pow(2,2+sizeIdx)) << costShift;\
        bits = (mpms & ((uint64_t)1 << angle)) ? bitsIntraModeMPM(&search->m_entropyCoder, mpmModes, angle) : rbits;\
        cost = calcRdSADCost(&search->m_rdCost, sad, bits);\
	    }

	if (search->m_param->bEnableFastIntra)
	{
		int asad = 0;
		uint32_t lowmode, highmode, amode = 5, abits = 0;
		uint64_t acost = MAX_INT64;

		// pick the best angle, sampling at distance of 5 //
		for (mode = 5; mode < 35; mode += 5)
		{
			TRY_ANGLE(mode);
			COPY4_IF_LT(acost, cost, amode, mode, asad, sad, abits, bits);
		}

		// refine best angle at distance 2, then distance 1 //
		for (uint32_t dist = 2; dist >= 1; dist--)
		{
			lowmode = amode - dist;
			highmode = amode + dist;

			X265_CHECK(lowmode >= 2 && lowmode <= 34, "low intra mode out of range\n");
			TRY_ANGLE(lowmode);
			COPY4_IF_LT(acost, cost, amode, lowmode, asad, sad, abits, bits);

			X265_CHECK(highmode >= 2 && highmode <= 34, "high intra mode out of range\n");
			TRY_ANGLE(highmode);
			COPY4_IF_LT(acost, cost, amode, highmode, asad, sad, abits, bits);
		}
		
		if (amode == 33)
		{
		TRY_ANGLE(34);//64位数据移位，但是ECS中最多能移位32位
		COPY4_IF_LT(acost, cost, amode, 34, asad, sad, abits, bits);
		}
		
		COPY4_IF_LT(bcost, acost, bmode, amode, bsad, asad, bbits, abits);
	}
	else // calculate and search all intra prediction angles for lowest cost
	{
		for (mode = 2; mode < 35; mode++)
		{
			TRY_ANGLE(mode);
			COPY4_IF_LT(bcost, cost, bmode, mode, bsad, sad, bbits, bits);
		}
	}

	setLumaIntraDirSubParts(cu, (uint8_t)bmode, absPartIdx, depth + initTuDepth);
	initCosts(intraMode);
	intraMode->totalBits = bbits;
	intraMode->distortion = bsad;
	intraMode->sa8dCost = bcost;
	intraMode->sa8dBits = bbits;
	X265_CHECK(ok(intraMode), "intra mode is not ok");
	*/
}

void Search_encodeIntraInInter(Search *search, Mode* intraMode, CUGeom* cuGeom)
{/*
	CUData* cu = &intraMode->cu;
	Yuv* reconYuv = &intraMode->reconYuv;

	X265_CHECK(cu->m_partSize[0] == SIZE_2Nx2N, "encodeIntraInInter does not expect NxN intra\n");
	X265_CHECK(!Slice_isIntra(cu->m_slice), "encodeIntraInInter does not expect to be used in I slices\n");

	uint32_t tuDepthRange[2];
	CUData_getIntraTUQtDepthRange(cu, tuDepthRange, 0);

	load(&search->m_entropyCoder, &search->m_rqt[cuGeom->depth].cur);

	struct Cost icosts;
	codeIntraLumaQT(search, intraMode, cuGeom, 0, 0, FALSE, &icosts, tuDepthRange);
	extractIntraResultQT(search, cu, reconYuv, 0, 0);

	intraMode->distortion = icosts.distortion;
	intraMode->distortion += estIntraPredChromaQT(search, intraMode, cuGeom, NULL);

	entropy_resetBits(&search->m_entropyCoder);
	if (search->m_slice->m_pps->bTransquantBypassEnabled)
		codeCUTransquantBypassFlag(&search->m_entropyCoder, cu->m_tqBypass[0]);
	codeSkipFlag(&search->m_entropyCoder, cu, 0);
	codePredMode(&search->m_entropyCoder, cu->m_predMode[0]);
	codePartSize(&search->m_entropyCoder, cu, 0, cuGeom->depth);
	codePredInfo(&search->m_entropyCoder, cu, 0);
	intraMode->mvBits += entropy_getNumberOfWrittenBits(&search->m_entropyCoder);

	bool bCodeDQP = search->m_slice->m_pps->bUseDQP;
	codeCoeff(&search->m_entropyCoder, cu, 0, bCodeDQP, tuDepthRange);

	intraMode->totalBits = entropy_getNumberOfWrittenBits(&search->m_entropyCoder);
	intraMode->coeffBits = intraMode->totalBits - intraMode->mvBits;
	if (search->m_rdCost.m_psyRd)
	{
		const Yuv* fencYuv = intraMode->fencYuv;
		intraMode->psyEnergy = psyCost(cuGeom->log2CUSize - 2, fencYuv->m_buf[0], fencYuv->m_size, reconYuv->m_buf[0], reconYuv->m_size);
	}

	store(&search->m_entropyCoder, &intraMode->contexts);
	updateModeCost(search, intraMode);
	checkDQP(search, intraMode, cuGeom);
	*/
}

void Search_residualTransformQuantInter(Search *search, Predict *predict, Mode* mode, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2])
{/*
	uint32_t depth = cuGeom->depth + tuDepth;
	CUData* cu = &mode->cu;
	uint32_t log2TrSize = cuGeom->log2CUSize - tuDepth;

	bool bCheckFull = log2TrSize <= depthRange[1];
	if (cu->m_partSize[0] != SIZE_2Nx2N && !tuDepth && log2TrSize > depthRange[0])
		bCheckFull = FALSE;

	if (bCheckFull)
	{
		// code full block
		uint32_t log2TrSizeC = log2TrSize - predict->m_hChromaShift;
		bool bCodeChroma = TRUE;
		uint32_t tuDepthC = tuDepth;
		if (log2TrSizeC < 2)
		{
			X265_CHECK(log2TrSize == 2 && predict->m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
			log2TrSizeC = 2;
			tuDepthC--;
			bCodeChroma = !(absPartIdx & 3);
		}

		uint32_t absPartIdxStep = cuGeom->numPartitions >> tuDepthC * 2;
		uint32_t setCbf = 1 << tuDepth;

		uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
		coeff_t* coeffCurY = cu->m_trCoeff[0] + coeffOffsetY;

		uint32_t sizeIdx = log2TrSize - 2;

		setTUDepthSubParts(cu, tuDepth, absPartIdx, depth);
		setTransformSkipSubParts(cu, 0, TEXT_LUMA, absPartIdx, depth);

		ShortYuv* resiYuv = &search->m_rqt[cuGeom->depth].tmpResiYuv;
		Yuv* fencYuv = mode->fencYuv;

		int16_t* curResiY = ShortYuv_getLumaAddr(resiYuv, absPartIdx);
		uint32_t strideResiY = resiYuv->m_size;

		pixel* fenc = Yuv_getLumaAddr(fencYuv, absPartIdx);
		uint32_t numSigY = Quant_transformNxN(&search->m_quant, cu, fenc, fencYuv->m_size, curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, absPartIdx, FALSE);

		if (numSigY)
		{
			Quant_invtransformNxN(&search->m_quant, curResiY, strideResiY, coeffCurY, log2TrSize, TEXT_LUMA, FALSE, FALSE, numSigY);
			setCbfSubParts(cu, setCbf, TEXT_LUMA, absPartIdx, depth);
		}
		else
		{
			primitives.cu[sizeIdx].blockfill_s(curResiY, strideResiY, 0, pow(2, 2 + sizeIdx));
			setCbfSubParts(cu, 0, TEXT_LUMA, absPartIdx, depth);
		}

		if (bCodeChroma)
		{
			uint32_t sizeIdxC = log2TrSizeC - 2;
			uint32_t strideResiC = resiYuv->m_csize;

			uint32_t coeffOffsetC = coeffOffsetY >> (predict->m_hChromaShift + predict->m_vChromaShift);
			coeff_t* coeffCurU = cu->m_trCoeff[1] + coeffOffsetC;
			coeff_t* coeffCurV = cu->m_trCoeff[2] + coeffOffsetC;
			bool splitIntoSubTUs = (predict->m_csp == X265_CSP_I422);

			TURecurse tuIterator;
			TURecurse_init(&tuIterator, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, absPartIdxStep, absPartIdx);
			do
			{
				uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;
				uint32_t subTUOffset = tuIterator.section << (log2TrSizeC * 2);

				setTransformSkipPartRange(cu, 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
				setTransformSkipPartRange(cu, 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);

				int16_t* curResiU = ShortYuv_getCbAddr(resiYuv, absPartIdxC);
				const pixel* fencCb = Yuv_getCbAddr(fencYuv, absPartIdxC);
				uint32_t numSigU = Quant_transformNxN(&search->m_quant, cu, fencCb, fencYuv->m_csize, curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, absPartIdxC, FALSE);
				if (numSigU)
				{
					Quant_invtransformNxN(&search->m_quant, curResiU, strideResiC, coeffCurU + subTUOffset, log2TrSizeC, TEXT_CHROMA_U, FALSE, FALSE, numSigU);
					setCbfPartRange(cu, setCbf, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
				}
				else
				{
					primitives.cu[sizeIdxC].blockfill_s(curResiU, strideResiC, 0, pow(2, 2 + sizeIdx));
					setCbfPartRange(cu, 0, TEXT_CHROMA_U, absPartIdxC, tuIterator.absPartIdxStep);
				}

				int16_t* curResiV = ShortYuv_getCrAddr(resiYuv, absPartIdxC);
				const pixel* fencCr = Yuv_getCrAddr(fencYuv, absPartIdxC);
				uint32_t numSigV = Quant_transformNxN(&search->m_quant, cu, fencCr, fencYuv->m_csize, curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, absPartIdxC, FALSE);
				if (numSigV)
				{
					Quant_invtransformNxN(&search->m_quant, curResiV, strideResiC, coeffCurV + subTUOffset, log2TrSizeC, TEXT_CHROMA_V, FALSE, FALSE, numSigV);
					setCbfPartRange(cu, setCbf, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
				}
				else
				{
					primitives.cu[sizeIdxC].blockfill_s(curResiV, strideResiC, 0, pow(2, 2 + sizeIdx));
					setCbfPartRange(cu, 0, TEXT_CHROMA_V, absPartIdxC, tuIterator.absPartIdxStep);
				}
			} while (isNextSection(&tuIterator));

			if (splitIntoSubTUs)
			{
				offsetSubTUCBFs(search, cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
				offsetSubTUCBFs(search, cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
			}
		}
	}
	else
	{
		X265_CHECK(log2TrSize > depthRange[0], "residualTransformQuantInter recursion check failure\n");

		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		uint32_t ycbf = 0, ucbf = 0, vcbf = 0;
		for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
		{
			Search_residualTransformQuantInter(search, predict, mode, cuGeom, qPartIdx, tuDepth + 1, depthRange);
			ycbf |= getCbf(cu, qPartIdx, TEXT_LUMA, tuDepth + 1);
			ucbf |= getCbf(cu, qPartIdx, TEXT_CHROMA_U, tuDepth + 1);
			vcbf |= getCbf(cu, qPartIdx, TEXT_CHROMA_V, tuDepth + 1);
		}
		for (uint32_t i = 0; i < 4 * qNumParts; ++i)
		{
			cu->m_cbf[0][absPartIdx + i] |= ycbf << tuDepth;
			cu->m_cbf[1][absPartIdx + i] |= ucbf << tuDepth;
			cu->m_cbf[2][absPartIdx + i] |= vcbf << tuDepth;
		}
	}*/
}
/* fast luma intra residual generation. Only perform the minimum number of TU splits required by the CU size */
void Search_residualTransformQuantIntra(Search *search, Predict *predict, Mode* mode, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2])
{/*
	CUData* cu = &mode->cu;
	uint32_t fullDepth = cuGeom->depth + tuDepth;
	uint32_t log2TrSize = cuGeom->log2CUSize - tuDepth;
	bool     bCheckFull = log2TrSize <= depthRange[1];

	X265_CHECK(search->m_slice->m_sliceType != I_SLICE, "residualTransformQuantIntra not intended for I slices\n");

	// we still respect rdPenalty == 2, we can forbid 32x32 intra TU. rdPenalty = 1 is impossible
	// since we are not measuring RD cost //
	if (search->m_param->rdPenalty == 2 && log2TrSize == 5 && depthRange[0] <= 4)
		bCheckFull = FALSE;

	if (bCheckFull)
	{
		const pixel* fenc = Yuv_getLumaAddr(mode->fencYuv, absPartIdx);
		pixel*   pred = Yuv_getLumaAddr(&mode->predYuv, absPartIdx);
		int16_t* residual = ShortYuv_getLumaAddr(&search->m_rqt[cuGeom->depth].tmpResiYuv, absPartIdx);
		uint32_t stride = mode->fencYuv->m_size;

		// init availability pattern
		uint32_t lumaPredMode = cu->m_lumaIntraDir[absPartIdx];
		struct IntraNeighbors intraNeighbors;
		initIntraNeighbors(cu, absPartIdx, tuDepth, TRUE, &intraNeighbors);
		initAdiPattern(predict, cu, cuGeom, absPartIdx, &intraNeighbors, lumaPredMode);

		// get prediction signal
		predIntraLumaAng(predict, lumaPredMode, pred, stride, log2TrSize);

		X265_CHECK(!cu->m_transformSkip[TEXT_LUMA][absPartIdx], "unexpected tskip flag in residualTransformQuantIntra\n");
		setTUDepthSubParts(cu, tuDepth, absPartIdx, fullDepth);

		uint32_t coeffOffsetY = absPartIdx << (LOG2_UNIT_SIZE * 2);
		coeff_t* coeffY = cu->m_trCoeff[0] + coeffOffsetY;

		uint32_t sizeIdx = log2TrSize - 2;
		primitives.cu[sizeIdx].calcresidual(fenc, pred, residual, stride, pow(2, log2TrSize));

		pixel*   picReconY = Picyuv_CUgetLumaAddr(search->m_frame->m_reconPic, cu->m_cuAddr, cuGeom->absPartIdx + absPartIdx);
		intptr_t picStride = search->m_frame->m_reconPic->m_stride;

		uint32_t numSig = Quant_transformNxN(&search->m_quant, cu, fenc, stride, residual, stride, coeffY, log2TrSize, TEXT_LUMA, absPartIdx, FALSE);
		if (numSig)
		{
			Quant_invtransformNxN(&search->m_quant, residual, stride, coeffY, log2TrSize, TEXT_LUMA, TRUE, FALSE, numSig);
			primitives.cu[sizeIdx].add_ps(picReconY, picStride, pred, residual, stride, stride, pow(2, log2TrSize), pow(2, log2TrSize));
			setCbfSubParts(cu, 1 << tuDepth, TEXT_LUMA, absPartIdx, fullDepth);
		}
		else
		{
			primitives.cu[sizeIdx].copy_pp(picReconY, picStride, pred, stride, pow(2, log2TrSize), pow(2, log2TrSize));
			setCbfSubParts(cu, 0, TEXT_LUMA, absPartIdx, fullDepth);
		}
	}
	else
	{
		X265_CHECK(log2TrSize > depthRange[0], "intra luma split state failure\n");

		// code split block //
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		uint32_t cbf = 0;
		for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
		{
			Search_residualTransformQuantIntra(search, predict, mode, cuGeom, qPartIdx, tuDepth + 1, depthRange);
			cbf |= getCbf(cu, qPartIdx, TEXT_LUMA, tuDepth + 1);
		}
		for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
			cu->m_cbf[0][absPartIdx + offs] |= (cbf << tuDepth);
	}*/
}
void Search_getBestIntraModeChroma(Predict *predict, Mode* intraMode, const CUGeom* cuGeom)
{/*
	CUData* cu = &intraMode->cu;
	const Yuv* fencYuv = intraMode->fencYuv;
	Yuv* predYuv = &intraMode->predYuv;

	uint32_t bestMode = 0;
	uint64_t bestCost = MAX_INT64;
	uint32_t modeList[NUM_CHROMA_MODE];

	uint32_t log2TrSizeC = cu->m_log2CUSize[0] - predict->m_hChromaShift;
	uint32_t tuSize = 1 << log2TrSizeC;
	uint32_t tuDepth = 0;
	int32_t costShift = 0;

	if (tuSize > 32)
	{
		tuDepth = 1;
		costShift = 2;
		log2TrSizeC = 5;
	}

	struct IntraNeighbors intraNeighbors;
	initIntraNeighbors(cu, 0, tuDepth, FALSE, &intraNeighbors);
	CUData_getAllowedChromaDir(cu, 0, modeList);

	// check chroma modes
	for (uint32_t mode = 0; mode < NUM_CHROMA_MODE; mode++)
	{
		uint32_t chromaPredMode = modeList[mode];
		if (chromaPredMode == DM_CHROMA_IDX)
			chromaPredMode = cu->m_lumaIntraDir[0];
		if (predict->m_csp == X265_CSP_I422)
			chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

		uint64_t cost = 0;
		for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
		{
			const pixel* fenc = fencYuv->m_buf[chromaId];
			pixel* pred = predYuv->m_buf[chromaId];
			initAdiPatternChroma(predict, cu, cuGeom, 0, &intraNeighbors, chromaId);
			// get prediction signal
			predIntraChromaAng(predict, chromaPredMode, pred, fencYuv->m_csize, log2TrSizeC);
			cost += primitives.cu[log2TrSizeC - 2].sa8d(fenc, predYuv->m_csize, pred, fencYuv->m_csize, tuSize, tuSize) << costShift;
		}

		if (cost < bestCost)
		{
			bestCost = cost;
			bestMode = modeList[mode];
		}
	}

	setChromIntraDirSubParts(cu, bestMode, 0, cuGeom->depth);
	*/
}

void Search_residualQTIntraChroma(Search *search, Predict *predict, Mode* mode, const CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth)
{/*
	CUData* cu = &mode->cu;
	uint32_t log2TrSize = cu->m_log2CUSize[absPartIdx] - tuDepth;

	if (tuDepth < cu->m_tuDepth[absPartIdx])
	{
		uint32_t qNumParts = 1 << (log2TrSize - 1 - LOG2_UNIT_SIZE) * 2;
		uint32_t splitCbfU = 0, splitCbfV = 0;
		for (uint32_t qIdx = 0, qPartIdx = absPartIdx; qIdx < 4; ++qIdx, qPartIdx += qNumParts)
		{
			Search_residualQTIntraChroma(search, predict, mode, cuGeom, qPartIdx, tuDepth + 1);
			splitCbfU |= getCbf(cu, qPartIdx, TEXT_CHROMA_U, tuDepth + 1);
			splitCbfV |= getCbf(cu, qPartIdx, TEXT_CHROMA_V, tuDepth + 1);
		}
		for (uint32_t offs = 0; offs < 4 * qNumParts; offs++)
		{
			cu->m_cbf[1][absPartIdx + offs] |= (splitCbfU << tuDepth);
			cu->m_cbf[2][absPartIdx + offs] |= (splitCbfV << tuDepth);
		}

		return;
	}

	uint32_t log2TrSizeC = log2TrSize - predict->m_hChromaShift;
	uint32_t tuDepthC = tuDepth;
	if (log2TrSizeC < 2)
	{
		X265_CHECK(log2TrSize == 2 && predict->m_csp != X265_CSP_I444 && tuDepth, "invalid tuDepth\n");
		if (absPartIdx & 3)
			return;
		log2TrSizeC = 2;
		tuDepthC--;
	}

	ShortYuv* resiYuv = &search->m_rqt[cuGeom->depth].tmpResiYuv;
	uint32_t stride = mode->fencYuv->m_csize;
	const uint32_t sizeIdxC = log2TrSizeC - 2;

	uint32_t curPartNum = cuGeom->numPartitions >> tuDepthC * 2;
	const SplitType splitType = (predict->m_csp == X265_CSP_I422) ? VERTICAL_SPLIT : DONT_SPLIT;

	TURecurse tuIterator;
	TURecurse_init(&tuIterator, splitType, curPartNum, absPartIdx);
	do
	{
		uint32_t absPartIdxC = tuIterator.absPartIdxTURelCU;

		struct IntraNeighbors intraNeighbors;
		initIntraNeighbors(cu, absPartIdxC, tuDepthC, FALSE, &intraNeighbors);

		for (uint32_t chromaId = TEXT_CHROMA_U; chromaId <= TEXT_CHROMA_V; chromaId++)
		{
			TextType ttype = (TextType)chromaId;

			const pixel* fenc = Yuv_getChromaAddr(mode->fencYuv, chromaId, absPartIdxC);
			pixel*   pred = Yuv_getChromaAddr(&mode->predYuv, chromaId, absPartIdxC);
			int16_t* residual = ShortYuv_getChromaAddr(resiYuv, chromaId, absPartIdxC);
			uint32_t coeffOffsetC = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (predict->m_hChromaShift + predict->m_vChromaShift));
			coeff_t* coeffC = cu->m_trCoeff[ttype] + coeffOffsetC;
			pixel*   picReconC = Picyuv_CUgetChromaAddr(search->m_frame->m_reconPic, chromaId, cu->m_cuAddr, cuGeom->absPartIdx + absPartIdxC);
			intptr_t picStride = search->m_frame->m_reconPic->m_strideC;

			uint32_t chromaPredMode = cu->m_chromaIntraDir[absPartIdxC];
			if (chromaPredMode == DM_CHROMA_IDX)
				chromaPredMode = cu->m_lumaIntraDir[(predict->m_csp == X265_CSP_I444) ? absPartIdxC : 0];
			if (predict->m_csp == X265_CSP_I422)
				chromaPredMode = g_chroma422IntraAngleMappingTable[chromaPredMode];

			// init availability pattern
			initAdiPatternChroma(predict, cu, cuGeom, absPartIdxC, &intraNeighbors, chromaId);

			// get prediction signal
			predIntraChromaAng(predict, chromaPredMode, pred, stride, log2TrSizeC);

			X265_CHECK(!cu->m_transformSkip[ttype][0], "transform skip not supported at low RD levels\n");

			primitives.cu[sizeIdxC].calcresidual(fenc, pred, residual, stride, pow(2, log2TrSizeC));
			uint32_t numSig = Quant_transformNxN(&search->m_quant, cu, fenc, stride, residual, stride, coeffC, log2TrSizeC, ttype, absPartIdxC, FALSE);
			if (numSig)
			{
				Quant_invtransformNxN(&search->m_quant, residual, stride, coeffC, log2TrSizeC, ttype, TRUE, FALSE, numSig);
				primitives.cu[sizeIdxC].add_ps(picReconC, picStride, pred, residual, stride, stride, pow(2, log2TrSizeC), pow(2, log2TrSizeC));
				setCbfPartRange(cu, 1 << tuDepth, ttype, absPartIdxC, tuIterator.absPartIdxStep);
			}
			else
			{
				// no coded residual, recon = pred
				primitives.cu[sizeIdxC].copy_pp(picReconC, picStride, pred, stride, pow(2, log2TrSizeC), pow(2, log2TrSizeC));
				setCbfPartRange(cu, 0, ttype, absPartIdxC, tuIterator.absPartIdxStep);
			}
		}
	} while (isNextSection(&tuIterator));

	if (splitType == VERTICAL_SPLIT)
	{
		offsetSubTUCBFs(search, cu, TEXT_CHROMA_U, tuDepth, absPartIdx);
		offsetSubTUCBFs(search, cu, TEXT_CHROMA_V, tuDepth, absPartIdx);
	}*/
}
void Search_checkDQPForSplitPred(Search *search, Mode* mode, const CUGeom* cuGeom)
{/*
	CUData* cu = &mode->cu;

	if ((cuGeom->depth == cu->m_slice->m_pps->maxCuDQPDepth) && cu->m_slice->m_pps->bUseDQP)
	{
		bool hasResidual = FALSE;

		// Check if any sub-CU has a non-zero QP //
		for (uint32_t blkIdx = 0; blkIdx < cuGeom->numPartitions; blkIdx++)
		{
			if (getQtRootCbf(cu, blkIdx))
			{
				hasResidual = TRUE;
				break;
			}
		}
		if (hasResidual)
		{
			if (search->m_param->rdLevel >= 3)
			{
				entropy_resetBits(&mode->contexts);
				codeDeltaQP(&mode->contexts, cu, 0);
				uint32_t bits = entropy_getNumberOfWrittenBits(&mode->contexts);
				mode->mvBits += bits;
				mode->totalBits += bits;
				updateModeCost(search, mode);
			}
			else if (search->m_param->rdLevel <= 1)
			{
				mode->sa8dBits++;
				mode->sa8dCost = calcRdSADCost(&search->m_rdCost, mode->distortion, mode->sa8dBits);
			}
			else
			{
				mode->mvBits++;
				mode->totalBits++;
				updateModeCost(search, mode);
			}
			// For all zero CBF sub-CUs, reset QP to RefQP (so that deltaQP is not signalled).
			//When the non-zero CBF sub-CU is found, stop //
			CUData_setQPSubCUs(cu, getRefQP(cu, 0), 0, cuGeom->depth);
		}
		else
			// No residual within this CU or subCU, so reset QP to RefQP //
			setQPSubParts(cu, getRefQP(cu, 0), 0, cuGeom->depth);
	}*/
}
