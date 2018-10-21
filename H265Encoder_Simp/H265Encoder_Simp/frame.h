
#ifndef FRAME_H_
#define FRAME_H_

#include "common.h"
//#include "lowres.h"
#include "x265.h"

struct SPS;

typedef struct Frame
{
	/* These two items will be NULL until the Frame begins to be encoded, at which point
	* it will be assigned a FrameData instance, which comes with a reconstructed image PicYuv */
	struct FrameData*      m_encData;
	struct PicYuv*         m_reconPic;

	/* Data associated with x265_picture */
	struct PicYuv*         m_fencPic;
	int                    m_poc;
	int64_t                m_pts;                // user provided presentation time stamp
	int64_t                m_reorderedPts;
	int64_t                m_dts;
	int32_t                m_forceqp;            // Force to use the qp specified in qp file
	void*                  m_userData;           // user provided pointer passed in with this picture

	struct Lowres          *m_lowres;
	int                    m_lowresInit;         // lowres init complete (pre-analysis)
	int                    m_bChromaExtended;    // orig chroma planes motion extended for weight analysis

	/* Frame Parallelism - notification between FrameEncoders of available motion reference rows */
	//ThreadSafeInteger      m_reconRowCount;      // count of CTU rows completely reconstructed and extended for motion reference
	uint32_t      m_countRefEncoders;   // count of FrameEncoder threads monitoring m_reconRowCount

	struct Frame*          m_next;               // PicList doubly linked list pointers
	struct Frame*          m_prev;
	x265_param*     m_param;              // Points to the latest param set for the frame.
	x265_analysis_data     m_analysisData;
}Frame;

void Frame_init(Frame *frame);

int Frame_create(Frame *frame, x265_param *param);

int Frame_allocEncodeData(Frame *frame, x265_param *param, struct SPS *sps);

void Frame_destroy(Frame *frame);

#endif /* FRAME_H_ */
