/*
* slice.h
*
*  Created on: 2015-10-11
*      Author: adminster
*/

#ifndef SLICE_H_
#define SLICE_H_

#include "common.h"
#include "reference.h"
#include "frame.h"
#include "piclist.h"

typedef enum SliceType
{
	B_SLICE,
	P_SLICE,
	I_SLICE
}SliceType;

typedef enum Profile_Name
{
	Profile_NONE = 0,
	Profile_MAIN = 1,
	MAIN10 = 2,
	MAINSTILLPICTURE = 3,
	MAINREXT = 4,
	HIGHTHROUGHPUTREXT = 5
}Profile_Name;

typedef enum Tier
{
	Tier_MAIN = 0,
	Tier_HIGH = 1,
}Tier;

typedef enum Name_name
{
	Name_NONE = 0,
	Name_LEVEL1 = 30,
	Name_LEVEL2 = 60,
	Name_LEVEL2_1 = 63,
	Name_LEVEL3 = 90,
	Name_LEVEL3_1 = 93,
	Name_LEVEL4 = 120,
	Name_LEVEL4_1 = 123,
	Name_LEVEL5 = 150,
	Name_LEVEL5_1 = 153,
	Name_LEVEL5_2 = 156,
	Name_LEVEL6 = 180,
	Name_LEVEL6_1 = 183,
	Name_LEVEL6_2 = 186,
	Name_LEVEL8_5 = 255,
}Name_name;

typedef struct ProfileTierLevel
{
	bool     tierFlag;
	bool     progressiveSourceFlag;
	bool     interlacedSourceFlag;
	bool     nonPackedConstraintFlag;
	bool     frameOnlyConstraintFlag;
	bool     profileCompatibilityFlag[32];
	bool     intraConstraintFlag;
	bool     lowerBitRateConstraintFlag;
	int      profileIdc;
	int      levelIdc;
	uint32_t minCrForLevel;
	uint32_t maxLumaSrForLevel;
	uint32_t bitDepthConstraint;
	int      chromaFormatConstraint;
}ProfileTierLevel;

typedef struct HRDInfo
{
	uint32_t bitRateScale;
	uint32_t cpbSizeScale;
	uint32_t initialCpbRemovalDelayLength;
	uint32_t cpbRemovalDelayLength;
	uint32_t dpbOutputDelayLength;
	uint32_t bitRateValue;
	uint32_t cpbSizeValue;
	bool     cbrFlag;

}HRDInfo;



typedef struct TimingInfo
{
	uint32_t numUnitsInTick;
	uint32_t timeScale;
}TimingInfo;

typedef struct Window
{
	bool bEnabled;
	int  leftOffset;
	int  rightOffset;
	int  topOffset;
	int  bottomOffset;
}Window;

typedef struct VUI
{
	bool       aspectRatioInfoPresentFlag;
	int        aspectRatioIdc;
	int        sarWidth;
	int        sarHeight;

	bool       overscanInfoPresentFlag;
	bool       overscanAppropriateFlag;

	bool       videoSignalTypePresentFlag;
	int        videoFormat;
	bool       videoFullRangeFlag;

	bool       colourDescriptionPresentFlag;
	int        colourPrimaries;
	int        transferCharacteristics;
	int        matrixCoefficients;

	bool       chromaLocInfoPresentFlag;
	int        chromaSampleLocTypeTopField;
	int        chromaSampleLocTypeBottomField;

	Window     defaultDisplayWindow;

	bool       frameFieldInfoPresentFlag;
	bool       fieldSeqFlag;

	bool       hrdParametersPresentFlag;
	HRDInfo    hrdParameters;

	TimingInfo timingInfo;
}VUI;

typedef struct VPS
{
	uint32_t         maxTempSubLayers;
	uint32_t         numReorderPics;
	uint32_t         maxDecPicBuffering;
	uint32_t         maxLatencyIncrease;
	HRDInfo          hrdParameters;
	ProfileTierLevel ptl;
}VPS;

typedef struct RPS
{
	int  numberOfPictures;
	int  numberOfNegativePictures;
	int  numberOfPositivePictures;

	int  poc[MAX_NUM_REF_PICS];
	int  deltaPOC[MAX_NUM_REF_PICS];
	int bUsed[MAX_NUM_REF_PICS];
}RPS;

typedef struct SPS
{
	int      chromaFormatIdc;        // use param
	uint32_t picWidthInLumaSamples;  // use param
	uint32_t picHeightInLumaSamples; // use param

	uint32_t numCuInWidth;
	uint32_t numCuInHeight;
	uint32_t numCUsInFrame;
	uint32_t numPartitions;
	uint32_t numPartInCUSize;

	int      log2MinCodingBlockSize;
	int      log2DiffMaxMinCodingBlockSize;

	uint32_t quadtreeTULog2MaxSize;
	uint32_t quadtreeTULog2MinSize;

	uint32_t quadtreeTUMaxDepthInter; // use param
	uint32_t quadtreeTUMaxDepthIntra; // use param

	int    bUseSAO; // use param
	int    bUseAMP; // use param
	uint32_t maxAMPDepth;

	uint32_t maxTempSubLayers;   // max number of Temporal Sub layers
	uint32_t maxDecPicBuffering; // these are dups of VPS values
	uint32_t maxLatencyIncrease;
	int      numReorderPics;

	int     bUseStrongIntraSmoothing; // use param
	int     bTemporalMVPEnabled;

	struct Window   conformanceWindow;
	struct VUI      vuiParameters;
}SPS;

typedef struct PPS
{
	uint32_t maxCuDQPDepth;

	int      chromaQpOffset[2];      // use param

	int     bUseWeightPred;         // use param
	int     bUseWeightedBiPred;     // use param
	int     bUseDQP;
	int     bConstrainedIntraPred;  // use param

	int     bTransquantBypassEnabled;  // Indicates presence of cu_transquant_bypass_flag in CUs.
	int     bTransformSkipEnabled;     // use param
	int     bEntropyCodingSyncEnabled; // use param
	int     bSignHideEnabled;          // use param

	int     bDeblockingFilterControlPresent;
	int     bPicDisableDeblockingFilter;
	int      deblockingFilterBetaOffsetDiv2;
	int      deblockingFilterTcOffsetDiv2;
}PPS;

typedef struct WeightParam
{
	// Explicit weighted prediction parameters parsed in slice header,
	bool     bPresentFlag;
	uint32_t log2WeightDenom;
	int      inputWeight;
	int      inputOffset;
}WeightParam;


typedef struct Slice
{
	struct SPS*  m_sps;
	struct PPS*  m_pps;
	WeightParam m_weightPredTable[2][MAX_NUM_REF][3]; // [list][refIdx][0:Y, 1:U, 2:V]
	enum SliceType   m_sliceType;
	struct MotionReference(*m_mref)[MAX_NUM_REF + 1];
	//struct MotionReference  m_mref[2][MAX_NUM_REF + 1];
	struct RPS   m_rps;

	NalUnitType m_nalUnitType;

	int         m_sliceQp;
	int         m_poc;
	int         m_lastIDR;
	int        m_bCheckLDC;       // TODO: is this necessary?
	int        m_sLFaseFlag;      // loop filter boundary flag
	int        m_colFromL0Flag;   // collocated picture from List0 or List1 flag
	uint32_t    m_colRefIdx;       // never modified

	int         m_numRefIdx[2];
	struct Frame*      m_refPicList[2][MAX_NUM_REF + 1];
	int         m_refPOCList[2][MAX_NUM_REF + 1];

	uint32_t    m_maxNumMergeCand; // use param
	uint32_t    m_endCUAddr;

}Slice;

void init_RPS(RPS *rps);
void initSlice(Slice *slice);
bool getRapPicFlag(Slice* slice);
bool getIdrPicFlag(Slice* slice);

void HRDInfo_Info(HRDInfo *info);
void Window_dow(Window *dow);
Frame* getRefPic(Slice *slice, int list, int refIdx);
bool isIRAP(Slice* slice);
bool Slice_isIntra(const Slice* slice);
bool isInterB(const Slice* slice);
bool isInterP(const Slice* slice);
const uint32_t Slice_realEndAddress(const Slice *slice, uint32_t endCUAddr);
void RPS_sortDeltaPOC(struct RPS* rps);
void Slice_setRefPicList(Slice *slice, struct PicList* picList);

#endif /* SLICE_H_ */
