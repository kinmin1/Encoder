/*
* encoder.c
*
*  Created on: 2015-11-6
*      Author: adminster
*/

#include "common.h"
#include "primitives.h"
#include "param.h"
#include "framedata.h"
#include "picyuv.h"
#include "encoder.h"
#include "slicetype.h"
#include "frameencoder.h"
#include "x265.h"
#include <stdlib.h>
#include <malloc.h>

const char g_sliceTypeToChar[] = { 'B', 'P', 'I' };
extern EncoderPrimitives primitives;
void Encoder_init(Encoder *encoder)
{
	encoder->m_conformanceWindow.leftOffset = 0;
	encoder->m_conformanceWindow.rightOffset = 0;
	encoder->m_conformanceWindow.bottomOffset = 0;
	encoder->m_aborted = FALSE;
	encoder->m_reconfigured = FALSE;
	encoder->m_encodedFrameNum = 0;
	encoder->m_pocLast = -1;
	encoder->m_curEncoder = 0;
	encoder->m_numLumaWPFrames = 0;
	encoder->m_numChromaWPFrames = 0;
	encoder->m_numLumaWPBiFrames = 0;
	encoder->m_numChromaWPBiFrames = 0;
	encoder->m_lookahead = NULL;

	encoder->m_exportedPic = NULL;
	encoder->m_numDelayedPic = 0;
	encoder->m_outputCount = 0;

	encoder->m_latestParam = NULL;
	encoder->m_cuOffsetY = NULL;
	encoder->m_cuOffsetC = NULL;
	encoder->m_buOffsetY = NULL;
	encoder->m_buOffsetC = NULL;

	encoder->m_frameEncoder = NULL;
	MotionEstimate_initScales();
}

void Encoder_create(Encoder *encoder)
{

	if (!primitives.pu[0].sad)
	{
		// this should be an impossible condition when using our public API, and indicates a serious bug.
		if (encoder->m_param)
			printf("Primitives must be initialized before encoder is created\n");
		exit(0);
	}

	x265_param* p = encoder->m_param;
	int rows = (p->sourceHeight + p->maxCUSize - 1) >> g_log2Size[p->maxCUSize];
	int cols = (p->sourceWidth + p->maxCUSize - 1) >> g_log2Size[p->maxCUSize];

	encoder->m_aborted = FALSE;
	encoder->m_frameEncoder = /*&FrameEncoder_1;*/(FrameEncoder*)malloc(sizeof(FrameEncoder));//完成所有帧编码后释放
	//printf("sizeof(FrameEncoder)=%d\n",sizeof(FrameEncoder));//---==-=-=-=-=-=-
	//if(!encoder->m_frameEncoder)
	//  printf( "malloc FrameEncoder failed!");

	encoder->m_scalingList = /*&ScalingList_1;*/(ScalingList*)malloc(sizeof(ScalingList));//完成所有帧编码后释放
	//printf("sizeof(ScalingList)=%d\n",sizeof(ScalingList));//--===-=-=-==-=-=
	//if(!encoder->m_scalingList)
	//printf("malloc ScalingList fail !\n");

	if (!(ScalingList_init(encoder->m_scalingList)))
	{
		printf("Unable to allocate scaling list arrays!\n");
		encoder->m_aborted = TRUE;
	}
	ScalingList_setupQuantMatrices(encoder->m_scalingList);

	encoder->m_dpb = /*&DPB_1;//*/(DPB *)malloc(sizeof(DPB));//完成所有帧编码后释放
	//if(!encoder->m_dpb)
	//	printf("Malloc DPB fail!");
	DPB_init(encoder->m_dpb, encoder->m_param);

	Encoder_initVPS(encoder, &encoder->m_vps);
	Encoder_initSPS(encoder, &encoder->m_sps);
	Encoder_initPPS(encoder, &encoder->m_pps);

	int numRows = (encoder->m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;
	int numCols = (encoder->m_param->sourceWidth + g_maxCUSize - 1) / g_maxCUSize;
	if (!FrameEncoder_init(encoder->m_frameEncoder, encoder, numRows, numCols))
	{
		printf("Unable to initialize frame encoder, aborting\n");
		encoder->m_aborted = TRUE;
	}
	//m_bZeroLatency = !m_param->bframes && !m_param->lookaheadDepth && m_param->frameNumThreads == 1;
	encoder->m_bZeroLatency = 0;
}

void Encoder_initVPS(Encoder *encoder, VPS *vps)
{
	/* Note that much of the VPS is initialized by determineLevel() */
	vps->ptl.progressiveSourceFlag = !encoder->m_param->interlaceMode;
	vps->ptl.interlacedSourceFlag = !!encoder->m_param->interlaceMode;
	vps->ptl.nonPackedConstraintFlag = FALSE;
	vps->ptl.frameOnlyConstraintFlag = !encoder->m_param->interlaceMode;
}

void Encoder_initSPS(Encoder *encoder, SPS *sps)
{
	sps->conformanceWindow = encoder->m_conformanceWindow;
	sps->chromaFormatIdc = encoder->m_param->internalCsp;
	sps->picWidthInLumaSamples = encoder->m_param->sourceWidth;
	sps->picHeightInLumaSamples = encoder->m_param->sourceHeight;
	sps->numCuInWidth = (encoder->m_param->sourceWidth + g_maxCUSize - 1) / g_maxCUSize;
	sps->numCuInHeight = (encoder->m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;
	sps->numCUsInFrame = sps->numCuInWidth * sps->numCuInHeight;
	sps->numPartitions = NUM_4x4_PARTITIONS;
	sps->numPartInCUSize = 1 << g_unitSizeDepth;

	sps->log2MinCodingBlockSize = g_maxLog2CUSize - g_maxCUDepth;
	sps->log2DiffMaxMinCodingBlockSize = g_maxCUDepth;
	uint32_t maxLog2TUSize = (uint32_t)g_log2Size[encoder->m_param->maxTUSize];
	sps->quadtreeTULog2MaxSize = X265_MIN(g_maxLog2CUSize, maxLog2TUSize);
	sps->quadtreeTULog2MinSize = 2;
	sps->quadtreeTUMaxDepthInter = encoder->m_param->tuQTMaxInterDepth;
	sps->quadtreeTUMaxDepthIntra = encoder->m_param->tuQTMaxIntraDepth;

	sps->bUseSAO = encoder->m_param->bEnableSAO;

	sps->bUseAMP = encoder->m_param->bEnableAMP;
	sps->maxAMPDepth = encoder->m_param->bEnableAMP ? g_maxCUDepth : 0;

	sps->maxTempSubLayers = encoder->m_param->bEnableTemporalSubLayers ? 2 : 1;
	sps->maxDecPicBuffering = encoder->m_vps.maxDecPicBuffering;
	sps->numReorderPics = encoder->m_vps.numReorderPics;
	sps->maxLatencyIncrease = encoder->m_vps.maxLatencyIncrease = encoder->m_param->bframes;

	sps->bUseStrongIntraSmoothing = encoder->m_param->bEnableStrongIntraSmoothing;
	sps->bTemporalMVPEnabled = encoder->m_param->bEnableTemporalMvp;

	VUI *vui = &sps->vuiParameters;
	vui->aspectRatioInfoPresentFlag = !!encoder->m_param->vui.aspectRatioIdc;
	vui->aspectRatioIdc = encoder->m_param->vui.aspectRatioIdc;
	vui->sarWidth = encoder->m_param->vui.sarWidth;
	vui->sarHeight = encoder->m_param->vui.sarHeight;

	vui->overscanInfoPresentFlag = encoder->m_param->vui.bEnableOverscanInfoPresentFlag;
	vui->overscanAppropriateFlag = encoder->m_param->vui.bEnableOverscanAppropriateFlag;

	vui->videoSignalTypePresentFlag = encoder->m_param->vui.bEnableVideoSignalTypePresentFlag;
	vui->videoFormat = encoder->m_param->vui.videoFormat;
	vui->videoFullRangeFlag = encoder->m_param->vui.bEnableVideoFullRangeFlag;

	vui->colourDescriptionPresentFlag = encoder->m_param->vui.bEnableColorDescriptionPresentFlag;
	vui->colourPrimaries = encoder->m_param->vui.colorPrimaries;
	vui->transferCharacteristics = encoder->m_param->vui.transferCharacteristics;
	vui->matrixCoefficients = encoder->m_param->vui.matrixCoeffs;

	vui->chromaLocInfoPresentFlag = encoder->m_param->vui.bEnableChromaLocInfoPresentFlag;
	vui->chromaSampleLocTypeTopField = encoder->m_param->vui.chromaSampleLocTypeTopField;
	vui->chromaSampleLocTypeBottomField = encoder->m_param->vui.chromaSampleLocTypeBottomField;

	vui->defaultDisplayWindow.bEnabled = encoder->m_param->vui.bEnableDefaultDisplayWindowFlag;
	vui->defaultDisplayWindow.rightOffset = encoder->m_param->vui.defDispWinRightOffset;
	vui->defaultDisplayWindow.topOffset = encoder->m_param->vui.defDispWinTopOffset;
	vui->defaultDisplayWindow.bottomOffset = encoder->m_param->vui.defDispWinBottomOffset;
	vui->defaultDisplayWindow.leftOffset = encoder->m_param->vui.defDispWinLeftOffset;

	vui->frameFieldInfoPresentFlag = !!encoder->m_param->interlaceMode;
	vui->fieldSeqFlag = !!encoder->m_param->interlaceMode;

	vui->hrdParametersPresentFlag = encoder->m_param->bEmitHRDSEI;

	vui->timingInfo.numUnitsInTick = encoder->m_param->fpsDenom;
	vui->timingInfo.timeScale = encoder->m_param->fpsNum;
}

void Encoder_initPPS(Encoder *encoder, PPS *pps)
{
	//bool bIsVbv = this->m_param->rc.vbvBufferSize > 0 && this->m_param->rc.vbvMaxBitrate > 0;
	//if (!this->m_param->bLossless && (this->m_param->rc.aqMode||bIsVbv))
	if (!encoder->m_param->bLossless)
	{
		pps->bUseDQP = TRUE;
		pps->maxCuDQPDepth = g_log2Size[encoder->m_param->maxCUSize] - g_log2Size[encoder->m_param->rc.qgSize];
		if (!(pps->maxCuDQPDepth <= 2))
			printf("max CU DQP depth cannot be greater than 2\n");
	}
	else
	{
		pps->bUseDQP = FALSE;
		pps->maxCuDQPDepth = 0;
	}

	pps->chromaQpOffset[0] = encoder->m_param->cbQpOffset;
	pps->chromaQpOffset[1] = encoder->m_param->crQpOffset;

	pps->bConstrainedIntraPred = encoder->m_param->bEnableConstrainedIntra;
	pps->bUseWeightPred = encoder->m_param->bEnableWeightedPred;
	pps->bUseWeightedBiPred = encoder->m_param->bEnableWeightedBiPred;
	pps->bTransquantBypassEnabled = encoder->m_param->bCULossless || encoder->m_param->bLossless;
	pps->bTransformSkipEnabled = encoder->m_param->bEnableTransformSkip;
	pps->bSignHideEnabled = encoder->m_param->bEnableSignHiding;

	pps->bDeblockingFilterControlPresent = !encoder->m_param->bEnableLoopFilter || encoder->m_param->deblockingFilterBetaOffset || encoder->m_param->deblockingFilterTCOffset;
	pps->bPicDisableDeblockingFilter = !encoder->m_param->bEnableLoopFilter;
	pps->deblockingFilterBetaOffsetDiv2 = encoder->m_param->deblockingFilterBetaOffset;
	pps->deblockingFilterTcOffsetDiv2 = encoder->m_param->deblockingFilterTCOffset;

	pps->bEntropyCodingSyncEnabled = 0;//encoder->m_param->bEnableWavefront;
}

int encode(Encoder *enc, x265_picture* pic_in, x265_picture* pic_out)
{
	if (enc->m_aborted)
		return -1;
	Frame *inFrame; //即将creat用于存储视频帧
	inFrame = (Frame *)malloc(sizeof(Frame));//申请空间
	if (0)
	{
		enc->m_exportedPic = NULL;
		DPB_recycleUnreferenced(enc->m_dpb);
	}
	if (pic_in)
	{

		if (empty(&enc->m_dpb->m_freeList))
		{
			//printf("sizeof(Frame)=%d\n",sizeof(Frame));
			if (!inFrame)
				printf("malloc inFrame fail!");

			if (Frame_create(inFrame, enc->m_param))
			{
				/* the first PicYuv created is asked to generate the CU and block unit offset
				* arrays which are then shared with all subsequent PicYuv (orig and recon)
				* allocated by this top level encoder */
				if (enc->m_cuOffsetY)//已经申请过空间，不用再进入else将 encoder offset指针赋值到对应m_fencPic
				{
					inFrame->m_fencPic->m_cuOffsetC = enc->m_cuOffsetC;//空间为一帧LCU个数，按照行列对应色度LCU的pixel地址
					inFrame->m_fencPic->m_cuOffsetY = enc->m_cuOffsetY;
					inFrame->m_fencPic->m_buOffsetC = enc->m_buOffsetC;//空间为一个LCU的part个数（默认256个4x4），为当前色度位置与LCU首地址的偏移地址
					inFrame->m_fencPic->m_buOffsetY = enc->m_buOffsetY;
				}
				else
				{
					if (!PicYuv_createOffsets(inFrame->m_fencPic, &enc->m_sps))
					{
						//报错 正常情况下不会进入
						enc->m_aborted = TRUE;
						printf("memory allocation failure,aborting encode\n");
						Frame_destroy(inFrame);
						//free(inFrame);inFrame=NULL;
						return -1;
					}
					else
					{
						enc->m_cuOffsetC = inFrame->m_fencPic->m_cuOffsetC;
						enc->m_cuOffsetY = inFrame->m_fencPic->m_cuOffsetY;
						enc->m_buOffsetC = inFrame->m_fencPic->m_buOffsetC;
						enc->m_buOffsetY = inFrame->m_fencPic->m_buOffsetY;
					}
				}
			}
			else
			{
				//报错 正常情况下不会进入
				enc->m_aborted = TRUE;
				printf("memory allocation failure,aborting encode\n");
				Frame_destroy(inFrame);
				return -1;
			}
		}
		else
		{
			inFrame = PicList_popBack(&enc->m_dpb->m_freeList);//从m_dpb->m_freeList开始取帧
			inFrame->m_lowresInit = FALSE;
		}
		/* Copy input picture into a Frame and PicYuv, send to lookahead */
		PicYuv_copyFromPicture(inFrame->m_fencPic, pic_in, enc->m_sps.conformanceWindow.rightOffset, enc->m_sps.conformanceWindow.bottomOffset);

		inFrame->m_poc = ++enc->m_pocLast; //累加读入帧数
		inFrame->m_userData = pic_in->userData; //一般都为0
		inFrame->m_pts = pic_in->pts; //一般就是对应poc的值
		inFrame->m_forceqp = pic_in->forceqp;
		inFrame->m_param = enc->m_reconfigured ? enc->m_latestParam : enc->m_param;
	}
	FrameEncoder *curEncoder = enc->m_frameEncoder;
	push_1(&curEncoder->m_bs);
	Entropy_entropy(&curEncoder->m_entropyCoder);
	Entropy_entropy(&curEncoder->m_initSliceContext);
	NAL_nal(&curEncoder->m_nalList);

	enc->m_curEncoder = 0;
	int ret = 0;
	Frame *outFrame = NULL;
	Frame *frameEnc = NULL;
	int pass = 0;

	do
	{
		/* getEncodedPicture() should block until the FrameEncoder has completed
		* encoding the frame.  This is how back-pressure through the API is
		* accomplished when the encoder is full */
		if (!enc->m_bZeroLatency || pass)
			outFrame = FrameEncoder_getEncodedPicture(curEncoder, &enc->m_nalList);
		if (outFrame)
		{
			Slice *slice = outFrame->m_encData->m_slice;

			/* Free up pic_in->analysisData since it has already been used */
			//if (enc->m_param->analysisMode == X265_ANALYSIS_LOAD)
			//  freeAnalysis(&outFrame->m_analysisData);
			if (pic_out)
			{
				PicYuv *recpic = outFrame->m_reconPic;
				pic_out->poc = slice->m_poc;
				pic_out->bitDepth = X265_DEPTH;
				pic_out->userData = outFrame->m_userData;
				pic_out->colorSpace = enc->m_param->internalCsp;

				pic_out->pts = outFrame->m_pts;
				pic_out->dts = outFrame->m_dts;

				switch (slice->m_sliceType)
				{
				case I_SLICE:
					pic_out->sliceType = outFrame->m_lowres->bKeyframe ? X265_TYPE_IDR : X265_TYPE_I;
					break;
				case P_SLICE:
					pic_out->sliceType = X265_TYPE_P;
					break;
				case B_SLICE:
					pic_out->sliceType = X265_TYPE_B;
					break;
				}

				pic_out->planes[0] = recpic->m_picOrg[0];
				pic_out->stride[0] = (int)(recpic->m_stride * sizeof(pixel));
				pic_out->planes[1] = recpic->m_picOrg[1];
				pic_out->stride[1] = (int)(recpic->m_strideC * sizeof(pixel));
				pic_out->planes[2] = recpic->m_picOrg[2];
				pic_out->stride[2] = (int)(recpic->m_strideC * sizeof(pixel));
			}

			if (enc->m_aborted)
				return -1;
			enc->m_exportedPic = outFrame;

			enc->m_numDelayedPic--;
			ret = 1;
		}

		/* pop a single frame from decided list, then provide to frame encoder
		* curEncoder is guaranteed to be idle at this point */
		if (!pass)
			//frameEnc = m_lookahead->getDecidedPicture();
			frameEnc = inFrame;
		if (frameEnc && !pass)
		{
			/* give this frame a FrameData instance before encoding */
			if (enc->m_dpb->m_picSymFreeList)
			{//暂时不执行
				frameEnc->m_encData = enc->m_dpb->m_picSymFreeList;
				enc->m_dpb->m_picSymFreeList = enc->m_dpb->m_picSymFreeList->m_freeListNext;
			}
			else
			{
				if (!Frame_allocEncodeData(frameEnc, enc->m_param, &enc->m_sps))
					printf("initial allocEncodeData fail!\n");
				Slice* slice = frameEnc->m_encData->m_slice;
				slice->m_sps = &enc->m_sps;
				slice->m_pps = &enc->m_pps;
				slice->m_maxNumMergeCand = enc->m_param->maxNumMergeCand;
				slice->m_endCUAddr = Slice_realEndAddress(slice, enc->m_sps.numCUsInFrame * NUM_4x4_PARTITIONS);
				//slice->m_endCUAddr = slice->realEndAddress(m_sps.numCUsInFrame * NUM_4x4_PARTITIONS);
				frameEnc->m_reconPic->m_cuOffsetC = enc->m_cuOffsetC;
				frameEnc->m_reconPic->m_cuOffsetY = enc->m_cuOffsetY;
				frameEnc->m_reconPic->m_buOffsetC = enc->m_buOffsetC;
				frameEnc->m_reconPic->m_buOffsetY = enc->m_buOffsetY;
			}

			if (enc->m_bframeDelay)
			{//暂时不执行
				int64_t *prevReorderedPts = enc->m_prevReorderedPts;
				frameEnc->m_dts = enc->m_encodedFrameNum > enc->m_bframeDelay
					? prevReorderedPts[(enc->m_encodedFrameNum - enc->m_bframeDelay) % enc->m_bframeDelay]
					: frameEnc->m_reorderedPts - enc->m_bframeDelayTime;
				prevReorderedPts[enc->m_encodedFrameNum % enc->m_bframeDelay] = frameEnc->m_reorderedPts;
			}
			else
				frameEnc->m_dts = frameEnc->m_reorderedPts;

			/* Allocate analysis data before encode in save mode. This is allocated in frameEnc */
			if (enc->m_param->analysisMode == X265_ANALYSIS_SAVE)
			{//暂时不执行
				x265_analysis_data* analysis = &frameEnc->m_analysisData;
				analysis->poc = frameEnc->m_poc;
				analysis->sliceType = frameEnc->m_lowres->sliceType;
				uint32_t widthInCU = (enc->m_param->sourceWidth + g_maxCUSize - 1) >> g_maxLog2CUSize;
				uint32_t heightInCU = (enc->m_param->sourceHeight + g_maxCUSize - 1) >> g_maxLog2CUSize;

				uint32_t numCUsInFrame = widthInCU * heightInCU;
				analysis->numCUsInFrame = numCUsInFrame;
				analysis->numPartitions = NUM_4x4_PARTITIONS;
			}

			/* determine references, setup RPS, etc */
			if ((frameEnc->m_poc<NUM_OF_I_FRAME) || ((frameEnc->m_poc + 1) % 10 == 0))
			{
				DPB_prepareEncode(enc->m_dpb, frameEnc);
				curEncoder->m_sliceType = 1;
			}
			/*
			else if(frameEnc->m_poc==20)
			{
			DPB_prepareEncode(enc->m_dpb,frameEnc);
			curEncoder->m_sliceType = 1;
			}*/
			else
			{
				DPB_prepareEncode2(enc->m_dpb, frameEnc);
				curEncoder->m_sliceType = 3;
			}

			/* Allow FrameEncoder::compressFrame() to start in the frame encoder thread */
			if (!FrameEncoder_startCompressFrame(curEncoder, frameEnc))
				enc->m_aborted = TRUE;
		}
	} while (enc->m_bZeroLatency && ++pass < 2);
	return ret;
}

void Encoder_configure(Encoder * encoder, x265_param *p)
{
	encoder->m_param = p;

	/* initialize the conformance window */
	encoder->m_conformanceWindow.bEnabled = FALSE;
	encoder->m_conformanceWindow.rightOffset = 0;
	encoder->m_conformanceWindow.topOffset = 0;
	encoder->m_conformanceWindow.bottomOffset = 0;
	encoder->m_conformanceWindow.leftOffset = 0;

	/* set pad size if width is not multiple of the minimum CU size*/
	if (p->sourceWidth & (p->minCUSize - 1))
	{
		uint32_t rem = p->sourceWidth & (p->minCUSize - 1);
		uint32_t padsize = p->minCUSize - rem;
		p->sourceWidth += padsize;
		encoder->m_conformanceWindow.bEnabled = TRUE;
		encoder->m_conformanceWindow.rightOffset = padsize;
	}
	/* set pad size if height is not multiple of the minimum CU size*/
	if (p->sourceHeight & (p->minCUSize - 1))
	{
		uint32_t rem = p->sourceHeight & (p->minCUSize - 1);
		uint32_t padsize = p->minCUSize - rem;
		p->sourceHeight += padsize;
		encoder->m_conformanceWindow.bEnabled = TRUE;
		encoder->m_conformanceWindow.bottomOffset = padsize;
	}
	encoder->m_param->rc.qgSize = p->maxCUSize;
}

void Encoder_destroy(Encoder *encoder)
{
	//FrameEncoder_destroy(encoder->m_frameEncoder);
	free(encoder->m_frameEncoder); encoder->m_frameEncoder = NULL;
	free(encoder->m_dpb); encoder->m_dpb = NULL;
	free(encoder->m_scalingList); encoder->m_scalingList = NULL;
	if (encoder->m_param)
	{
		free(encoder->m_param); encoder->m_param = NULL;
	}
}
