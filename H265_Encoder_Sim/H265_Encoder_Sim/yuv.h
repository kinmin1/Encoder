/*
* yuv.h
*
*  Created on: 2015-10-26
*      Author: adminster
*/

#ifndef YUV_H_
#define YUV_H_

#include "common.h"
#include "primitives.h"
#include "constants.h"
#include "picyuv.h"
#include "shortyuv.h"
#include "cudata.h"

typedef struct Yuv
{
	pixel*   m_buf[3];

	uint32_t m_size;
	uint32_t m_csize;
	int      m_part;         // cached partition enum size

	int      m_csp;
	int      m_hChromaShift;
	int      m_vChromaShift;
}Yuv;

struct ShortYuv;

void Yuv_init(Yuv *yuv);
int Yuv_create(Yuv*yuv, uint32_t size, int csp);

int Yuv_create_md(Yuv*yuv, uint32_t size, int csp);
int Yuv_create_md_pred_0(Yuv*yuv, uint32_t size, int csp);
int Yuv_create_md_pred_1(Yuv*yuv, uint32_t size, int csp);

int Yuv_create_md_pred_pred2Nx2N_0(Yuv*yuv, uint32_t size, int csp);
int Yuv_create_md_pred_pred2Nx2N_1(Yuv*yuv, uint32_t size, int csp);

int Yuv_create_search(Yuv*yuv, uint32_t size, int csp, int num);
int Yuv_create_search_1(Yuv*yuv, uint32_t size, int csp, int num);
int Yuv_create_search_2(Yuv*yuv, uint32_t size, int csp, int num);
int Yuv_create_search_3(Yuv*yuv, uint32_t size, int csp, int num);


void Yuv_copyToPicYuv(Yuv *yuv, PicYuv *dstPic, uint32_t cuAddr, uint32_t absPartIdx);
void Yuv_copyFromPicYuv(Yuv *yuv, PicYuv *srcPic, uint32_t cuAddr, uint32_t absPartIdx);
void Yuv_copyFromYuv(Yuv *yuv, const Yuv srcYuv);
void Yuv_copyPUFromYuv(Yuv *yuv, const Yuv* srcYuv, uint32_t absPartIdx, int partEnum, int bChroma, int pwidth, int pheight);
void Yuv_copyToPartYuv(Yuv *yuv, Yuv* dstYuv, uint32_t absPartIdx);
void Yuv_copyPartToYuv(Yuv *yuv, Yuv* dstYuv, uint32_t absPartIdx);

void Yuv_copyPartToPartLuma(Yuv *yuv, Yuv *dstYuv, uint32_t absPartIdx, uint32_t log2Size);
void Yuv_copyPartToPartChroma(Yuv *yuv, Yuv *dstYuv, uint32_t absPartIdx, uint32_t log2SizeL);

pixel* Yuv_getLumaAddr(Yuv* yuv, uint32_t absPartIdx);
pixel* Yuv_getCbAddr(Yuv* yuv, uint32_t absPartIdx);
pixel* Yuv_getCrAddr(Yuv *yuv, uint32_t absPartIdx);
pixel* Yuv_getChromaAddr(Yuv* yuv, uint32_t chromaId, uint32_t absPartIdx);
int Yuv_getAddrOffset(uint32_t absPartIdx, uint32_t width);
int Yuv_getChromaAddrOffset(const Yuv *yuv, uint32_t absPartIdx);
pixel* Yuv_getLumaAddr_const(Yuv* yuv, uint32_t absPartIdx);
const pixel* Yuv_getCbAddr_const(const Yuv* yuv, uint32_t absPartIdx);
const pixel* Yuv_getCrAddr_const(const Yuv* yuv, uint32_t absPartIdx);
const pixel* Yuv_getChromaAddr_const(const  Yuv* yuv, uint32_t chromaId, uint32_t absPartIdx);
void Yuv_addClip(Yuv *yuv, const Yuv* srcYuv0, const struct ShortYuv* srcYuv1, uint32_t log2SizeL);

#endif /* YUV_H_ */
