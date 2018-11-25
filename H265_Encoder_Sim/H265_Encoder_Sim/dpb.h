/*
* dpb.h
*
*  Created on: 2016-8-2
*      Author: SCL
*/

#ifndef DPB_H_
#define DPB_H_

#include "piclist.h"
#include "framedata.h"
#include "frame.h"
#include "frameencoder.h"

typedef struct DPB
{
	int                m_lastIDR;
	int                m_pocCRA;
	int                m_maxRefL0;
	int                m_maxRefL1;
	int                m_bOpenGOP;
	bool               m_bRefreshPending;
	bool               m_bTemporalSublayer;
	struct PicList            m_picList;
	struct PicList            m_freeList;
	FrameData*         m_picSymFreeList;
}DPB;

void DPB_Destroy(DPB *dpb);
void DPB_init(DPB *dpb, x265_param *param);
void DPB_prepareEncode(DPB * dpb, Frame *newFrame);
void DPB_prepareEncode2(DPB * dpb, Frame *newFrame);
int getNalUnitType(DPB *dpb, int curPOC, bool bIsKeyFrame);
void DPB_decodingRefreshMarking(DPB * dpb, int pocCurr, NalUnitType nalUnitType);
void DP_computeRPS(DPB * dpb, int curPoc, bool isRAP, RPS * rps, unsigned int maxDecPicBuffer);
void DPB_applyReferencePictureSet(DPB *dpb, RPS *rps, int curPoc);
void DPB_recycleUnreferenced(DPB* dpb);

#endif /* DPB_H_ */
