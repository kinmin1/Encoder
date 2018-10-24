/*
* nal.c
*
*  Created on: 2016-8-10
*      Author: Administrator
*/

#include"nal.h"
int MAX_NAL_UNITS = 16;
void NAL_nal(NALList *nal)
{
	nal->m_numNal = 0;
	nal->m_buffer = NULL;
	nal->m_occupancy = 0;
	nal->m_allocSize = 0;
	nal->m_extraBuffer = NULL;
	nal->m_extraOccupancy = 0;
	nal->m_extraAllocSize = 0;
	nal->m_annexB = TRUE;
}

void  takeContents(NALList *nal, NALList* other)
{
	/* take other NAL buffer, discard our old one */
	free(nal->m_buffer); nal->m_buffer = NULL;
	nal->m_buffer = other->m_buffer;
	nal->m_allocSize = other->m_allocSize;
	nal->m_occupancy = other->m_occupancy;

	/* copy packet data */
	nal->m_numNal = other->m_numNal;
	memcpy(nal->m_nal, other->m_nal, sizeof(x265_nal) * nal->m_numNal);

	/* reset other list, re-allocate their buffer with same size */
	other->m_numNal = 0;
	other->m_occupancy = 0;
	other->m_buffer = (uint8_t *)malloc(sizeof(uint8_t)*nal->m_allocSize);//ÒÑÊÍ·Å
	//printf("sizeof(uint8_t)*nal->m_allocSize=\n",sizeof(uint8_t)*nal->m_allocSize);
}

void serialize(NALList *nal, NalUnitType nalUnitType, Bitstream* bs)
{
	static const char startCodePrefix[] = { 0, 0, 0, 1 };

	uint32_t payloadSize = Bitstream_getNumberOfWrittenBytes(bs);
	const uint8_t* bpayload = getFIFO(bs);
	if (!bpayload)
		return;

	int count = sizeof(startCodePrefix);
	uint32_t nextSize = nal->m_occupancy + sizeof(startCodePrefix) + 2 + payloadSize + (payloadSize >> 1) + nal->m_extraOccupancy;
	//printf("nextSize=%d,nal->m_occupancy=%d,nal->m_extraOccupancy=%d",nextSize,nal->m_occupancy,nal->m_extraOccupancy);
	if (nextSize > nal->m_allocSize)
	{
		uint8_t *temp = (uint8_t *)malloc(sizeof(uint8_t)*nextSize);
		//printf("sizeof(uint8_t)*nextSize=\n",sizeof(uint8_t)*nextSize);
		if (temp)
		{
			memcpy(temp, nal->m_buffer, nal->m_occupancy);

			/* fixup existing payload pointers */
			for (uint32_t i = 0; i < nal->m_numNal; i++)
				nal->m_nal[i].payload = temp + (nal->m_nal[i].payload - nal->m_buffer);

			free(nal->m_buffer); nal->m_buffer = NULL;
			nal->m_buffer = temp;
			nal->m_allocSize = nextSize;
		}
		else
		{
			X265_CHECK((temp != NULL), "Unable to realloc access_1 unit buffer\n");
			return;
		}
	}

	uint8_t *out = nal->m_buffer + nal->m_occupancy;
	uint32_t bytes = 0;

	if (!nal->m_annexB)
	{
		/* Will write size later */
		bytes += 4;
	}
	else if (0/*!nal->m_numNal*/ || nalUnitType == NAL_UNIT_VPS || nalUnitType == NAL_UNIT_SPS || nalUnitType == NAL_UNIT_PPS)
	{
		memcpy(out, startCodePrefix, 4);
		bytes += 4;
	}
	else
	{
		memcpy(out, startCodePrefix + 1, 3);
		bytes += 3;
	}

	/* 16 bit NAL header:
	* forbidden_zero_bit       1-bit
	* nal_unit_type            6-bits
	* nuh_reserved_zero_6bits  6-bits
	* nuh_temporal_id_plus1    3-bits */
	out[bytes++] = (uint8_t)nalUnitType << 1;
	out[bytes++] = 1 + (nalUnitType == NAL_UNIT_CODED_SLICE_TSA_N);

	/* 7.4.1 ...
	* Within the NAL unit, the following three-byte sequences shall not occur at
	* any byte-aligned position:
	*  - 0x000000
	*  - 0x000001
	*  - 0x000002 */
	for (uint32_t i = 0; i < payloadSize; i++)
	{
		if (i > 2 && !out[bytes - 2] && !out[bytes - 3] && out[bytes - 1] <= 0x03)
		{
			/* inject 0x03 to prevent emulating a start code */
			out[bytes] = out[bytes - 1];
			out[bytes - 1] = 0x03;
			bytes++;
		}

		out[bytes++] = bpayload[i];
	}

	X265_CHECK(bytes <= 4 + 2 + payloadSize + (payloadSize >> 1), "NAL buffer overflow\n");

	if (nal->m_extraOccupancy)
	{
		/* these bytes were escaped by serializeSubstreams */
		memcpy(out + bytes, nal->m_extraBuffer, nal->m_extraOccupancy);
		bytes += nal->m_extraOccupancy;
		nal->m_extraOccupancy = 0;
	}

	/* 7.4.1.1
	* ... when the last byte of the RBSP data is equal to 0x00 (which can
	* only occur when the RBSP ends in a cabac_zero_word), a final byte equal
	* to 0x03 is appended to the end of the data.  */
	if (!out[bytes - 1])
		out[bytes++] = 0x03;

	if (!nal->m_annexB)
	{
		uint32_t dataSize = bytes - 4;
		out[0] = (uint8_t)(dataSize >> 24);
		out[1] = (uint8_t)(dataSize >> 16);
		out[2] = (uint8_t)(dataSize >> 8);
		out[3] = (uint8_t)dataSize;
	}

	nal->m_occupancy += bytes;

	X265_CHECK(nal->m_numNal < (uint32_t)MAX_NAL_UNITS, "NAL count overflow\n");

	x265_nal* nall = &nal->m_nal[nal->m_numNal++];
	nall->type = nalUnitType;
	nall->sizeBytes = bytes;
	nall->payload = out;
}

/* concatenate and escape WPP sub-streams, return escaped row lengths.
* These streams will be appended to the next serialized NAL */
uint32_t serializeSubstreams(NALList *nal, uint32_t* streamSizeBytes, uint32_t streamCount, Bitstream* streams)
{
	uint32_t maxStreamSize = 0;
	uint32_t estSize = 0;
	for (uint32_t s = 0; s < streamCount; s++)
		estSize += Bitstream_getNumberOfWrittenBytes(&streams[s]);
	estSize += estSize >> 1;

	if (estSize > nal->m_extraAllocSize)
	{
		uint8_t *temp = (uint8_t *)malloc(sizeof(uint8_t)*estSize);
		if (temp)
		{
			free(nal->m_extraBuffer); nal->m_extraBuffer = NULL;
			nal->m_extraBuffer = temp;
			nal->m_extraAllocSize = estSize;
		}
		else
		{
			X265_CHECK((temp != NULL), "Unable to realloc access unit buffer\n");
			return 0;
		}
	}

	uint32_t bytes = 0;
	uint8_t *out = nal->m_extraBuffer;
	for (uint32_t s = 0; s < streamCount; s++)
	{
		Bitstream* stream = &streams[s];
		uint32_t inSize = Bitstream_getNumberOfWrittenBytes(stream);
		const uint8_t *inBytes = getFIFO(stream);
		uint32_t prevBufSize = bytes;

		if (inBytes)
		{
			for (uint32_t i = 0; i < inSize; i++)
			{
				if (bytes >= 2 && !out[bytes - 2] && !out[bytes - 1] && inBytes[i] <= 0x03)
				{
					/* inject 0x03 to prevent emulating a start code */
					out[bytes++] = 3;
				}

				out[bytes++] = inBytes[i];
			}
		}

		if (s < streamCount - 1)
		{
			streamSizeBytes[s] = bytes - prevBufSize;
			if (streamSizeBytes[s] > maxStreamSize)
				maxStreamSize = streamSizeBytes[s];
		}
	}

	nal->m_extraOccupancy = bytes;
	return maxStreamSize;
}
void NAL_unal(NALList *nal)
{
	free(nal->m_buffer); nal->m_buffer = NULL;
	free(nal->m_extraBuffer); nal->m_extraBuffer = NULL;
}

