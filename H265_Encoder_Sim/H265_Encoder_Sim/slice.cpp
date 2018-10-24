/*
* slice.c
*
*  Created on: 2015-10-28
*      Author: adminster
*/

#include "common.h"
#include "frame.h"
#include "picyuv.h"
#include "slice.h"
#include <string.h>
#include "constants.h"

void init_RPS(struct RPS *rps)
{
	rps->numberOfPictures = 0;
	rps->numberOfNegativePictures = 0;
	rps->numberOfPositivePictures = 0;

	memset(rps->deltaPOC, 0, sizeof(rps->deltaPOC));
	memset(rps->poc, 0, sizeof(rps->poc));
	memset(rps->bUsed, 0, sizeof(rps->bUsed));
}

void Slice_setRefPicList(Slice *slice, struct PicList* picList)
{
	if (slice->m_sliceType == I_SLICE)
	{
		memset(slice->m_refPicList, 0, sizeof(slice->m_refPicList));
		slice->m_numRefIdx[1] = slice->m_numRefIdx[0] = 0;
		return;
	}

	struct Frame* refPic = NULL;
	struct Frame* refPicSetStCurr0[MAX_NUM_REF];
	struct Frame* refPicSetStCurr1[MAX_NUM_REF];
	struct Frame* refPicSetLtCurr[MAX_NUM_REF];
	int numPocStCurr0 = 0;
	int numPocStCurr1 = 0;
	int numPocLtCurr = 0;
	int i;

	for (i = 0; i < slice->m_rps.numberOfNegativePictures; i++)
	{
		if (slice->m_rps.bUsed[i])
		{
			//picList->getPOC = PicList_getPOC;
			refPic = PicList_getPOC(picList, slice->m_poc + slice->m_rps.deltaPOC[i]);
			refPicSetStCurr0[numPocStCurr0] = refPic;
			numPocStCurr0++;
		}
	}

	for (; i < slice->m_rps.numberOfNegativePictures + slice->m_rps.numberOfPositivePictures; i++)
	{
		if (slice->m_rps.bUsed[i])
		{
			//picList->getPOC = PicList_getPOC;
			refPic = PicList_getPOC(picList, slice->m_poc + slice->m_rps.deltaPOC[i]);
			refPicSetStCurr1[numPocStCurr1] = refPic;
			numPocStCurr1++;
		}
	}

	if (!(slice->m_rps.numberOfPictures == slice->m_rps.numberOfNegativePictures + slice->m_rps.numberOfPositivePictures))
		printf("unexpected picture in RPS\n");

	// ref_pic_list_init
	struct Frame* rpsCurrList0[MAX_NUM_REF + 1];
	struct Frame* rpsCurrList1[MAX_NUM_REF + 1];
	int numPocTotalCurr = numPocStCurr0 + numPocStCurr1 + numPocLtCurr;

	int cIdx = 0;
	for (i = 0; i < numPocStCurr0; i++, cIdx++)
		rpsCurrList0[cIdx] = refPicSetStCurr0[i];

	for (i = 0; i < numPocStCurr1; i++, cIdx++)
		rpsCurrList0[cIdx] = refPicSetStCurr1[i];

	for (i = 0; i < numPocLtCurr; i++, cIdx++)
		rpsCurrList0[cIdx] = refPicSetLtCurr[i];

	if (!(cIdx == numPocTotalCurr))
		printf("RPS index check fail\n");

	if (slice->m_sliceType == B_SLICE)
	{
		cIdx = 0;
		for (i = 0; i < numPocStCurr1; i++, cIdx++)
			rpsCurrList1[cIdx] = refPicSetStCurr1[i];

		for (i = 0; i < numPocStCurr0; i++, cIdx++)
			rpsCurrList1[cIdx] = refPicSetStCurr0[i];

		for (i = 0; i < numPocLtCurr; i++, cIdx++)
			rpsCurrList1[cIdx] = refPicSetLtCurr[i];

		if (!(cIdx == numPocTotalCurr))
			printf("RPS index check fail\n");
	}

	int rIdx;
	for (rIdx = 0; rIdx < slice->m_numRefIdx[0]; rIdx++)
	{
		cIdx = rIdx % numPocTotalCurr;
		if (!(cIdx >= 0 && cIdx < numPocTotalCurr))
			printf("RPS index check fail\n");
		slice->m_refPicList[0][rIdx] = rpsCurrList0[cIdx];
	}

	if (slice->m_sliceType != B_SLICE)
	{
		slice->m_numRefIdx[1] = 0;
		memset(slice->m_refPicList[1], 0, sizeof(slice->m_refPicList[1]));
	}
	else
	{
		int rIdx;
		for (rIdx = 0; rIdx < slice->m_numRefIdx[1]; rIdx++)
		{
			cIdx = rIdx % numPocTotalCurr;
			if (!(cIdx >= 0 && cIdx < numPocTotalCurr))
				printf("RPS index check fail\n");
			slice->m_refPicList[1][rIdx] = rpsCurrList1[cIdx];
		}
	}

	int dir, numRefIdx;
	for (dir = 0; dir < 2; dir++)
		for (numRefIdx = 0; numRefIdx < slice->m_numRefIdx[dir]; numRefIdx++)
			slice->m_refPOCList[dir][numRefIdx] = slice->m_refPicList[dir][numRefIdx]->m_poc;
}

/* Sorts the deltaPOC and Used by current values in the RPS based on the
* deltaPOC values.  deltaPOC values are sorted with -ve values before the +ve
* values.  -ve values are in decreasing order.  +ve values are in increasing
* order */
void RPS_sortDeltaPOC(struct RPS* rps)
{
	int j;
	// sort in increasing order (smallest first)
	for (j = 1; j < rps->numberOfPictures; j++)
	{
		int dPOC = rps->deltaPOC[j];
		int used = rps->bUsed[j];
		int k;
		for (k = j - 1; k >= 0; k--)
		{
			int temp = rps->deltaPOC[k];
			if (dPOC < temp)
			{
				rps->deltaPOC[k + 1] = temp;
				rps->bUsed[k + 1] = rps->bUsed[k];
				rps->deltaPOC[k] = dPOC;
				rps->bUsed[k] = used;
			}
		}
	}

	// flip the negative values to largest first
	int numNegPics = rps->numberOfNegativePictures;
	int k;
	for (j = 0, k = numNegPics - 1; j < numNegPics >> 1; j++, k--)
	{
		int dPOC = rps->deltaPOC[j];
		int used = rps->bUsed[j];
		rps->deltaPOC[j] = rps->deltaPOC[k];
		rps->bUsed[j] = rps->bUsed[k];
		rps->deltaPOC[k] = dPOC;
		rps->bUsed[k] = used;
	}
}

void initSlice(struct Slice *slice)
{
	slice->m_lastIDR = 0;
	slice->m_sLFaseFlag = TRUE;
	slice->m_numRefIdx[0] = slice->m_numRefIdx[1] = 0;
	int i;
	for (i = 0; i < MAX_NUM_REF; i++)
	{
		slice->m_refPicList[0][i] = NULL;
		slice->m_refPicList[1][i] = NULL;
		slice->m_refPOCList[0][i] = 0;
		slice->m_refPOCList[1][i] = 0;
	}
}

bool getRapPicFlag(Slice* slice)
{
	return slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
		|| slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA;
}

bool getIdrPicFlag(Slice* slice)
{
	return slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL;
}

void HRDInfo_Info(HRDInfo *info)
{
	info->bitRateScale = 0;
	info->cpbSizeScale = 0;
	info->initialCpbRemovalDelayLength = 1;
	info->cpbRemovalDelayLength = 1;
	info->dpbOutputDelayLength = 1;
	info->cbrFlag = FALSE;
}

void Window_dow(Window *dow)
{
	dow->bEnabled = FALSE;
}

Frame* getRefPic(Slice *slice, int list, int refIdx)  { return refIdx >= 0 ? slice->m_refPicList[list][refIdx] : NULL; }

bool isIRAP(Slice* slice)    { return slice->m_nalUnitType >= 16 && slice->m_nalUnitType <= 23; }

bool Slice_isIntra(const Slice* slice)   { return slice->m_sliceType == I_SLICE; }

bool isInterB(const Slice* slice)  { return slice->m_sliceType == B_SLICE; }

bool isInterP(const Slice* slice)  { return slice->m_sliceType == P_SLICE; }

const uint32_t Slice_realEndAddress(const Slice *slice, uint32_t endCUAddr)
{
	// Calculate end address
	uint32_t internalAddress = (endCUAddr - 1) % NUM_4x4_PARTITIONS;
	uint32_t externalAddress = (endCUAddr - 1) / NUM_4x4_PARTITIONS;
	uint32_t xmax = slice->m_sps->picWidthInLumaSamples - (externalAddress % slice->m_sps->numCuInWidth) * g_maxCUSize;
	uint32_t ymax = slice->m_sps->picHeightInLumaSamples - (externalAddress / slice->m_sps->numCuInWidth) * g_maxCUSize;

	while (g_zscanToPelX[internalAddress] >= xmax || g_zscanToPelY[internalAddress] >= ymax)
		internalAddress--;

	internalAddress++;
	if (internalAddress == NUM_4x4_PARTITIONS)
	{
		internalAddress = 0;
		externalAddress++;
	}

	return externalAddress * NUM_4x4_PARTITIONS + internalAddress;
}

