/*
* search.h
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#ifndef SEARCH_H_
#define SEARCH_H_

#include "common.h"
#include "predict.h"
#include "quant.h"
#include "bitcost.h"
#include "framedata.h"
#include "yuv.h"
#include "rdcost.h"
#include "entropy.h"
#include "motion.h"


/* All the CABAC contexts that Analysis needs to keep track of at each depth
* and temp buffers for residual, coeff, and recon for use during residual
* quad-tree depth recursion */
typedef struct RQTData
{
	Entropy  cur;     /* starting context for current CU */

	/* these are indexed by qtLayer (log2size - 2) so nominally 0=4x4, 1=8x8, 2=16x16, 3=32x32
	* the coeffRQT and reconQtYuv are allocated to the max CU size at every depth. The parts
	* which are reconstructed at each depth are valid. At the end, the transform depth table
	* is walked and the coeff and recon at the final split depths are collected */
	Entropy  rqtRoot;      /* residual quad-tree start context */
	Entropy  rqtTemp;      /* residual quad-tree temp context */
	Entropy  rqtTest;      /* residual quad-tree test context */
	coeff_t* coeffRQT[3];  /* coeff storage for entire CTU for each RQT layer */
	struct Yuv      reconQtYuv;   /* recon storage for entire CTU for each RQT layer (intra) */
	struct ShortYuv resiQtYuv;    /* residual storage for entire CTU for each RQT layer (inter) */

	/* per-depth temp buffers for inter prediction */
	struct ShortYuv tmpResiYuv;
	struct Yuv      tmpPredYuv;
	struct Yuv      bidirPredYuv[2];
}RQTData;


typedef struct MotionData
{
	MV       mv;
	MV       mvp;
	int      mvpIdx;
	int      ref;
	uint32_t cost;
	int      bits;
}MotionData;
#define MAX_INTER_PARTS  2
typedef struct Mode
{
	CUData     cu;
	struct Yuv       *fencYuv;
	struct Yuv        predYuv;
	struct Yuv        reconYuv;
	Entropy    contexts;

	MotionData bestME[MAX_INTER_PARTS][2];
	MV         amvpCand[2][MAX_NUM_REF][AMVP_NUM_CANDS];

	// Neighbour MVs of the current partition. 5 spatial candidates and the
	// temporal candidate.
	struct InterNeighbourMV interNeighbours[6];

	uint64_t   rdCost;     // sum of partition (psy) RD costs          (sse(fenc, recon) + lambda2 * bits)
	uint64_t   sa8dCost;   // sum of partition sa8d distortion costs   (sa8d(fenc, pred) + lambda * bits)
	uint32_t   sa8dBits;   // signal bits used in sa8dCost calculation
	uint32_t   psyEnergy;  // sum of partition psycho-visual energy difference
	uint32_t   distortion; // sum of partition SSE distortion
	uint32_t   totalBits;  // sum of partition bits (mv + coeff)
	uint32_t   mvBits;     // Mv bits + Ref + block type (or intra mode)
	uint32_t   coeffBits;  // Texture bits (DCT Coeffs)

}Mode;

/* intra helper functions */
#define MAX_RD_INTRA_MODES 16 

typedef struct Search //: public Predict
{
	//public:

	struct Predict predict;
	int16_t zeroShort[MAX_CU_SIZE];

	MotionEstimate  m_me;
	Quant           m_quant;
	RDCost          m_rdCost;
	x265_param*     m_param;
	Frame*          m_frame;
	Slice*          m_slice;

	Entropy         m_entropyCoder;
	RQTData         m_rqt[NUM_FULL_DEPTH];

	uint8_t*        m_qtTempCbf[3];
	uint8_t*        m_qtTempTransformSkipFlag[3];

	pixel*          m_fencScaled;     /* 32x32 buffer for down-scaled version of 64x64 CU fenc */
	pixel*          m_fencTransposed; /* 32x32 buffer for transposed copy of fenc */
	pixel*          m_intraPred;      /* 32x32 buffer for individual intra predictions */
	pixel*          m_intraPredAngs;  /* allocation for 33 consecutive (all angular) 32x32 intra predictions */

	coeff_t*        m_tsCoeff;        /* transform skip coeff 32x32 */
	int16_t*        m_tsResidual;     /* transform skip residual 32x32 */
	pixel*          m_tsRecon;        /* transform skip reconstructed pixels 32x32 */

	bool            m_bFrameParallel;
	bool            m_bEnableRDOQ;
	uint32_t        m_numLayers;
	uint32_t        m_refLagPixels;

#if DETAILED_CU_STATS
	/* Accumulate CU statistics separately for each frame encoder */
	CUStats         m_stats[X265_MAX_FRAME_THREADS];
#endif

	struct PME
	{
		struct Search*       master;
		Mode*         mode;
		const CUGeom* cuGeom;
		const struct PredictionUnit* pu;
		int           puIdx;
	}PME;

	/* motion estimation distribution */
	//ThreadLocalData* m_tld;

	uint32_t      m_listSelBits[3];
}Search;

typedef struct Cost
{
	uint64_t rdcost;
	uint32_t bits;
	uint32_t distortion;
	uint32_t energy;
	//Cost() { rdcost = 0; bits = 0; distortion = 0; energy = 0; }
}Cost;

/* output of mergeEstimation, best merge candidate */
typedef struct MergeData
{
	struct MVField  mvField[2];
	uint32_t dir;
	uint32_t index;
	uint32_t bits;
}MergeData;
void initCosts(Mode *mode);

bool initSearch(Search* search, x265_param* param, ScalingList *scalingList);
int  setLambdaFromQP(Search* search, CUData* ctu, int qp); /* returns real quant QP in valid spec range */

// mark temp RD entropy contexts as uninitialized; useful for finding loads without stores
void invalidateContexts(Search* sea, int fromDepth);

// select best intra mode using only sa8d costs, cannot measure NxN intra
void checkIntra(Search* search, Mode* intraMode, CUGeom* cuGeom, PartSize partSize, uint8_t* sharedModes, uint8_t* sharedChromaModes);

void Search_encodeResAndCalcRdSkipCU(Search *search, Predict *predict, Mode* interMode);
// encode residual without rd-cost
void Search_residualTransformQuantIntra(Search *search, Predict *predict, Mode* mode, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2]);

void Search_residualQTIntraChroma(Search *search, Predict *predict, Mode* mode, const CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth);
// pick be chroma mode from available using just sa8d costs
void Search_getBestIntraModeChroma(Predict *predict, Mode* intraMode, const CUGeom* cuGeom);

/* update CBF flags and QP values to be internally consistent */
void checkDQP(Search* search, Mode* mode, const CUGeom* cuGeom);

// RDO search of luma intra modes; result is fully encoded luma. luma distortion is returned
uint32_t estIntraPredQT(Search* search, Mode *intraMode, CUGeom* cuGeom, uint32_t depthRange[2], uint8_t* sharedModes);

// RDO select best chroma mode from luma; result is fully encode chroma. chroma distortion is returned
uint32_t estIntraPredChromaQT(Search* search, Mode *intraMode, const CUGeom* cuGeom, uint8_t* sharedChromaModes);

void codeSubdivCbfQTChroma(Search* search, CUData* cu, uint32_t tuDepth, uint32_t absPartIdx);
//void     codeInterSubdivCbfQT(CUData& cu, uint32_t absPartIdx, const uint32_t tuDepth, const uint32_t depthRange[2]);
void codeCoeffQTChroma(Search* search, CUData* cu, uint32_t tuDepth, uint32_t absPartIdx, enum TextType ttype);

// generate prediction, generate residual and recon. if bAllowSplit, find optimal RQT splits
void codeIntraLumaQT(Search* search, Mode* mode, CUGeom* cuGeom, uint32_t tuDepth, uint32_t absPartIdx, bool bAllowSplit, struct Cost* outCost, uint32_t depthRange[2]);
//void     codeIntraLumaTSkip(Mode* mode, const CUGeom* cuGeom, uint32_t tuDepth, uint32_t absPartIdx, Cost* costs);
void     extractIntraResultQT(Search* search, CUData* cu, struct Yuv* reconYuv, uint32_t tuDepth, uint32_t absPartIdx);

// generate chroma prediction, generate residual and recon
uint32_t codeIntraChromaQt(Search* search, Mode* mode, const CUGeom* cuGeom, uint32_t tuDepth, uint32_t absPartIdx, uint32_t* psyEnergy);
uint32_t codeIntraChromaTSkip(Mode* mode, const CUGeom* cuGeom, uint32_t tuDepth, uint32_t tuDepthC, uint32_t absPartIdx, uint32_t* psyEnergy);
void     extractIntraResultChromaQT(Search* search, CUData* cu, struct Yuv* reconYuv, uint32_t absPartIdx, uint32_t tuDepth);

void offsetCBFs(uint8_t subTUCBF[2]);

// reshuffle CBF flags after coding a pair of 4:2:2 chroma blocks
void     offsetSubTUCBFs(Search* search, CUData* cu, enum TextType ttype, uint32_t tuDepth, uint32_t absPartIdx);

void invalidate(Mode *mode);

bool ok(const Mode *mode);

void addSubCosts(Mode* dstMode, const Mode* subMode);

unsigned int getIntraRemModeBits(const Search* search, CUData* cu, uint32_t absPartIdx, uint32_t mpmModes[3], uint64_t* mpms);

void updateCandList(uint32_t mode, uint64_t cost, int maxCandCount, uint32_t* candModeList, uint64_t* candCostList);

void updateModeCost(const Search* sch, Mode* m);
int getTUBits(int idx, int numIdx);
void ShortYuv_subtract(ShortYuv* shortYuv, Yuv* srcYuv0, Yuv* srcYuv1, uint32_t log2Size);
void Search_estimateResidualQT(Search *search, Predict *predict, Mode* mode, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth, ShortYuv* resiYuv, struct Cost* outCosts, uint32_t depthRange[2]);
void Search_saveResidualQTData(Search *search, CUData* cu, ShortYuv* resiYuv, uint32_t absPartIdx, uint32_t tuDepth);
void Search_encodeResAndCalcRdInterCU(Search *search, Mode* interMode, CUGeom* cuGeom);
uint32_t Search_mergeEstimation(Search *search, CUData* cu, const CUGeom* cuGeom, struct PredictionUnit* pu, int puIdx, struct MergeData* m, int width, int height);
void Search_getBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3]);
int Search_selectMVP(Search *search, CUData* cu, struct PredictionUnit* pu, MV amvp[AMVP_NUM_CANDS], int list, int ref, int width, int height);
void Search_setSearchRange(Search *search, const CUData* cu, const MV* mvp, int merange, MV* mvmin, MV* mvmax);
void Search_predInterSearch(Search *search, Mode* interMode, const CUGeom* cuGeom, bool bChromaMC);
void Search_checkIntraInInter(Search *search, Mode* intraMode, CUGeom* cuGeom);
void Search_encodeIntraInInter(Search *search, Mode* intraMode, CUGeom* cuGeom);
void Search_residualTransformQuantInter(Search *search, Predict *predict, Mode* mode, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t tuDepth, const uint32_t depthRange[2]);
void Search_checkDQPForSplitPred(Search *search, Mode* mode, const CUGeom* cuGeom);


#endif /* SEARCH_H_ */
