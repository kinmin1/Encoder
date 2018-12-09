/*
* framedata.h
*
*  Created on: 2015-10-11
*      Author: adminster
*/

#ifndef FRAMEDATA_H_
#define FRAMEDATA_H_

#include "slice.h"
#include "cudata.h"

#include "common.h"
struct SPS;
/* Rate control data used during encode and by references */
typedef struct RCStatCU
{
	uint32_t totalBits;     /* total bits to encode this CTU */
	uint32_t vbvCost;       /* sum of lowres costs for 16x16 sub-blocks */
	uint32_t intraVbvCost;  /* sum of lowres intra costs for 16x16 sub-blocks */
	uint64_t avgCost[4];    /* stores the avg cost of CU's in frame for each depth */
	uint32_t count[4];      /* count and avgCost only used by Analysis at RD0..4 */
	double   baseQp;        /* Qp of Cu set from RateControl/Vbv (only used by frame encoder) */
}RCStatCU;

typedef struct RCStatRow
{
	uint32_t numEncodedCUs; /* ctuAddr of last encoded CTU in row */
	uint32_t encodedBits;   /* sum of 'totalBits' of encoded CTUs */
	uint32_t satdForVbv;    /* sum of lowres (estimated) costs for entire row */
	uint32_t intraSatdForVbv; /* sum of lowres (estimated) intra costs for entire row */
	uint32_t diagSatd;
	uint32_t diagIntraSatd;
	double   diagQp;
	double   diagQpScale;
	double   sumQpRc;
	double   sumQpAq;
}RCStatRow;

typedef struct FrameData
{
	Slice   *m_slice;
	SAOParam*      m_saoParam;
	x265_param*    m_param;
	struct CUData*  m_picCTU;

	struct FrameData*     m_freeListNext;
	struct PicYuv*        m_reconPic;

	int           m_bHasReferences;   /* used during DPB/RPS updates */
	int            m_frameEncoderID;   /* the ID of the FrameEncoder encoding this frame(±àÂëÖ¡µÄ±àÂëÆ÷IDºÅ) */

	struct CUDataMemPool *m_cuMemPool;

	/* Rate control data used during encode and by references */

	struct RCStatCU*      m_cuStat;
	struct RCStatRow*     m_rowStat;

	double         m_avgQpRc;    /* avg QP as decided by rate-control */
	double         m_avgQpAq;    /* avg QP as decided by AQ in addition to rate-control */
	double         m_rateFactor; /* calculated based on the Frame QP */

}FrameData;

struct CUData* framedata_getPicCTU(FrameData *framedata, uint32_t ctuAddr);
bool FrameData_create(Frame *frame, FrameData *framedata, x265_param *param, struct SPS* sps);
void FrameData_reinit(FrameData *framedata, struct SPS *sps);
void FrameData_FrameData(FrameData* framedata);
void FrameData_destory(FrameData *framedata);

#endif /* FRAMEDATA_H_ */
