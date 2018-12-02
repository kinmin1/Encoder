/*
* analysis.h
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#ifndef ANALYSIS_H_
#define ANALYSIS_H_

#include "common.h"
#include "predict.h"
#include "shortyuv.h"
#include "search.h"

enum {
	PRED_MERGE,
	PRED_SKIP,
	PRED_INTRA,
	PRED_2Nx2N,
	PRED_BIDIR,
	PRED_Nx2N,
	PRED_2NxN,
	PRED_SPLIT,
	PRED_2NxnU,
	PRED_2NxnD,
	PRED_nLx2N,
	PRED_nRx2N,
	PRED_INTRA_NxN, /* 4x4 intra PU blocks for 8x8 CU */
	PRED_LOSSLESS,  /* lossless encode of best mode */
	MAX_PRED_TYPES
};

typedef struct Analysis
{
	Search sear;
	
	/* Analysis data for load/save modes, keeps getting incremented as CTU analysis proceeds and data is consumed or read */
	analysis_intra_data* m_reuseIntraDataCTU;//保存模式，不断增加CTU分析过程中的数据消耗或读取
	analysis_inter_data* m_reuseInterDataCTU;
	int32_t*             m_reuseRef;
	uint32_t*            m_reuseBestMergeCand;
	struct ModeDepth
	{
		Mode           pred[MAX_PRED_TYPES];
		Mode*          bestMode;
		Yuv            fencYuv;
		CUDataMemPool  cuMemPool;
	}ModeDepth;
	struct ModeDepth m_modeDepth[NUM_CU_DEPTH];
	bool      m_bTryLossless;
	bool      m_bChromaSa8d;

}Analysis;

Mode* compressCTU(Analysis* analysis, CUData* ctu, Frame* frame, CUGeom* cuGeom, Entropy* initialContext);

/* full analysis for an I-slice CU */
void compressIntraCU(Analysis* analysis, CUData* parentCTU, CUGeom* cuGeom, uint32_t zOrder, int32_t qp);

/* full analysis for a P or B slice CU */
void compressInterCU_rd0_4(struct Predict *predict, struct Search* search, struct Analysis* analysis, struct CUData* parentCTU, struct CUGeom* cuGeom, int32_t qp, struct Frame *frame);

/* measure merge and skip */
void Analysis_checkMerge2Nx2N_rd0_4(struct Search *search, struct CUData* cu, struct Mode* skip, struct Mode* merge, struct CUGeom* cuGeom, struct Analysis *analysis);

/* add the RD cost of coding a split flag (0 or 1) to the given mode */
void addSplitFlagCost(Analysis* aly, Mode* mode, uint32_t depth);

/* work-avoidance heuristics for RD levels < 5 */
uint32_t Analysis_topSkipMinDepth(Search *search, struct CUData *parentCTU, struct CUGeom* cuGeom);

/* generate residual and recon pixels for an entire CTU recursively (RD0) */

void checkBestMode(Analysis* analysis, Mode* mode, uint32_t depth);
bool Analysis_create(Analysis *analysis);

typedef struct ThreadLocalData
{
	Analysis analysis;
}ThreadLocalData;

void std_swap(Mode *a, Mode*b);
void Analysis_checkInter_rd0_4(struct Analysis* analysis, Search *search, Mode* interMode, CUGeom* cuGeom, PartSize partSize);
void checkBestMode(Analysis* analysis, Mode* mode, uint32_t depth);
bool Analysis_recursionDepthCheck(Search *search, const CUData* parentCTU, const CUGeom* cuGeom, Mode* bestMode);

#endif /* ANALYSIS_H_ */
