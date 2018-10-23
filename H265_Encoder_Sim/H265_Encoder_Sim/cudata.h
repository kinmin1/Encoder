/*
* cudata.h
*
*  Created on: 2015-10-11
*      Author: adminster
*/

#ifndef CUDATA_H_
#define CUDATA_H_

#include "common.h"
#include "mv.h"
#include "slice.h"

struct Frame;

typedef enum PartSize
{
	SIZE_2Nx2N, // symmetric motion partition,  2Nx2N
	SIZE_2NxN,  // symmetric motion partition,  2Nx N
	SIZE_Nx2N,  // symmetric motion partition,   Nx2N
	SIZE_NxN,   // symmetric motion partition,   Nx N
	SIZE_2NxnU, // asymmetric motion partition, 2Nx( N/2) + 2Nx(3N/2)
	SIZE_2NxnD, // asymmetric motion partition, 2Nx(3N/2) + 2Nx( N/2)
	SIZE_nLx2N, // asymmetric motion partition, ( N/2)x2N + (3N/2)x2N
	SIZE_nRx2N, // asymmetric motion partition, (3N/2)x2N + ( N/2)x2N
	// NUM_SIZES
}PartSize;

enum PredMode //预测类型
{
	MODE_NONE = 0,
	MODE_INTER = (1 << 0),
	MODE_INTRA = (1 << 1),
	MODE_SKIP = (1 << 2) | MODE_INTER
};

// motion vector predictor direction used in AMVP
typedef enum MVP_DIR
{
	MD_LEFT = 0,    // MVP of left block
	MD_ABOVE,       // MVP of above block
	MD_ABOVE_RIGHT, // MVP of above right block
	MD_BELOW_LEFT,  // MVP of below left block
	MD_ABOVE_LEFT,  // MVP of above left block
	MD_COLLOCATED   // MVP of temporal neighbour
}MVP_DIR;

enum { BytesPerPartition = 21 };  // combined sizeof() of all per-part data
enum { TMVP_UNIT_MASK = 0xF0 };  // mask for mapping index to into a compressed (reference) MV field
enum Type
{
	INTRA = 1 << 0, // CU is intra predicted
	PRESENT = 1 << 1, // CU is not completely outside the frame CU不是完全在图像外（只要有一部分在图像内就有此标志位）值2
	SPLIT_MANDATORY = 1 << 2, // CU split is mandatory if CU is inside frame and can be split CU节点为被推荐划分（一般是处于边界情况，当前CU一部分内容在图像外面）值为4
	LEAF = 1 << 3, // CU is a leaf node of the CTU CU是叶子节点，即是8x8块（默认配置最小CU是8x8的情况下）值为8
	SPLIT = 1 << 4, // CU is currently split in four child CUs. 确定划分
};
#define MAX_GEOMS  85
typedef struct CUGeom //CU的几何信息 在FrameEncoder类被引用
{
	enum Type type;
	uint32_t log2CUSize;    // Log of the CU size.
	uint32_t childOffset;   // offset of the first child CU from current CU 表示从当前到第一个子CU的偏移
	uint32_t absPartIdx;    // Part index of this CU in terms of 4x4 blocks. 当前CU在LCU中4x4 zizag地址
	uint32_t numPartitions; // Number of 4x4 blocks in the CU 当前CU有多少个4x4块
	uint32_t flags;         // CU flags. 当前CU的块状态
	uint32_t depth;         // depth of this CU relative from CTU 当前CU的深度

}CUGeom;

struct MVField
{
	struct MV  mv;
	int refIdx;
};

struct InterNeighbourMV
{
	// Neighbour MV. The index represents the list.
	struct MV mv[2];

	// Collocated right bottom CU addr.
	uint32_t cuAddr[2];

	// For spatial prediction, this field contains the reference index
	// in each list (-1 if not available).
	//
	// For temporal prediction, the first value is used for the
	// prediction with list 0. The second value is used for the prediction
	// with list 1. For each value, the first four bits are the reference index
	// associated to the PMV, and the fifth bit is the list associated to the PMV.
	// if both reference indices are -1, then unifiedRef is also -1
	int16_t refIdx[2];
	int32_t unifiedRef;
};

typedef void(*cucopy_t)(uint8_t* dst, uint8_t* src); // dst and src are aligned to MIN(size, 32)
typedef void(*cubcast_t)(uint8_t* dst, uint8_t val); // dst is aligned to MIN(size, 32)

// Holds part data for a CU of a given size, from an 8x8 CU to a CTU
typedef struct CUData
{
	cubcast_t s_partSet[NUM_FULL_DEPTH]; // pointer to broadcast set functions per absolute depth
	uint32_t  s_numPartInCUSize;
	struct FrameData* m_encData;

	struct Slice*  m_slice;

	cucopy_t      m_partCopy;         // pointer to function that copies m_numPartitions elements
	cubcast_t     m_partSet;          // pointer to function that sets m_numPartitions elements
	cucopy_t      m_subPartCopy;      // pointer to function that copies m_numPartitions/4 elements, may be NULL
	cubcast_t     m_subPartSet;       // pointer to function that sets m_numPartitions/4 elements, may be NULL


	uint32_t      m_cuAddr;           // address of CTU within the picture in raster order
	uint32_t      m_absIdxInCTU;      // address of CU within its CTU in Z scan order
	uint32_t      m_cuPelX;           // CU position within the picture, in pixels (X)
	uint32_t      m_cuPelY;           // CU position within the picture, in pixels (Y)
	uint32_t      m_numPartitions;    // maximum number of 4x4 partitions within this CU

	uint32_t      m_chromaFormat;
	uint32_t      m_hChromaShift;
	uint32_t      m_vChromaShift;

	/* Per-part data, stored contiguously */
	int8_t*       m_qp;               // array of QP values
	uint8_t*      m_log2CUSize;       // array of cu log2Size TODO: seems redundant to depth
	uint8_t*      m_lumaIntraDir;     // array of intra directions (luma)
	uint8_t*      m_tqBypass;         // array of CU lossless flags
	int8_t*       m_refIdx[2];        // array of motion reference indices per list
	uint8_t*      m_cuDepth;          // array of depths
	uint8_t*      m_predMode;         // array of prediction modes
	uint8_t*      m_partSize;         // array of partition sizes
	uint8_t*      m_mergeFlag;        // array of merge flags
	uint8_t*      m_interDir;         // array of inter directions
	uint8_t*      m_mvpIdx[2];        // array of motion vector predictor candidates or merge candidate indices [0]
	uint8_t*      m_tuDepth;          // array of transform indices
	uint8_t*      m_transformSkip[3]; // array of transform skipping flags per plane
	uint8_t*      m_cbf[3];           // array of coded block flags (CBF) per plane
	uint8_t*      m_chromaIntraDir;   // array of intra directions (chroma)


	coeff_t*      m_trCoeff[3];       // transformed coefficient buffer per plane

	struct MV*           m_mv[2];            // array of motion vectors per list
	struct MV*           m_mvd[2];           // array of coded motion vector deltas per list

	struct CUData *m_cuAboveLeft;      // pointer to above-left neighbor CTU
	struct CUData *m_cuAboveRight;     // pointer to above-right neighbor CTU
	struct CUData *m_cuAbove;          // pointer to above neighbor CTU
	struct CUData *m_cuLeft;           // pointer to left neighbor CTU
}CUData;

typedef struct CUDataMemPool
{
	uint8_t* charMemBlock;
	coeff_t* trCoeffMemBlock;
	struct MV* mvMemBlock;
	int testnum;

}CUDataMemPool;

typedef struct TUEntropyCodingParameters
{
	uint16_t *scan;
	uint16_t *scanCG;
	enum ScanType        scanType;
	uint32_t        log2TrSizeCG;
	uint32_t        firstSignificanceMapContext;
}TUEntropyCodingParameters;

void CUData_initialize(struct CUData *cu, struct CUDataMemPool *dataPool, uint32_t depth, int instance);

int CUDataMemPool_create(struct CUDataMemPool *MemPool, uint32_t depth, uint32_t numInstances);
int CUDataMemPool_create_analysis(CUDataMemPool *MemPool, uint32_t depth, uint32_t numInstances);
int CUDataMemPool_create_frame(CUDataMemPool *MemPool, uint32_t depth, uint32_t numInstances);

//void CUData_initCTU(CUData* cu,const Frame* frame, uint32_t cuAddr, int qp);
void getTUEntropyCodingParameters(CUData* cu, TUEntropyCodingParameters *result, uint32_t absPartIdx, uint32_t log2TrSize, char bIsLuma);
void CUData_initSubCU(struct CUData *cu, struct CUData* ctu, struct CUGeom* cuGeom, int qp);
void CUData_copyPartFrom(struct CUData *cu, const struct CUData* subCU, const struct CUGeom* childGeom, uint32_t subPartIdx);
void CUData_setEmptyPart(struct CUData *cu, const struct CUGeom* childGeom, uint32_t subPartIdx);
void CUData_initLosslessCU(struct CUData *ctu, const struct CUData* cu, const struct CUGeom* cuGeom);
void CUData_copyToPic(struct CUData *cu, uint32_t depth);
void CUData_copyFromPic(struct CUData *cu, const struct CUData* ctu, const struct CUGeom* cuGeom);
void CUData_updatePic(struct CUData *cu, uint32_t depth);
struct CUData *CUData_getPULeft(struct CUData *cu, uint32_t *lPartUnitIdx, uint32_t curPartUnitIdx);
struct CUData* CUData_getPUAbove(struct CUData *cu, uint32_t *aPartUnitIdx, uint32_t curPartUnitIdx);
struct CUData* CUData_getPUAboveLeft(CUData *cu, uint32_t *alPartUnitIdx, uint32_t curPartUnitIdx);
struct CUData* CUData_getPUAboveRight(CUData *cu, uint32_t *arPartUnitIdx, uint32_t curPartUnitIdx);
struct CUData* CUData_getPUBelowLeftAdi(struct CUData *cu, uint32_t *blPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset);
struct CUData* CUData_getPUAboveRightAdi(struct CUData *cu, uint32_t *arPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset);
const struct CUData* CUData_getQpMinCuLeft(const struct CUData *cu, uint32_t *lPartUnitIdx, uint32_t curAbsIdxInCTU);
void CUData_calcCTUGeoms(uint32_t ctuWidth, uint32_t ctuHeight, uint32_t maxCUSize, uint32_t minCUSize, struct CUGeom cuDataArray[85]);
const int8_t CUData_getRefQP(const struct CUData *cu, uint32_t curAbsIdxInCTU);
int CUData_getIntraDirLumaPredictor(struct CUData *cu, uint32_t absPartIdx, uint32_t *intraDirPred);
const int8_t CUData_getLastCodedQP(const CUData *cu, uint32_t absPartIdx);
const int CUData_getLastValidPartIdx(const struct CUData *cu, int absPartIdx);
void CUData_getAllowedChromaDir(struct CUData *cu, uint32_t absPartIdx, uint32_t* modeList);
void CUData_getIntraTUQtDepthRange(struct CUData *cu, uint32_t tuDepthRange[2], uint32_t absPartIdx);
void CUData_getInterTUQtDepthRange(struct CUData *cu, uint32_t tuDepthRange[2], uint32_t absPartIdx);
const uint32_t CUData_getCtxSkipFlag(struct CUData *cu, uint32_t absPartIdx);
int CUData_setQPSubCUs(struct CUData *cu, int8_t qp, uint32_t absPartIdx, uint32_t depth);
void CUData_setPUInterDir(struct CUData *cu, uint8_t dir, uint32_t absPartIdx, uint32_t puIdx);
void CUData_setAllPU(struct CUData *cu, struct MV *p, const struct MV *val, int absPartIdx, int puIdx);
void CUData_initCTU(CUData* cu, struct Frame* frame, uint32_t cuAddr, int qp);
void     setQPSubParts(struct CUData *cu, int8_t qp, uint32_t absPartIdx, uint32_t depth);
void     setTUDepthSubParts(struct CUData *cu, uint8_t tuDepth, uint32_t absPartIdx, uint32_t depth);
void     setLumaIntraDirSubParts(struct CUData *cu, uint8_t dir, uint32_t absPartIdx, uint32_t depth);
void     setChromIntraDirSubParts(struct CUData *cu, uint8_t dir, uint32_t absPartIdx, uint32_t depth);
void     setCbfSubParts(struct CUData *cu, uint8_t cbf, enum TextType ttype, uint32_t absPartIdx, uint32_t depth);
void     setCbfPartRange(struct CUData *cu, uint8_t cbf, enum TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes);
void     setTransformSkipSubParts(struct CUData *cu, uint8_t tskip, enum TextType ttype, uint32_t absPartIdx, uint32_t depth);
void     setTransformSkipPartRange(struct CUData *cu, uint8_t tskip, enum TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes);
const uint32_t CUData_getCtxSplitFlag(struct CUData *cu, uint32_t absPartIdx, uint32_t depth);
int8_t getRefQP(const CUData* cu, const uint32_t curAbsIdxInCTU);
const CUData* getQpMinCuLeft(const CUData *cu, uint32_t lPartUnitIdx, uint32_t curAbsIdxInCTU);
const CUData* getQpMinCuAbove(const CUData *cu, uint32_t aPartUnitIdx, uint32_t curAbsIdxInCTU);
const uint8_t  getQtRootCbf(struct CUData *cu, uint32_t absPartIdx);
uint32_t getSCUAddr(const CUData *cu);
void CUDataMemPool_init(CUDataMemPool* cumem);

uint32_t getNumPartInter(struct CUData *cu);
int     isIntra_cudata(struct CUData *cu, uint32_t absPartIdx);
int     isInter_cudata(const struct CUData *cu, uint32_t absPartIdx);
int     isSkipped(struct CUData *cu, uint32_t absPartIdx);
int     isBipredRestriction(struct CUData *cu);
uint8_t  getCbf(CUData* cu, uint32_t absPartIdx, enum TextType ttype, uint32_t tuDepth);
void     setPartSizeSubParts(CUData *cu, enum PartSize size);
void     setPredModeSubParts(CUData *cu, enum PredMode mode);
void     clearCbf(CUData *cu);

void CUMemPool_destory(CUDataMemPool *Pool);

uint32_t CUData_getInterMergeCandidates(struct CUData *cu, uint32_t absPartIdx, uint32_t puIdx, struct MVField(*candMvField)[2], uint8_t* candDir);
uint32_t CUData_deriveLeftBottomIdx(struct CUData *cu, uint32_t puIdx);
uint32_t CUData_deriveRightBottomIdx(struct CUData *cu, uint32_t puIdx);
bool isDiffMER(int xN, int yN, int xP, int yP);
void CUData_getMvField(const CUData* cu, uint32_t absPartIdx, int picList, struct MVField *outMvField);
void CUData_deriveLeftRightTopIdx(struct CUData *cu, uint32_t partIdx, uint32_t* partIdxLT, uint32_t* partIdxRT);
bool CUData_hasEqualMotion(const CUData *cu, uint32_t absPartIdx, const CUData* candCU, uint32_t candAbsPartIdx);
bool CUData_getColMVP(struct CUData *cu, MV* outMV, int* outRefIdx, int picList, int cuAddr, int partUnitIdx);
MV CUData_scaleMvByPOCDist(MV *inMV, int curPOC, int curRefPOC, int colPOC, int colRefPOC);
uint32_t CUData_deriveCenterIdx(struct CUData *cu, uint32_t puIdx);
void CUData_getPartIndexAndSize(struct CUData *cu, uint32_t partIdx, uint32_t *outPartAddr, int *outWidth, int *outHeight);
void CUData_clipMv(const CUData *cu, MV *outMV);
void CUData_setPUMv(struct CUData *cu, int list, struct MV *mv, int absPartIdx, int puIdx);
void CUData_setPURefIdx(struct CUData *cu, int list, int8_t refIdx, int absPartIdx, int puIdx);

bool CUData_getCollocatedMV(CUData *cu, int cuAddr, int partUnitIdx, struct InterNeighbourMV *neighbour);
void CUData_getInterNeighbourMV(CUData *cu, struct InterNeighbourMV *neighbour, uint32_t partUnitIdx, MVP_DIR dir);
void CUData_getNeighbourMV(CUData *cu, uint32_t puIdx, uint32_t absPartIdx, struct InterNeighbourMV* neighbours);
int CUData_getPMV(CUData *cu, struct InterNeighbourMV *neighbours, uint32_t picList, uint32_t refIdx, MV* amvpCand, MV* pmv);

#endif /* CUDATA_H_ */
