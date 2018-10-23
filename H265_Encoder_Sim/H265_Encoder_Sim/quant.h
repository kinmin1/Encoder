/*
* quant.h
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#ifndef QUANT_H_
#define QUANT_H_

#include "common.h"
#include "scalinglist.h"
#include "entropy.h"
#include "contexts.h"

#define dep 8		//深度
#define QP 26       //quantize parameter
#define MAX_TR_DYNAMIC_RANGE 15
#define X265_DEPTH 8

#define QUANT_IQUANT_SHIFT 20
#define QUANT_SHIFT 14

extern int MF[6];//量化缩放因子

extern int V[6];//反量化缩放因子
//变换矩阵
extern int g_t4[4][4];
extern int ig_t4[4][4];
extern int g_t8[8][8];
extern int ig_t8[8][8];
extern int g_t16[16][16];
extern int ig_t16[16][16];
extern int g_t32[32][32];

#define MAX_NUM_TR_COEFFS        MAX_TR_SIZE * MAX_TR_SIZE /* Maximum number of transform coefficients, for a 32x32 transform */
#define MAX_NUM_TR_CATEGORIES    16                        /* 32, 16, 8, 4 transform categories each for luma and chroma */

typedef struct NoiseReduction
{
	/* 0 = luma 4x4,   1 = luma 8x8,   2 = luma 16x16,   3 = luma 32x32
	* 4 = chroma 4x4, 5 = chroma 8x8, 6 = chroma 16x16, 7 = chroma 32x32
	* Intra 0..7 - Inter 8..15 */
	uint16_t offsetDenoise[MAX_NUM_TR_CATEGORIES][MAX_NUM_TR_COEFFS];
	uint32_t residualSum[MAX_NUM_TR_CATEGORIES][MAX_NUM_TR_COEFFS];
	uint32_t count[MAX_NUM_TR_CATEGORIES];
}NoiseReduction;

typedef struct QpParam
{
	int rem;
	int per;
	int qp;
	int64_t lambda2; /* FIX8 */
	int32_t lambda;  /* FIX8, dynamic range is 18-bits in 8bpp and 20-bits in 16bpp */

}QpParam;

#define IEP_RATE 32768  /* FIX15 cost of an equal probable bit */
typedef struct Quant
{
	ScalingList       *m_scalingList;
	struct Entropy    *m_entropyCoder;

	QpParam            m_qpParam[3];

	int                m_rdoqLevel;
	int32_t            m_psyRdoqScale;  // dynamic range [0,50] * 256 = 14-bits
	int16_t*           m_resiDctCoeff;
	int16_t*           m_fencDctCoeff;
	int16_t*           m_fencShortBuf;

	NoiseReduction*    m_nr;
	NoiseReduction*    m_frameNr; // Array of NR structures, one for each frameEncoder
	bool               m_tqBypass;
}Quant;

uint32_t calcPatternSigCtx(uint64_t sigCoeffGroupFlag64, uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG);
uint32_t getSigCoeffGroupCtxInc(uint64_t cgGroupMask, uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG);
uint32_t getSigCtxInc(uint32_t patternSigCtx, uint32_t log2TrSize, uint32_t trSize, uint32_t blkPos, char bIsLuma, uint32_t firstSignificanceMapContext);

bool Quant_init(Quant *quant, int rdoqLevel, double psyScale, ScalingList* scalingList, Entropy* entropy);

void setQpParam(QpParam *QpParam, int qpScaled);
void Quant_setQPforQuant(Quant *quant, CUData* ctu, int qp);
void Quant_invtransformNxN(Quant *quant, int16_t* residual, uint32_t resiStride, coeff_t* coeff, uint32_t log2TrSize, TextType ttype, bool bIntra, bool useTransformSkip, uint32_t numSig);
uint32_t Quant_transformNxN(Quant *quant, struct CUData* cu, const pixel* fenc, uint32_t fencStride, int16_t* residual, uint32_t resiStride, coeff_t* coeff, uint32_t log2TrSize, TextType ttype, uint32_t absPartIdx, bool useTransformSkip);

#endif /* QUANT_H_ */
