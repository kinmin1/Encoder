/*
* Bitstream.c
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#include"common.h"
#include"Bitstream.h"

uint8_t fifo_1[5000] = { 0 };
uint8_t fifo_2[100] = { 0 };
void resetBits(struct Bitstream *pbit)
{
	pbit->m_partialByteBits = 0;
	pbit->m_byteOccupancy = 0;
	pbit->m_partialByte = 0;
}
void push(struct Bitstream *pbit)
{
	pbit->m_fifo = fifo_1;// (uint8_t  *)malloc(sizeof(uint8_t) * 5000);
	resetBits(pbit);
}
void push_1(struct Bitstream *pbit)
{
	pbit->m_fifo = fifo_2;// (uint8_t  *)malloc(sizeof(uint8_t) * 5000);
	resetBits(pbit);
}
void push_back(struct Bitstream *pbit, uint8_t val)
{
	if (!pbit->m_fifo)
		return;
	pbit->m_fifo[pbit->m_byteOccupancy++] = val;
}
void enc_write(struct Bitstream *pbit, uint32_t val, uint32_t numBits)
{
	X265_CHECK(numBits <= 32, "numBits out of range\n");
	if (!(numBits == 32 || ((val & (~0u << numBits)) == 0)))
		printf("numBits & val out of range\n");

	uint32_t totalPartialBits = pbit->m_partialByteBits + numBits;
	uint32_t nextPartialBits = totalPartialBits & 7;
	uint8_t  nextHeldByte = (val << (8 - nextPartialBits)) & 0xFF;
	uint32_t writeBytes = totalPartialBits >> 3;

	if (writeBytes)
	{
		// topword aligns m_partialByte with the msb of val //
		uint32_t topword = (numBits - nextPartialBits) & ~7;

		uint32_t write_bits = (pbit->m_partialByte << topword) | (val >> nextPartialBits);
		switch (writeBytes)
		{
		case 4: push_back(pbit, (write_bits >> 24) & 0xff);
		case 3: push_back(pbit, (write_bits >> 16) & 0xff);
		case 2: push_back(pbit, (write_bits >> 8) & 0xff);
		case 1: push_back(pbit, (write_bits)& 0xff);
		}

		pbit->m_partialByte = nextHeldByte;
		pbit->m_partialByteBits = nextPartialBits;
	}
	else
	{
		pbit->m_partialByte |= nextHeldByte;
		pbit->m_partialByteBits = nextPartialBits;
	}
}
void writeAlignOne(struct Bitstream *pbit)
{
	uint32_t numBits = (8 - pbit->m_partialByteBits) & 0x7;

	enc_write(pbit, (1 << numBits) - 1, numBits);
}
void writeAlignZero(struct Bitstream *pbit)
{
	if (pbit->m_partialByteBits)
	{
		push_back(pbit, pbit->m_partialByte & 0xff);
		pbit->m_partialByte = 0;
		pbit->m_partialByteBits = 0;
	}
}
void writeByteAlignment(struct Bitstream *pbit)
{
	enc_write(pbit, 1, 1);
	writeAlignZero(pbit);
}
void writeByte(struct Bitstream *pbit, uint32_t val)
{
	// Only CABAC will call writeByte, the fifo must be byte aligned
	X265_CHECK(!pbit->m_partialByteBits, "expecting m_partialByteBits = 0\n");

	push_back(pbit, val & 0xff);
}
uint32_t Bitstream_getNumberOfWrittenBytes(struct Bitstream *pbit)
{
	return pbit->m_byteOccupancy;
}
uint32_t Bitstream_getNumberOfWrittenBits(struct Bitstream *pbit)
{
	return pbit->m_byteOccupancy * 8 + pbit->m_partialByteBits;
}
void writeUvlc(struct Bitstream *pbit, uint32_t code)
{
	uint32_t length = 1;
	uint32_t temp = ++code;

	X265_CHECK(temp, "writing -1 code, will cause infinite loop\n");

	while (1 != temp)
	{
		temp >>= 1;
		length += 2;
	}

	// Take care of cases where length > 32
	enc_write(pbit, 0, length >> 1);
	enc_write(pbit, code, (length + 1) >> 1);
}
void writeCode(struct Bitstream *pbit, uint32_t code, uint32_t length) { enc_write(pbit, code, length); }
void writeSvlc(struct Bitstream *pbit, int32_t code)                   { uint32_t ucode = (code <= 0) ? -code << 1 : (code << 1) - 1; writeUvlc(pbit, ucode); }
void writeFlag(struct Bitstream *pbit, char code)                      { enc_write(pbit, code, 1); }

void WRITE_CODE(struct Bitstream *pbit, uint32_t code, uint32_t length, const char *a) { writeCode(pbit, code, length); }
void WRITE_UVLC(struct Bitstream *pbit, uint32_t code, const char *a) { writeUvlc(pbit, code); }
void WRITE_SVLC(struct Bitstream *pbit, int32_t  code, const char *a) { writeSvlc(pbit, code); }
void WRITE_FLAG(struct Bitstream *pbit, char flag, const char *a) { writeFlag(pbit, flag); }
uint8_t* getFIFO(struct Bitstream *pbit)
{
	return pbit->m_fifo;
}
static inline int bs_size_ue(unsigned int val)
{
	return bitSize[val + 1];
}

static inline int bs_size_ue_big(unsigned int val)
{
	if (val < 255)
		return bitSize[val + 1];
	else
		return bitSize[(val + 1) >> 8] + 16;
}

static inline int bs_size_se(int val)
{
	int tmp = 1 - val * 2;

	if (tmp < 0) tmp = val * 2;
	if (tmp < 256)
		return bitSize[tmp];
	else
		return bitSize[tmp >> 8] + 16;
}
