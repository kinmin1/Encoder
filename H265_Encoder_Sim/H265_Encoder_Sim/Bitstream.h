/*
* Bitstream.h
*
*  Created on: 2016-5-15
*      Author: Administrator
*/

#ifndef BITSTREAM_H_
#define BITSTREAM_H_

#include"x265.h"

#define MIN_FIFO_SIZE 5000

typedef struct Bitstream
{
	uint8_t  *m_fifo;
	uint32_t m_byteAlloc;
	uint32_t m_byteOccupancy;
	uint32_t m_partialByteBits;
	uint8_t  m_partialByte;
}Bitstream;

typedef struct SyntaxElementWriter
{
	Bitstream* m_bitIf;
}SyntaxElementWriter;

void resetBits(Bitstream *pbit);
void push(Bitstream *pbit);
void push_1(struct Bitstream *pbit);
void push_back(Bitstream *pbit, uint8_t val);
void enc_write(Bitstream *pbit, uint32_t val, uint32_t numBits);
void writeAlignOne(Bitstream *pbit);
void writeAlignZero(Bitstream *pbit);
void writeByteAlignment(Bitstream *pbit);
void writeByte(Bitstream *pbit, uint32_t val);
uint32_t Bitstream_getNumberOfWrittenBytes(Bitstream *pbit);
uint32_t Bitstream_getNumberOfWrittenBits(Bitstream *pbit);

void WRITE_CODE(Bitstream *pbit, uint32_t code, uint32_t length, const char *a);
void WRITE_UVLC(Bitstream *pbit, uint32_t code, const char *a);
void WRITE_SVLC(Bitstream *pbit, int32_t  code, const char *a);
void WRITE_FLAG(Bitstream *pbit, char flag, const char *a);
void writeCode(Bitstream *pbit, uint32_t code, uint32_t length);
void writeUvlc(Bitstream *pbit, uint32_t code);
void writeSvlc(Bitstream *pbit, int32_t code);
void writeFlag(Bitstream *pbit, char code);
static const uint8_t bitSize[256] =
{
	1, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
};

static inline int bs_size_ue(unsigned int val);
static inline int bs_size_ue_big(unsigned int val);
static inline int bs_size_se(int val);
uint8_t* getFIFO(struct Bitstream *pbit);

#endif /* BITSTREAM_H_ */
