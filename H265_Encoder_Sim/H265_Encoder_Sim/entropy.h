/*
* entropy.h
*
*  Created on: 2015-7-30
*      Author: Eva Shen
*/

#ifndef ENTROPY_H_
#define ENTROPY_H_

#include "Bitstream.h"
#include "common.h"
#include "contexts.h"
#include "cudata.h"
#include "frame.h"
#include "slice.h"

struct ScalingList;

typedef enum SplitType
{
	DONT_SPLIT = 0,
	VERTICAL_SPLIT = 1,
	QUAD_SPLIT = 2,
	NUMBER_OF_SPLIT_MODES = 3
}SplitType;

typedef struct TURecurse
{
	uint32_t section;
	uint32_t splitMode;
	uint32_t absPartIdxTURelCU;
	uint32_t absPartIdxStep;
}TURecurse;


typedef struct EstBitsSbac
{
	int significantCoeffGroupBits[NUM_SIG_CG_FLAG_CTX][2];
	int significantBits[2][NUM_SIG_FLAG_CTX];
	int lastBits[2][10];
	int greaterOneBits[NUM_ONE_FLAG_CTX][2];
	int levelAbsBits[NUM_ABS_FLAG_CTX][2];
	int blockCbpBits[NUM_QT_CBF_CTX][2];
	int blockRootCbpBits[2];
}EstBitsSbac;

typedef struct Entropy
{

	SyntaxElementWriter syn;
	uint8_t       m_contextState[160]; // MAX_OFF_CTX_MOD + padding
	/* CABAC state */
	uint32_t      m_low;
	uint32_t      m_range;
	uint32_t      m_bufferedByte;
	int           m_numBufferedBytes;
	int           m_bitsLeft;
	uint64_t      m_fracBits;
	uint64_t      m_pad;
	bool m_valid;
	EstBitsSbac   m_estBitsSbac;

}Entropy;

//#if CHECKED_BUILD || _DEBUG
void markInvalid(Entropy* entropy);
void markValid(Entropy* entropy);
//#else
void markValid(Entropy* entropy);
//#endif
void zeroFract(Entropy* entropy);

void Entropy_init(Entropy* pdata);
uint8_t sbacInit(int qp, int initValue);
static void initBuffer(uint8_t* contextModel, enum SliceType sliceType, int qp, uint8_t* ctxModel, int size);
void resetEntropy(Entropy* entropy, Slice *slice);
void resetEntropy2(Entropy* entropy, Slice *slice);
/* CABAC private methods */
void writeOut(Entropy* entropy);
void start(Entropy* entropy);
void finish(Entropy* entropy);
int clz(unsigned int a);
int ctz(unsigned int a);
void encodeBin(Entropy* entropy, uint32_t binValue, uint8_t *ctxModel);
void encodeBinEP(Entropy* entropy, uint32_t binValue);
void encodeBinsEP(Entropy* entropy, uint32_t binValues, int numBins);
void encodeBinTrm(Entropy* entropy, uint32_t binValue);
void codeTransformSubdivFlag(Entropy* entropy, uint32_t symbol, uint32_t ctx);
void codeCUTransquantBypassFlag(Entropy* entropy, uint32_t symbol);
void codeQtCbfLuma(Entropy* entropy, uint32_t cbf, uint32_t tuDepth);
void codeQtCbfChroma(Entropy* entropy, uint32_t cbf, uint32_t tuDepth);
void codeQtRootCbf(Entropy* entropy, uint32_t cbf);
void codeSplitFlag(Entropy* entropy, CUData *cu, uint32_t absPartIdx, uint32_t depth);
void codeSkipFlag(Entropy* entropy, CUData* cu, uint32_t absPartIdx);
void codePredMode(Entropy* entropy, int predMode);
void codeTransformSkipFlags(Entropy* entropy, uint32_t transformSkip, enum TextType ttype);
void codeDeltaQP(Entropy* entropy, CUData* cu, uint32_t absPartIdx);
/* SBac private methods */
void writeUnaryMaxSymbol(Entropy* entropy, uint32_t symbol, uint8_t* scmModel, int offset, uint32_t maxSymbol);
void writeEpExGolomb(Entropy* entropy, uint32_t symbol, uint32_t count);
void writeCoefRemainExGolomb(Entropy* entropy, uint32_t codeNumber, uint32_t absGoRice);
void codeMergeIndex(Entropy* entropy, CUData* cu, uint32_t absPartIdx);
void entropy_resetBits(Entropy* entropy);
void finishCU(Entropy* entropy, const CUData* cu, uint32_t absPartIdx, uint32_t depth, bool bCodeDQP);
void codePartSize(Entropy* entropy, struct CUData* cu, uint32_t absPartIdx, uint32_t depth);
void swap(int *x, int *y);
void codeIntraDirChroma(struct Entropy *entropy, const  CUData* cu, uint32_t absPartIdx, uint32_t *chromaDirMode);
void codeIntraDirLumaAng(struct Entropy *entropy, CUData *cu, uint32_t absPartIdx, bool isMultiple);
void codeInterDir(Entropy *entropy, const CUData* cu, uint32_t absPartIdx);//Ö¡¼ä
void codeRefFrmIdx(Entropy *entropy, CUData* cu, uint32_t absPartIdx, int list);
void codeRefFrmIdxPU(Entropy *entropy, CUData* cu, uint32_t absPartIdx, int list);
void codeMvd(Entropy *entropy, const CUData* cu, uint32_t absPartIdx, int list);//Ö¡¼ä
void codeMVPIdx(Entropy *entropy, uint32_t symbol);
void codeMergeFlag(Entropy *entropy, const CUData* cu, uint32_t absPartIdx);
void codePUWise(Entropy *entropy, CUData* cu, uint32_t absPartIdx);
void codePredInfo(Entropy *entropy, CUData *cu, uint32_t absPartIdx);
void codeProfileTier(Entropy* entropy, struct ProfileTierLevel* ptl, int maxTempSubLayers);
void codeVPS(Entropy* entropy, struct VPS* vps);
void codeSPS(Entropy* entropy, struct SPS* sps, struct ScalingList* scalingList, struct ProfileTierLevel* ptl);
void codeScalingListss(Entropy* entropy, struct ScalingList* scalingList);
void codeScalingLists(Entropy* entropy, struct ScalingList* scalingList, uint32_t sizeId, uint32_t listId);
void codeVUI(Entropy* entropy, struct VUI* vui, int maxSubTLayers);
void codeHrdParameters(Entropy* entropy, struct HRDInfo* hrd, int maxSubTLayers);

void copyFrom(Entropy* dst, const Entropy* src);
void copyState(Entropy* dst, const Entropy* other);
void load(Entropy* dst, const Entropy* src);
void store(const Entropy* src, Entropy* dst);
void copyContextsFrom(Entropy* dst, const Entropy* src);
void loadIntraDirModeLuma(Entropy* dst, const Entropy* src);

void codeCoeffNxN(Entropy* entropy, CUData* cu, coeff_t* coeff, uint32_t absPartIdx, uint32_t log2TrSize, enum TextType ttype);
void codeSaoMaxUvlc(Entropy* entropy, uint32_t code, uint32_t maxSymbol);
void codeSaoOffset(Entropy* entropy, const SaoCtuParam* ctuParam, int plane);
void Entropy_codeQtCbfChroma(Entropy* entropy, CUData* cu, uint32_t absPartIdx, enum TextType ttype, uint32_t tuDepth, bool lowestLevel);
void Entropy_codeQtCbfLuma(Entropy* entropy, CUData* cu, uint32_t absPartIdx, uint32_t tuDepth);
void TURecurse_init(struct TURecurse *tURecurse, enum SplitType splitType, uint32_t _absPartIdxStep, uint32_t _absPartIdxTU);
char isNextSection(struct TURecurse *tURecurse);
char isLastSection(struct TURecurse *tURecurse);
void encodeTransform(Entropy* entropy, struct CUData* cu, uint32_t absPartIdx, uint32_t curDepth, uint32_t log2CurSize, bool bCodeDQP, uint32_t depthRange[2]);
void codeCoeff(Entropy* entropy, CUData* cu, uint32_t absPartIdx, bool bCodeDQP, uint32_t depthRange[2]);
void encodeCU(Entropy* entropy, CUData* cu, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t depth, bool bEncodeDQP);
void encodeCTU(Entropy* entropy, CUData* cu, CUGeom* cuGeom);

void codeShortTermRefPicSet(Entropy* entropy, const struct RPS* rps);
void codePredWeightTable(Entropy* entropy, struct Slice*slice);
void codeSliceHeader(Entropy* entropy, Slice* slice /*struct FrameData* encData,uint32_t absPartIdx, struct RPS* rps*/);
void  estCBFBit(const Entropy* entropy, struct EstBitsSbac* estBitsSbac);
void  estSignificantCoeffGroupMapBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, char bIsLuma);
void estSignificantMapBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, uint32_t log2TrSize, char bIsLuma);
void estSignificantCoefficientsBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, char bIsLuma);
void estBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, uint32_t log2TrSize, char bIsLuma);
uint32_t bitsIntraModeNonMPM(const Entropy* entroy);
uint32_t bitsIntraModeMPM(Entropy* entroy, const uint32_t preds[3], uint32_t dir);
uint32_t estimateCbfBits(Entropy* entroy, uint32_t cbf, enum TextType ttype, uint32_t tuDepth);
uint32_t bitsIntraMode(Entropy* entroy, struct  CUData* cu, uint32_t absPartIdx);
void codeQtRootCbfZero(Entropy* entropy);
uint32_t bitsInterMode(Entropy* entroy, struct CUData*cu, uint32_t absPartIdx, uint32_t depth);
/* return the bits of encoding the context bin without updating */
uint32_t bitsCodeBin(const Entropy* entroy, uint32_t binValue, uint32_t ctxModel);
uint32_t entropy_getNumberOfWrittenBits(Entropy* entropy);
void Entropy_entropy(Entropy* entropy);
void finishSlice(Entropy* entropy);
void setBitstream(Entropy* entropy, Bitstream* pbit);
void codePPS(Entropy *entropy, PPS *pps);
//int count_nonzero_c32(coeff_t* coeff,uint32_t trSize);
//int count_nonzero_c16(coeff_t* coeff,uint32_t trSize);
uint32_t calc_sigPattern(uint32_t *x, uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG);
uint32_t get_Sigctx(uint32_t * x, uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG);

#endif /* ENTROPY_H_ */
