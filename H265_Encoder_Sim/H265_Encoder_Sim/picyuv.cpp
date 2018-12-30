/*
* picyuv.c
*
*  Created on: 2015-10-26
*      Author: adminster
*/
#include"picyuv.h"
#include<malloc.h>
#include<string.h>
#include "constants.h"
#include "slice.h"
#include "primitives.h"

void PicYuv_init(PicYuv *picyuv)
{
	picyuv->m_picBuf[0] = NULL;
	picyuv->m_picBuf[1] = NULL;
	picyuv->m_picBuf[2] = NULL;

	picyuv->m_picOrg[0] = NULL;
	picyuv->m_picOrg[1] = NULL;
	picyuv->m_picOrg[2] = NULL;

	picyuv->m_cuOffsetY = NULL;
	picyuv->m_cuOffsetC = NULL;
	picyuv->m_buOffsetY = NULL;
	picyuv->m_buOffsetC = NULL;
}
//pixel m_picBuf_0[101376] = { 0 };
//pixel m_picBuf_1[25344] = { 0 };
//pixel m_picBuf_2[25344] = { 0 };

pixel m_picBuf_0[96*96] = { 0 };
pixel m_picBuf_1[48*48] = { 0 };
pixel m_picBuf_2[48*48] = { 0 };
int PicYuv_create(PicYuv *picyuv, uint32_t picWidth, uint32_t picHeight)
{
	PicYuv_init(picyuv);
	picyuv->m_picWidth = picWidth;
	picyuv->m_picHeight = picHeight;
	uint32_t numCuInWidth = (picyuv->m_picWidth + g_maxCUSize - 1) / (g_maxCUSize);
	uint32_t numCuInHeight = (picyuv->m_picHeight + g_maxCUSize - 1) / (g_maxCUSize);

	picyuv->m_lumaMarginX = (g_maxCUSize >> 1) + 32; // search margin and 8-tap filter half-length, padded for 32-byte alignment
	picyuv->m_lumaMarginY = (g_maxCUSize >> 1) + 16; // margin for 8-tap filter and infinite padding
	picyuv->m_stride = numCuInWidth * g_maxCUSize;

	picyuv->m_chromaMarginX = picyuv->m_lumaMarginX;  // keep 16-byte alignment for chroma CTUs
	picyuv->m_chromaMarginY = picyuv->m_lumaMarginY >> 1;

	picyuv->m_strideC = (numCuInWidth * g_maxCUSize) >> 1;
	int maxHeight = numCuInHeight * g_maxCUSize;

	//CHECKED_MALLOC(picyuv->m_picBuf[0], pixel, picyuv->m_stride * maxHeight );

	picyuv->m_picBuf[0] = m_picBuf_0;//43008
	if (!picyuv->m_picBuf[0])
	{
		printf("malloc of size %d failed\n", 43008);
		goto fail;
	}
	//CHECKED_MALLOC(picyuv->m_picBuf[1], pixel, picyuv->m_strideC * (maxHeight >> 1));
	picyuv->m_picBuf[1] = m_picBuf_1;
	if (!picyuv->m_picBuf[1])
	{
		printf("malloc of size %d failed\n", 16896);
		goto fail;
	}
	//CHECKED_MALLOC(picyuv->m_picBuf[2], pixel, picyuv->m_strideC * (maxHeight >> 1));
	picyuv->m_picBuf[2] = m_picBuf_2;
	if (!picyuv->m_picBuf[2])
	{
		printf("malloc of size %d failed\n", 16896);
		goto fail;
	}
	picyuv->m_picOrg[0] = picyuv->m_picBuf[0];// +picyuv->m_lumaMarginY   * picyuv->m_stride + picyuv->m_lumaMarginX;
	picyuv->m_picOrg[1] = picyuv->m_picBuf[1];// +picyuv->m_chromaMarginY * picyuv->m_strideC + picyuv->m_chromaMarginX;
	picyuv->m_picOrg[2] = picyuv->m_picBuf[2];// +picyuv->m_chromaMarginY * picyuv->m_strideC + picyuv->m_chromaMarginX;

	return TRUE;

fail:
	return FALSE;
}
pixel m_reconpicBuf_0[96 * 96] = { 0 };
pixel m_reconpicBuf_1[48 * 48] = { 0 };
pixel m_reconpicBuf_2[48 * 48] = { 0 };

int PicYuv_create_reconPic(PicYuv *picyuv, uint32_t picWidth, uint32_t picHeight)
{
	PicYuv_init(picyuv);
	picyuv->m_picWidth = picWidth;
	picyuv->m_picHeight = picHeight;
	uint32_t numCuInWidth = (picyuv->m_picWidth + g_maxCUSize - 1) / (g_maxCUSize);
	uint32_t numCuInHeight = (picyuv->m_picHeight + g_maxCUSize - 1) / (g_maxCUSize);

	//picyuv->m_lumaMarginX = (g_maxCUSize>>1) + 32; // search margin and 8-tap filter half-length, padded for 32-byte alignment
	//picyuv->m_lumaMarginY = (g_maxCUSize>>1) + 16; // margin for 8-tap filter and infinite padding
	picyuv->m_stride = numCuInWidth * g_maxCUSize;

	//picyuv->m_chromaMarginX = picyuv->m_lumaMarginX;  // keep 16-byte alignment for chroma CTUs
	//picyuv->m_chromaMarginY = picyuv->m_lumaMarginY >> 1;

	picyuv->m_strideC = (numCuInWidth * g_maxCUSize) >> 1;
	int maxHeight = numCuInHeight * g_maxCUSize;

	//CHECKED_MALLOC(picyuv->m_picBuf[0], pixel, picyuv->m_stride * maxHeight );

	picyuv->m_picBuf[0] = m_reconpicBuf_0;
	if (!picyuv->m_picBuf[0])
	{
		printf("malloc of size %d failed\n", 9216);
		goto fail;
	}
	//CHECKED_MALLOC(picyuv->m_picBuf[1], pixel, picyuv->m_strideC * (maxHeight >> 1));
	picyuv->m_picBuf[1] = m_reconpicBuf_1;
	if (!picyuv->m_picBuf[1])
	{
		printf("malloc of size %d failed\n", 2304);
		goto fail;
	}
	//CHECKED_MALLOC(picyuv->m_picBuf[2], pixel, picyuv->m_strideC * (maxHeight >> 1));
	picyuv->m_picBuf[2] = m_reconpicBuf_2;
	if (!picyuv->m_picBuf[2])
	{
		printf("malloc of size %d failed\n", 2304);
		goto fail;
	}
	picyuv->m_picOrg[0] = picyuv->m_picBuf[0];
	picyuv->m_picOrg[1] = picyuv->m_picBuf[1];
	picyuv->m_picOrg[2] = picyuv->m_picBuf[2];

	return TRUE;

fail:
	return FALSE;
}
intptr_t m_cuOffsetY[9] = { 0 };
intptr_t m_cuOffsetC[9] = { 0 };
intptr_t m_buOffsetY[64] = { 0 };
intptr_t m_buOffsetC[64] = { 0 };
/* the first picture allocated by the encoder will be asked to generate these
* offset arrays. Once generated, they will be provided to all future PicYuv
* allocated by the same encoder. */
int PicYuv_createOffsets(PicYuv *picyuv, struct SPS *sps)
{
	uint32_t numPartitions = 1 << (g_unitSizeDepth * 2);
	//CHECKED_MALLOC(picyuv->m_cuOffsetY, intptr_t, sps->numCuInWidth * sps->numCuInHeight);
	picyuv->m_cuOffsetY = m_cuOffsetY;
	if (!picyuv->m_cuOffsetY)
	{
		printf("malloc of size %d failed\n", 9);
		goto fail;
	}
	CHECKED_MALLOC(picyuv->m_cuOffsetC, intptr_t, sps->numCuInWidth * sps->numCuInHeight);
	picyuv->m_cuOffsetC = m_cuOffsetC;
	if (!picyuv->m_cuOffsetC)
	{
		printf("malloc of size %d failed\n", 9);
		goto fail;
	}

	uint32_t cuRow;
	uint32_t cuCol;
	uint32_t idx;
	for (cuRow = 0; cuRow < sps->numCuInHeight; cuRow++)
	{
		for (cuCol = 0; cuCol < sps->numCuInWidth; cuCol++)
		{
			picyuv->m_cuOffsetY[cuRow * sps->numCuInWidth + cuCol] = picyuv->m_stride * cuRow * g_maxCUSize + cuCol * g_maxCUSize;
			picyuv->m_cuOffsetC[cuRow * sps->numCuInWidth + cuCol] = picyuv->m_strideC * cuRow * (g_maxCUSize >> 1) + cuCol * (g_maxCUSize >> 1);
		}
	}
	//CHECKED_MALLOC(picyuv->m_buOffsetY, intptr_t, (size_t)numPartitions);
	picyuv->m_buOffsetY = m_buOffsetY;
	if (!picyuv->m_buOffsetY)
	{
		printf("malloc of size %d failed\n", 64);
		goto fail;
	}
	//CHECKED_MALLOC(picyuv->m_buOffsetC, intptr_t, (size_t)numPartitions);
	picyuv->m_buOffsetC = m_buOffsetC;
	if (!picyuv->m_buOffsetC)
	{
		printf("malloc of size %d failed\n", 64);
		goto fail;
	}
	
	for (idx = 0; idx < numPartitions; ++idx)
	{
		intptr_t x = g_zscanToPelX[idx];
		intptr_t y = g_zscanToPelY[idx];
		picyuv->m_buOffsetY[idx] = picyuv->m_stride * y + x;
		picyuv->m_buOffsetC[idx] = picyuv->m_strideC * (y >> 1) + (x >> 1);
	}

	return TRUE;

fail:
	return FALSE;
}
intptr_t m_cuOffsetY_recon[9];
intptr_t m_cuOffsetC_recon[9];
intptr_t m_buOffsetY_recon[64];
intptr_t m_buOffsetC_recon[64];
int PicYuv_createOffsets_recon(PicYuv *picyuv, struct SPS *sps)
{
	uint32_t numPartitions = 1 << (g_unitSizeDepth * 2);
	//CHECKED_MALLOC(picyuv->m_cuOffsetY, intptr_t, sps->numCuInWidth * sps->numCuInHeight);
	picyuv->m_cuOffsetY = m_cuOffsetY_recon;
	if (!picyuv->m_cuOffsetY)
	{
		printf("malloc of size %d failed\n", 9);
		goto fail;
	}

	//CHECKED_MALLOC(picyuv->m_cuOffsetC, intptr_t, sps->numCuInWidth * sps->numCuInHeight);
	picyuv->m_cuOffsetC = m_cuOffsetC_recon;
	if (!picyuv->m_cuOffsetC)
	{
		printf("malloc of size %d failed\n", 9);
		goto fail;
	}

	uint32_t cuRow;
	uint32_t cuCol;
	uint32_t idx;
	for (cuRow = 0; cuRow < sps->numCuInHeight; cuRow++)
	{
		for (cuCol = 0; cuCol < sps->numCuInWidth; cuCol++)
		{
			picyuv->m_cuOffsetY[cuRow * sps->numCuInWidth + cuCol] = picyuv->m_stride * cuRow * g_maxCUSize + cuCol * g_maxCUSize;
			picyuv->m_cuOffsetC[cuRow * sps->numCuInWidth + cuCol] = picyuv->m_strideC * cuRow * (g_maxCUSize >> 1) + cuCol * (g_maxCUSize >> 1);
		}
	}
	//CHECKED_MALLOC(picyuv->m_buOffsetY, intptr_t, (size_t)numPartitions);
	picyuv->m_buOffsetY = m_buOffsetY_recon;
	if (!picyuv->m_buOffsetY)
	{
		printf("malloc of size %d failed\n", 64);
		goto fail;
	}
	//CHECKED_MALLOC(picyuv->m_buOffsetC, intptr_t, (size_t)numPartitions);
	picyuv->m_buOffsetC = m_buOffsetC_recon;
	if (!picyuv->m_buOffsetC)
	{
		printf("malloc of size %d failed\n", 64);
		goto fail;
	}
	
	for (idx = 0; idx < numPartitions; ++idx)
	{
		intptr_t x = g_zscanToPelX[idx];
		intptr_t y = g_zscanToPelY[idx];
		picyuv->m_buOffsetY[idx] = picyuv->m_stride * y + x;
		picyuv->m_buOffsetC[idx] = picyuv->m_strideC * (y >> 1) + (x >> 1);
	}

	return TRUE;

fail:
	return FALSE;
}

void PicYuv_destroy(PicYuv *picyuv)
{
	free(picyuv->m_picBuf[0]); picyuv->m_picBuf[0] = NULL;
	free(picyuv->m_picBuf[1]); picyuv->m_picBuf[1] = NULL;
	free(picyuv->m_picBuf[2]); picyuv->m_picBuf[2] = NULL;
}

/* Copy pixels from an x265_picture into internal PicYuv instance.
* Shift pixels as necessary, mask off bits above X265_DEPTH for safety. */
void PicYuv_copyFromPicture(PicYuv *picyuv, x265_picture* pic, int padx, int pady)
{
	/* m_picWidth is the width that is being encoded, padx indicates how many
	* of those pixels are padding to reach multiple of MinCU(4) size.
	*
	* Internally, we need to extend rows out to a multiple of 16 for lowres
	* downscale and other operations. But those padding pixels are never
	* encoded.
	*
	* The same applies to m_picHeight and pady */

	/* width and height - without padsize (input picture raw width and height) */
	int width = picyuv->m_picWidth - padx;
	int height = picyuv->m_picHeight - pady;

	/* internal pad to multiple of 16x16 blocks */
	uint8_t rem = width & 15;

	//padx = rem ? 16 - rem : padx;
	if (rem)
		padx = 16 - rem;

	rem = height & 15;
	//pady = rem ? 16 - rem : pady;
	if (rem)
		pady = 16 - rem;

	/* add one more row and col of pad for downscale interpolation, fixes
	* warnings from valgrind about using uninitialized pixels */
	padx++;
	pady++;

	if (pic->bitDepth < X265_DEPTH)
	{
		pixel *yPixel = picyuv->m_picOrg[0];
		pixel *uPixel = picyuv->m_picOrg[1];
		pixel *vPixel = picyuv->m_picOrg[2];

		uint8_t *yChar = (uint8_t*)pic->planes[0];
		uint8_t *uChar = (uint8_t*)pic->planes[1];
		uint8_t *vChar = (uint8_t*)pic->planes[2];
		int shift = X265_MAX(0, X265_DEPTH - pic->bitDepth);

		primitives.planecopy_cp(yChar, pic->stride[0] / sizeof(*yChar), yPixel, picyuv->m_stride, width, height, shift);
		primitives.planecopy_cp(uChar, pic->stride[1] / sizeof(*uChar), uPixel, picyuv->m_strideC, width >> 1, height >> 1, shift);
		primitives.planecopy_cp(vChar, pic->stride[2] / sizeof(*vChar), vPixel, picyuv->m_strideC, width >> 1, height >> 1, shift);
	}
	else if (pic->bitDepth == 8)
	{
		pixel *yPixel = picyuv->m_picOrg[0];
		pixel *uPixel = picyuv->m_picOrg[1];
		pixel *vPixel = picyuv->m_picOrg[2];

		uint8_t *yChar = (uint8_t*)pic->planes[0];
		uint8_t *uChar = (uint8_t*)pic->planes[1];
		uint8_t *vChar = (uint8_t*)pic->planes[2];

		int r;
		for (r = 0; r < height; r++)
		{
			memcpy(yPixel, yChar, width * sizeof(pixel));

			yPixel += picyuv->m_stride;


			yChar += pic->stride[0] / sizeof(*yChar);
		}

		for (r = 0; r < height >> 1; r++)
		{
			memcpy(uPixel, uChar, (width >> 1) * sizeof(pixel));
			memcpy(vPixel, vChar, (width >> 1) * sizeof(pixel));

			uPixel += picyuv->m_strideC;
			vPixel += picyuv->m_strideC;
			uChar += pic->stride[1] / sizeof(*uChar);
			vChar += pic->stride[2] / sizeof(*vChar);
		}
	}
	else /* pic.bitDepth > 8 */
	{
		pixel *yPixel = picyuv->m_picOrg[0];
		pixel *uPixel = picyuv->m_picOrg[1];
		pixel *vPixel = picyuv->m_picOrg[2];

		uint16_t *yShort = (uint16_t*)pic->planes[0];
		uint16_t *uShort = (uint16_t*)pic->planes[1];
		uint16_t *vShort = (uint16_t*)pic->planes[2];

		/* defensive programming, mask off bits that are supposed to be zero */
		uint16_t mask = (1 << X265_DEPTH) - 1;
		int shift = X265_MAX(0, pic->bitDepth - X265_DEPTH);

		/* shift and mask pixels to final size */

		primitives.planecopy_sp(yShort, pic->stride[0] / sizeof(*yShort), yPixel, picyuv->m_stride, width, height, shift, mask);
		primitives.planecopy_sp(uShort, pic->stride[1] / sizeof(*uShort), uPixel, picyuv->m_strideC, width >> 1, height >> 1, shift, mask);
		primitives.planecopy_sp(vShort, pic->stride[2] / sizeof(*vShort), vPixel, picyuv->m_strideC, width >> 1, height >> 1, shift, mask);
	}

	/* extend the right edge if width was not multiple of the minimum CU size */
	if (padx)
	{
		int x, r;
		pixel *Y = picyuv->m_picOrg[0];
		pixel *U = picyuv->m_picOrg[1];
		pixel *V = picyuv->m_picOrg[2];

		for (r = 0; r < height; r++)
		{
			for (x = 0; x < padx; x++)
				Y[width + x] = Y[width - 1];

			Y += picyuv->m_stride;
		}

		for (r = 0; r < height >> 1; r++)
		{
			for (x = 0; x < padx >> 1; x++)
			{
				U[(width >> 1) + x] = U[(width >> 1) - 1];
				V[(width >> 1) + x] = V[(width >> 1) - 1];
			}

			U += picyuv->m_strideC;
			V += picyuv->m_strideC;
		}
	}

	/* extend the bottom if height was not multiple of the minimum CU size */
	if (pady)
	{
		pixel *Y = picyuv->m_picOrg[0] + (height - 1) * picyuv->m_stride;
		pixel *U = picyuv->m_picOrg[1] + ((height >> 1) - 1) * picyuv->m_strideC;
		pixel *V = picyuv->m_picOrg[2] + ((height >> 1) - 1) * picyuv->m_strideC;

		for (int i = 1; i <= pady; i++)
			memcpy(Y + i * picyuv->m_stride, Y, (width + padx) * sizeof(pixel));

		for (int j = 1; j <= pady >> 1; j++)
		{
			memcpy(U + j * picyuv->m_strideC, U, ((width + padx) >> 1) * sizeof(pixel));
			memcpy(V + j * picyuv->m_strideC, V, ((width + padx) >> 1) * sizeof(pixel));
		}
	}
}

/* get pointer to CTU start address */
pixel*  Picyuv_CTUgetLumaAddr(PicYuv *picyuv, uint32_t ctuAddr)
{
	return picyuv->m_picOrg[0] + picyuv->m_cuOffsetY[ctuAddr];
}

pixel*  Picyuv_CTUgetCbAddr(PicYuv *picyuv, uint32_t ctuAddr)
{
	return picyuv->m_picOrg[1] + picyuv->m_cuOffsetC[ctuAddr];
}

pixel*  Picyuv_CTUgetCrAddr(PicYuv *picyuv, uint32_t ctuAddr)
{
	return picyuv->m_picOrg[2] + picyuv->m_cuOffsetC[ctuAddr];
}

pixel*  Picyuv_CTUgetChromaAddr(PicYuv *picyuv, uint32_t chromaId, uint32_t ctuAddr)
{
	return picyuv->m_picOrg[chromaId] + picyuv->m_cuOffsetC[ctuAddr];
}

pixel*  Picyuv_CTUgetPlaneAddr(PicYuv *picyuv, uint32_t plane, uint32_t ctuAddr)
{
	return picyuv->m_picOrg[plane] + (plane ? picyuv->m_cuOffsetC[ctuAddr] : picyuv->m_cuOffsetY[ctuAddr]);
}

/* get pointer to CU start address */
pixel* Picyuv_CUgetLumaAddr(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx)
{
	return (picyuv->m_picOrg[0] + picyuv->m_cuOffsetY[ctuAddr] + picyuv->m_buOffsetY[absPartIdx]);
}

pixel*  Picyuv_CUgetCbAddr(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx)
{
	return picyuv->m_picOrg[1] + picyuv->m_cuOffsetC[ctuAddr] + picyuv->m_buOffsetC[absPartIdx];
}

pixel*  Picyuv_CUgetCrAddr(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx)
{
	return picyuv->m_picOrg[2] + picyuv->m_cuOffsetC[ctuAddr] + picyuv->m_buOffsetC[absPartIdx];
}

pixel*  Picyuv_CUgetChromaAddr(PicYuv *picyuv, uint32_t chromaId, uint32_t ctuAddr, uint32_t absPartIdx)
{
	return picyuv->m_picOrg[chromaId] + picyuv->m_cuOffsetC[ctuAddr] + picyuv->m_buOffsetC[absPartIdx];
}

intptr_t Picyuv_getChromaAddrOffset(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx)
{
	return picyuv->m_cuOffsetC[ctuAddr] + picyuv->m_buOffsetC[absPartIdx];
}

