#include "bitcost.h"
#include "common.h"
#include "primitives.h"
#include "math.h"
#include "constants.h"

float BitCost_s_bitsizes[2 * BC_MAX_MV + 1];
uint16_t BitCost_s_costs[4 * BC_MAX_MV + 1];

void setMVP(BitCost *bitCost, MV* mvp)
{
	bitCost->m_mvp = *mvp;
	bitCost->m_cost_mvx = bitCost->m_cost - mvp->x;
	bitCost->m_cost_mvy = bitCost->m_cost - mvp->y;
}
void BitCost_init(BitCost * bitcost, unsigned int qp)
{
	bitcost->m_cost_mvx = NULL;
	bitcost->m_cost_mvy = NULL;
	bitcost->m_cost = NULL;
	bitcost->m_mvp.x = 0;
	bitcost->m_mvp.y = 0;
	bitcost->m_mvp.word = 0;

	bitcost->s_bitsizes = NULL;
	bitcost->s_costs[qp] = NULL;
}
void CalculateLogs(BitCost *bitcost)
{
	if (!bitcost->s_bitsizes)
	{
		bitcost->s_bitsizes = BitCost_s_bitsizes;//(float *)malloc(sizeof(float)*(2 * BC_MAX_MV + 1));
		bitcost->s_bitsizes[0] = 0.718f;
		float log2_2 = 2.0f / log(2.0f);  // 2 x 1/log(2)//log()函数是以e为底的对数函数
		for (int i = 1; i <= 2 * BC_MAX_MV; i++)
			bitcost->s_bitsizes[i] = log((float)(i + 1)) * log2_2 + 1.718f;
	}
}

void BitCost_destroy(BitCost *bitcost)
{
	for (int i = 0; i < BC_MAX_QP; i++)
	{
		if (bitcost->s_costs[i])
		{
			free(bitcost->s_costs[i]);//(bitcost->s_costs[i] - 2 * BC_MAX_MV);

			bitcost->s_costs[i] = NULL;
		}
	}
}

void BitCost_setQP(BitCost *bitcost, unsigned int qp)
{
	if (!bitcost->s_costs[qp])
	{
		// Now that we have acquired the lock, check again if another thread calculated
		// this row while we were blocked
		if (!bitcost->s_costs[qp])
		{
			CalculateLogs(bitcost);
			bitcost->s_costs[qp] = BitCost_s_costs + (2 * BC_MAX_MV);//(uint16_t *)malloc((uint16_t)(4 * BC_MAX_MV + 1)) + (2 * BC_MAX_MV);
			double lambda = x265_lambda_tab[qp];

			// estimate same cost for negative and positive MVD
			for (int i = 0; i <= 2 * BC_MAX_MV; i++)
				bitcost->s_costs[qp][i] = bitcost->s_costs[qp][-i] = (uint16_t)X265_MIN(bitcost->s_bitsizes[i] * lambda + 0.5f, (1 << 16) - 1);
		}
	}

	bitcost->m_cost = bitcost->s_costs[qp];
}

// return bit cost of motion vector difference, multiplied by lambda
uint16_t mvcost(BitCost *bitCost, int16_t x, int16_t y)
{
	return bitCost->m_cost_mvx[x] + bitCost->m_cost_mvy[y];
}

// return bit cost of motion vector difference, without lambda
uint32_t bitcost_1(BitCost *bitcost, const MV* mv)
{
	return (uint32_t)(bitcost->s_bitsizes[abs(mv->x - bitcost->m_mvp.x)] +
		bitcost->s_bitsizes[abs(mv->y - bitcost->m_mvp.y)] + 0.5f);
}

uint32_t bitcost_2(BitCost *bitcost, const MV *mv, const MV* mvp)
{
	return (uint32_t)(bitcost->s_bitsizes[abs(mv->x - mvp->x)] +
		bitcost->s_bitsizes[abs(mv->y - mvp->y)] + 0.5f);
}
