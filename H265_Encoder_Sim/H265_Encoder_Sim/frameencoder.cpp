/*
* frameencoder.c
*
*  Created on: 2015-11-6
*      Author: adminster
*/

#include "common.h"
#include "frame.h"
#include "framedata.h"
#include "param.h"
#include "frameencoder.h"
#include "slicetype.h"
#include "nal.h"
#include "stdio.h"
#include "malloc.h"
#include "reference.h"
#include "search.h"
#include "encoder.h"

void FrameEncoder_FrameEncoder(FrameEncoder *frencoder)
{
	frencoder->m_prevOutputTime = x265_mdate();
	frencoder->m_isFrameEncoder = TRUE;
	frencoder->m_threadActive = TRUE;
	frencoder->m_slicetypeWaitTime = 0;
	frencoder->m_activeWorkerCount = 0;
	frencoder->m_completionCount = 0;
	frencoder->m_bAllRowsStop = FALSE;
	frencoder->m_vbvResetTriggerRow = -1;
	frencoder->m_outStreams = NULL;
	frencoder->m_substreamSizes = NULL;

	frencoder->m_rows = NULL;
	frencoder->m_top = NULL;
	frencoder->m_param = NULL;
	frencoder->m_frame = NULL;
	frencoder->m_cuGeoms = NULL;
	frencoder->m_ctuGeomMap = NULL;
	frencoder->m_localTldIdx = 0;
}

bool FrameEncoder_init(FrameEncoder *frencoder, struct Encoder *top, int numRows, int numCols)
{
	FrameEncoder_FrameEncoder(frencoder);
	frencoder->m_top = top;
	frencoder->m_param = top->m_param;
	frencoder->m_numRows = numRows;
	frencoder->m_numCols = numCols;
	
	frencoder->m_rows = (CTURow *)malloc(sizeof(CTURow)*frencoder->m_numRows); //获取当前row存放位置

	if (!frencoder->m_rows)
		printf("malloc m_rows fail !");

	frencoder->m_frameFilter = (FrameFilter *)malloc(sizeof(FrameFilter)); //获取当前row存放位置
	if (!frencoder->m_frameFilter)
		printf("malloc FrameFilter fail !");
	
	FrameFilter_FrameFilter(frencoder->m_frameFilter);
	
	Entropy_entropy(&frencoder->m_rows->bufferedEntropy);
	Entropy_entropy(&frencoder->m_rows->rowGoOnCoder);
	
	frencoder->m_filterRowDelay = (frencoder->m_param->bEnableSAO && frencoder->m_param->bSaoNonDeblocked) ?
		2 : (frencoder->m_param->bEnableSAO || frencoder->m_param->bEnableLoopFilter ? 1 : 0);
	frencoder->m_filterRowDelayCus = frencoder->m_filterRowDelay * numCols;

	FrameFilter_init(frencoder->m_frameFilter, top, frencoder, numRows);
	return TRUE;
}

void CTURow_init(CTURow *cturow, Entropy *initContext)
{/*
	(*cturow).active = FALSE;
	(*cturow).busy = TRUE;
	(*cturow).completed = 0;
	load(&(*cturow).rowGoOnCoder, initContext);*/
}

/* Generate a complete list of unique geom sets for the current picture dimensions */
//计算CU所有情况的几何信息
bool FrameEncoder_initializeGeoms(FrameEncoder *frencoder)
{
	
	// Geoms only vary between CTUs in the presence of picture edges //
	int maxCUSize = frencoder->m_param->maxCUSize;
	int minCUSize = frencoder->m_param->minCUSize;
	int heightRem = frencoder->m_param->sourceHeight & (maxCUSize - 1);//高度不够CTU的余数
	int widthRem = frencoder->m_param->sourceWidth & (maxCUSize - 1);//宽度不够CTU的余数
	int allocGeoms = 1; // body 存储的个数：分别为：CTU中全部有像素值  CTU右边不够像素值   CTU下边不够像素值  CTU右边和下边不够像素值
	if (heightRem && widthRem)
		allocGeoms = 4; // body, right, bottom, corner
	else if (heightRem || widthRem)
		allocGeoms = 2; // body, right or bottom

	frencoder->m_ctuGeomMap = X265_MALLOC(uint32_t, frencoder->m_numRows * frencoder->m_numCols);//已释放

	frencoder->m_cuGeoms = X265_MALLOC(CUGeom, allocGeoms * MAX_GEOMS);//已释放

	
	if (!frencoder->m_cuGeoms || !frencoder->m_ctuGeomMap)
	{
		printf("malloc ctuGeomMap and cuGeoms fail!");
		return FALSE;
	}
	// body  申请 CTU中全部有像素值 部分
	CUData_calcCTUGeoms(maxCUSize, maxCUSize, maxCUSize, minCUSize, frencoder->m_cuGeoms);
	memset(frencoder->m_ctuGeomMap, 0, sizeof(uint32_t) * frencoder->m_numRows * frencoder->m_numCols);
	if (allocGeoms == 1)
		return TRUE;
	
	int countGeoms = 1;
	if (widthRem)
	{
		// right 申请 CTU右边不够像素值 部分
		CUData_calcCTUGeoms(widthRem, maxCUSize, maxCUSize, minCUSize, frencoder->m_cuGeoms + countGeoms * 85);
		for (uint32_t i = 0; i < frencoder->m_numRows; i++)
		{
			uint32_t ctuAddr = frencoder->m_numCols * (i + 1) - 1;
			frencoder->m_ctuGeomMap[ctuAddr] = countGeoms * 85;
		}
		countGeoms++;
	}
	if (heightRem)
	{
		// bottom 申请 CTU下边不够像素值 部分
		CUData_calcCTUGeoms(maxCUSize, heightRem, maxCUSize, minCUSize, frencoder->m_cuGeoms + countGeoms * 85);
		for (uint32_t i = 0; i < frencoder->m_numCols; i++)
		{
			uint32_t ctuAddr = frencoder->m_numCols * (frencoder->m_numRows - 1) + i;
			frencoder->m_ctuGeomMap[ctuAddr] = countGeoms * 85;
		}
		countGeoms++;

		if (widthRem)
		{
			// corner  申请 CTU右边和下边不够像素值 部分
			CUData_calcCTUGeoms(widthRem, heightRem, maxCUSize, minCUSize, frencoder->m_cuGeoms + countGeoms * 85);

			uint32_t ctuAddr = frencoder->m_numCols * frencoder->m_numRows - 1;
			frencoder->m_ctuGeomMap[ctuAddr] = countGeoms * 85;
			countGeoms++;
		}
		X265_CHECK(countGeoms == allocGeoms, "geometry match check failure\n");
	}
	
	return TRUE;
}

bool FrameEncoder_startCompressFrame(FrameEncoder *frencoder, Frame* curFrame)
{
	frencoder->m_frame = curFrame;
	frencoder->m_param = curFrame->m_param;
	curFrame->m_encData->m_slice->m_mref = frencoder->m_mref;

	if (!frencoder->m_cuGeoms)
	{
		if (!FrameEncoder_initializeGeoms(frencoder))
			return FALSE;
	}
	
	Analysis analysis;
	Entropy_entropy(&analysis.sear.m_entropyCoder);
	analysis.sear.m_entropyCoder.syn.m_bitIf = NULL;
	Quant *quant = (struct Quant *)malloc(sizeof(struct Quant));//已释放

	initSearch(&(analysis.sear), frencoder->m_param, frencoder->m_top->m_scalingList);
	/*
	Analysis_create(&analysis);//申请模式决策内存 初始化相关CU内存

	FrameEncoder_compressFrame(frencoder, &analysis);
	
//	DPB_Destroy()
//	free(analysis.sear.m_me.fencPUYuv.m_buf[0]); analysis.sear.m_me.fencPUYuv.m_buf[0] = NULL;
//	FrameEncoder_destroy(frencoder);
	free(quant); quant = NULL;
//	free(curFrame->m_encData->m_slice); curFrame->m_encData->m_slice = NULL;//--------------free
	*/
	return TRUE;
}

/** 函数功能       ：compressframe()进行编码
/*  调用范围       ： 只在FrameEncoder::threadMain()函数中被调用
*   返回值         ： null
**/
void FrameEncoder_compressFrame(FrameEncoder *frencoder, Analysis *analysis)
{/*
	Analysis *ana = analysis;
	// Emit access unit delimiter unless this is the first frame and the user is
	// not repeating headers (since AUD is supposed to be the first NAL in the access
	// unit) 
	Slice* slice = frencoder->m_frame->m_encData->m_slice; //获取当前slice

	// Generate motion references//产生运动参考帧
	int numPredDir = isInterP(slice) ? 1 : isInterB(slice) ? 2 : 0;
	for (int l = 0; l < numPredDir; l++)
	{
		for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
		{
			struct WeightParam *w = NULL;
			frencoder->m_mref[l][ref].referenceplanes = (ReferencePlanes *)malloc(sizeof(ReferencePlanes));
			//printf("sizeof(ReferencePlanes)=%d\n",sizeof(ReferencePlanes));
			//ECS_memcpy(slice->m_refPicList[l][ref]->
			[0], reconFrameBuf_Y, sizeof(pixel) * 21376);
			//ECS_memcpy(slice->m_refPicList[l][ref]->
			[1], reconFrameBuf_U, sizeof(pixel) * 8320);
			//ECS_memcpy(slice->m_refPicList[l][ref]->m_reconPic->m_picOrg[2], reconFrameBuf_V, sizeof(pixel) * 8320);
			MotionReference_init(&frencoder->m_mref[l][ref], slice->m_refPicList[l][ref]->m_reconPic, frencoder->m_param);
		}
	}

	// Get the QP for this frame from rate control. This call may block until
	// frames ahead of it in encode order have called rateControlEnd() 
	int qp = 26; //计算估计当前帧应用的量化参数

	// Clip slice QP to 0-51 spec range before encoding //
	slice->m_sliceQp = x265_clip3(-QP_BD_OFFSET, QP_MAX_SPEC, qp); //获取当前估计的量化参数

	resetEntropy(&frencoder->m_initSliceContext, frencoder->m_frame->m_encData->m_slice);
	FrameFilter_start(frencoder->m_frameFilter, frencoder->m_frame, &frencoder->m_initSliceContext, qp);
	// reset entropy coders //
	load(&frencoder->m_entropyCoder, &frencoder->m_initSliceContext);
	for (uint32_t i = 0; i < frencoder->m_numRows; i++)
		CTURow_init(&frencoder->m_rows[i], &frencoder->m_initSliceContext);

	uint32_t numSubstreams = 1;
	if (!frencoder->m_outStreams)
	{
		frencoder->m_outStreams = (Bitstream *)malloc(sizeof(Bitstream));//已释放
		if (!frencoder->m_outStreams)
			printf("malloc outStreams fail !");
		push(frencoder->m_outStreams);
		frencoder->m_substreamSizes = X265_MALLOC(uint32_t, numSubstreams);//已释放
		if (!frencoder->m_substreamSizes)
			printf("malloc numSubstreams fail !");
		setBitstream(&frencoder->m_rows->rowGoOnCoder, frencoder->m_outStreams);
	}
	else
		resetBits(frencoder->m_outStreams);

	frencoder->m_rows->active = TRUE;
	for (uint32_t i = 0; i < frencoder->m_numRows + frencoder->m_filterRowDelay; i++)
	{
		// compress
		if (i < frencoder->m_numRows)
		{
			FrameEncoder_processRowEncoder(frencoder, i, ana);
		}
		// filter
		if (i >= frencoder->m_filterRowDelay)
		{
			FrameFilter_processRow(frencoder->m_frameFilter, i - frencoder->m_filterRowDelay);
		}
	}
	resetBits(&frencoder->m_bs);
	load(&frencoder->m_entropyCoder, &frencoder->m_initSliceContext);
	setBitstream(&frencoder->m_entropyCoder, &frencoder->m_bs);
	codeSliceHeader(&frencoder->m_entropyCoder, slice);

	// serialize each row, record final lengths in slice header
	uint32_t maxStreamSize = serializeSubstreams(&frencoder->m_nalList, frencoder->m_substreamSizes, numSubstreams, frencoder->m_outStreams);

	writeByteAlignment(&frencoder->m_bs);
	serialize(&frencoder->m_nalList, slice->m_nalUnitType, &frencoder->m_bs);

	FILE *fp = NULL;
	fp = fopen("str.bin", "ab+");
	if (!fp)
	{
		printf("open file error\n");
	}
	fwrite(frencoder->m_nalList.m_nal[0].payload, sizeof(uint8_t), frencoder->m_nalList.m_occupancy, fp);
	fflush(fp);
	fclose(fp);
	free(frencoder->m_nalList.m_extraBuffer); frencoder->m_nalList.m_extraBuffer = NULL;
	free(frencoder->m_nalList.m_buffer); frencoder->m_nalList.m_buffer = NULL;
	free(frencoder->m_outStreams); frencoder->m_outStreams = NULL;*/
}

// Called by worker threads
void FrameEncoder_processRowEncoder(FrameEncoder *frencoder, int intRow, Analysis *tld)
{/*
	int count = 0;
	uint32_t row = (uint32_t)intRow; //获取当前CTU行号
	CTURow *curRow = &frencoder->m_rows[row];
	tld->sear.m_param = frencoder->m_param; //获取配置参数
	// When WPP is enabled, every row has its own row coder instance. Otherwise
	// they share row 0 
	Entropy* rowCoder = &frencoder->m_rows[0].rowGoOnCoder;

	if (!rowCoder)
		printf("malloc Entropy fail!\n");

	FrameData *curEncData = frencoder->m_frame->m_encData; //获取当前编码帧相关数据
	Slice *slice = curEncData->m_slice; //获取当前帧的slice

	const uint32_t numCols = frencoder->m_numCols; //获取当前行有多少CTU
	uint32_t lineStartCUAddr = row * numCols; //获取当前CTU行的第一个CTU在帧中的号

	uint32_t cuAddr = lineStartCUAddr; //获取CTU在帧中的编号
	while (curRow->completed < numCols)
	{
		uint32_t col = curRow->completed; //获取当前CTU在CTU行的索引
		uint32_t cuAddr = lineStartCUAddr + col; //获取CTU在帧中的编号
		CUData *ctu = framedata_getPicCTU(frencoder->m_frame->m_encData, cuAddr); //获取在帧中对应位置的CTU
		CUData_initCTU(ctu, frencoder->m_frame, cuAddr, slice->m_sliceQp); //初始化CTU

		Mode *best = (Mode*) malloc(sizeof(Mode));
		if (!best)
			printf("malloc Mode fail!");
		//Does all the CU analysis, returns best top level mode decision
		best = compressCTU(tld, ctu, frencoder->m_frame, &frencoder->m_cuGeoms[frencoder->m_ctuGeomMap[cuAddr]], rowCoder);
		encodeCTU(rowCoder, ctu, &frencoder->m_cuGeoms[frencoder->m_ctuGeomMap[cuAddr]]);

		curRow->completed++;
		count = curRow->completed;
		printf("Finish %d CTU\n", cuAddr + 1);
		cuAddr++;

	}

	// flush row bitstream (if WPP and no SAO) or flush frame if no WPP and no SAO //
	if (!frencoder->m_param->bEnableSAO && (row == frencoder->m_numRows - 1))
		finishSlice(rowCoder);

	int flag = lineStartCUAddr + count;

	if (flag == 9)
	{
		frencoder->m_frameFilter->m_frame->m_reconPic = frencoder->m_frame->m_reconPic;
		free(tld->sear.m_quant.m_resiDctCoeff); free(tld->sear.m_quant.m_fencShortBuf);
		tld->sear.m_quant.m_resiDctCoeff = NULL; tld->sear.m_quant.m_fencShortBuf = NULL;
		//free(frencoder->m_frameFilter);frencoder->m_frameFilter=NULL;
	}
	*/
}

void FrameEncoder_destroy(FrameEncoder *frencoder)
{/*
	if (frencoder->m_outStreams)
	{
		free(frencoder->m_outStreams);
		frencoder->m_outStreams = NULL;
	}

	if (frencoder->m_cuGeoms)
	{
		free(frencoder->m_cuGeoms);
		frencoder->m_cuGeoms = NULL;
	}

	if (frencoder->m_ctuGeomMap)
	{
		free(frencoder->m_ctuGeomMap);
		frencoder->m_ctuGeomMap = NULL;
	}

	if (frencoder->m_substreamSizes)
	{
		free(frencoder->m_substreamSizes);
		frencoder->m_substreamSizes = NULL;
	}*/
}

Frame *FrameEncoder_getEncodedPicture(FrameEncoder *frameE, NALList* output)
{/*
	if (frameE->m_frame)
	{
		// block here until worker thread completes //

		Frame *ret = frameE->m_frame;
		frameE->m_frame = NULL;
		takeContents(output, &frameE->m_nalList);
		return ret;
	}
	*/
	return NULL;
}
