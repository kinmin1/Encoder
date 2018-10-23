/*
* rdcost.h
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#ifndef RDCOST_H_
#define RDCOST_H_

#include "common.h"
#include "slice.h"

typedef struct RDCost
{
	/* all weights and factors stored as FIX8 */
	uint64_t  m_lambda2;
	uint64_t  m_lambda;
	uint32_t  m_chromaDistWeight[2];
	uint32_t  m_psyRdBase;
	uint32_t  m_psyRd;
	int       m_qp; /* QP used to configure lambda, may be higher than QP_MAX_SPEC but <= QP_MAX_MAX */

}RDCost;

void setPsyRdScale(RDCost* cost, double scale);

void setQP(RDCost* cost, const Slice* slice, int qp);

void setLambda(RDCost* cost, double lambda2, double lambda);

uint64_t calcRdCost(const RDCost* cost, uint32_t distortion, uint32_t bits);

int psyCost(int size, const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);

int psyCost_int16_t(int size, const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);

uint64_t calcPsyRdCost(const RDCost* cost, uint32_t distortion, uint32_t bits, uint32_t psycost);

uint64_t calcRdSADCost(RDCost* cost, uint32_t sadCost, uint32_t bits);

uint32_t scaleChromaDist(RDCost* cost, uint32_t plane, uint32_t dist);

uint32_t getCost(RDCost* cost, uint32_t bits);

#endif /* RDCOST_H_ */


