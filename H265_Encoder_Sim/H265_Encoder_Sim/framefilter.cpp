/*
* framefilter.c
*
*  Created on: 2017-6-5
*      Author: SheChengLong
*/

#include "framedata.h"
#include "framefilter.h"

extern EncoderPrimitives primitives;
extern void extendCURowColBorder(pixel* txt, intptr_t stride, int width, int height, int marginX);
void FrameFilter_FrameFilter(FrameFilter *filter)
{
	filter->m_param = NULL;
	filter->m_frame = NULL;
	filter->m_frameEncoder = NULL;
	filter->m_ssimBuf = NULL;
}

void FrameFilter_destroy(FrameFilter *filter)
{
	free(filter->m_ssimBuf);
	filter->m_ssimBuf = NULL;
}

void FrameFilter_init(FrameFilter *filter, struct Encoder *top, struct FrameEncoder *frame, int numRows)
{
	filter->m_param = top->m_param;
	filter->m_frameEncoder = frame;
	filter->m_numRows = numRows;
	filter->m_hChromaShift = CHROMA_H_SHIFT(filter->m_param->internalCsp);
	filter->m_vChromaShift = CHROMA_V_SHIFT(filter->m_param->internalCsp);
	filter->m_pad[0] = top->m_sps.conformanceWindow.rightOffset;
	filter->m_pad[1] = top->m_sps.conformanceWindow.bottomOffset;
	filter->m_saoRowDelay = filter->m_param->bEnableLoopFilter ? 1 : 0;
	filter->m_lastHeight = filter->m_param->sourceHeight % g_maxCUSize ? filter->m_param->sourceHeight % g_maxCUSize : g_maxCUSize;

	if (filter->m_param->bEnableSsim)
		filter->m_ssimBuf = X265_MALLOC(int, 8 * (filter->m_param->sourceWidth / 4 + 3));
}

void FrameFilter_start(FrameFilter *filter, Frame *frame, Entropy*initState, int qp)
{
	filter->m_frame = frame;

}

void FrameFilter_processRow(FrameFilter *filter, int row)
{/*
	if (!filter->m_param->bEnableLoopFilter && !filter->m_param->bEnableSAO)
	{
		//FrameFilter_processRowPost(filter,row);
		return;
	}
	FrameData* encData = filter->m_frame->m_encData;
	const uint32_t numCols = encData->m_slice->m_sps->numCuInWidth;
	const uint32_t lineStartCUAddr = row * numCols;

	if (filter->m_param->bEnableLoopFilter)
	{
		CUGeom* cuGeoms = filter->m_frameEncoder->m_cuGeoms;
		uint32_t* ctuGeomMap = filter->m_frameEncoder->m_ctuGeomMap;

		for (uint32_t col = 0; col < numCols; col++)
		{
			uint32_t cuAddr = lineStartCUAddr + col;
			CUData* ctu = framedata_getPicCTU(encData, cuAddr);
			deblockCTU(ctu, &cuGeoms[ctuGeomMap[cuAddr]], EDGE_VER);

			if (col > 0)
			{
				CUData* ctuPrev = framedata_getPicCTU(encData, cuAddr - 1);
				deblockCTU(ctuPrev, &cuGeoms[ctuGeomMap[cuAddr - 1]], EDGE_HOR);
			}
		}

		CUData* ctuPrev = framedata_getPicCTU(encData, lineStartCUAddr + numCols - 1);
		deblockCTU(ctuPrev, &cuGeoms[ctuGeomMap[lineStartCUAddr + numCols - 1]], EDGE_HOR);
	}
*/
}

uint32_t FrameFilter_getCUHeight(FrameFilter *filter, int rowNum)
{
	return rowNum == filter->m_numRows - 1 ? filter->m_lastHeight : g_maxCUSize;
}

void FrameFilter_processRowPost(FrameFilter *filter, int row)
{/*
	PicYuv *reconPic = filter->m_frame->m_reconPic;
	const uint32_t numCols = filter->m_frame->m_encData->m_slice->m_sps->numCuInWidth;
	const uint32_t lineStartCUAddr = row * numCols;
	const int realH = FrameFilter_getCUHeight(filter, row);

	pixel *pixel_0 = Picyuv_CTUgetLumaAddr(reconPic, lineStartCUAddr);
	pixel *pixel_Cb = Picyuv_CTUgetCbAddr(reconPic, lineStartCUAddr);
	pixel *pixel_Cr = Picyuv_CTUgetCrAddr(reconPic, lineStartCUAddr);
	// Border extend Left and Right
	extendCURowColBorder(pixel_0, reconPic->m_stride, reconPic->m_picWidth, realH, reconPic->m_lumaMarginX);
	extendCURowColBorder(pixel_Cb, reconPic->m_strideC, reconPic->m_picWidth >> filter->m_hChromaShift, realH >> filter->m_vChromaShift, reconPic->m_chromaMarginX);
	extendCURowColBorder(pixel_Cr, reconPic->m_strideC, reconPic->m_picWidth >> filter->m_hChromaShift, realH >> filter->m_vChromaShift, reconPic->m_chromaMarginX);

	//primitives.extendRowBorder(Picyuv_CTUgetLumaAddr(reconPic,lineStartCUAddr), reconPic->m_stride, reconPic->m_picWidth, realH, reconPic->m_lumaMarginX);
	//primitives.extendRowBorder(Picyuv_CTUgetCbAddr(reconPic,lineStartCUAddr), reconPic->m_strideC, reconPic->m_picWidth >> filter->m_hChromaShift, realH >> filter->m_vChromaShift, reconPic->m_chromaMarginX);
	//primitives.extendRowBorder(Picyuv_CTUgetCrAddr(reconPic,lineStartCUAddr), reconPic->m_strideC, reconPic->m_picWidth >> filter->m_hChromaShift, realH >> filter->m_vChromaShift, reconPic->m_chromaMarginX);

	// Border extend Top
	if (!row)
	{
		const intptr_t stride = reconPic->m_stride;
		const intptr_t strideC = reconPic->m_strideC;
		pixel *pixY = Picyuv_CTUgetLumaAddr(reconPic, lineStartCUAddr) - reconPic->m_lumaMarginX;
		pixel *pixU = Picyuv_CTUgetCbAddr(reconPic, lineStartCUAddr) - reconPic->m_chromaMarginX;
		pixel *pixV = Picyuv_CTUgetCrAddr(reconPic, lineStartCUAddr) - reconPic->m_chromaMarginX;

		for (uint32_t y = 0; y < reconPic->m_lumaMarginY; y++)
			memcpy(pixY - (y + 1) * stride, pixY, stride * sizeof(pixel));

		for (uint32_t y = 0; y < reconPic->m_chromaMarginY; y++)
		{
			memcpy(pixU - (y + 1) * strideC, pixU, strideC * sizeof(pixel));
			memcpy(pixV - (y + 1) * strideC, pixV, strideC * sizeof(pixel));
		}
	}

	// Border extend Bottom
	if (row == filter->m_numRows - 1)
	{
		const intptr_t stride = reconPic->m_stride;
		const intptr_t strideC = reconPic->m_strideC;
		pixel *pixY = Picyuv_CTUgetLumaAddr(reconPic, lineStartCUAddr) - reconPic->m_lumaMarginX + (realH - 1) * stride;
		pixel *pixU = Picyuv_CTUgetCbAddr(reconPic, lineStartCUAddr) - reconPic->m_chromaMarginX + ((realH >> filter->m_vChromaShift) - 1) * strideC;
		pixel *pixV = Picyuv_CTUgetCrAddr(reconPic, lineStartCUAddr) - reconPic->m_chromaMarginX + ((realH >> filter->m_vChromaShift) - 1) * strideC;
		for (uint32_t y = 0; y < reconPic->m_lumaMarginY; y++)
			memcpy(pixY + (y + 1) * stride, pixY, stride * sizeof(pixel));

		for (uint32_t y = 0; y < reconPic->m_chromaMarginY; y++)
		{
			memcpy(pixU + (y + 1) * strideC, pixU, strideC * sizeof(pixel));
			memcpy(pixV + (y + 1) * strideC, pixV, strideC * sizeof(pixel));
		}
	}


	uint32_t cuAddr = lineStartCUAddr;
	if (filter->m_param->bEnablePsnr)
	{
		PicYuv* fencPic = filter->m_frame->m_fencPic;

		intptr_t stride = reconPic->m_stride;
		uint32_t width = reconPic->m_picWidth - filter->m_pad[0];
		uint32_t height = FrameFilter_getCUHeight(filter, row);

		uint64_t ssdY = computeSSD(Picyuv_CTUgetLumaAddr(fencPic, cuAddr), Picyuv_CTUgetLumaAddr(reconPic, cuAddr), stride, width, height);
		height >>= filter->m_vChromaShift;
		width >>= filter->m_hChromaShift;
		stride = reconPic->m_strideC;

		uint64_t ssdU = computeSSD(Picyuv_CTUgetCbAddr(fencPic, cuAddr), Picyuv_CTUgetCbAddr(reconPic, cuAddr), stride, width, height);
		uint64_t ssdV = computeSSD(Picyuv_CTUgetCrAddr(fencPic, cuAddr), Picyuv_CTUgetCrAddr(reconPic, cuAddr), stride, width, height);

		filter->m_frameEncoder->m_SSDY += ssdY;
		filter->m_frameEncoder->m_SSDU += ssdU;
		filter->m_frameEncoder->m_SSDV += ssdV;
	}
	if (filter->m_param->bEnableSsim && filter->m_ssimBuf)
	{
		pixel *rec = filter->m_frame->m_reconPic->m_picOrg[0];
		pixel *fenc = filter->m_frame->m_fencPic->m_picOrg[0];
		intptr_t stride1 = filter->m_frame->m_fencPic->m_stride;
		intptr_t stride2 = filter->m_frame->m_reconPic->m_stride;
		uint32_t bEnd = ((row + 1) == (filter->m_numRows - 1));
		uint32_t bStart = (row == 0);
		uint32_t minPixY = row * g_maxCUSize - 4 * !bStart;
		uint32_t maxPixY = (row + 1) * g_maxCUSize - 4 * !bEnd;
		uint32_t ssim_cnt;

		// SSIM is done for each row in blocks of 4x4 . The First blocks are offset by 2 pixels to the right
		// to avoid alignment of ssim blocks with DCT blocks. 
		minPixY += bStart ? 2 : -6;
		filter->m_frameEncoder->m_ssim += calculateSSIM(rec + 2 + minPixY * stride1, stride1, fenc + 2 + minPixY * stride2, stride2,
			filter->m_param->sourceWidth - 2, maxPixY - minPixY, filter->m_ssimBuf, &ssim_cnt);
		filter->m_frameEncoder->m_ssimCnt += ssim_cnt;
	}*/

	//以下代码暂时不用
	/*   
	if (filter->m_param->decodedPictureHashSEI == 1)
	{
	uint32_t height = getCUHeight(row);
	uint32_t width = reconPic->m_picWidth;
	intptr_t stride = reconPic->m_stride;

	if (!row)
	{
	for (int i = 0; i < 3; i++)
	MD5Init(&filter->m_frameEncoder->m_state[i]);
	}

	updateMD5Plane(m_frameEncoder->m_state[0], reconPic->getLumaAddr(cuAddr), width, height, stride);
	width  >>= m_hChromaShift;
	height >>= m_vChromaShift;
	stride = reconPic->m_strideC;

	updateMD5Plane(m_frameEncoder->m_state[1], reconPic->getCbAddr(cuAddr), width, height, stride);
	updateMD5Plane(m_frameEncoder->m_state[2], reconPic->getCrAddr(cuAddr), width, height, stride);
	}
	else if (m_param->decodedPictureHashSEI == 2)
	{
	uint32_t height = getCUHeight(row);
	uint32_t width = reconPic->m_picWidth;
	intptr_t stride = reconPic->m_stride;
	if (!row)
	m_frameEncoder->m_crc[0] = m_frameEncoder->m_crc[1] = m_frameEncoder->m_crc[2] = 0xffff;
	updateCRC(reconPic->getLumaAddr(cuAddr), m_frameEncoder->m_crc[0], height, width, stride);
	width  >>= m_hChromaShift;
	height >>= m_vChromaShift;
	stride = reconPic->m_strideC;

	updateCRC(reconPic->getCbAddr(cuAddr), m_frameEncoder->m_crc[1], height, width, stride);
	updateCRC(reconPic->getCrAddr(cuAddr), m_frameEncoder->m_crc[2], height, width, stride);
	}
	else if (m_param->decodedPictureHashSEI == 3)
	{
	uint32_t width = reconPic->m_picWidth;
	uint32_t height = getCUHeight(row);
	intptr_t stride = reconPic->m_stride;
	uint32_t cuHeight = g_maxCUSize;
	if (!row)
	m_frameEncoder->m_checksum[0] = m_frameEncoder->m_checksum[1] = m_frameEncoder->m_checksum[2] = 0;
	updateChecksum(reconPic->m_picOrg[0], m_frameEncoder->m_checksum[0], height, width, stride, row, cuHeight);
	width  >>= m_hChromaShift;
	height >>= m_vChromaShift;
	stride = reconPic->m_strideC;
	cuHeight >>= m_vChromaShift;

	updateChecksum(reconPic->m_picOrg[1], m_frameEncoder->m_checksum[1], height, width, stride, row, cuHeight);
	updateChecksum(reconPic->m_picOrg[2], m_frameEncoder->m_checksum[2], height, width, stride, row, cuHeight);
	}

	if (ATOMIC_INC(&m_frameEncoder->m_completionCount) == 2 * (int)m_frameEncoder->m_numRows)
	m_frameEncoder->m_completionEvent.trigger();*/
}

static uint64_t computeSSD(pixel *fenc, pixel *rec, intptr_t stride, uint32_t width, uint32_t height)
{
	uint64_t ssd = 0;

	if ((width | height) & 3)
	{
		// Slow Path //
		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				int diff = (int)(fenc[x] - rec[x]);
				ssd += diff * diff;
			}

			fenc += stride;
			rec += stride;
		}

		return ssd;
	}

	uint32_t y = 0;

	// Consume rows in ever narrower chunks of height //
	for (int size = BLOCK_64x64; size >= BLOCK_4x4 && y < height; size--)
	{
		uint32_t rowHeight = 1 << (size + 2);

		for (; y + rowHeight <= height; y += rowHeight)
		{
			uint32_t y1, x = 0;

			// Consume each row using the largest square blocks possible //
			if (size == BLOCK_64x64 && !(stride & 31))
				for (; x + 64 <= width; x += 64)
					ssd += primitives.cu[BLOCK_64x64].sse_pp(fenc + x, stride, rec + x, stride, 64, 64);

			if (size >= BLOCK_32x32 && !(stride & 15))
				for (; x + 32 <= width; x += 32)
					for (y1 = 0; y1 + 32 <= rowHeight; y1 += 32)
						ssd += primitives.cu[BLOCK_32x32].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride, 32, 32);

			if (size >= BLOCK_16x16)
				for (; x + 16 <= width; x += 16)
					for (y1 = 0; y1 + 16 <= rowHeight; y1 += 16)
						ssd += primitives.cu[BLOCK_16x16].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride, 16, 16);

			if (size >= BLOCK_8x8)
				for (; x + 8 <= width; x += 8)
					for (y1 = 0; y1 + 8 <= rowHeight; y1 += 8)
						ssd += primitives.cu[BLOCK_8x8].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride, 8, 8);

			for (; x + 4 <= width; x += 4)
				for (y1 = 0; y1 + 4 <= rowHeight; y1 += 4)
					ssd += primitives.cu[BLOCK_4x4].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride, 4, 4);

			fenc += stride * rowHeight;
			rec += stride * rowHeight;
		}
	}

	return ssd;
}

/* Function to calculate SSIM for each row */
static float calculateSSIM(pixel *pix1, intptr_t stride1, pixel *pix2, intptr_t stride2, uint32_t width, uint32_t height, void *buf, uint32_t* cnt)
{
	uint32_t z = 0;
	float ssim = 0.0;

	int(*sum0)[4] = (int(*)[4])buf;
	int(*sum1)[4] = sum0 + (width >> 2) + 3;
	width >>= 2;
	height >>= 2;

	for (uint32_t y = 1; y < height; y++)
	{
		for (; z <= y; z++)
		{
			//swap(&sum0, &sum1);
			for (uint32_t x = 0; x < width; x += 2)
				primitives.ssim_4x4x2_core(&pix1[(4 * x + (z * stride1))], stride1, &pix2[(4 * x + (z * stride2))], stride2, &sum0[x]);
		}

		for (uint32_t x = 0; x < width - 1; x += 4)
			ssim += primitives.ssim_end_4(sum0 + x, sum1 + x, X265_MIN(4, width - x - 1));
	}

	*cnt = (height - 1) * (width - 1);
	return ssim;
}

