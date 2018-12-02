/*
* rdcost.c
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#include "common.h"
#include "constants.h"
#include "rdcost.h"
#include "primitives.h"
#include <math.h>
//#include "globledata.h"

void setPsyRdScale(RDCost* cost, double scale)
{
	double a = 0;
	a = 65536.0 * scale * 0.33;
	cost->m_psyRdBase = (uint32_t)a;//floor(65536.0 * scale * 0.33);
}

void setQP(RDCost* cost, const Slice* slice, int qp)
{
	//x265_emms(); /* TODO: if the lambda tables were ints, this would not be necessary */
	cost->m_qp = qp;
	setLambda(cost, x265_lambda2_tab[qp], x265_lambda_tab[qp]);

	/* Scale PSY RD factor by a slice type factor */
	static const uint32_t psyScaleFix8[3] = { 300, 256, 96 }; /* B, P, I */
	cost->m_psyRd = (cost->m_psyRdBase * psyScaleFix8[slice->m_sliceType]) >> 8;

	/* Scale PSY RD factor by QP, at high QP psy-rd can cause artifacts */
	if (qp >= 40)
	{
		int scale = qp >= QP_MAX_SPEC ? 0 : (QP_MAX_SPEC - qp) * 23;
		cost->m_psyRd = (cost->m_psyRd * scale) >> 8;
	}

	int qpCb, qpCr;
	if (slice->m_sps->chromaFormatIdc == X265_CSP_I420)
	{
		qpCb = (int)g_chromaScale[x265_clip3_int(QP_MIN, QP_MAX_MAX, qp + slice->m_pps->chromaQpOffset[0])];
		qpCr = (int)g_chromaScale[x265_clip3_int(QP_MIN, QP_MAX_MAX, qp + slice->m_pps->chromaQpOffset[1])];
	}
	else
	{
		qpCb = x265_clip3_int(QP_MIN, QP_MAX_SPEC, qp + slice->m_pps->chromaQpOffset[0]);
		qpCr = x265_clip3_int(QP_MIN, QP_MAX_SPEC, qp + slice->m_pps->chromaQpOffset[1]);
	}

	int chroma_offset_idx = X265_MIN(qp - qpCb + 12, MAX_CHROMA_LAMBDA_OFFSET);
	uint16_t lambdaOffset = cost->m_psyRd ? x265_chroma_lambda2_offset_tab[chroma_offset_idx] : 256;
	cost->m_chromaDistWeight[0] = lambdaOffset;

	chroma_offset_idx = X265_MIN(qp - qpCr + 12, MAX_CHROMA_LAMBDA_OFFSET);
	lambdaOffset = cost->m_psyRd ? x265_chroma_lambda2_offset_tab[chroma_offset_idx] : 256;
	cost->m_chromaDistWeight[1] = lambdaOffset;
}


void setLambda(RDCost* cost, double lambda2, double lambda)
{
	cost->m_lambda2 = (uint64_t)floor(256.0 * lambda2);
	cost->m_lambda = (uint64_t)floor(256.0 * lambda);
}

uint64_t calcRdCost(const RDCost* cost, uint32_t distortion, uint32_t bits)
{
	//X265_CHECK(bits <= (UINT32_MAX - 128) /cost->m_lambda2,
	//"calcRdCost wrap detected dist: %u, bits %u, lambda: "X265_LL"\n"/*, distortion, bits, cost->m_lambda2*/);
	return distortion + ((bits * cost->m_lambda2 + 128) >> 8);
}

/* return the difference in energy between the source block and the recon block */
int psyCost(int size, const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride)
{
	//return primitives.cu[size].psy_cost_pp(source, sstride, recon, rstride);
	return primitives.cu[size].psy_cost_pp(source, sstride, recon, rstride, size);
}

/* return the difference in energy between the source block and the recon block */
int psyCost_int16_t(int size, const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride)
{
	return primitives.cu[size].psy_cost_ss(source, sstride, recon, rstride, size);
}

/* return the RD cost of this prediction, including the effect of psy-rd */
uint64_t calcPsyRdCost(const RDCost* cost, uint32_t distortion, uint32_t bits, uint32_t psycost)
{
	return distortion + ((cost->m_lambda * cost->m_psyRd * psycost) >> 24) + ((bits * cost->m_lambda2) >> 8);
}

uint64_t calcRdSADCost(RDCost* cost, uint32_t sadCost, uint32_t bits)
{
	return sadCost + ((bits * cost->m_lambda + 128) >> 8);
}

uint32_t scaleChromaDist(RDCost* cost, uint32_t plane, uint32_t dist)
{
	return (uint32_t)((dist * 256 + 128) >> 8);
}

uint32_t getCost(RDCost* cost, uint32_t bits)
{
	return (uint32_t)((bits * cost->m_lambda + 128) >> 8);
}

