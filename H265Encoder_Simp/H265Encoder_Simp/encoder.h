
#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "common.h"
#include "frame.h"
#include "slice.h"
#include "nal.h"
#include "scalinglist.h"
#include "x265.h"
//#include "dpb.h"
//#include "frameencoder.h"
struct EncStats
{
	double        m_psnrSumY;
	double        m_psnrSumU;
	double        m_psnrSumV;
	double        m_globalSsim;
	double        m_totalQp;
	uint64_t      m_accBits;
	uint32_t      m_numPics;
};

typedef struct Encoder
{
	int                m_pocLast;         // time index (POC)
	int                m_encodedFrameNum;
	int                m_outputCount;

	int                m_bframeDelay;
	int64_t            m_firstPts;
	int64_t            m_bframeDelayTime;
	int64_t            m_prevReorderedPts[2];

	struct FrameEncoder*      m_frameEncoder;
	struct DPB*               m_dpb;
	Frame*             m_exportedPic;
	int                m_numPools;
	int                m_curEncoder;

	/* cached PicYuv offset arrays, shared by all instances of
	* PicYuv created by this encoder */
	intptr_t*          m_cuOffsetY;
	intptr_t*          m_cuOffsetC;
	intptr_t*          m_buOffsetY;
	intptr_t*          m_buOffsetC;

	/* Collect statistics globally */
	struct EncStats           m_analyzeAll;
	struct EncStats           m_analyzeI;
	struct EncStats           m_analyzeP;
	struct EncStats           m_analyzeB;
	int64_t            m_encodeStartTime;

	// weighted prediction
	int                m_numLumaWPFrames;    // number of P frames with weighted luma reference
	int                m_numChromaWPFrames;  // number of P frames with weighted chroma reference
	int                m_numLumaWPBiFrames;  // number of B frames with weighted luma reference
	int                m_numChromaWPBiFrames; // number of B frames with weighted chroma reference

	int                m_conformanceMode;
	struct VPS                m_vps;
	struct SPS                m_sps;
	struct PPS                m_pps;
	struct NALList            m_nalList;
	ScalingList        *m_scalingList;      // quantization matrix information

	int                m_lastBPSEI;
	uint32_t           m_numDelayedPic;

	x265_param*        m_param;
	x265_param*        m_latestParam;
	struct Lookahead*         m_lookahead;
	//Window             m_conformanceWindow;

	bool               m_bZeroLatency;     // x265_encoder_encode() returns NALs for the input picture, zero lag
	bool               m_aborted;          // fatal error detected
	bool               m_reconfigured;      // reconfigure of encoder detected

}Encoder;

void Encoder_init(Encoder *encoder);
void Encoder_create(Encoder *encoder);
void Encoder_initVPS(struct Encoder *encoder, VPS *vps);
void Encoder_initSPS(struct Encoder *encoder, SPS *sps);
void Encoder_initPPS(struct Encoder *encoder, PPS *pps);
int encode(Encoder *enc, x265_picture* pic_in, x265_picture* pic_out);
void Encoder_configure(Encoder * encoder, x265_param *p);
void Encoder_destroy(Encoder *encoder);

#endif /* ENCODER_H_ */
