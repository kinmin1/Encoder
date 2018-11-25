/*
* dpb.c
*
*  Created on: 2016-8-2
*      Author: SCL
*/

#include "common.h"
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"
#include "slice.h"
#include "constants.h"
#include "dpb.h"


void DPB_Destroy(DPB *dpb)
{
	while (!empty(&dpb->m_freeList))
	{
		Frame* curFrame = PicList_popFront(&dpb->m_freeList);
		Frame_destroy(curFrame);
		free(curFrame);
		curFrame = NULL;
	}

	while (!empty(&dpb->m_picList))
	{
		Frame* curFrame = PicList_popFront(&dpb->m_picList);
		Frame_destroy(curFrame);
		curFrame = NULL;
	}

	while (dpb->m_picSymFreeList)
	{
		FrameData* next = dpb->m_picSymFreeList->m_freeListNext;
		FrameData_destory(dpb->m_picSymFreeList);

		PicYuv_destroy(dpb->m_picSymFreeList->m_reconPic);
		free(dpb->m_picSymFreeList->m_reconPic);
		dpb->m_picSymFreeList->m_reconPic = NULL;

		free(dpb->m_picSymFreeList);
		//dpb->m_picSymFreeList = NULL;
		dpb->m_picSymFreeList = next;
	}
}
void DPB_init(DPB *dpb, x265_param *param)
{
	dpb->m_lastIDR = 0;
	dpb->m_pocCRA = 0;
	dpb->m_bRefreshPending = FALSE;
	dpb->m_picSymFreeList = NULL;
	dpb->m_maxRefL0 = param->maxNumReferences;
	dpb->m_maxRefL1 = param->bBPyramid ? 2 : 1;
	dpb->m_bOpenGOP = param->bOpenGOP;
	dpb->m_bTemporalSublayer = !!param->bEnableTemporalSubLayers;
}

void DPB_prepareEncode(DPB * dpb, Frame *newFrame)
{
	Slice* slice = newFrame->m_encData->m_slice;
	slice->m_poc = newFrame->m_poc;

	int pocCurr = slice->m_poc;
	int type = 1;//newFrame->m_lowres.sliceType;
	bool bIsKeyFrame = TRUE;//newFrame->m_lowres.bKeyframe;

	slice->m_nalUnitType = (NalUnitType)getNalUnitType(dpb, pocCurr, bIsKeyFrame);
	if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL)
		dpb->m_lastIDR = pocCurr;
	slice->m_lastIDR = dpb->m_lastIDR;
	slice->m_sliceType = IS_X265_TYPE_B(type) ? B_SLICE : (type == X265_TYPE_P) ? P_SLICE : I_SLICE;

	if (type == X265_TYPE_B)
	{
		newFrame->m_encData->m_bHasReferences = FALSE;

		// Adjust NAL type for unreferenced B frames (change from _R "referenced"
		// to _N "non-referenced" NAL unit type)
		switch (slice->m_nalUnitType)
		{
		case NAL_UNIT_CODED_SLICE_TRAIL_R:
			slice->m_nalUnitType = dpb->m_bTemporalSublayer ? NAL_UNIT_CODED_SLICE_TSA_N : NAL_UNIT_CODED_SLICE_TRAIL_N;
			break;
		case NAL_UNIT_CODED_SLICE_RADL_R:
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RADL_N;
			break;
		case NAL_UNIT_CODED_SLICE_RASL_R:
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RASL_N;
			break;
		default:
			break;
		}
	}
	else
	{
		// m_bHasReferences starts out as true for non-B pictures, and is set to false
		// once no more pictures reference it 
		newFrame->m_encData->m_bHasReferences = TRUE;
	}

	//PicList_pushFront(&dpb->m_picList, newFrame);
	// Do decoding refresh marking if any
	//DPB_decodingRefreshMarking(dpb, pocCurr, slice->m_nalUnitType);
	//DP_computeRPS(dpb, pocCurr, isIRAP(slice), &slice->m_rps, slice->m_sps->maxDecPicBuffering);

	// Mark pictures in m_piclist as unreferenced if they are not included in RPS
	//DPB_applyReferencePictureSet(dpb, &slice->m_rps, pocCurr);
	slice->m_rps.numberOfNegativePictures = 1;

	slice->m_numRefIdx[0] = X265_MIN(dpb->m_maxRefL0, slice->m_rps.numberOfNegativePictures); // Ensuring L0 contains just the -ve POC
	slice->m_numRefIdx[1] = X265_MIN(dpb->m_maxRefL1, slice->m_rps.numberOfPositivePictures);
	//Slice_setRefPicList(slice, &dpb->m_picList);

	if (slice->m_sliceType == B_SLICE)
	{
		/// TODO: the lookahead should be able to tell which reference picture
		// had the least motion residual.  We should be able to use that here to
		// select a colocation reference list and index 
		slice->m_colFromL0Flag = FALSE;
		slice->m_colRefIdx = 0;
		slice->m_bCheckLDC = FALSE;
	}
	else
	{
		slice->m_bCheckLDC = TRUE;
		slice->m_colFromL0Flag = TRUE;
		slice->m_colRefIdx = 0;
	}
	slice->m_sLFaseFlag = (SLFASE_CONSTANT & (1 << (pocCurr % 31))) > 0;

	// Increment reference count of all motion-referenced frames to prevent them
	// from being recycled. These counts are decremented at the end of
	// compressFrame() 
	int numPredDir = isInterP(slice) ? 1 : isInterB(slice) ? 2 : 0;
	for (int l = 0; l < numPredDir; l++)
	{
		for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
		{
			Frame *refpic = slice->m_refPicList[l][ref];
			//slice->m_refPicList[l][ref]->m_reconPic->m_picOrg[0] = reconFrameBuf_Y;
			//slice->m_refPicList[l][ref]->m_reconPic->m_picOrg[1] = reconFrameBuf_U;
			//slice->m_refPicList[l][ref]->m_reconPic->m_picOrg[2] = reconFrameBuf_V;
			//ATOMIC_INC(&refpic->m_countRefEncoders);
		}
	}
}
void DPB_prepareEncode2(DPB * dpb, Frame *newFrame)
{/*
	Slice* slice = newFrame->m_encData->m_slice;
	slice->m_poc = newFrame->m_poc;

	int pocCurr = slice->m_poc;
	int type = 3;//newFrame->m_lowres.sliceType;
	bool bIsKeyFrame = FALSE;//newFrame->m_lowres.bKeyframe;

	slice->m_nalUnitType = getNalUnitType(dpb, pocCurr, bIsKeyFrame);
	if (slice->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL)
		dpb->m_lastIDR = pocCurr;
	slice->m_lastIDR = dpb->m_lastIDR;
	slice->m_sliceType = IS_X265_TYPE_B(type) ? B_SLICE : (type == X265_TYPE_P) ? P_SLICE : I_SLICE;

	if (type == X265_TYPE_B)
	{
		newFrame->m_encData->m_bHasReferences = FALSE;

		// Adjust NAL type for unreferenced B frames (change from _R "referenced"
		// to _N "non-referenced" NAL unit type)
		switch (slice->m_nalUnitType)
		{
		case NAL_UNIT_CODED_SLICE_TRAIL_R:
			slice->m_nalUnitType = dpb->m_bTemporalSublayer ? NAL_UNIT_CODED_SLICE_TSA_N : NAL_UNIT_CODED_SLICE_TRAIL_N;
			break;
		case NAL_UNIT_CODED_SLICE_RADL_R:
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RADL_N;
			break;
		case NAL_UNIT_CODED_SLICE_RASL_R:
			slice->m_nalUnitType = NAL_UNIT_CODED_SLICE_RASL_N;
			break;
		default:
			break;
		}
	}
	else
	{
		// m_bHasReferences starts out as true for non-B pictures, and is set to false
		// once no more pictures reference it 
		newFrame->m_encData->m_bHasReferences = TRUE;
	}
	dpb->m_picList.m_start->m_reconPic->m_picBuf[0] = reconFrameBuf_Y;
	dpb->m_picList.m_start->m_reconPic->m_picBuf[1] = reconFrameBuf_U;
	dpb->m_picList.m_start->m_reconPic->m_picBuf[2] = reconFrameBuf_V;

	PicList_pushFront(&dpb->m_picList, newFrame);
	// Do decoding refresh marking if any
	DPB_decodingRefreshMarking(dpb, pocCurr, slice->m_nalUnitType);

	DP_computeRPS(dpb, pocCurr, isIRAP(slice), &slice->m_rps, slice->m_sps->maxDecPicBuffering);

	// Mark pictures in m_piclist as unreferenced if they are not included in RPS
	DPB_applyReferencePictureSet(dpb, &slice->m_rps, pocCurr);

	slice->m_numRefIdx[0] = X265_MIN(dpb->m_maxRefL0, slice->m_rps.numberOfNegativePictures); // Ensuring L0 contains just the -ve POC
	slice->m_numRefIdx[1] = X265_MIN(dpb->m_maxRefL1, slice->m_rps.numberOfPositivePictures);
	Slice_setRefPicList(slice, &dpb->m_picList);

	//X265_CHECK(slice->m_sliceType != B_SLICE || slice->m_numRefIdx[1], "B slice without L1 references (non-fatal)\n");

	if (slice->m_sliceType == B_SLICE)
	{
		// TODO: the lookahead should be able to tell which reference picture
		// had the least motion residual.  We should be able to use that here to
		// select a colocation reference list and index 
		slice->m_colFromL0Flag = FALSE;
		slice->m_colRefIdx = 0;
		slice->m_bCheckLDC = FALSE;
	}
	else
	{
		slice->m_bCheckLDC = TRUE;
		slice->m_colFromL0Flag = TRUE;
		slice->m_colRefIdx = 0;
	}
	slice->m_sLFaseFlag = (SLFASE_CONSTANT & (1 << (pocCurr % 31))) > 0;

	// Increment reference count of all motion-referenced frames to prevent them
	// from being recycled. These counts are decremented at the end of
	// compressFrame() 
	int numPredDir = isInterP(slice) ? 1 : isInterB(slice) ? 2 : 0;
	for (int l = 0; l < numPredDir; l++)
	{
		for (int ref = 0; ref < slice->m_numRefIdx[l]; ref++)
		{
			Frame *refpic = slice->m_refPicList[l][ref];
			//ATOMIC_INC(&refpic->m_countRefEncoders);
			slice->m_refPicList[l][ref]->m_reconPic->m_picOrg[0] = reconFrameBuf_Y;
			slice->m_refPicList[l][ref]->m_reconPic->m_picOrg[1] = reconFrameBuf_U;
			slice->m_refPicList[l][ref]->m_reconPic->m_picOrg[2] = reconFrameBuf_V;
		}
	}*/
}
int getNalUnitType(DPB *dpb, int curPOC, bool bIsKeyFrame)
{
	if (!curPOC)
		return NAL_UNIT_CODED_SLICE_IDR_W_RADL;

	if (bIsKeyFrame)
		return dpb->m_bOpenGOP ? NAL_UNIT_CODED_SLICE_CRA : NAL_UNIT_CODED_SLICE_IDR_W_RADL;

	if (dpb->m_pocCRA && curPOC < dpb->m_pocCRA)
		// All leading pictures are being marked as TFD pictures here since
		// current encoder uses all reference pictures while encoding leading
		// pictures. An encoder can ensure that a leading picture can be still
		// decodable when random accessing to a CRA/CRANT/BLA/BLANT picture by
		// controlling the reference pictures used for encoding that leading
		// picture. Such a leading picture need not be marked as a TFD picture.
		return NAL_UNIT_CODED_SLICE_RASL_R;

	if (dpb->m_lastIDR && curPOC < dpb->m_lastIDR)
		return NAL_UNIT_CODED_SLICE_RADL_R;
		
	return NAL_UNIT_CODED_SLICE_TRAIL_R;
}

/* Marking reference pictures when an IDR/CRA is encountered. */
void DPB_decodingRefreshMarking(DPB * dpb, int pocCurr, NalUnitType nalUnitType)
{/*
	if (nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL)
	{
		// If the nal_unit_type is IDR, all pictures in the reference picture
		// list are marked as "unused for reference" 
		Frame* iterFrame = first(&dpb->m_picList);
		while (iterFrame)
		{
			if (iterFrame->m_poc != pocCurr)
				iterFrame->m_encData->m_bHasReferences = FALSE;
			iterFrame = iterFrame->m_next;
		}
	}
	else // CRA or No DR
	{
		if (dpb->m_bRefreshPending && pocCurr > dpb->m_pocCRA)
		{
			// If the bRefreshPending flag is true (a deferred decoding refresh
			// is pending) and the current temporal reference is greater than
			// the temporal reference of the latest CRA picture (pocCRA), mark
			// all reference pictures except the latest CRA picture as "unused
			// for reference" and set the bRefreshPending flag to false 
			Frame* iterFrame = first(&dpb->m_picList);
			while (iterFrame)
			{
				if (iterFrame->m_poc != pocCurr && iterFrame->m_poc != dpb->m_pocCRA)
					iterFrame->m_encData->m_bHasReferences = FALSE;
				iterFrame = iterFrame->m_next;
			}

			dpb->m_bRefreshPending = FALSE;
		}
		if (nalUnitType == NAL_UNIT_CODED_SLICE_CRA)
		{
			//If the nal_unit_type is CRA, set the bRefreshPending flag to true
			// and pocCRA to the temporal reference of the current picture 
			dpb->m_bRefreshPending = TRUE;
			dpb->m_pocCRA = pocCurr;
		}
	}
	*/
	/* Note that the current picture is already placed in the reference list and
	* its marking is not changed.  If the current picture has a nal_ref_idc
	* that is not 0, it will remain marked as "used for reference" */
}
void DP_computeRPS(DPB * dpb, int curPoc, bool isRAP, RPS * rps, unsigned int maxDecPicBuffer)
{/*
	unsigned int poci = 0, numNeg = 0, numPos = 0;

	Frame* iterPic = first(&dpb->m_picList);

	while (iterPic && (poci < maxDecPicBuffer - 1))
	{
		if ((iterPic->m_poc != curPoc) && iterPic->m_encData->m_bHasReferences)
		{
			rps->poc[poci] = iterPic->m_poc;
			rps->deltaPOC[poci] = rps->poc[poci] - curPoc;
			(rps->deltaPOC[poci] < 0) ? numNeg++ : numPos++;
			rps->bUsed[poci] = !isRAP;
			poci++;
		}
		iterPic = iterPic->m_next;
	}

	rps->numberOfPictures = poci;
	rps->numberOfPositivePictures = numPos;
	rps->numberOfNegativePictures = numNeg;

	RPS_sortDeltaPOC(rps);*/
}
/** Function for applying picture marking based on the Reference Picture Set */
void DPB_applyReferencePictureSet(DPB *dpb, RPS *rps, int curPoc)
{/*
	// loop through all pictures in the reference picture buffer
	Frame* iterFrame = first(&dpb->m_picList);
	while (iterFrame)
	{
		if (iterFrame->m_poc != curPoc && iterFrame->m_encData->m_bHasReferences)
		{
			// loop through all pictures in the Reference Picture Set
			// to see if the picture should be kept as reference picture
			bool referenced = FALSE;
			for (int i = 0; i < rps->numberOfPositivePictures + rps->numberOfNegativePictures; i++)
			{
				if (iterFrame->m_poc == curPoc + rps->deltaPOC[i])
				{
					referenced = TRUE;
					break;
				}
			}
			if (!referenced)
				iterFrame->m_encData->m_bHasReferences = FALSE;
		}
		iterFrame = iterFrame->m_next;
	}*/
}
// move unreferenced pictures from picList to freeList for recycle

void DPB_recycleUnreferenced(DPB* dpb)
{/*
	Frame *iterFrame = first(&dpb->m_picList);

	while (iterFrame)
	{
		Frame *curFrame = iterFrame;
		iterFrame = iterFrame->m_next;
		if (!curFrame->m_encData->m_bHasReferences && !curFrame->m_countRefEncoders)
		{
			curFrame->m_bChromaExtended = FALSE;

			// iterator is invalidated by remove, restart scan
			PicList_remove(&dpb->m_picList, curFrame);
			iterFrame = first(&dpb->m_picList);

			PicList_pushBack(&dpb->m_freeList, curFrame);
			curFrame->m_encData->m_freeListNext = dpb->m_picSymFreeList;
			dpb->m_picSymFreeList = curFrame->m_encData;
			curFrame->m_encData = NULL;
			curFrame->m_reconPic = NULL;
		}
	}*/
}
