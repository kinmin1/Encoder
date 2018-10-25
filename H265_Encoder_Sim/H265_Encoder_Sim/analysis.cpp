/*
* analysis.c
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#include "common.h"
#include "analysis.h"
#include "constants.h"
#include "search.h"
#include "yuv.h"
#include "primitives.h"
#include "math.h"
#include "frame.h"
#include <string.h>
#include "mv.h"
#include "cudata.h"
#include "slice.h"

extern EncoderPrimitives primitives;
bool Analysis_create(Analysis *analysis)
{/*
	analysis->m_bChromaSa8d = analysis->sear.m_param->rdLevel >= 3;

	int csp = 1;//m_param->internalCsp;
	uint32_t cuSize = g_maxCUSize;

	bool ok = TRUE;
	for (uint32_t depth = 0; depth <= 0; depth++, cuSize >>= 1)//depth <= g_maxCUDepth;
	{
		struct ModeDepth *md = &(analysis->m_modeDepth[depth]);

		CUDataMemPool_create_analysis(&md->cuMemPool, depth, 2);//MAX_PRED_TYPES
		ok &= Yuv_create_md(&md->fencYuv, cuSize, csp);

		CUData_initialize(&md->pred[PRED_INTRA].cu, &md->cuMemPool, depth, 0);
		ok &= Yuv_create_md_pred_0(&md->pred[PRED_INTRA].predYuv, cuSize, csp);
		ok &= Yuv_create_md_pred_1(&md->pred[PRED_INTRA].reconYuv, cuSize, csp);
		md->pred[PRED_INTRA].fencYuv = &md->fencYuv;

		CUData_initialize(&md->pred[PRED_2Nx2N].cu, &md->cuMemPool, depth, 1);
		ok &= Yuv_create_md_pred_pred2Nx2N_0(&md->pred[PRED_2Nx2N].predYuv, cuSize, csp);
		ok &= Yuv_create_md_pred_pred2Nx2N_1(&md->pred[PRED_2Nx2N].reconYuv, cuSize, csp);
		md->pred[PRED_2Nx2N].fencYuv = &md->fencYuv;
	}
	
	return ok;*/return 0;
}


/* check whether current mode is the new best */
void checkBestMode(Analysis* analysis, Mode* mode, uint32_t depth)
{
	//X265_CHECK(ok(mode), "mode costs are uninitialized\n");
	/*
	struct ModeDepth* md = &(analysis->m_modeDepth[depth]);
	if (md->bestMode)
	{
		if (mode->rdCost < md->bestMode->rdCost)
			md->bestMode = mode;
	}
	else
		md->bestMode = mode;*/
}

Mode* compressCTU(Analysis* analysis, CUData* ctu, Frame* frame, CUGeom* cuGeom, Entropy* initialContext)
{/*
	analysis->sear.m_slice = ctu->m_slice;
	analysis->sear.m_frame = frame;

	int qp = setLambdaFromQP(&analysis->sear, ctu, 26);

	setQPSubParts(ctu, (int8_t)qp, 0, 0);

	load(&(analysis->sear.m_rqt[0].cur), initialContext);
	Yuv_copyFromPicYuv(&(analysis->m_modeDepth[0].fencYuv), analysis->sear.m_frame->m_fencPic, ctu->m_cuAddr, 0);

	uint32_t numPartition = ctu->m_numPartitions;
	if (analysis->sear.m_param->analysisMode)
	{
	if (analysis->sear.m_slice->m_sliceType == I_SLICE)
	analysis->m_reuseIntraDataCTU = (analysis_intra_data*)analysis->sear.m_frame->m_analysisData.intraData;
	else
	{
	int numPredDir = isInterP(analysis->sear.m_slice) ? 1 : 2;
	analysis->m_reuseInterDataCTU = (analysis_inter_data*)analysis->sear.m_frame->m_analysisData.interData;
	analysis->m_reuseRef = &(analysis->m_reuseInterDataCTU->ref[ctu->m_cuAddr * X265_MAX_PRED_MODE_PER_CTU * numPredDir]);
	analysis->m_reuseBestMergeCand = &(analysis->m_reuseInterDataCTU->bestMergeCand[ctu->m_cuAddr * MAX_GEOMS]);
	}
	}

	uint32_t zOrder = 0;
	if (analysis->sear.m_slice->m_sliceType == I_SLICE)
	{
	compressIntraCU(analysis, ctu, cuGeom, zOrder, qp);

	if (analysis->sear.m_param->analysisMode == X265_ANALYSIS_SAVE && analysis->sear.m_frame->m_analysisData.intraData)
	{
	CUData* bestCU = &(analysis->m_modeDepth[0].bestMode->cu);
	memcpy(&(analysis->m_reuseIntraDataCTU->depth[ctu->m_cuAddr * numPartition]), bestCU->m_cuDepth, sizeof(uint8_t) * numPartition);
	memcpy(&(analysis->m_reuseIntraDataCTU->modes[ctu->m_cuAddr * numPartition]), bestCU->m_lumaIntraDir, sizeof(uint8_t) * numPartition);
	memcpy(&(analysis->m_reuseIntraDataCTU->partSizes[ctu->m_cuAddr * numPartition]), bestCU->m_partSize, sizeof(uint8_t) * numPartition);
	memcpy(&(analysis->m_reuseIntraDataCTU->chromaModes[ctu->m_cuAddr * numPartition]), bestCU->m_chromaIntraDir, sizeof(uint8_t) * numPartition);
	}
	}
	else
	{
	compressInterCU_rd0_4(&analysis->sear.predict, &analysis->sear, analysis, ctu, cuGeom, qp, frame);
	}

	return analysis->m_modeDepth[0].bestMode;*/return 0;
}



void compressIntraCU(Analysis* analysis, CUData* parentCTU, CUGeom* cuGeom, uint32_t zOrder, int32_t qp)
{/*
	uint32_t depth = cuGeom->depth;//当前CU深度
	struct ModeDepth* md = &(analysis->m_modeDepth[depth]);
	md->bestMode = NULL;

	bool mightSplit = 0;
	bool mightNotSplit = 1;

	if (analysis->sear.m_param->analysisMode == X265_ANALYSIS_LOAD)
	{
		uint8_t* reuseDepth = &(analysis->m_reuseIntraDataCTU->depth[parentCTU->m_cuAddr * parentCTU->m_numPartitions]);
		uint8_t* reuseModes = &(analysis->m_reuseIntraDataCTU->modes[parentCTU->m_cuAddr * parentCTU->m_numPartitions]);
		char* reusePartSizes = &(analysis->m_reuseIntraDataCTU->partSizes[parentCTU->m_cuAddr * parentCTU->m_numPartitions]);
		uint8_t* reuseChromaModes = &(analysis->m_reuseIntraDataCTU->chromaModes[parentCTU->m_cuAddr * parentCTU->m_numPartitions]);

		if (mightNotSplit && depth == reuseDepth[zOrder] && zOrder == cuGeom->absPartIdx)
		{
			PartSize size = (PartSize)reusePartSizes[zOrder];
			Mode* mode = (size == SIZE_2Nx2N) ? &(md->pred[PRED_INTRA]) : &(md->pred[PRED_INTRA_NxN]);
			CUData_initSubCU(&mode->cu, parentCTU, cuGeom, qp);
			checkIntra(&analysis->sear, mode, cuGeom, size, &reuseModes[zOrder], &reuseChromaModes[zOrder]);
			checkBestMode(analysis, mode, depth);

			if (mightSplit)
				addSplitFlagCost(analysis, md->bestMode, cuGeom->depth);//熵编码部分

			// increment zOrder offset to point to next best depth in sharedDepth buffer
			zOrder += g_depthInc[g_maxCUDepth - 1][reuseDepth[zOrder]];
			mightSplit = FALSE;
		}
	}

	if (mightNotSplit)
	{
		CUData_initSubCU(&md->pred[PRED_INTRA].cu, parentCTU, cuGeom, qp);
		checkIntra(&analysis->sear, &md->pred[PRED_INTRA], cuGeom, SIZE_2Nx2N, NULL, NULL);
		checkBestMode(analysis, &md->pred[PRED_INTRA], depth);

		if (cuGeom->log2CUSize == 3 && analysis->sear.m_slice->m_sps->quadtreeTULog2MinSize < 3)
		{
			CUData_initSubCU(&md->pred[PRED_INTRA_NxN].cu, parentCTU, cuGeom, qp);
			checkIntra(&analysis->sear, &md->pred[PRED_INTRA_NxN], cuGeom, SIZE_NxN, NULL, NULL);
			checkBestMode(analysis, &md->pred[PRED_INTRA_NxN], depth);
		}

		if (mightSplit)
			addSplitFlagCost(analysis, md->bestMode, cuGeom->depth);
	}

	if (mightSplit)
	{
		Mode* splitPred = &md->pred[PRED_SPLIT];
		initCosts(splitPred);
		CUData* splitCU = &splitPred->cu;
		CUData_initSubCU(splitCU, parentCTU, cuGeom, qp);

		uint32_t nextDepth = depth + 1;
		struct ModeDepth* nd = &(analysis->m_modeDepth[nextDepth]);
		invalidateContexts(&analysis->sear, nextDepth);
		Entropy* nextContext = &(analysis->sear.m_rqt[depth].cur);
		int32_t nextQP = qp;

		for (uint32_t subPartIdx = 0; subPartIdx < 4; subPartIdx++)
		{
			CUGeom* childGeom = cuGeom + cuGeom->childOffset + subPartIdx;
			if (childGeom->flags & PRESENT)
			{
				Yuv_copyPartToYuv(&(analysis->m_modeDepth[0].fencYuv), &(nd->fencYuv), childGeom->absPartIdx);
				load(&(analysis->sear.m_rqt[nextDepth].cur), nextContext);

				compressIntraCU(analysis, parentCTU, childGeom, zOrder, nextQP);

				// Save best CU and pred data for this sub CU
				CUData_copyPartFrom(splitCU, &(nd->bestMode->cu), childGeom, subPartIdx);
				addSubCosts(splitPred, nd->bestMode);
				Yuv_copyToPartYuv(&(nd->bestMode->reconYuv), &(splitPred->reconYuv), childGeom->numPartitions * subPartIdx);
				nextContext = &(nd->bestMode->contexts);
			}
			else
			{
				// record the depth of this non-present sub-CU
				CUData_setEmptyPart(splitCU, childGeom, subPartIdx);
				zOrder += g_depthInc[g_maxCUDepth - 1][nextDepth];
			}
		}
		store(nextContext, &(splitPred->contexts));
		if (mightNotSplit)
			addSplitFlagCost(analysis, splitPred, cuGeom->depth);
		else
			updateModeCost(&(analysis->sear), splitPred);

		//checkDQPForSplitPred(*splitPred, cuGeom);
		checkBestMode(analysis, splitPred, depth);
	}

	// Copy best data to encData CTU and recon
	CUData_copyToPic(&(md->bestMode->cu), depth);
	if (md->bestMode != &(md->pred[PRED_SPLIT]))
		Yuv_copyToPicYuv(&(md->bestMode->reconYuv), analysis->sear.m_frame->m_reconPic, parentCTU->m_cuAddr, cuGeom->absPartIdx);
*/
}

//帧间预测
void compressInterCU_rd0_4(struct Predict *predict, struct Search* search, struct Analysis* analysis, struct CUData* parentCTU, CUGeom* cuGeom, int32_t qp, struct Frame *frame)
{/*
	uint32_t depth = cuGeom->depth;
	uint32_t cuAddr = parentCTU->m_cuAddr;
	struct ModeDepth *md = &(analysis->m_modeDepth[depth]);
	md->bestMode = NULL;

	bool mightSplit = FALSE;
	bool mightNotSplit = TRUE;
	uint32_t minDepth = Analysis_topSkipMinDepth(search, parentCTU, cuGeom);

	if (mightNotSplit && depth >= minDepth)
	{
		bool bTryIntra = 0;

		bool earlyskip = FALSE;

		if (!earlyskip)
		{
			CUData_initSubCU(&md->pred[PRED_2Nx2N].cu, parentCTU, cuGeom, qp);
			Analysis_checkInter_rd0_4(analysis, search, &md->pred[PRED_2Nx2N], cuGeom, SIZE_2Nx2N);

			Mode *bestInter = &md->pred[PRED_2Nx2N];

			if (search->m_param->rdLevel >= 3)
			{
				// Calculate RD cost of best inter option //
				if (!analysis->m_bChromaSa8d) // When m_bChromaSa8d is enabled, chroma MC has already been done //
				{
					for (uint32_t puIdx = 0; puIdx < getNumPartInter(&bestInter->cu); puIdx++)
					{
						struct PredictionUnit pu;
						PredictionUnit_PredictionUnit(&pu, &bestInter->cu, cuGeom, puIdx);
						Predict_motionCompensation(predict, &bestInter->cu, &pu, &bestInter->predYuv, FALSE, TRUE);
					}
				}
				Search_encodeResAndCalcRdInterCU(search, bestInter, cuGeom);
				checkBestMode(analysis, bestInter, depth);
			}
			else
			{
				// SA8D choice between merge/skip, inter, bidir, and intra //
				if (!md->bestMode || bestInter->sa8dCost < md->bestMode->sa8dCost)
					md->bestMode = bestInter;

				// finally code the best mode selected by SA8D costs:
				// RD level 2 - fully encode the best mode
				// RD level 1 - generate recon pixels
				// RD level 0 - generate chroma prediction //
				if (md->bestMode->cu.m_mergeFlag[0] && md->bestMode->cu.m_partSize[0] == SIZE_2Nx2N)
				{
					// prediction already generated for this CU, and if rd level
					// is not 0, it is already fully encoded //
				}
				else if (isInter_cudata(&md->bestMode->cu, 0))
				{
					for (uint32_t puIdx = 0; puIdx < getNumPartInter(&md->bestMode->cu); puIdx++)
					{
						struct PredictionUnit pu;
						PredictionUnit_PredictionUnit(&pu, &md->bestMode->cu, cuGeom, puIdx);
						Predict_motionCompensation(&search->predict, &md->bestMode->cu, &pu, &md->bestMode->predYuv, FALSE, TRUE);//色度补偿
					}
					if (search->m_param->rdLevel == 2)
						Search_encodeResAndCalcRdInterCU(search, md->bestMode, cuGeom);
					else if (search->m_param->rdLevel == 1)
					{
						// generate recon pixels with no rate distortion considerations //
						CUData* cu = &md->bestMode->cu;

						uint32_t tuDepthRange[2];
						CUData_getInterTUQtDepthRange(cu, tuDepthRange, 0);

						ShortYuv_subtract(&search->m_rqt[cuGeom->depth].tmpResiYuv, md->bestMode->fencYuv, &md->bestMode->predYuv, cuGeom->log2CUSize);
						Search_residualTransformQuantInter(search, predict, md->bestMode, cuGeom, 0, 0, tuDepthRange);

						if (getQtRootCbf(cu, 0))
							Yuv_addClip(&md->bestMode->reconYuv, &md->bestMode->predYuv, &search->m_rqt[cuGeom->depth].tmpResiYuv, cu->m_log2CUSize[0]);
						else
						{
							Yuv_copyFromYuv(&md->bestMode->reconYuv, md->bestMode->predYuv);
							if (cu->m_mergeFlag[0] && cu->m_partSize[0] == SIZE_2Nx2N)
								setPredModeSubParts(cu, MODE_SKIP);
						}
					}
				}
				else
				{
					if (search->m_param->rdLevel == 2)
						Search_encodeIntraInInter(search, md->bestMode, cuGeom);
					else if (search->m_param->rdLevel == 1)
					{
						// generate recon pixels with no rate distortion considerations //
						CUData* cu = &md->bestMode->cu;

						uint32_t tuDepthRange[2];
						CUData_getIntraTUQtDepthRange(cu, tuDepthRange, 0);

						Search_residualTransformQuantIntra(search, predict, md->bestMode, cuGeom, 0, 0, tuDepthRange);
						Search_getBestIntraModeChroma(predict, md->bestMode, cuGeom);
						Search_residualQTIntraChroma(search, predict, md->bestMode, cuGeom, 0, 0);
						Yuv_copyFromPicYuv(&md->bestMode->reconYuv, search->m_frame->m_reconPic, cu->m_cuAddr, cuGeom->absPartIdx); // TODO:
					}
				}
			}

		} // !earlyskip

		if (mightSplit)
			addSplitFlagCost(analysis, md->bestMode, cuGeom->depth);

	}

	bool bNoSplit = FALSE;
	if (md->bestMode)
	{
		bNoSplit = isSkipped(&md->bestMode->cu, 0);
		if (mightSplit && depth && depth >= minDepth && !bNoSplit)
			bNoSplit = Analysis_recursionDepthCheck(search, parentCTU, cuGeom, md->bestMode);
	}

	if (mightNotSplit)
	{
		// early-out statistics //
		FrameData* curEncData = search->m_frame->m_encData;
		RCStatCU* cuStat = &curEncData->m_cuStat[parentCTU->m_cuAddr];
		uint64_t temp = cuStat->avgCost[depth] * cuStat->count[depth];
		cuStat->count[depth] += 1;
		cuStat->avgCost[depth] = (temp + md->bestMode->rdCost) / cuStat->count[depth];
	}

	// Copy best data to encData CTU and recon //
	//X265_CHECK(ok(md->bestMode), "best mode is not ok");
	CUData_copyToPic(&md->bestMode->cu, depth);
	if (md->bestMode != &md->pred[PRED_SPLIT] && search->m_param->rdLevel)
		Yuv_copyToPicYuv(&md->bestMode->reconYuv, search->m_frame->m_reconPic, cuAddr, cuGeom->absPartIdx);
*/
}

void addSplitFlagCost(Analysis* aly, Mode* mode, uint32_t depth)
{/*
	if (aly->sear.m_param->rdLevel >= 3)
	{
		// code the split flag (0 or 1) and update bit costs //
		entropy_resetBits(&(mode->contexts));
		codeSplitFlag(&(mode->contexts), &(mode->cu), 0, depth);
		uint32_t bits = entropy_getNumberOfWrittenBits(&(mode->contexts));
		mode->mvBits += bits;
		mode->totalBits += bits;
		updateModeCost(&(aly->sear), mode);
	}
	else if (aly->sear.m_param->rdLevel <= 1)
	{
		mode->sa8dBits++;
		mode->sa8dCost = calcRdSADCost(&(aly->sear.m_rdCost), mode->distortion, mode->sa8dBits);
	}
	else
	{
		mode->mvBits++;
		mode->totalBits++;
		updateModeCost(&(aly->sear), mode);
	}*/
}

uint32_t Analysis_topSkipMinDepth(Search *search, struct CUData *parentCTU, struct CUGeom* cuGeom)
{/*
	// Do not attempt to code a block larger than the largest block in the
	// co-located CTUs in L0 and L1 //
	int currentQP = parentCTU->m_qp[0];
	int previousQP = currentQP;
	uint32_t minDepth0 = 4, minDepth1 = 4;
	uint32_t sum = 0;
	int numRefs = 0;
	if (parentCTU->m_slice->m_numRefIdx[0])
	{
	numRefs++;
	struct CUData* cu = framedata_getPicCTU(search->m_slice->m_refPicList[0][0]->m_encData, parentCTU->m_cuAddr);
	previousQP = cu->m_qp[0];
	if (!cu->m_cuDepth[cuGeom->absPartIdx])
	return 0;
	for (uint32_t i = 0; i < cuGeom->numPartitions; i += 4)
	{
	uint32_t d = cu->m_cuDepth[cuGeom->absPartIdx + i];
	minDepth0 = X265_MIN(d, minDepth0);
	sum += d;
	}
	}
	if (parentCTU->m_slice->m_numRefIdx[1])
	{
	numRefs++;
	struct CUData* cu = framedata_getPicCTU(search->m_slice->m_refPicList[1][0]->m_encData, parentCTU->m_cuAddr);
	if (!cu->m_cuDepth[cuGeom->absPartIdx])
	return 0;
	for (uint32_t i = 0; i < cuGeom->numPartitions; i += 4)
	{
	uint32_t d = cu->m_cuDepth[cuGeom->absPartIdx + i];
	minDepth1 = X265_MIN(d, minDepth1);
	sum += d;
	}
	}
	if (!numRefs)
	return 0;

	uint32_t minDepth = X265_MIN(minDepth0, minDepth1);
	uint32_t thresh = minDepth * numRefs * (cuGeom->numPartitions >> 2);

	// allow block size growth if QP is raising or avg depth is
	// less than 1.5 of min depth //
	if (minDepth && currentQP >= previousQP && (sum <= thresh + (thresh >> 1)))
	minDepth -= 1;

	return minDepth;*/return 0;
}
/* sets md.bestMode if a valid merge candidate is found, else leaves it NULL */
void Analysis_checkMerge2Nx2N_rd0_4(struct Search *search, struct CUData *cu, struct Mode* skip, struct Mode* merge, struct CUGeom* cuGeom, struct Analysis *analysis)
{/*
	uint32_t depth = cuGeom->depth;
	struct ModeDepth* md = &analysis->m_modeDepth[depth];
	Yuv *fencYuv = &md->fencYuv;

	// Note that these two Mode instances are named MERGE and SKIP but they may
	// hold the reverse when the function returns. We toggle between the two modes //
	Mode* tempPred = merge;
	Mode* bestPred = skip;

	initCosts(tempPred);
	setPartSizeSubParts(&tempPred->cu, SIZE_2Nx2N);
	setPredModeSubParts(&tempPred->cu, MODE_INTER);
	tempPred->cu.m_mergeFlag[0] = TRUE;

	initCosts(bestPred);
	setPartSizeSubParts(&bestPred->cu, SIZE_2Nx2N);
	setPredModeSubParts(&bestPred->cu, MODE_INTER);
	bestPred->cu.m_mergeFlag[0] = TRUE;

	struct MVField candMvField[MRG_MAX_NUM_CANDS][2]; // double length for mv of both lists
	uint8_t candDir[MRG_MAX_NUM_CANDS];
	uint32_t numMergeCand = CUData_getInterMergeCandidates(&tempPred->cu, 0, 0, candMvField, candDir);

	struct PredictionUnit pu;// = (struct PredictionUnit *)malloc(sizeof(struct PredictionUnit));
	PredictionUnit_PredictionUnit(&pu, &merge->cu, cuGeom, 0);

	bestPred->sa8dCost = MAX_INT64;
	int bestSadCand = -1;
	int sizeIdx = cuGeom->log2CUSize - 2;

	for (uint32_t i = 0; i < numMergeCand; ++i)
	{
		if (search->m_bFrameParallel &&
			(candMvField[i][0].mv.y >= (search->m_param->searchRange + 1) * 4 ||
			candMvField[i][1].mv.y >= (search->m_param->searchRange + 1) * 4))
			continue;

		tempPred->cu.m_mvpIdx[0][0] = (uint8_t)i; // merge candidate ID is stored in L0 MVP idx
		tempPred->cu.m_interDir[0] = candDir[i];
		tempPred->cu.m_mv[0][0] = candMvField[i][0].mv;
		tempPred->cu.m_mv[1][0] = candMvField[i][1].mv;
		tempPred->cu.m_refIdx[0][0] = (int8_t)candMvField[i][0].refIdx;
		tempPred->cu.m_refIdx[1][0] = (int8_t)candMvField[i][1].refIdx;

		Predict_motionCompensation(&search->predict, &tempPred->cu, &pu, &tempPred->predYuv, TRUE, analysis->m_bChromaSa8d);
		//EncoderPrimitives primitives;
		tempPred->sa8dBits = getTUBits(i, numMergeCand);
		tempPred->distortion = primitives.cu[sizeIdx].sa8d(fencYuv->m_buf[0], fencYuv->m_size, tempPred->predYuv.m_buf[0], tempPred->predYuv.m_size, pow(2.0, 2 + sizeIdx), pow(2.0, 2 + sizeIdx));
		if (analysis->m_bChromaSa8d)
		{
			tempPred->distortion += primitives.chroma[search->predict.m_csp].cu[sizeIdx].sa8d(fencYuv->m_buf[1], fencYuv->m_csize, tempPred->predYuv.m_buf[1], tempPred->predYuv.m_csize, pow(2.0, 2 + sizeIdx), pow(2.0, 2 + sizeIdx));
			tempPred->distortion += primitives.chroma[search->predict.m_csp].cu[sizeIdx].sa8d(fencYuv->m_buf[2], fencYuv->m_csize, tempPred->predYuv.m_buf[2], tempPred->predYuv.m_csize, pow(2.0, 2 + sizeIdx), pow(2.0, 2 + sizeIdx));
		}
		tempPred->sa8dCost = calcRdSADCost(&search->m_rdCost, tempPred->distortion, tempPred->sa8dBits);

		if (tempPred->sa8dCost < bestPred->sa8dCost)
		{
			bestSadCand = i;
			std_swap(tempPred, bestPred);
		}
	}

	// force mode decision to take inter or intra //
	if (bestSadCand < 0)
		return;

	// calculate the motion compensation for chroma for the best mode selected //
	if (!analysis->m_bChromaSa8d) // Chroma MC was done above //
		Predict_motionCompensation(&search->predict, &bestPred->cu, &pu, &bestPred->predYuv, FALSE, TRUE);

	if (search->m_param->rdLevel)
	{
		if (search->m_param->bLossless)
			bestPred->rdCost = MAX_INT64;
		else
			Search_encodeResAndCalcRdSkipCU(search, &search->predict, bestPred);

		// Encode with residual //
		tempPred->cu.m_mvpIdx[0][0] = (uint8_t)bestSadCand;
		CUData_setPUInterDir(&tempPred->cu, candDir[bestSadCand], 0, 0);
		CUData_setPUMv(&tempPred->cu, 0, &candMvField[bestSadCand][0].mv, 0, 0);
		CUData_setPUMv(&tempPred->cu, 1, &candMvField[bestSadCand][1].mv, 0, 0);
		CUData_setPURefIdx(&tempPred->cu, 0, (int8_t)&candMvField[bestSadCand][0].refIdx, 0, 0);
		CUData_setPURefIdx(&tempPred->cu, 1, (int8_t)&candMvField[bestSadCand][1].refIdx, 0, 0);
		tempPred->sa8dCost = bestPred->sa8dCost;
		tempPred->sa8dBits = bestPred->sa8dBits;
		Yuv_copyFromYuv(&tempPred->predYuv, bestPred->predYuv);

		Search_encodeResAndCalcRdInterCU(search, tempPred, cuGeom);

		md->bestMode = tempPred->rdCost < bestPred->rdCost ? tempPred : bestPred;
	}
	else
		md->bestMode = bestPred;

	// broadcast sets of MV field data //
	CUData_setPUInterDir(&md->bestMode->cu, candDir[bestSadCand], 0, 0);
	CUData_setPUMv(&md->bestMode->cu, 0, &candMvField[bestSadCand][0].mv, 0, 0);
	CUData_setPUMv(&md->bestMode->cu, 1, &candMvField[bestSadCand][1].mv, 0, 0);
	CUData_setPURefIdx(&md->bestMode->cu, 0, (int8_t)candMvField[bestSadCand][0].refIdx, 0, 0);
	CUData_setPURefIdx(&md->bestMode->cu, 1, (int8_t)candMvField[bestSadCand][1].refIdx, 0, 0);
	checkDQP(search, md->bestMode, cuGeom);
	*/
}
void std_swap(Mode *a, Mode*b)
{/*
	Mode temp;
	temp = *a;
	*a = *b;
	*b = temp;
	*/
}

void Analysis_checkInter_rd0_4(struct Analysis* analysis, Search *search, Mode* interMode, CUGeom* cuGeom, PartSize partSize)
{/*
	initCosts(interMode);
	setPartSizeSubParts(&interMode->cu, partSize);
	setPredModeSubParts(&interMode->cu, MODE_INTER);
	int numPredDir = isInterP(search->m_slice) ? 1 : 2;

	if (search->m_param->analysisMode == X265_ANALYSIS_LOAD && analysis->m_reuseInterDataCTU)
	{
		for (uint32_t part = 0; part < getNumPartInter(&interMode->cu); part++)
		{
			MotionData* bestME = interMode->bestME[part];
			for (int32_t i = 0; i < numPredDir; i++)
			{
				bestME[i].ref = *analysis->m_reuseRef;
				analysis->m_reuseRef++;
			}
		}
	}
	Search_predInterSearch(search, interMode, cuGeom, analysis->m_bChromaSa8d);

	// predInterSearch sets interMode.sa8dBits //
	Yuv* fencYuv = interMode->fencYuv;
	Yuv* predYuv = &interMode->predYuv;
	int part = partitionFromLog2Size(cuGeom->log2CUSize);
	interMode->distortion = primitives.cu[part].sa8d(fencYuv->m_buf[0], fencYuv->m_size, predYuv->m_buf[0], predYuv->m_size, pow(2.0, partSize), pow(2.0, partSize));
	if (analysis->m_bChromaSa8d)
	{
		interMode->distortion += primitives.chroma[search->predict.m_csp].cu[part].sa8d(fencYuv->m_buf[1], fencYuv->m_csize, predYuv->m_buf[1], predYuv->m_csize, pow(2.0, partSize), pow(2.0, partSize));
		interMode->distortion += primitives.chroma[search->predict.m_csp].cu[part].sa8d(fencYuv->m_buf[2], fencYuv->m_csize, predYuv->m_buf[2], predYuv->m_csize, pow(2.0, partSize), pow(2.0, partSize));
	}
	interMode->sa8dCost = calcRdSADCost(&search->m_rdCost, interMode->distortion, interMode->sa8dBits);

	if (search->m_param->analysisMode == X265_ANALYSIS_SAVE && analysis->m_reuseInterDataCTU)
	{
		for (uint32_t puIdx = 0; puIdx < getNumPartInter(&interMode->cu); puIdx++)
		{
			MotionData* bestME = interMode->bestME[puIdx];
			for (int32_t i = 0; i < numPredDir; i++)
			{
				*analysis->m_reuseRef = bestME[i].ref;
				analysis->m_reuseRef++;
			}
		}
	}*/
}
/* returns true if recursion should be stopped */
bool Analysis_recursionDepthCheck(Search *search, const CUData* parentCTU, const CUGeom* cuGeom, Mode* bestMode)
{
	/* early exit when the RD cost of best mode at depth n is less than the sum
	* of average of RD cost of the neighbor CU's(above, aboveleft, aboveright,
	* left, colocated) and avg cost of that CU at depth "n" with weightage for
	* each quantity */
	/*
	uint32_t depth = cuGeom->depth;
	FrameData* curEncData = search->m_frame->m_encData;
	RCStatCU* cuStat = &curEncData->m_cuStat[parentCTU->m_cuAddr];
	uint64_t cuCost = cuStat->avgCost[depth] * cuStat->count[depth];
	uint64_t cuCount = cuStat->count[depth];

	uint64_t neighCost = 0, neighCount = 0;
	const CUData* above = parentCTU->m_cuAbove;
	if (above)
	{
		RCStatCU* astat = &curEncData->m_cuStat[above->m_cuAddr];
		neighCost += astat->avgCost[depth] * astat->count[depth];
		neighCount += astat->count[depth];

		const CUData* aboveLeft = parentCTU->m_cuAboveLeft;
		if (aboveLeft)
		{
			RCStatCU* lstat = &curEncData->m_cuStat[aboveLeft->m_cuAddr];
			neighCost += lstat->avgCost[depth] * lstat->count[depth];
			neighCount += lstat->count[depth];
		}

		const CUData* aboveRight = parentCTU->m_cuAboveRight;
		if (aboveRight)
		{
			RCStatCU* rstat = &curEncData->m_cuStat[aboveRight->m_cuAddr];
			neighCost += rstat->avgCost[depth] * rstat->count[depth];
			neighCount += rstat->count[depth];
		}
	}
	const CUData* left = parentCTU->m_cuLeft;
	if (left)
	{
		RCStatCU* nstat = &curEncData->m_cuStat[left->m_cuAddr];
		neighCost += nstat->avgCost[depth] * nstat->count[depth];
		neighCount += nstat->count[depth];
	}

	// give 60% weight to all CU's and 40% weight to neighbour CU's
	if (neighCount + cuCount)
	{
		uint64_t avgCost = ((3 * cuCost) + (2 * neighCost)) / ((3 * cuCount) + (2 * neighCount));
		uint64_t curCost = search->m_param->rdLevel > 1 ? bestMode->rdCost : bestMode->sa8dCost;
		if (curCost < avgCost && avgCost)
			return TRUE;
	}
	*/
	return FALSE;
}
