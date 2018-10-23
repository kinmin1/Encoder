/*
* predict.h
*
*  Created on: 2015-10-26
*      Author: adminster
*/

#ifndef PREDICT_H_
#define PREDICT_H_
#include "quant.h"
#include "common.h"
#include "yuv.h"
#include "shortyuv.h"
#include "frame.h"

struct PredictionUnit
{
	uint32_t     ctuAddr;      // raster index of current CTU within its picture
	uint32_t     cuAbsPartIdx; // z-order offset of current CU within its CTU
	uint32_t     puAbsPartIdx; // z-order offset of current PU with its CU
	int          width;
	int          height;
};

void PredictionUnit_PredictionUnit(struct PredictionUnit *predictionUnit, struct CUData* cu, const CUGeom* cuGeom, int puIdx);
#define ADI_BUF_STRIDE  (2 * MAX_CU_SIZE + 1 + 15) // alignment to 16 bytes
typedef struct Predict
{

	/* Weighted prediction scaling values built from slice parameters (bitdepth scaled) */
	struct WeightValues
	{
		int w, o, offset, shift, round;
	}WeightValues;

	struct IntraNeighbors
	{
		int      numIntraNeighbor;
		int      totalUnits;
		int      aboveUnits;
		int      leftUnits;
		int      unitWidth;
		int      unitHeight;
		int      log2TrSize;
		bool     bNeighborFlags[4 * MAX_NUM_SPU_W + 1];
	}IntraNeighbors;

	struct ShortYuv  m_predShortYuv[2]; /* temporary storage for weighted prediction */
	int16_t*  m_immedVals;

	// Unfiltered/filtered neighbours of the current partition.
	pixel     intraNeighbourBuf[2][258];

	/* Slice information */
	int       m_csp;
	int       m_hChromaShift;
	int       m_vChromaShift;

}Predict;

bool allocBuffers(Predict* pred, int csp);

/* Angular Intra */
void predIntraLumaAng(Predict* pred, uint32_t dirMode, pixel* dst, int stride, uint32_t log2TrSize);
void predIntraChromaAng(Predict* pred, uint32_t dirMode, pixel* dst, int stride, uint32_t log2TrSizeC);
void initAdiPattern(Predict* predict, CUData* cu, CUGeom* cuGeom, uint32_t puAbsPartIdx, struct IntraNeighbors* intraNeighbors, int dirMode);
void initAdiPatternChroma(Predict* pdata, CUData* cu, const CUGeom* cuGeom, uint32_t puAbsPartIdx, struct IntraNeighbors* intraNeighbors, uint32_t chromaId);

/* Intra prediction helper functions */
void initIntraNeighbors(CUData* cu, uint32_t absPartIdx, uint32_t tuDepth, bool isLuma, struct IntraNeighbors *IntraNeighbors);
void fillReferenceSamples(pixel* adiOrigin, int picStride, struct IntraNeighbors* intraNeighbors, pixel dst[258]);
//void fillReferenceSamples_Chroma(pixel* adiOrigin, int picStride, struct IntraNeighbors* intraNeighbors, pixel dst[258]);

static bool isAboveLeftAvailable(CUData* cu, uint32_t partIdxLT, bool cip);

static int  isAboveAvailable(CUData* cu, uint32_t partIdxLT, uint32_t partIdxRT, bool* bValidFlags, bool cip);

static int  isLeftAvailable(CUData* cu, uint32_t partIdxLT, uint32_t partIdxLB, bool* bValidFlags, bool cip);

static int  isAboveRightAvailable(CUData* cu, uint32_t partIdxRT, bool* bValidFlags, uint32_t numUnits, bool cip);

static int  isBelowLeftAvailable(CUData* cu, uint32_t partIdxLB, bool* bValidFlags, uint32_t numUnits, bool cip);

void Predict_motionCompensation(struct Predict *predict, CUData* cu, struct PredictionUnit* pu, Yuv* predYuv, bool bLuma, bool bChroma);
void Predict_addWeightUni(struct PredictionUnit* pu, Yuv* predYuv, ShortYuv* srcYuv, struct WeightValues wp[3], bool bLuma, bool bChroma);
void Predict_addWeightUni(struct PredictionUnit* pu, Yuv* predYuv, ShortYuv* srcYuv, struct WeightValues wp[3], bool bLuma, bool bChroma);
void Predict_predInterLumaShort(struct PredictionUnit* pu, ShortYuv* dstSYuv, PicYuv* refPic, MV *mv);
void Predict_predInterLumaPixel(struct PredictionUnit* pu, Yuv* dstYuv, PicYuv* refPic, MV *mv);
void Predict_predInterChromaShort(struct Predict *predict, struct PredictionUnit* pu, ShortYuv* dstSYuv, PicYuv* refPic, MV *mv);
void Predict_predInterChromaPixel(struct Predict *predict, struct PredictionUnit* pu, Yuv* dstYuv, PicYuv* refPic, MV *mv);

#endif /* PREDICT_H_ */
