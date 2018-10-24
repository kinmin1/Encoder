/*
* slicetype.h
*
*  Created on: 2015-11-6
*      Author: adminster
*/

#ifndef SLICETYPE_H_
#define SLICETYPE_H_

#include "common.h"
#include "slice.h"
#include "motion.h"
#include "piclist.h"

struct Lowres;
struct Frame;
struct Lookahead;

#define LOWRES_COST_MASK  ((1 << 14) - 1)
#define LOWRES_COST_SHIFT 14
struct Lookahead
{
	struct PicList       m_inputQueue;      // input pictures in order received
	struct PicList       m_outputQueue;     // pictures to be encoded, in encode order

	/* pre-lookahead */
	int           m_fullQueueSize;
	bool          m_isActive;
	bool          m_sliceTypeBusy;
	bool          m_bAdaptiveQuant;
	bool          m_outputSignalRequired;
	bool          m_bBatchMotionSearch;
	bool          m_bBatchFrameCosts;

	x265_param*   m_param;
	int*          m_scratch;         // temp buffer for cutree propagate

	int           m_histogram[X265_BFRAME_MAX + 1];
	int           m_lastKeyframe;
	int           m_8x8Width;
	int           m_8x8Height;
	int           m_8x8Blocks;
	int           m_numCoopSlices;
	int           m_numRowsPerSlice;
	bool          m_filled;
};
#endif /* SLICETYPE_H_ */
