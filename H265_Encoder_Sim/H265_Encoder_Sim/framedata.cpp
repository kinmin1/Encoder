/*
* framedata.c
*
*  Created on: 2015-10-28
*      Author: adminster
*/
#include"framedata.h"
#include "picyuv.h"
#include "common.h"
#include<string.h>

void FrameData_FrameData(FrameData* framedata)
{
	memset(framedata, 0, sizeof(struct FrameData));
}

void FrameData_reinit(FrameData *framedata, struct SPS *sps)
{
	memset(framedata->m_cuStat, 0, sps->numCUsInFrame * sizeof((framedata->m_cuStat)));
	memset(framedata->m_rowStat, 0, sps->numCuInHeight * sizeof((framedata->m_rowStat)));
}

struct CUData* framedata_getPicCTU(FrameData *framedata, uint32_t ctuAddr)
{
	return &framedata->m_picCTU[ctuAddr];
}
Slice slice_1;
CUData cudata_1;
CUDataMemPool cudatamenpool_1;
RCStatCU rcstatcu_1;
RCStatRow rcststrow_1;
bool FrameData_create(FrameData *framedata, x265_param *param, struct SPS *sps)
{
	framedata->m_param = param;
	framedata->m_slice = &slice_1;// (Slice *)malloc(sizeof(Slice));//已释放
	//printf("sizeof(Slice)=%d\n",sizeof(Slice));
	if (!framedata->m_slice)
		printf("malloc Slice fail!\n");

	framedata->m_picCTU = &cudata_1;// (CUData *)malloc(sizeof(CUData)*sps->numCUsInFrame);
	//printf("sizeof(CUData)*sps->numCUsInFrame=%d\n",sizeof(Slice)*sps->numCUsInFrame);//-=-=-=-=-=-=-
	if (!framedata->m_picCTU)
		printf("malloc CUData fail!\n");

	framedata->m_cuMemPool = &cudatamenpool_1;// (CUDataMemPool*)malloc(sizeof(CUDataMemPool));//已释放
	//printf("sizeof(CUDataMemPool)=%d\n",sizeof(CUDataMemPool));//-=-=-=-=-=-=-
	if (!framedata->m_cuMemPool)
		printf("malloc CUDataMemPool fail!\n");

	CUDataMemPool_init(framedata->m_cuMemPool);
	CUDataMemPool_create_frame(framedata->m_cuMemPool, 0, sps->numCUsInFrame);
	for (uint32_t ctuAddr = 0; ctuAddr < sps->numCUsInFrame; ctuAddr++)
	{
		CUData_initialize(&(framedata->m_picCTU[ctuAddr]), framedata->m_cuMemPool, 0, ctuAddr);
		//framedata->m_picCTU += 1;
	}
	//framedata->m_picCTU -= sps->numCUsInFrame;
	//CHECKED_MALLOC(framedata->m_cuStat, RCStatCU, sps->numCUsInFrame);

	framedata->m_cuStat = &rcstatcu_1;
	framedata->m_rowStat = &rcststrow_1;
	//printf("sizeof(RCStatCU) * (sps->numCUsInFrame)=%d\n",sizeof(RCStatCU) * (sps->numCUsInFrame));
	//CHECKED_MALLOC(framedata->m_rowStat, RCStatRow, sps->numCuInHeight);
	//printf("sizeof(RCStatRow) * (sps->numCuInHeight)=%d\n",sizeof(RCStatRow) * (sps->numCuInHeight));
	FrameData_reinit(framedata, sps);
	return TRUE;

fail:
	return FALSE;
}

void FrameData_destory(FrameData *framedata)
{
	free(framedata->m_picCTU); framedata->m_picCTU = NULL;
	free(framedata->m_slice); framedata->m_slice = NULL;
	free(framedata->m_cuStat); framedata->m_cuStat = NULL;
	CUMemPool_destory(framedata->m_cuMemPool);
}
