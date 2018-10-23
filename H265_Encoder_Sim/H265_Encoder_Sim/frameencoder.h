/*
* frameencoder.h
*
*  Created on: 2015-11-6
*      Author: adminster
*/
#ifndef FRAMEENCODER_H_
#define FRAMEENCODER_H_

#include "common.h"
#include "Bitstream.h"
#include "frame.h"
#include "picyuv.h"
#include "analysis.h"
#include "entropy.h"
#include "reference.h"
#include "framefilter.h"
#include "nal.h"

#define ANGULAR_MODE_ID 2
#define AMP_ID 3
#define INTER_MODES 4
#define INTRA_MODES 3

struct StatisticLog
{
	uint64_t cntInter[4];
	uint64_t cntIntra[4];
	uint64_t cuInterDistribution[4][INTER_MODES];
	uint64_t cuIntraDistribution[4][INTRA_MODES];
	uint64_t cntIntraNxN;
	uint64_t cntSkipCu[4];
	uint64_t cntTotalCu[4];
	uint64_t totalCu;
};

/* manages the state of encoding one row of CTU blocks.  When
* WPP is active, several rows will be simultaneously encoded. */
typedef struct CTURow
{
	Entropy           bufferedEntropy;  /* store CTU2 context for next row CTU0 */
	Entropy           rowGoOnCoder;     /* store context between CTUs, code bitstream if !SAO */

	/* row is ready to run, has no neighbor dependencies. The row may have
	* external dependencies (reference frame pixels) that prevent it from being
	* processed, so it may stay with m_active=true for some time before it is
	* encoded by a worker thread. */
	bool     active;

	/* row is being processed by a worker thread.  This flag is only true when a
	* worker thread is within the context of FrameEncoder::processRow(). This
	* flag is used to detect multiple possible wavefront problems. */
	bool     busy;

	/* count of completed CUs in this row */
	uint32_t completed;

}CTURow;

// Manages the wave-front processing of a single encoding frame
typedef struct FrameEncoder
{
	int                      m_localTldIdx;
	int						 m_sliceType;
	volatile bool            m_threadActive;
	volatile bool            m_bAllRowsStop;
	volatile int             m_completionCount;
	volatile int             m_vbvResetTriggerRow;
	bool          			 m_isFrameEncoder; /* rather ugly hack, but nothing better presents itself */
	uint32_t                 m_numRows;
	uint32_t                 m_numCols;
	uint32_t                 m_filterRowDelay;
	uint32_t                 m_filterRowDelayCus;
	uint32_t                 m_refLagRows;

	CTURow*                  m_rows;

	uint64_t                 m_SSDY;
	uint64_t                 m_SSDU;
	uint64_t                 m_SSDV;
	double                   m_ssim;
	uint64_t                 m_accessUnitBits;
	uint32_t                 m_ssimCnt;

	uint32_t                 m_crc[3];
	uint32_t                 m_checksum[3];
	struct StatisticLog             m_sliceTypeLog[3];     // per-slice type CU statistics

	volatile int             m_activeWorkerCount;        // count of workers currently encoding or filtering CTUs
	volatile int             m_totalActiveWorkerCount;   // sum of m_activeWorkerCount sampled at end of each CTU
	volatile int             m_activeWorkerCountSamples; // count of times m_activeWorkerCount was sampled (think vbv restarts)
	volatile int             m_countRowBlocks;           // count of workers forced to abandon a row because of top dependency
	int64_t                  m_startCompressTime;        // timestamp when frame encoder is given a frame
	int64_t                  m_row0WaitTime;             // timestamp when row 0 is allowed to start
	int64_t                  m_allRowsAvailableTime;     // timestamp when all reference dependencies are resolved
	int64_t                  m_endCompressTime;          // timestamp after all CTUs are compressed
	int64_t                  m_endFrameTime;             // timestamp after RCEnd, NR updates, etc
	int64_t                  m_stallStartTime;           // timestamp when worker count becomes 0
	int64_t                  m_prevOutputTime;           // timestamp when prev frame was retrieved by API thread
	int64_t                  m_slicetypeWaitTime;        // total elapsed time waiting for decided frame
	int64_t                  m_totalWorkerElapsedTime;   // total elapsed time spent by worker threads processing CTUs
	int64_t                  m_totalNoWorkerTime;        // total elapsed time without any active worker threads

	struct Encoder*          m_top;
	x265_param*              m_param;
	struct Frame*            m_frame;
	ThreadLocalData*         m_tld; /* for --no-wpp */
	Bitstream*               m_outStreams;
	uint32_t*                m_substreamSizes;

	struct CUGeom*                  m_cuGeoms;
	uint32_t*                m_ctuGeomMap;

	Bitstream                m_bs;
	struct MotionReference          m_mref[2][MAX_NUM_REF + 1];
	Entropy                  m_entropyCoder;//帧的头信息
	Entropy                  m_initSliceContext;//主路熵编码
	struct FrameFilter              *m_frameFilter;
	struct NALList                  m_nalList;
}FrameEncoder;

void FrameEncoder_FrameEncoder(FrameEncoder *frencoder);
bool FrameEncoder_init(FrameEncoder *frencoder, struct Encoder *top, int numRows, int numCols);
bool FrameEncoder_initializeGeoms(FrameEncoder *frencoder);
void FrameEncoder_compressFrame(FrameEncoder *frencoder, Analysis *analysis);
void FrameEncoder_processRowEncoder(FrameEncoder *frencoder, int intRow, Analysis *tld);
bool FrameEncoder_startCompressFrame(FrameEncoder *frencoder, Frame* curFrame);
void CTURow_init(CTURow *cturow, Entropy *initContext);
void FrameEncoder_destroy(FrameEncoder *frencoder);
Frame *FrameEncoder_getEncodedPicture(FrameEncoder *frameE, NALList* output);

#endif /* FRAMEENCODER_H_ */
