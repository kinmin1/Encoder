/*
* bitcost.h
*
*  Created on: 2017-5-17
*      Author: Administrator
*/

#ifndef BITCOST_H_
#define BITCOST_H_

#include "common.h"
#include "mv.h"

#define BC_MAX_MV 1 << 10

#define BC_MAX_QP  82

typedef struct BitCost
{
	uint16_t *m_cost_mvx;

	uint16_t *m_cost_mvy;

	uint16_t *m_cost;

	MV        m_mvp;

	/* default log2_max_mv_length_horizontal and log2_max_mv_length_horizontal
	* are 15, specified in quarter-pel luma sample units. making the maximum
	* signaled ful-pel motion distance 4096, max qpel is 32768 */
	float *s_bitsizes;
	uint16_t *s_costs[BC_MAX_QP];

}BitCost;

uint16_t mvcost(BitCost *bitCost, int16_t x, int16_t y);
uint32_t bitcost_1(BitCost *bitcost, const MV* mv);
uint32_t bitcost_2(BitCost *bitcost, const MV *mv, const MV* mvp);
void setMVP(BitCost *bitCost, MV* mvp);
void BitCost_init(BitCost * bitcost, unsigned int qp);
void BitCost_setQP(BitCost *bitCost, unsigned int qp);
void BitCost_destroy(BitCost *bitcost);

#endif /* BITCOST_H_ */
