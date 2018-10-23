/*
* framefiletr.h
*
*  Created on: 2017-8-22
*      Author: Administrator
*/

#ifndef FRAMEFILETR_H_
#define FRAMEFILETR_H_

#include "common.h"
#include "frame.h"
#include "deblock.h"
#include "frameencoder.h"
#include "encoder.h"


// Manages the processing of a single frame loopfilter
typedef struct FrameFilter
{

	x265_param*   m_param;
	Frame*        m_frame;
	struct FrameEncoder* m_frameEncoder;
	int           m_hChromaShift;
	int           m_vChromaShift;
	int           m_pad[2];

	//SAO           m_sao;
	int           m_numRows;
	int           m_saoRowDelay;
	int           m_lastHeight;

	void*         m_ssimBuf; /* Temp storage for ssim computation */
}FrameFilter;

void FrameFilter_init(FrameFilter *filter, struct Encoder *top, struct FrameEncoder *frame, int numRows);
void FrameFilter_FrameFilter(FrameFilter *filter);
void FrameFilter_destroy(FrameFilter *filter);
void FrameFilter_start(FrameFilter *filter, Frame *frame, Entropy*initState, int qp);
void FrameFilter_processRow(FrameFilter *filter, int row);
uint32_t FrameFilter_getCUHeight(FrameFilter *filter, int rowNum);
void FrameFilter_processRowPost(FrameFilter *filter, int row);
static uint64_t computeSSD(pixel *fenc, pixel *rec, intptr_t stride, uint32_t width, uint32_t height);
static float calculateSSIM(pixel *pix1, intptr_t stride1, pixel *pix2, intptr_t stride2, uint32_t width, uint32_t height, void *buf, uint32_t* cnt);


#endif /* FRAMEFILETR_H_ */
