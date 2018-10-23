/*
* shortyuv.h
*
*  Created on: 2015-10-26
*      Author: adminster
*/

#ifndef SHORTYUV_H_
#define SHORTYUV_H_

#include "common.h"
struct Yuv;
#define SIZE32 32
#define SIZE16 16
typedef struct ShortYuv
{
	int16_t* m_buf[3];

	uint32_t m_size;
	uint32_t m_csize;

	int      m_csp;
	int      m_hChromaShift;
	int      m_vChromaShift;
}ShortYuv;

bool ShortYuv_create(ShortYuv* shyuv, uint32_t size, int csp);
bool ShortYuv_create1(ShortYuv* shyuv, uint32_t size, int csp);

bool ShortYuv_create_search(ShortYuv* shyuv, uint32_t size, int csp, int num);
bool ShortYuv_create_search_1(ShortYuv* shyuv, uint32_t size, int csp, int num);

int16_t* ShortYuv_getLumaAddr(ShortYuv* shortyuv, uint32_t absPartIdx);
int16_t* ShortYuv_getCbAddr(ShortYuv* shortyuv, uint32_t absPartIdx);
int16_t* ShortYuv_getCrAddr(ShortYuv* shortyuv, uint32_t absPartIdx);
int16_t* ShortYuv_getChromaAddr(ShortYuv* shortyuv, uint32_t chromaId, uint32_t absPartIdx);

const int16_t* ShortYuv_getLumaAddr_const(ShortYuv* shortyuv, uint32_t absPartIdx);
const int16_t* ShortYuv_getCbAddr_const(ShortYuv* shortyuv, uint32_t absPartIdx);
const int16_t* ShortYuv_getCrAddr_const(ShortYuv* shortyuv, uint32_t absPartIdx);
const int16_t* ShortYuv_getChromaAddr_const(ShortYuv* shortyuv, uint32_t chromaId, uint32_t absPartIdx);

void subtract(const struct Yuv* srcYuv0, const struct Yuv* srcYuv1, uint32_t log2Size);

void ShortYuv_copyPartToPartLuma(ShortYuv* srcYuv, ShortYuv* dstYuv, uint32_t absPartIdx, uint32_t log2Size);
void ShortYuv_copyPartToPartChroma(ShortYuv* srcYuv, ShortYuv* dstYuv, uint32_t absPartIdx, uint32_t log2SizeL);

void StoYuv_copyPartToPartLuma(ShortYuv* srcYuv, struct Yuv* dstYuv, uint32_t absPartIdx, uint32_t log2Size);
void StoYuv_copyPartToPartChroma(ShortYuv* srcYuv, struct Yuv* dstYuv, uint32_t absPartIdx, uint32_t log2SizeL);

int ShortYuv_getChromaAddrOffset(ShortYuv* shortYuv, uint32_t idx);

static int ShortYuv_getAddrOffset(uint32_t idx, uint32_t width);

#endif /* SHORTYUV_H_ */
