/*
* lowres.h
*
*  Created on: 2017-5-15
*      Author: Administrator
*/

#ifndef LOWRES_H_
#define LOWRES_H_

#include "primitives.h"
#include "common.h"
#include "picyuv.h"
#include "mv.h"

typedef	struct W{
	int  weight;
	int  offset;
	int  shift;
	int  round;
}W;

typedef struct ReferencePlanes
{
	pixel*   fpelPlane[3];
	pixel*   lowresPlane[4];
	struct PicYuv*  reconPic;

	bool     isWeighted;
	bool     isLowres;

	intptr_t lumaStride;
	intptr_t chromaStride;

	W  w[3];
}ReferencePlanes;

int lowresQPelCost(ReferencePlanes *ref, pixel *fenc, intptr_t blockOffset, const MV* qmv, pixelcmp_t comp, int width, int height);
pixel* ReferencePlanes_getLumaAddr(ReferencePlanes *ref, uint32_t ctuAddr, uint32_t absPartIdx);
pixel* ReferencePlanes_getCbAddr(ReferencePlanes *ref, uint32_t ctuAddr, uint32_t absPartIdx);
pixel* ReferencePlanes_getCrAddr(ReferencePlanes *ref, uint32_t ctuAddr, uint32_t absPartIdx);

/* lowres buffers, sizes and strides */
typedef struct Lowres
{
	ReferencePlanes referenceplanes;
	pixel*   fpelPlane[3];
	pixel*   lowresPlane[4];
	struct PicYuv*  reconPic;

	bool     isWeighted;
	bool     isLowres;

	intptr_t lumaStride;
	intptr_t chromaStride;

	struct {
		int      weight;
		int      offset;
		int      shift;
		int      round;
	} w[3];

	pixel *buffer[4];

	int    frameNum;         // Presentation frame number
	int    sliceType;        // Slice type decided by lookahead
	int    width;            // width of lowres frame in pixels
	int    lines;            // height of lowres frame in pixel lines
	int    leadingBframes;   // number of leading B frames for P or I

	bool   bScenecut;        // Set to false if the frame cannot possibly be part of a real scenecut.
	bool   bKeyframe;
	bool   bLastMiniGopBFrame;

	/* lookahead output data */
	int64_t   costEst[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
	int64_t   costEstAq[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
	int32_t*  rowSatds[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
	int       intraMbs[X265_BFRAME_MAX + 2];
	int32_t*  intraCost;
	uint8_t*  intraMode;
	int64_t   satdCost;
	uint16_t* lowresCostForRc;
	uint16_t(*lowresCosts[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2]);
	int32_t*  lowresMvCosts[2][X265_BFRAME_MAX + 1];
	MV*       lowresMvs[2][X265_BFRAME_MAX + 1];

	/* used for vbvLookahead */
	int       plannedType[X265_LOOKAHEAD_MAX + 1];
	int64_t   plannedSatd[X265_LOOKAHEAD_MAX + 1];
	int       indB;
	int       bframes;

	/* rate control / adaptive quant data */
	double*   qpAqOffset;      // AQ QP offset values for each 16x16 CU
	double*   qpCuTreeOffset;  // cuTree QP offset values for each 16x16 CU
	int*      invQscaleFactor; // qScale values for qp Aq Offsets
	uint64_t  wp_ssd[3];       // This is different than SSDY, this is sum(pixel^2) - sum(pixel)^2 for entire frame
	uint64_t  wp_sum[3];

	/* cutree intermediate data */
	uint16_t* propagateCost;
	double    weightedCostDelta[X265_BFRAME_MAX + 2];

}Lowres;

#endif /* LOWRES_H_ */
