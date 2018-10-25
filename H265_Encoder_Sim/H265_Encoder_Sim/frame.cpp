/*
* frame.c
*
*  Created on: 2015-10-30
*      Author: adminster
*/
#include "frame.h"
#include<string.h>
#include "frameencoder.h"

bool IS_REFERENCED(Frame *frame)
{/*
	return (frame->m_analysisData.sliceType != X265_TYPE_B);*/return 0;
}
void Frame_init(Frame *frame)
{/*
	frame->m_bChromaExtended = FALSE;
	frame->m_lowresInit = FALSE;

	frame->m_countRefEncoders = 0;
	frame->m_encData = NULL;
	frame->m_reconPic = NULL;
	frame->m_next = NULL;
	frame->m_prev = NULL;
	frame->m_param = NULL;*/
}

int Frame_create(Frame *frame, x265_param *param)
{
	/*
	frame->m_fencPic = (PicYuv *)malloc(sizeof(PicYuv));//&Pic_m_fencPic;//
	//printf("sizeof(PicYuv)=%d\n",sizeof(PicYuv));
	if (!frame->m_fencPic)
	printf("malloc PicYuv fail!");

	frame->m_param = param;
	return PicYuv_create(frame->m_fencPic, param->sourceWidth, param->sourceHeight);*/return 1;
}

int Frame_allocEncodeData(Frame *frame, x265_param *param, struct SPS *sps)
{/*
	int num1 = 0, num2 = 0;
	frame->m_encData = (FrameData *)malloc(sizeof(FrameData));//&Frame_m_encData;
	//printf("sizeof(FrameData)=%d\n",sizeof(FrameData));
	if (!frame->m_encData)
		printf("malloc FrameData memory fail!\n");

	frame->m_reconPic = (PicYuv *)malloc(sizeof(PicYuv));//&Pic_m_reconPic;
	//printf("sizeof(PicYuv)=%d\n",sizeof(PicYuv));
	if (!frame->m_reconPic)
		printf("malloc m_reconPic memory fail!\n");

	if (!PicYuv_createOffsets_recon(frame->m_reconPic, sps))
		printf("create offset reconpic fail!\n");

	frame->m_encData->m_reconPic = frame->m_reconPic;
	num1 = FrameData_create(frame->m_encData, param, sps);
	num2 = PicYuv_create_reconPic(frame->m_reconPic, param->sourceWidth, param->sourceHeight);

	int ok = num1 && num2;
	if (!PicYuv_createOffsets_recon(frame->m_reconPic, sps))
		printf("create offset reconpic fail!\n");

	if (ok)
	{
		// initialize right border of m_reconpicYuv as SAO may read beyond the
		// end of the picture accessing uninitialized pixels //
		int maxHeight = sps->numCuInHeight * g_maxCUSize;
		memset(frame->m_reconPic->m_picOrg[0], 0, sizeof(pixel) * frame->m_reconPic->m_stride * maxHeight);
		memset(frame->m_reconPic->m_picOrg[1], 0, sizeof(pixel) * frame->m_reconPic->m_strideC * (maxHeight >> 2));
		memset(frame->m_reconPic->m_picOrg[2], 0, sizeof(pixel) * frame->m_reconPic->m_strideC * (maxHeight >> 2));
	}
	*/
	return TRUE;
}

void Frame_destroy(Frame *frame)
{/*
	if(frame->m_encData)
	{
		FrameData_destory(frame->m_encData);
		free(frame->m_encData);
		frame->m_encData = NULL;
	}

	if (frame->m_fencPic)
	{
		PicYuv_destroy(frame->m_fencPic);
		free(frame->m_fencPic);
		frame->m_fencPic = NULL;
	}

	if (frame->m_reconPic)
	{
		PicYuv_destroy(frame->m_reconPic);
		free(frame->m_reconPic);
		frame->m_reconPic = NULL;
	}*/
}
