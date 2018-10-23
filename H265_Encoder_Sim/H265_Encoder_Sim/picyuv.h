/*
* picyuv.h
*
*  Created on: 2015-10-26
*      Author: adminster
*/

#ifndef PICYUV_H_
#define PICYUV_H_

#include "common.h"
#include "x265.h"

struct SPS;
typedef struct PicYuv
{
	pixel*   m_picBuf[3];  // full allocated buffers, including margins
	pixel*   m_picOrg[3];  // pointers to plane starts

	uint32_t m_picWidth;
	uint32_t m_picHeight;
	uint32_t m_stride;
	uint32_t m_strideC;

	intptr_t* m_cuOffsetY;  /* these four buffers are owned by the top-level encoder */
	intptr_t* m_cuOffsetC;
	intptr_t* m_buOffsetY;
	intptr_t* m_buOffsetC;

	uint32_t m_lumaMarginX;
	uint32_t m_lumaMarginY;
	uint32_t m_chromaMarginX;
	uint32_t m_chromaMarginY;
}PicYuv;

void PicYuv_init(PicYuv *picyuv);
int PicYuv_create(PicYuv *picyuv, uint32_t picWidth, uint32_t picHeight);
int PicYuv_create_reconPic(PicYuv *picyuv, uint32_t picWidth, uint32_t picHeight);
int PicYuv_createOffsets(PicYuv *picyuv, struct SPS *sps);
int PicYuv_createOffsets_recon(PicYuv *picyuv, struct SPS *sps);

void PicYuv_copyFromPicture(PicYuv *picyuv, x265_picture* pic, int padx, int pady);

/* get pointer to CTU start address */
pixel*  Picyuv_CTUgetLumaAddr(PicYuv *picyuv, uint32_t ctuAddr);
pixel*  Picyuv_CTUgetCbAddr(PicYuv *picyuv, uint32_t ctuAddr);
pixel*  Picyuv_CTUgetCrAddr(PicYuv *picyuv, uint32_t ctuAddr);
pixel*  Picyuv_CTUgetChromaAddr(PicYuv *picyuv, uint32_t chromaId, uint32_t ctuAddr);
pixel*  Picyuv_CTUgetPlaneAddr(PicYuv *picyuv, uint32_t plane, uint32_t ctuAddr);

/* get pointer to CU start address */
pixel* Picyuv_CUgetLumaAddr(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx);
pixel* Picyuv_CUgetCbAddr(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx);
pixel* Picyuv_CUgetCrAddr(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx);
intptr_t Picyuv_getChromaAddrOffset(PicYuv *picyuv, uint32_t ctuAddr, uint32_t absPartIdx);
pixel* Picyuv_CUgetChromaAddr(PicYuv *picyuv, uint32_t chromaId, uint32_t ctuAddr, uint32_t absPartIdx);
void PicYuv_destroy(PicYuv *picyuv);

#endif /* PICYUV_H_ */
