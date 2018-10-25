/*
* motion.c
*
*  Created on: 2015-11-6
*      Author: adminster
*/
#include "common.h"
#include "primitives.h"
#include "lowres.h"
#include "motion.h"
#include "x265.h"
#include "pixel.h"
#include "bitcost.h"

extern EncoderPrimitives primitives;
struct ReferencePlanes;
struct x265_picture *x265_pic;
typedef struct SubpelWorkload
{
	int hpel_iters;
	int hpel_dirs;
	int qpel_iters;
	int qpel_dirs;
	bool hpel_satd;
}SubpelWorkload;

const struct SubpelWorkload workload[X265_MAX_SUBPEL_LEVEL + 1] =
{
	{ 1, 4, 0, 4, FALSE }, // 4 SAD HPEL only
	{ 1, 4, 1, 4, FALSE }, // 4 SAD HPEL + 4 SATD QPEL
	{ 1, 4, 1, 4, TRUE },  // 4 SATD HPEL + 4 SATD QPEL
	{ 2, 4, 1, 4, TRUE },  // 2x4 SATD HPEL + 4 SATD QPEL
	{ 2, 4, 2, 4, TRUE },  // 2x4 SATD HPEL + 2x4 SATD QPEL
	{ 1, 8, 1, 8, TRUE },  // 8 SATD HPEL + 8 SATD QPEL (default)
	{ 2, 8, 1, 8, TRUE },  // 2x8 SATD HPEL + 8 SATD QPEL
	{ 2, 8, 2, 8, TRUE },  // 2x8 SATD HPEL + 2x8 SATD QPEL
};

int sizeScale[NUM_PU_SIZES];
#define SAD_THRESH(v) (bcost < (((v >> 4) * sizeScale[partEnum])))

/* radius 2 hexagon. repeated entries are to avoid having to compute mod6 every time. */
//const hex2_x[8] = { MV(-1, -2), MV(-2, 0), MV(-1, 2), MV(1, 2), MV(2, 0), MV(1, -2), MV(-1, -2), MV(-2, 0) };
const int hex2_x[8] = { -1, -2, -1, 1, 2, 1, -1, -2 };
const int hex2_y[8] = { -2, 0, 2, 2, 0, -2, -2, 0 };
const uint8_t mod6m1[8] = { 5, 0, 1, 2, 3, 4, 5, 0 };  /* (x-1)%6 */
//const MV square1[9] = { MV(0, 0), MV(0, -1), MV(0, 1), MV(-1, 0), MV(1, 0), MV(-1, -1), MV(-1, 1), MV(1, -1), MV(1, 1) };
const int square1_x[9] = { 0, 0, 0, -1, 1, -1, -1, 1, 1 };
const int square1_y[9] = { 0, -1, 1, 0, 0, -1, 1, -1, 1 };

const int hex4_x[16] = {
	0, 0, -2, 2,
	-4, 4, -4, 4,
	-4, 4, -4, 4,
	-4, 4, -2, 2
};
const int hex4_y[16] = {
	-4, 4, -3, -3,
	-2, -2, -1, -1,
	0, 0, 1, 1,
	2, 2, 3, 3
};

const MV offsets_x[] = {
	-1, 0,
	-1, 1,
	-1, 1,
	-1, -1,
	1, 1,
	-1, 0,
	-1, 1,
	1, 0
};
const MV offsets_y[] = {
	0, -1,
	-1, -1,
	0, 0,
	1, -1,
	-1, 1,
	0, 1,
	1, 1,
	0, 1
};

void MotionEstimate_init(MotionEstimate *me, int method, int refine, int csp)
{/*
	me->searchMethod = method;
	me->subpelRefine = refine;
	Yuv_create(&me->fencPUYuv, FENC_STRIDE, csp);*/
}

void MotionEstimate_initScales(void)
{/*
#define SETUP_SCALE(W, H) \
    sizeScale[LUMA_ ## W ## x ## H] = (H * H) >> 4;
	SETUP_SCALE(4, 4);
	SETUP_SCALE(8, 8);
	SETUP_SCALE(8, 4);
	SETUP_SCALE(4, 8);
	SETUP_SCALE(16, 16);
	SETUP_SCALE(16, 8);
	SETUP_SCALE(8, 16);
	SETUP_SCALE(16, 12);
	SETUP_SCALE(12, 16);
	SETUP_SCALE(4, 16);
	SETUP_SCALE(16, 4);
	SETUP_SCALE(32, 32);
	SETUP_SCALE(32, 16);
	SETUP_SCALE(16, 32);
	SETUP_SCALE(32, 24);
	SETUP_SCALE(24, 32);
	SETUP_SCALE(32, 8);
	SETUP_SCALE(8, 32);
	SETUP_SCALE(64, 64);
	SETUP_SCALE(64, 32);
	SETUP_SCALE(32, 64);
	SETUP_SCALE(64, 48);
	SETUP_SCALE(48, 64);
	SETUP_SCALE(64, 16);
	SETUP_SCALE(16, 64);
#undef SETUP_SCALE*/
}

int MotionEstimate_hpelIterationCount(int subme)
{/*
	return workload[subme].hpel_iters +
		workload[subme].qpel_iters / 2;*/return 0;
}
/* Called by Search::predInterSearch() or --pme equivalent, chroma residual might be considered */
void MotionEstimate_setSourcePU(MotionEstimate *me, const Yuv* srcFencYuv, int _ctuAddr, int cuPartIdx, int puPartIdx, int pwidth, int pheight)
{/*
	me->partEnum = partitionFromSizes(pwidth, pheight);
	X265_CHECK(LUMA_4x4 != me->partEnum, "4x4 inter partition detected!\n");
	me->sad = primitives.pu[me->partEnum].sad;
	me->satd = primitives.pu[me->partEnum].satd;
	me->sad_x3 = primitives.pu[me->partEnum].sad_x3;
	me->sad_x4 = primitives.pu[me->partEnum].sad_x4;
	me->chromaSatd = primitives.chroma[me->fencPUYuv.m_csp].pu[me->partEnum].satd;

	// Enable chroma residual cost if subpelRefine level is greater than 2 and chroma block size
	// is an even multiple of 4x4 pixels (indicated by non-null chromaSatd pointer) //
	me->bChromaSATD = me->subpelRefine > 2 && me->chromaSatd;
	X265_CHECK(!(me->bChromaSATD && !workload[me->subpelRefine].hpel_satd), "Chroma SATD cannot be used with SAD hpel\n");

	me->ctuAddr = _ctuAddr;
	me->absPartIdx = cuPartIdx + puPartIdx;
	me->blockwidth = pwidth;
	me->blockOffset = 0;

	// copy PU from CU Yuv //
	Yuv_copyPUFromYuv(&me->fencPUYuv, srcFencYuv, puPartIdx, me->partEnum, me->bChromaSATD, pwidth, pheight);*/
}

/* buf*() and motionEstimate() methods all use cached fenc pixels and thus
* require setSourcePU() to be called prior. */

int bufSAD(MotionEstimate  *motionEstimate, const pixel* fref, intptr_t stride, int width, int height)
{/*
	return motionEstimate->sad(motionEstimate->fencPUYuv.m_buf[0], FENC_STRIDE, fref, stride, width, height);*/return 0;
}

int bufSATD(MotionEstimate  *motionEstimate, const pixel* fref, intptr_t stride, int width, int height)
{/*
	return motionEstimate->satd(motionEstimate->fencPUYuv.m_buf[0], FENC_STRIDE, fref, stride, width, height);*/return 0;
}

int bufChromaSATD(MotionEstimate  *motionEstimate, Yuv* refYuv, int puPartIdx, int width, int height)
{/*
	return motionEstimate->chromaSatd(Yuv_getCbAddr(refYuv, puPartIdx), refYuv->m_csize, motionEstimate->fencPUYuv.m_buf[1], motionEstimate->fencPUYuv.m_csize, width, height) +
		motionEstimate->chromaSatd(Yuv_getCrAddr(refYuv, puPartIdx), refYuv->m_csize, motionEstimate->fencPUYuv.m_buf[2], motionEstimate->fencPUYuv.m_csize, width, height);
	*/ return 0;
}

void COST_MV_X4_DIR(BitCost *bitcost, intptr_t stride, pixel *fenc, pixel *fref, MV *bmv, int m0x, int m0y, int m1x, int m1y, int m2x, int m2y, int m3x, int m3y, int *costs, int width, int height)
{/*
	pixel *pix_base = fref + bmv->x + bmv->y * stride;
	sad_x4(fenc, pix_base + (m0x)+(m0y)* stride, pix_base + (m1x)+(m1y)* stride, pix_base + (m2x)+(m2y)* stride, pix_base + (m3x)+(m3y)* stride, stride, costs, width, height);
	// TODO: use restrict keyword in ICL //
	const uint16_t *base_mvx = &bitcost->m_cost_mvx[(bmv->x << 2)];
	const uint16_t *base_mvy = &bitcost->m_cost_mvy[(bmv->y << 2)];

	(costs)[0] += (base_mvx[(m0x) << 2] + base_mvy[(m0y) << 2]);
	(costs)[1] += (base_mvx[(m1x) << 2] + base_mvy[(m1y) << 2]);
	(costs)[2] += (base_mvx[(m2x) << 2] + base_mvy[(m2y) << 2]);
	(costs)[3] += (base_mvx[(m3x) << 2] + base_mvy[(m3y) << 2]);*/
}
void COST_MV_X3_DIR(BitCost *bitcost, intptr_t stride, pixel *fenc, pixel *fref, MV bmv, int m0x, int m0y, int m1x, int m1y, int m2x, int m2y, int *costs, int width, int height)
{/*
	pixel *pix_base = fref + bmv.x + bmv.y * stride;
	sad_x3(fenc,
		pix_base + (m0x)+(m0y)* stride,
		pix_base + (m1x)+(m1y)* stride,
		pix_base + (m2x)+(m2y)* stride,
		stride, costs, width, height);
	const uint16_t *base_mvx = &bitcost->m_cost_mvx[(bmv.x + (m0x)) << 2];
	const uint16_t *base_mvy = &bitcost->m_cost_mvy[(bmv.y + (m0y)) << 2];

	(costs)[0] += (base_mvx[((m0x)-(m0x)) << 2] + base_mvy[((m0y)-(m0y)) << 2]);
	(costs)[1] += (base_mvx[((m1x)-(m0x)) << 2] + base_mvy[((m1y)-(m0y)) << 2]);
	(costs)[2] += (base_mvx[((m2x)-(m0x)) << 2] + base_mvy[((m2y)-(m0y)) << 2]);*/
}
int MotionEstimate_subpelCompare(MotionEstimate *motionEstimate, ReferencePlanes *ref, const MV* qmv, pixelcmp_t cmp, int width, int height)
{/*
	intptr_t refStride = ref->lumaStride;
	pixel *fref = ref->fpelPlane[0] + motionEstimate->blockOffset + (qmv->x >> 2) + (qmv->y >> 2) * refStride;
	int xFrac = qmv->x & 0x3;
	int yFrac = qmv->y & 0x3;
	int cost;
	intptr_t lclStride = motionEstimate->fencPUYuv.m_size;
	X265_CHECK(lclStride == FENC_STRIDE, "fenc buffer is assumed to have FENC_STRIDE by sad_x3 and sad_x4\n");

	if (!(yFrac | xFrac))
		cost = cmp(motionEstimate->fencPUYuv.m_buf[0], lclStride, fref, refStride, width, height);
	else
	{
		// we are taking a short-cut here if the reference is weighted. To be
		// accurate we should be interpolating unweighted pixels and weighting
		// the final 16bit values prior to rounding and down shifting. Instead we
		// are simply interpolating the weighted full-pel pixels. Not 100%
		// accurate but good enough for fast qpel ME //
		pixel subpelbuf[64 * 64];
		if (!yFrac)
			primitives.pu[motionEstimate->partEnum].luma_hpp(fref, refStride, subpelbuf, lclStride, xFrac, width, height, 8);
		else if (!xFrac)
			primitives.pu[motionEstimate->partEnum].luma_vpp(fref, refStride, subpelbuf, lclStride, yFrac, width, height, 8);
		else
			primitives.pu[motionEstimate->partEnum].luma_hvpp(fref, refStride, subpelbuf, lclStride, xFrac, yFrac, width, height, 8);

		cost = cmp(motionEstimate->fencPUYuv.m_buf[0], lclStride, subpelbuf, lclStride, width, height);
	}

	if (motionEstimate->bChromaSATD)
	{
		int csp = motionEstimate->fencPUYuv.m_csp;
		int hshift = motionEstimate->fencPUYuv.m_hChromaShift;
		int vshift = motionEstimate->fencPUYuv.m_vChromaShift;
		int shiftHor = (2 + hshift);
		int shiftVer = (2 + vshift);
		lclStride = motionEstimate->fencPUYuv.m_csize;

		intptr_t refStrideC = ref->reconPic->m_strideC;
		intptr_t refOffset = (qmv->x >> shiftHor) + (qmv->y >> shiftVer) * refStrideC;

		const pixel* refCb = ReferencePlanes_getLumaAddr(ref, motionEstimate->ctuAddr, motionEstimate->absPartIdx) + refOffset;
		const pixel* refCr = ReferencePlanes_getCrAddr(ref, motionEstimate->ctuAddr, motionEstimate->absPartIdx) + refOffset;

		xFrac = qmv->x & ((1 << shiftHor) - 1);
		yFrac = qmv->y & ((1 << shiftVer) - 1);

		if (!(yFrac | xFrac))
		{
			cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[1], lclStride, refCb, refStrideC, width, height);
			cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[2], lclStride, refCr, refStrideC, width, height);
		}
		else
		{
			//ALIGN_VAR_32(pixel, subpelbuf[64 * 64]);
			pixel subpelbuf[64 * 64];
			if (!yFrac)
			{
				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_hpp(refCb, refStrideC, subpelbuf, lclStride, xFrac << (1 - hshift), width, height, 4);
				cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[1], lclStride, subpelbuf, lclStride, width, height);

				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_hpp(refCr, refStrideC, subpelbuf, lclStride, xFrac << (1 - hshift), width, height, 4);
				cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[2], lclStride, subpelbuf, lclStride, width, height);
			}
			else if (!xFrac)
			{
				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_vpp(refCb, refStrideC, subpelbuf, lclStride, yFrac << (1 - vshift), width, height, 4);
				cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[1], lclStride, subpelbuf, lclStride, width, height);

				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_vpp(refCr, refStrideC, subpelbuf, lclStride, yFrac << (1 - vshift), width, height, 4);
				cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[2], lclStride, subpelbuf, lclStride, width, height);
			}
			else
			{
				//ALIGN_VAR_32(int16_t, immed[64 * (64 + NTAPS_CHROMA)]);
				int16_t immed[64 * (64 + NTAPS_CHROMA)];
				int extStride = motionEstimate->blockwidth >> hshift;
				int filterSize = NTAPS_CHROMA;
				int halfFilterSize = (filterSize >> 1);

				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_hps(refCb, refStrideC, immed, extStride, xFrac << (1 - hshift), 1, width, height, 4);
				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_vsp(immed + (halfFilterSize - 1) * extStride, extStride, subpelbuf, lclStride, yFrac << (1 - vshift), width, height, 4);
				cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[1], lclStride, subpelbuf, lclStride, width, height);

				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_hps(refCr, refStrideC, immed, extStride, xFrac << (1 - hshift), 1, width, height, 4);
				primitives.chroma[csp].pu[motionEstimate->partEnum].filter_vsp(immed + (halfFilterSize - 1) * extStride, extStride, subpelbuf, lclStride, yFrac << (1 - vshift), width, height, 4);
				cost += motionEstimate->chromaSatd(motionEstimate->fencPUYuv.m_buf[2], lclStride, subpelbuf, lclStride, width, height);
			}
		}
	}

	return cost;*/return 0;
}
int MotionEstimate_motionEstimate(MotionEstimate *motionEstimate, ReferencePlanes *ref, MV *mvmin, MV *mvmax, MV *qmvp, int numCandidates, MV *mvc, int merange, MV *outQMv, int width, int height)
{/*
	int costs[16];
	if (motionEstimate->ctuAddr >= 0)
		motionEstimate->blockOffset = Picyuv_CUgetLumaAddr(ref->reconPic, motionEstimate->ctuAddr, motionEstimate->absPartIdx) - Picyuv_CTUgetLumaAddr(ref->reconPic, 0);
	intptr_t stride = ref->lumaStride;
	pixel* fenc = motionEstimate->fencPUYuv.m_buf[0];
	pixel* fref = ref->fpelPlane[0] + motionEstimate->blockOffset;

	setMVP(&motionEstimate->bitcost, qmvp);

	MV qmvmin;
	qmvmin.x = toQPel(mvmin->x);
	qmvmin.y = toQPel(mvmin->y);
	qmvmin.word = 0;
	MV qmvmax;
	qmvmax.x = toQPel(mvmax->x);
	qmvmax.y = toQPel(mvmax->y);
	qmvmax.word = 0;
	// The term cost used here means satd/sad values for that particular search.
	// The costs used in ME integer search only includes the SAD cost of motion
	// residual and sqrtLambda times MVD bits.  The subpel refine steps use SATD
	// cost of residual and sqrtLambda * MVD bits.  Mode decision will be based
	// on video distortion cost (SSE/PSNR) plus lambda times all signaling bits
	// (mode + MVD bits).

	// measure SAD cost at clipped QPEL MVP
	MV pmv;
	pmv.x = clipped(qmvp->x, qmvmin.x, qmvmax.x);
	pmv.y = clipped(qmvp->y, qmvmin.y, qmvmax.y);
	pmv.word = 0;
	MV bestpre = pmv;
	int bprecost = 65536;

	// re-measure full pel rounded MVP with SAD as search start point //
	MV bmv;
	bmv.x = roundToFPel(pmv.x);
	bmv.y = roundToFPel(pmv.y);
	bmv.word = 0;
	int bcost = bprecost;
	bmv.x = bmv.x << 2;
	bmv.y = bmv.y << 2;
	if (isSubpel(&pmv))
		bcost = sad(fenc, FENC_STRIDE, fref + bmv.x + bmv.y * stride, stride, width, height) + mvcost(&motionEstimate->bitcost, bmv.x, bmv.y);

	// measure SAD cost at MV(0) if MVP is not zero
	if (notZero(&pmv))
	{
		int cost = sad(fenc, FENC_STRIDE, fref, stride, width, height) + mvcost(&motionEstimate->bitcost, 0, 0);
		if (cost < bcost)
		{
			bcost = cost;
			bmv.x = 0;
			bmv.y = 0;
		}
	}

	pmv.x = roundToFPel(pmv.x);
	pmv.y = roundToFPel(pmv.y);
	MV omv = bmv;  // current search origin or starting point

	switch (motionEstimate->searchMethod)
	{
	case X265_DIA_SEARCH:
	{
		// diamond search, radius 1 //
		bcost <<= 4;
		int i = merange;
		do
		{
			COST_MV_X4_DIR(&motionEstimate->bitcost, stride, fenc, fref, &bmv, 0, -1, 0, 1, -1, 0, 1, 0, costs, width, height);
			COPY1_IF_LT(bcost, (costs[0] << 4) + 1);
			COPY1_IF_LT(bcost, (costs[1] << 4) + 3);
			COPY1_IF_LT(bcost, (costs[2] << 4) + 4);
			COPY1_IF_LT(bcost, (costs[3] << 4) + 12);
			if (!(bcost & 15))
				break;
			bmv.x -= (bcost << 28) >> 30;
			bmv.y -= (bcost << 30) >> 30;
			bcost &= ~15;
		} while (--i && checkRange(bmv.x, bmv.y, mvmin, mvmax));
		bcost >>= 4;
		break;
	}

	case X265_HEX_SEARCH:
	{
	//me_hex2:
		// hexagon search, radius 2 //
#if 0
		for (int i = 0; i < merange / 2; i++)
		{
			omv = bmv;
			COST_MV(omv.x - 2, omv.y);
			COST_MV(omv.x - 1, omv.y + 2);
			COST_MV(omv.x + 1, omv.y + 2);
			COST_MV(omv.x + 2, omv.y);
			COST_MV(omv.x + 1, omv.y - 2);
			COST_MV(omv.x - 1, omv.y - 2);
			if (omv == bmv)
				break;
			if (!bmv.checkRange(mvmin, mvmax))
				break;
		}

#else // if 0
		// equivalent to the above, but eliminates duplicate candidates //
		COST_MV_X3_DIR(&motionEstimate->bitcost, stride, fenc, fref, bmv, -2, 0, -1, 2, 1, 2, costs, width, height);
		bcost <<= 3;
		COPY1_IF_LT(bcost, (costs[0] << 3) + 2);
		COPY1_IF_LT(bcost, (costs[1] << 3) + 3);
		COPY1_IF_LT(bcost, (costs[2] << 3) + 4);
		COST_MV_X3_DIR(&motionEstimate->bitcost, stride, fenc, fref, bmv, 2, 0, 1, -2, -1, -2, costs, width, height);
		COPY1_IF_LT(bcost, (costs[0] << 3) + 5);
		COPY1_IF_LT(bcost, (costs[1] << 3) + 6);
		COPY1_IF_LT(bcost, (costs[2] << 3) + 7);

		if (bcost & 7)
		{
			int dir = (bcost & 7) - 2;
			//bmv += hex2[dir + 1];
			bmv.x += hex2_x[dir + 1];
			bmv.y += hex2_y[dir + 1];
			// half hexagon, not overlapping the previous iteration //
			for (int i = (merange >> 1) - 1; i > 0 && checkRange(bmv.x, bmv.y, mvmin, mvmax); i--)
			{
				COST_MV_X3_DIR(&motionEstimate->bitcost, stride, fenc, fref, bmv, hex2_x[dir + 0], hex2_y[dir + 0], hex2_x[dir + 1], hex2_y[dir + 1], hex2_x[dir + 2], hex2_y[dir + 2], costs, width, height);
				bcost &= ~7;
				COPY1_IF_LT(bcost, (costs[0] << 3) + 1);
				COPY1_IF_LT(bcost, (costs[1] << 3) + 2);
				COPY1_IF_LT(bcost, (costs[2] << 3) + 3);
				if (!(bcost & 7))
					break;
				dir += (bcost & 7) - 2;
				dir = mod6m1[dir + 1];
				bmv.x += hex2_x[dir + 1];
				bmv.y += hex2_y[dir + 1];
			}
		}
		bcost >>= 3;
#endif // if 0

		// square refine //
		int dir = 0;
		COST_MV_X4_DIR(&motionEstimate->bitcost, stride, fenc, fref, &bmv, 0, -1, 0, 1, -1, 0, 1, 0, costs, width, height);
		COPY2_IF_LT(bcost, costs[0], dir, 1);
		COPY2_IF_LT(bcost, costs[1], dir, 2);
		COPY2_IF_LT(bcost, costs[2], dir, 3);
		COPY2_IF_LT(bcost, costs[3], dir, 4);
		COST_MV_X4_DIR(&motionEstimate->bitcost, stride, fenc, fref, &bmv, -1, -1, -1, 1, 1, -1, 1, 1, costs, width, height);
		COPY2_IF_LT(bcost, costs[0], dir, 5);
		COPY2_IF_LT(bcost, costs[1], dir, 6);
		COPY2_IF_LT(bcost, costs[2], dir, 7);
		COPY2_IF_LT(bcost, costs[3], dir, 8);

		bmv.x += square1_x[dir];
		bmv.y += square1_y[dir];
		break;
	}

	default:
		X265_CHECK(0, "invalid motion estimate mode\n");
		break;
	}

	if (bprecost < bcost)//如果当前搜索的最优mv的编码代价不如周边块预测mv的编码代价，更新当前的最优mv：bmv 和最优cost：bcost
	{
		bmv = bestpre;
		bcost = bprecost;
	}

	*outQMv = bmv;
	return bcost;
	*/return 0;
}
