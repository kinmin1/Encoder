
#ifndef _NAL_H_
#define _NAL_H_

#include "common.h"
#include "x265.h"
#include "Bitstream.h"

typedef struct NALList
{

	x265_nal    m_nal[16];
	uint32_t    m_numNal;
	uint8_t*    m_buffer;
	uint32_t    m_occupancy;
	uint32_t    m_allocSize;
	uint8_t*    m_extraBuffer;
	uint32_t    m_extraOccupancy;
	uint32_t    m_extraAllocSize;
	bool        m_annexB;
}NALList;

void NAL_nal(NALList *nal);
void NAL_unal(NALList *nal);
void  takeContents(NALList *nal, NALList* other);
void serialize(NALList *nal, NalUnitType nalUnitType, Bitstream* bs);
uint32_t serializeSubstreams(NALList *nal, uint32_t* streamSizeBytes, uint32_t streamCount, Bitstream* streams);

#endif /* NAL_H_ */
