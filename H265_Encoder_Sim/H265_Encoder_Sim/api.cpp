/*
* api.c
*
*  Created on: 2015-11-5
*      Author: adminster
*/

#include "common.h"
#include "param.h"
#include "api.h"
#include "level.h"
#include "nal.h"
#include "constants.h"
#include "nal.h"
#include <string.h>
#include <stdlib.h>

Encoder *x265_encoder_open(x265_param *p)
{
	if (!p)
		return NULL;
	Encoder *encoder = NULL;
	if (x265_set_globals(p))
		goto fail;
	
	x265_setup_primitives();
	
	encoder = (Encoder *)malloc(sizeof(Encoder));//完成所有帧编码后释放
	//printf("sizeof(Encoder)=%d\n",sizeof(Encoder));
	if (!encoder)
		printf("malloc Encoder fail!\n");
	
	HRDInfo_Info(&encoder->m_vps.hrdParameters);
	Window_dow(&encoder->m_sps.conformanceWindow);
	HRDInfo_Info(&encoder->m_sps.vuiParameters.hrdParameters);
	Window_dow(&encoder->m_sps.vuiParameters.defaultDisplayWindow);
	NAL_nal(&encoder->m_nalList);
	Window_dow(&encoder->m_conformanceWindow);
	
	Encoder_init(encoder);
	Encoder_configure(encoder, p);
	
	// may change rate control and CPB params
	if (!enforceLevel(p, &encoder->m_vps))
		goto fail;

	// will detect and set profile/tier/level in VPS
	determineLevel(p, &encoder->m_vps);

	Encoder_create(encoder);

	if (encoder->m_aborted)
		goto fail;

	return (Encoder *)encoder;
	
fail:
	free(encoder);
	encoder = NULL;
	return NULL;
}

void x265_encoder_parameters(Encoder *enc, x265_param *out)
{
	if (enc && out)
	{
		struct Encoder *encoder = (struct Encoder*)(enc);
		memcpy(out, encoder->m_param, sizeof(x265_param));
	}
}

void x265_picture_init(x265_param *param, x265_picture *pic)
{
	memset(pic, 0, sizeof(x265_picture));
	pic->bitDepth = param->internalBitDepth;
	pic->forceqp = X265_QP_AUTO;
	if (param->analysisMode)
	{
		uint32_t widthInCU = (param->sourceWidth + g_maxCUSize - 1) >> g_maxLog2CUSize;
		uint32_t heightInCU = (param->sourceHeight + g_maxCUSize - 1) >> g_maxLog2CUSize;
		uint32_t numCUsInFrame = widthInCU*heightInCU;
		pic->analysisData.numCUsInFrame = numCUsInFrame;
		pic->analysisData.numPartitions = NUM_4x4_PARTITIONS;
	}
}

void  getStreamHeaders(Encoder *encoder, NALList *list, Entropy* sbacCoder, Bitstream* bs)
{
	
	setBitstream(sbacCoder, bs);

	// headers for start of bitstream //
	resetBits(bs);
	codeVPS(sbacCoder, &encoder->m_vps);
	writeByteAlignment(bs);
	serialize(list, NAL_UNIT_VPS, bs);

	resetBits(bs);
	codeSPS(sbacCoder, &encoder->m_sps, encoder->m_scalingList, &encoder->m_vps.ptl);
	writeByteAlignment(bs);
	serialize(list, NAL_UNIT_SPS, bs);

	resetBits(bs);
	codePPS(sbacCoder, &encoder->m_pps);
	writeByteAlignment(bs);
	serialize(list, NAL_UNIT_PPS, bs);
	
}
int x265_encoder_headers(Encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal)
{
	if (pp_nal && enc)
	{
		//Encoder *encoder = static_cast<Encoder*>(enc);
		Entropy sbacCoder;
		Entropy_entropy(&sbacCoder);
		Bitstream bs;
		push(&bs);
		getStreamHeaders(enc, &enc->m_nalList, &sbacCoder, &bs);
		*pp_nal = &enc->m_nalList.m_nal[0];
		if (pi_nal) *pi_nal = enc->m_nalList.m_numNal;
		return enc->m_nalList.m_occupancy;
	}
	
	return -1;
}

int x265_encoder_encode(Encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal, x265_picture *pic_in, x265_picture *pic_out)
{/*
	if (!enc)
		return -1;

	Encoder *encoder = enc;
	int numEncoded;

	// While flushing, we cannot return 0 until the entire stream is flushed
	do
	{
		numEncoded = encode(enc, pic_in, pic_out);
	} while (numEncoded == 0 && !pic_in && encoder->m_numDelayedPic);

	if (pp_nal && numEncoded > 0)
	{
		*pp_nal = &encoder->m_nalList.m_nal[0];
		if (pi_nal) *pi_nal = encoder->m_nalList.m_numNal;
	}
	else if (pi_nal)
		*pi_nal = 0;
	return numEncoded;*/return 0;
}

x265_picture *x265_picture_alloc()
{
	//printf("sizeof(x265_picture)=\n",sizeof(x265_picture));
	return (x265_picture*)malloc(sizeof(x265_picture));//完成所有帧编码后释放
}

void x265_picture_free(x265_picture *p)
{
	return free(p);
}

void x265_encoder_close(Encoder *enc)
{
	if (enc)
	{
		Encoder_destroy(enc);
		free(enc);
		enc = NULL;
	}
}
