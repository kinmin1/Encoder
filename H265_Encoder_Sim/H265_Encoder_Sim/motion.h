/*
* motion.h
*
*  Created on: 2015-11-6
*      Author: adminster
*/

#ifndef MOTION_H_
#define MOTION_H_

#include "primitives.h"
#include "reference.h"
#include "mv.h"
#include "bitcost.h"
#include "yuv.h"

struct Search;

static const int COST_MAX = 1 << 28;
typedef struct MotionEstimate
{
	BitCost bitcost;
	int blockOffset;

	int ctuAddr;
	int absPartIdx;  // part index of PU, including CU offset within CTU

	int searchMethod;
	int subpelRefine;

	int blockwidth;
	int blockheight;

	pixelcmp_t sad;
	pixelcmp_x3_t sad_x3;
	pixelcmp_x4_t sad_x4;
	pixelcmp_t satd;
	pixelcmp_t chromaSatd;

	struct Yuv fencPUYuv;
	int partEnum;
	bool bChromaSATD;

}MotionEstimate;

void MotionEstimate_init(MotionEstimate *me, int method, int refine, int csp);
void MotionEstimate_initScales(void);
int x265_mdate(void);
int MotionEstimate_hpelIterationCount(int subme);
void MotionEstimate_setSourcePU(MotionEstimate *me, const Yuv* srcFencYuv, int _ctuAddr, int cuPartIdx, int puPartIdx, int pwidth, int pheight);
int bufSAD(MotionEstimate  *motionEstimate, const pixel* fref, intptr_t stride, int width, int height);
int bufSATD(MotionEstimate  *motionEstimate, const pixel* fref, intptr_t stride, int width, int height);
int bufChromaSATD(MotionEstimate  *motionEstimate, Yuv* refYuv, int puPartIdx, int width, int height);

int MotionEstimate_subpelCompare(MotionEstimate *motionEstimate, ReferencePlanes *ref, const MV* qmv, pixelcmp_t cmp, int width, int height);
void COST_MV_X4_DIR(BitCost *bitcost, intptr_t stride, pixel *fenc, pixel *fref, MV *bmv, int m0x, int m0y, int m1x, int m1y, int m2x, int m2y, int m3x, int m3y, int *costs, int width, int height);
void COST_MV_X3_DIR(BitCost *bitcost, intptr_t stride, pixel *fenc, pixel *fref, MV bmv, int m0x, int m0y, int m1x, int m1y, int m2x, int m2y, int *costs, int width, int height);
int MotionEstimate_motionEstimate(MotionEstimate *motionEstimate, ReferencePlanes *ref, MV *mvmin, MV *mvmax, MV *qmvp, int numCandidates, MV *mvc, int merange, MV *outQMv, int width, int height);

#endif /* MOTION_H_ */
