/*
* entropy.c
*
*  Created on: 2015-10-23
*      Author: 汪辉
*/
#include "Bitstream.h"
#include "contexts.h"
#include "constants.h"
#include "sao.h"
#include "scalingList.h"
#include "x265.h"
#include "primitives.h"
#include "dct.h"
#include "quant.h"
#include "framedata.h"
#include "stdio.h"
#include <math.h>
#define CU_DQP_TU_CMAX 5 // max number bins for truncated unary
#define CU_DQP_EG_k    0 // exp-golomb order
#define START_VALUE    8 // start value for dpcm mode

uint32_t g_puOffset[8] = { 0, 8, 4, 4, 2, 10, 1, 5 };
void Entropy_entropy(Entropy* entropy)
{
	markValid(entropy);
	entropy->m_fracBits = 0;
	entropy->m_pad = 0;
	X265_CHECK(sizeof(entropy->m_contextState) >= sizeof(entropy->m_contextState[0]) * MAX_OFF_CTX_MOD, "context state table is too small\n");

}
void codeProfileTier(Entropy* entropy, struct ProfileTierLevel* ptl, int maxTempSubLayers)
{/*
	int j;
	WRITE_CODE(entropy->syn.m_bitIf, 0, 2, "XXX_profile_space[]");
	WRITE_FLAG(entropy->syn.m_bitIf, ptl->tierFlag, "XXX_tier_flag[]");
	WRITE_CODE(entropy->syn.m_bitIf, ptl->profileIdc, 5, "XXX_profile_idc[]");
	for (j = 0; j < 32; j++)
		WRITE_FLAG(entropy->syn.m_bitIf, ptl->profileCompatibilityFlag[j], "XXX_profile_compatibility_flag[][j]");

	WRITE_FLAG(entropy->syn.m_bitIf, ptl->progressiveSourceFlag, "general_progressive_source_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, ptl->interlacedSourceFlag, "general_interlaced_source_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, ptl->nonPackedConstraintFlag, "general_non_packed_constraint_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, ptl->frameOnlyConstraintFlag, "general_frame_only_constraint_flag");

	if (ptl->profileIdc == MAINREXT || ptl->profileIdc == HIGHTHROUGHPUTREXT)
	{
		uint32_t bitDepthConstraint = ptl->bitDepthConstraint;
		int csp = ptl->chromaFormatConstraint;
		WRITE_FLAG(entropy->syn.m_bitIf, bitDepthConstraint <= 12, "general_max_12bit_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, bitDepthConstraint <= 10, "general_max_10bit_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, bitDepthConstraint <= 8 && csp != X265_CSP_I422, "general_max_8bit_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, csp == X265_CSP_I422 || csp == X265_CSP_I420 || csp == X265_CSP_I400, "general_max_422chroma_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, csp == X265_CSP_I420 || csp == X265_CSP_I400, "general_max_420chroma_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, csp == X265_CSP_I400, "general_max_monochrome_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, ptl->intraConstraintFlag, "general_intra_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, 0, "general_one_picture_only_constraint_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, ptl->lowerBitRateConstraintFlag, "general_lower_bit_rate_constraint_flag");
		WRITE_CODE(entropy->syn.m_bitIf, 0, 16, "XXX_reserved_zero_35bits[0..15]");
		WRITE_CODE(entropy->syn.m_bitIf, 0, 16, "XXX_reserved_zero_35bits[16..31]");
		WRITE_CODE(entropy->syn.m_bitIf, 0, 3, "XXX_reserved_zero_35bits[32..34]");
	}
	else
	{
		WRITE_CODE(entropy->syn.m_bitIf, 0, 16, "XXX_reserved_zero_44bits[0..15]");
		WRITE_CODE(entropy->syn.m_bitIf, 0, 16, "XXX_reserved_zero_44bits[16..31]");
		WRITE_CODE(entropy->syn.m_bitIf, 0, 12, "XXX_reserved_zero_44bits[32..43]");
	}

	WRITE_CODE(entropy->syn.m_bitIf, ptl->levelIdc, 8, "general_level_idc");

	if (maxTempSubLayers > 1)
	{
		int i;
		WRITE_FLAG(entropy->syn.m_bitIf, 0, "sub_layer_profile_present_flag[i]");
		WRITE_FLAG(entropy->syn.m_bitIf, 0, "sub_layer_level_present_flag[i]");
		for (i = maxTempSubLayers - 1; i < 8; i++)
			WRITE_CODE(entropy->syn.m_bitIf, 0, 2, "reserved_zero_2bits");
	}*/
}
void codeVPS(Entropy* entropy, struct VPS* vps)
{/*
	uint32_t i;
	WRITE_CODE(entropy->syn.m_bitIf, 0, 4, "vps_video_parameter_set_id");
	WRITE_CODE(entropy->syn.m_bitIf, 3, 2, "vps_reserved_three_2bits");
	WRITE_CODE(entropy->syn.m_bitIf, 0, 6, "vps_reserved_zero_6bits");
	WRITE_CODE(entropy->syn.m_bitIf, vps->maxTempSubLayers - 1, 3, "vps_max_sub_layers_minus1");
	WRITE_FLAG(entropy->syn.m_bitIf, vps->maxTempSubLayers == 1, "vps_temporal_id_nesting_flag");
	WRITE_CODE(entropy->syn.m_bitIf, 0xffff, 16, "vps_reserved_ffff_16bits");

	codeProfileTier(entropy, &vps->ptl, vps->maxTempSubLayers);

	WRITE_FLAG(entropy->syn.m_bitIf, 1, "vps_sub_layer_ordering_info_present_flag");

	for (i = 0; i < vps->maxTempSubLayers; i++)
	{
		WRITE_UVLC(entropy->syn.m_bitIf, vps->maxDecPicBuffering - 1, "vps_max_dec_pic_buffering_minus1[i]");
		WRITE_UVLC(entropy->syn.m_bitIf, vps->numReorderPics, "vps_num_reorder_pics[i]");
		WRITE_UVLC(entropy->syn.m_bitIf, vps->maxLatencyIncrease + 1, "vps_max_latency_increase_plus1[i]");
	}

	WRITE_CODE(entropy->syn.m_bitIf, 0, 6, "vps_max_nuh_reserved_zero_layer_id");
	WRITE_UVLC(entropy->syn.m_bitIf, 0, "vps_max_op_sets_minus1");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "vps_timing_info_present_flag"); // we signal timing info in SPS-VUI //
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "vps_extension_flag"); */
}
void codeScalingLists(Entropy* entropy, struct ScalingList* scalingList, uint32_t sizeId, uint32_t listId)
{/*
	int i;
	int coefNum = X265_MIN(MAX_MATRIX_COEF_NUM, (int)scalingList->s_numCoefPerSize[sizeId]);
	const uint16_t* scan = (sizeId == 0 ? g_scan4x4[SCAN_DIAG] : g_scan8x8diag);
	int nextCoef = START_VALUE;
	int32_t *src = scalingList->m_scalingListCoef[sizeId][listId];
	int data;

	if (sizeId > BLOCK_8x8)
	{
		WRITE_SVLC(entropy->syn.m_bitIf, scalingList->m_scalingListDC[sizeId][listId] - 8, "scaling_list_dc_coef_minus8");
		nextCoef = scalingList->m_scalingListDC[sizeId][listId];
	}
	for (i = 0; i < coefNum; i++)
	{
		data = src[scan[i]] - nextCoef;
		nextCoef = src[scan[i]];
		if (data > 127)
			data = data - 256;
		if (data < -128)
			data = data + 256;

		WRITE_SVLC(entropy->syn.m_bitIf, data, "scaling_list_delta_coef");
	}*/
}
void codeScalingListss(Entropy* entropy, struct ScalingList* scalingList)
{/*
	int sizeId, listId;
	for (sizeId = 0; sizeId < NUM_SIZES1; sizeId++)
	{
		for (listId = 0; listId <NUM_LISTS; listId++)
		{
			int predList = checkPredMode(scalingList, sizeId, listId);
			WRITE_FLAG(entropy->syn.m_bitIf, predList < 0, "scaling_list_pred_mode_flag");
			if (predList >= 0)
				WRITE_UVLC(entropy->syn.m_bitIf, listId - predList, "scaling_list_pred_matrix_id_delta");
			else // DPCM Mode
				codeScalingLists(entropy, scalingList, sizeId, listId);
		}
	}*/
}
void codeHrdParameters(Entropy* entropy, struct HRDInfo* hrd, int maxSubTLayers)
{/*
	int i;
	WRITE_FLAG(entropy->syn.m_bitIf, 1, "nal_hrd_parameters_present_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "vcl_hrd_parameters_present_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "sub_pic_hrd_params_present_flag");

	WRITE_CODE(entropy->syn.m_bitIf, hrd->bitRateScale, 4, "bit_rate_scale");
	WRITE_CODE(entropy->syn.m_bitIf, hrd->cpbSizeScale, 4, "cpb_size_scale");

	WRITE_CODE(entropy->syn.m_bitIf, hrd->initialCpbRemovalDelayLength - 1, 5, "initial_cpb_removal_delay_length_minus1");
	WRITE_CODE(entropy->syn.m_bitIf, hrd->cpbRemovalDelayLength - 1, 5, "au_cpb_removal_delay_length_minus1");
	WRITE_CODE(entropy->syn.m_bitIf, hrd->dpbOutputDelayLength - 1, 5, "dpb_output_delay_length_minus1");

	for (i = 0; i < maxSubTLayers; i++)
	{
		WRITE_FLAG(entropy->syn.m_bitIf, 1, "fixed_pic_rate_general_flag");
		WRITE_UVLC(entropy->syn.m_bitIf, 0, "elemental_duration_in_tc_minus1");
		WRITE_UVLC(entropy->syn.m_bitIf, 0, "cpb_cnt_minus1");

		WRITE_UVLC(entropy->syn.m_bitIf, hrd->bitRateValue - 1, "bit_rate_value_minus1");
		WRITE_UVLC(entropy->syn.m_bitIf, hrd->cpbSizeValue - 1, "cpb_size_value_minus1");
		WRITE_FLAG(entropy->syn.m_bitIf, hrd->cbrFlag, "cbr_flag");
	}*/
}
void codeVUI(Entropy* entropy, struct VUI* vui, int maxSubTLayers)
{/*
	WRITE_FLAG(entropy->syn.m_bitIf, vui->aspectRatioInfoPresentFlag, "aspect_ratio_info_present_flag");
	if (vui->aspectRatioInfoPresentFlag)
	{
		WRITE_CODE(entropy->syn.m_bitIf, vui->aspectRatioIdc, 8, "aspect_ratio_idc");
		if (vui->aspectRatioIdc == 255)
		{
			WRITE_CODE(entropy->syn.m_bitIf, vui->sarWidth, 16, "sar_width");
			WRITE_CODE(entropy->syn.m_bitIf, vui->sarHeight, 16, "sar_height");
		}
	}

	WRITE_FLAG(entropy->syn.m_bitIf, vui->overscanInfoPresentFlag, "overscan_info_present_flag");
	if (vui->overscanInfoPresentFlag)
		WRITE_FLAG(entropy->syn.m_bitIf, vui->overscanAppropriateFlag, "overscan_appropriate_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, vui->videoSignalTypePresentFlag, "video_signal_type_present_flag");
	if (vui->videoSignalTypePresentFlag)
	{
		WRITE_CODE(entropy->syn.m_bitIf, vui->videoFormat, 3, "video_format");
		WRITE_FLAG(entropy->syn.m_bitIf, vui->videoFullRangeFlag, "video_full_range_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, vui->colourDescriptionPresentFlag, "colour_description_present_flag");
		if (vui->colourDescriptionPresentFlag)
		{
			WRITE_CODE(entropy->syn.m_bitIf, vui->colourPrimaries, 8, "colour_primaries");
			WRITE_CODE(entropy->syn.m_bitIf, vui->transferCharacteristics, 8, "transfer_characteristics");
			WRITE_CODE(entropy->syn.m_bitIf, vui->matrixCoefficients, 8, "matrix_coefficients");
		}
	}

	WRITE_FLAG(entropy->syn.m_bitIf, vui->chromaLocInfoPresentFlag, "chroma_loc_info_present_flag");
	if (vui->chromaLocInfoPresentFlag)
	{
		WRITE_UVLC(entropy->syn.m_bitIf, vui->chromaSampleLocTypeTopField, "chroma_sample_loc_type_top_field");
		WRITE_UVLC(entropy->syn.m_bitIf, vui->chromaSampleLocTypeBottomField, "chroma_sample_loc_type_bottom_field");
	}

	WRITE_FLAG(entropy->syn.m_bitIf, 0, "neutral_chroma_indication_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, vui->fieldSeqFlag, "field_seq_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, vui->frameFieldInfoPresentFlag, "frame_field_info_present_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, vui->defaultDisplayWindow.bEnabled, "default_display_window_flag");
	if (vui->defaultDisplayWindow.bEnabled)
	{
		WRITE_UVLC(entropy->syn.m_bitIf, vui->defaultDisplayWindow.leftOffset, "def_disp_win_left_offset");
		WRITE_UVLC(entropy->syn.m_bitIf, vui->defaultDisplayWindow.rightOffset, "def_disp_win_right_offset");
		WRITE_UVLC(entropy->syn.m_bitIf, vui->defaultDisplayWindow.topOffset, "def_disp_win_top_offset");
		WRITE_UVLC(entropy->syn.m_bitIf, vui->defaultDisplayWindow.bottomOffset, "def_disp_win_bottom_offset");
	}

	WRITE_FLAG(entropy->syn.m_bitIf, 1, "vui_timing_info_present_flag");
	WRITE_CODE(entropy->syn.m_bitIf, vui->timingInfo.numUnitsInTick, 32, "vui_num_units_in_tick");
	WRITE_CODE(entropy->syn.m_bitIf, vui->timingInfo.timeScale, 32, "vui_time_scale");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "vui_poc_proportional_to_timing_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, vui->hrdParametersPresentFlag, "vui_hrd_parameters_present_flag");
	if (vui->hrdParametersPresentFlag)
		codeHrdParameters(entropy, &vui->hrdParameters, maxSubTLayers);

	WRITE_FLAG(entropy->syn.m_bitIf, 0, "bitstream_restriction_flag");*/
}
void codeSPS(Entropy* entropy, struct SPS* sps, struct ScalingList* scalingList, struct ProfileTierLevel* ptl)
{/*
	uint32_t i;
	WRITE_CODE(entropy->syn.m_bitIf, 0, 4, "sps_video_parameter_set_id");
	WRITE_CODE(entropy->syn.m_bitIf, sps->maxTempSubLayers - 1, 3, "sps_max_sub_layers_minus1");
	WRITE_FLAG(entropy->syn.m_bitIf, sps->maxTempSubLayers == 1, "sps_temporal_id_nesting_flag");

	codeProfileTier(entropy, ptl, sps->maxTempSubLayers);

	WRITE_UVLC(entropy->syn.m_bitIf, 0, "sps_seq_parameter_set_id");
	WRITE_UVLC(entropy->syn.m_bitIf, sps->chromaFormatIdc, "chroma_format_idc");

	if (sps->chromaFormatIdc == X265_CSP_I444)
		WRITE_FLAG(entropy->syn.m_bitIf, 0, "separate_colour_plane_flag");

	WRITE_UVLC(entropy->syn.m_bitIf, sps->picWidthInLumaSamples, "pic_width_in_luma_samples");
	WRITE_UVLC(entropy->syn.m_bitIf, sps->picHeightInLumaSamples, "pic_height_in_luma_samples");

	struct Window* conf = &sps->conformanceWindow;
	WRITE_FLAG(entropy->syn.m_bitIf, conf->bEnabled, "conformance_window_flag");
	if (conf->bEnabled)
	{
		int hShift = CHROMA_H_SHIFT(sps->chromaFormatIdc), vShift = CHROMA_V_SHIFT(sps->chromaFormatIdc);
		WRITE_UVLC(entropy->syn.m_bitIf, conf->leftOffset >> hShift, "conf_win_left_offset");
		WRITE_UVLC(entropy->syn.m_bitIf, conf->rightOffset >> hShift, "conf_win_right_offset");
		WRITE_UVLC(entropy->syn.m_bitIf, conf->topOffset >> vShift, "conf_win_top_offset");
		WRITE_UVLC(entropy->syn.m_bitIf, conf->bottomOffset >> vShift, "conf_win_bottom_offset");
	}

	WRITE_UVLC(entropy->syn.m_bitIf, X265_DEPTH - 8, "bit_depth_luma_minus8");
	WRITE_UVLC(entropy->syn.m_bitIf, X265_DEPTH - 8, "bit_depth_chroma_minus8");
	WRITE_UVLC(entropy->syn.m_bitIf, BITS_FOR_POC - 4, "log2_max_pic_order_cnt_lsb_minus4");
	WRITE_FLAG(entropy->syn.m_bitIf, 1, "sps_sub_layer_ordering_info_present_flag");

	for (i = 0; i < sps->maxTempSubLayers; i++)
	{
		WRITE_UVLC(entropy->syn.m_bitIf, sps->maxDecPicBuffering - 1, "sps_max_dec_pic_buffering_minus1[i]");
		WRITE_UVLC(entropy->syn.m_bitIf, sps->numReorderPics, "sps_num_reorder_pics[i]");
		WRITE_UVLC(entropy->syn.m_bitIf, sps->maxLatencyIncrease + 1, "sps_max_latency_increase_plus1[i]");
	}

	WRITE_UVLC(entropy->syn.m_bitIf, sps->log2MinCodingBlockSize - 3, "log2_min_coding_block_size_minus3");
	WRITE_UVLC(entropy->syn.m_bitIf, sps->log2DiffMaxMinCodingBlockSize, "log2_diff_max_min_coding_block_size");
	WRITE_UVLC(entropy->syn.m_bitIf, sps->quadtreeTULog2MinSize - 2, "log2_min_transform_block_size_minus2");
	WRITE_UVLC(entropy->syn.m_bitIf, sps->quadtreeTULog2MaxSize - sps->quadtreeTULog2MinSize, "log2_diff_max_min_transform_block_size");
	WRITE_UVLC(entropy->syn.m_bitIf, sps->quadtreeTUMaxDepthInter - 1, "max_transform_hierarchy_depth_inter");
	WRITE_UVLC(entropy->syn.m_bitIf, sps->quadtreeTUMaxDepthIntra - 1, "max_transform_hierarchy_depth_intra");
	WRITE_FLAG(entropy->syn.m_bitIf, scalingList->m_bEnabled, "scaling_list_enabled_flag");
	if (scalingList->m_bEnabled)
	{
		WRITE_FLAG(entropy->syn.m_bitIf, scalingList->m_bDataPresent, "sps_scaling_list_data_present_flag");
		if (scalingList->m_bDataPresent)
			codeScalingListss(entropy, scalingList);
	}
	WRITE_FLAG(entropy->syn.m_bitIf, sps->bUseAMP, "amp_enabled_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, sps->bUseSAO, "sample_adaptive_offset_enabled_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, 0, "pcm_enabled_flag");
	WRITE_UVLC(entropy->syn.m_bitIf, 0, "num_short_term_ref_pic_sets");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "long_term_ref_pics_present_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, sps->bTemporalMVPEnabled, "sps_temporal_mvp_enable_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, sps->bUseStrongIntraSmoothing, "sps_strong_intra_smoothing_enable_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, 1, "vui_parameters_present_flag");
	codeVUI(entropy, &sps->vuiParameters, sps->maxTempSubLayers);

	WRITE_FLAG(entropy->syn.m_bitIf, 0, "sps_extension_flag");*/
}

uint32_t bitsCodeBin(const Entropy* entroy, uint32_t binValue, uint32_t ctxModel)
{/*
	uint64_t fracBits = (entroy->m_fracBits & 32767) + sbacGetEntropyBits(ctxModel, binValue);
	return (uint32_t)(fracBits >> 15);*/return 0;
}

void codeShortTermRefPicSet(Entropy* entropy, const struct RPS* rps)
{/*
	WRITE_UVLC(entropy->syn.m_bitIf, rps->numberOfNegativePictures, "num_negative_pics");
	WRITE_UVLC(entropy->syn.m_bitIf, rps->numberOfPositivePictures, "num_positive_pics");
	int prev = 0;
	int j;
	for (j = 0; j < rps->numberOfNegativePictures; j++)
	{
		WRITE_UVLC(entropy->syn.m_bitIf, prev - rps->deltaPOC[j] - 1, "delta_poc_s0_minus1");
		prev = rps->deltaPOC[j];
		WRITE_FLAG(entropy->syn.m_bitIf, rps->bUsed[j], "used_by_curr_pic_s0_flag");
	}

	prev = 0;
	for (j = rps->numberOfNegativePictures; j < rps->numberOfNegativePictures + rps->numberOfPositivePictures; j++)
	{
		WRITE_UVLC(entropy->syn.m_bitIf, rps->deltaPOC[j] - prev - 1, "delta_poc_s1_minus1");
		prev = rps->deltaPOC[j];
		WRITE_FLAG(entropy->syn.m_bitIf, rps->bUsed[j], "used_by_curr_pic_s1_flag");
	}*/
}
void codePredWeightTable(Entropy* entropy, struct Slice*slice)
{/*
	int list, ref, plane;
	struct  WeightParam *wp=NULL;
	char           bChroma = 1; // 4:0:0 not yet supported
	char            bDenomCoded = 0;
	int             numRefDirs = slice->m_sliceType == B_SLICE ? 2 : 1;
	uint32_t        totalSignalledWeightFlags = 0;

	if ((slice->m_sliceType == P_SLICE && slice->m_pps->bUseWeightPred) ||
		(slice->m_sliceType == B_SLICE && slice->m_pps->bUseWeightedBiPred))
	{
		for (list = 0; list < numRefDirs; list++)
		{
			for (ref = 0; ref < slice->m_numRefIdx[list]; ref++)
			{
				wp = slice->m_weightPredTable[list][ref];
				if (!bDenomCoded)
				{
					WRITE_UVLC(entropy->syn.m_bitIf, wp[0].log2WeightDenom, "luma_log2_weight_denom");

					if (bChroma)
					{
						int deltaDenom = wp[1].log2WeightDenom - wp[0].log2WeightDenom;
						WRITE_SVLC(entropy->syn.m_bitIf, deltaDenom, "delta_chroma_log2_weight_denom");
					}
					bDenomCoded = 1;
				}
				WRITE_FLAG(entropy->syn.m_bitIf, wp[0].bPresentFlag, "luma_weight_lX_flag");
				totalSignalledWeightFlags += wp[0].bPresentFlag;
			}

			if (bChroma)
			{
				for (ref = 0; ref < slice->m_numRefIdx[list]; ref++)
				{
					wp = slice->m_weightPredTable[list][ref];
					WRITE_FLAG(entropy->syn.m_bitIf, wp[1].bPresentFlag, "chroma_weight_lX_flag");
					totalSignalledWeightFlags += 2 * wp[1].bPresentFlag;
				}
			}

			for (ref = 0; ref < slice->m_numRefIdx[list]; ref++)
			{

				if (wp[0].bPresentFlag)
				{
					int deltaWeight = (wp[0].inputWeight - (1 << wp[0].log2WeightDenom));
					WRITE_SVLC(entropy->syn.m_bitIf, deltaWeight, "delta_luma_weight_lX");
					WRITE_SVLC(entropy->syn.m_bitIf, wp[0].inputOffset, "luma_offset_lX");
				}

				if (bChroma)
				{
					if (wp[1].bPresentFlag)
					{
						for (plane = 1; plane < 3; plane++)
						{
							int deltaWeight = (wp[plane].inputWeight - (1 << wp[1].log2WeightDenom));
							WRITE_SVLC(entropy->syn.m_bitIf, deltaWeight, "delta_chroma_weight_lX");

							int pred = (128 - ((128 * wp[plane].inputWeight) >> (wp[plane].log2WeightDenom)));
							int deltaChroma = (wp[plane].inputOffset - pred);
							WRITE_SVLC(entropy->syn.m_bitIf, deltaChroma, "delta_chroma_offset_lX");
						}
					}
				}
			}
		}

		if (totalSignalledWeightFlags > 24)
		{
			printf("total weights must be <= 24\n");
			return;
		}
	}*/
}
void codeSliceHeader(Entropy* entropy, Slice* slice /*struct FrameData* encData*/)
{/*
	WRITE_FLAG(entropy->syn.m_bitIf, 1, "first_slice_segment_in_pic_flag");
	if (getRapPicFlag(slice))
		WRITE_FLAG(entropy->syn.m_bitIf, 0, "no_output_of_prior_pics_flag");

	WRITE_UVLC(entropy->syn.m_bitIf, 0, "slice_pic_parameter_set_id");

	// x265 does not use dependent slices, so always write all this data //
	WRITE_UVLC(entropy->syn.m_bitIf, slice->m_sliceType, "slice_type");

	if (!getIdrPicFlag(slice))
	{
		int picOrderCntLSB = (slice->m_poc - slice->m_lastIDR + (1 << BITS_FOR_POC)) % (1 << BITS_FOR_POC);
		WRITE_CODE(entropy->syn.m_bitIf, picOrderCntLSB, BITS_FOR_POC, "pic_order_cnt_lsb");

#if _DEBUG || CHECKED_BUILD
		// check for bitstream restriction stating that:
		// If the current picture is a BLA or CRA picture, the value of NumPocTotalCurr shall be equal to 0.
		// Ideally this process should not be repeated for each slice in a picture
		if (isIRAP(slice))
			for (int picIdx = 0; picIdx < slice->m_rps.numberOfPictures; picIdx++)
			{
				X265_CHECK(!slice->m_rps.bUsed[picIdx], "pic unused failure\n");
			}
#endif

		WRITE_FLAG(entropy->syn.m_bitIf, 0, "short_term_ref_pic_set_sps_flag");
		codeShortTermRefPicSet(entropy, &slice->m_rps);

		if (slice->m_sps->bTemporalMVPEnabled)
			WRITE_FLAG(entropy->syn.m_bitIf, 1, "slice_temporal_mvp_enable_flag");
	}

	// check if numRefIdx match the defaults (1, hard-coded in PPS). If not, override
	// TODO: this might be a place to optimize a few bits per slice, by using param->refs for L0 default

	if (!Slice_isIntra(slice))
	{
		char overrideFlag = (slice->m_numRefIdx[0] != 1 || (isInterB(slice) && slice->m_numRefIdx[1] != 1));
		WRITE_FLAG(entropy->syn.m_bitIf, overrideFlag, "num_ref_idx_active_override_flag");
		if (overrideFlag)
		{
			WRITE_UVLC(entropy->syn.m_bitIf, slice->m_numRefIdx[0] - 1, "num_ref_idx_l0_active_minus1");
			if (isInterB(slice))
				WRITE_UVLC(entropy->syn.m_bitIf, slice->m_numRefIdx[1] - 1, "num_ref_idx_l1_active_minus1");
			else
			{
				if (slice->m_numRefIdx[1] == 0)
					printf("expected no L1 references for P slice\n");
			}
		}
	}
	else
	{
		if (!slice->m_numRefIdx[0] && !slice->m_numRefIdx[1])
			printf("expected no references for I slice\n");
	}

	if (isInterB(slice))
		WRITE_FLAG(entropy->syn.m_bitIf, 0, "mvd_l1_zero_flag");

	if (slice->m_sps->bTemporalMVPEnabled)
	{
		if (slice->m_sliceType == B_SLICE)
			WRITE_FLAG(entropy->syn.m_bitIf, slice->m_colFromL0Flag, "collocated_from_l0_flag");

		if (slice->m_sliceType != I_SLICE &&
			((slice->m_colFromL0Flag && slice->m_numRefIdx[0] > 1) ||
			(!slice->m_colFromL0Flag && slice->m_numRefIdx[1] > 1)))
		{
			WRITE_UVLC(entropy->syn.m_bitIf, slice->m_colRefIdx, "collocated_ref_idx");
		}
	}
	if ((slice->m_pps->bUseWeightPred && slice->m_sliceType == P_SLICE) || (slice->m_pps->bUseWeightedBiPred && slice->m_sliceType == B_SLICE))

		codePredWeightTable(entropy, slice);

	if (slice->m_maxNumMergeCand > MRG_MAX_NUM_CANDS)
	{
		printf("too many merge candidates\n");
		return;
	}
	if (!Slice_isIntra(slice))
		WRITE_UVLC(entropy->syn.m_bitIf, MRG_MAX_NUM_CANDS - slice->m_maxNumMergeCand, "five_minus_max_num_merge_cand");

	int code = slice->m_sliceQp - 26;
	WRITE_SVLC(entropy->syn.m_bitIf, code, "slice_qp_delta");

	char isSAOEnabled = 0;//slice->m_sps->bUseSAO ? saoParam->bSaoFlag[0] || saoParam->bSaoFlag[1] : 0;
	char isDBFEnabled = !slice->m_pps->bPicDisableDeblockingFilter;

	if (isSAOEnabled || isDBFEnabled)
		WRITE_FLAG(entropy->syn.m_bitIf, slice->m_sLFaseFlag, "slice_loop_filter_across_slices_enabled_flag");
*/
}
void  estCBFBit(const Entropy* entropy, struct EstBitsSbac* estBitsSbac)
{/*
	uint32_t ctxInc;

	const uint8_t *ctx = &(entropy->m_contextState[OFF_QT_CBF_CTX]);

	for (ctxInc = 0; ctxInc < NUM_QT_CBF_CTX; ctxInc++)
	{
		estBitsSbac->blockCbpBits[ctxInc][0] = sbacGetEntropyBits(ctx[ctxInc], 0);
		estBitsSbac->blockCbpBits[ctxInc][1] = sbacGetEntropyBits(ctx[ctxInc], 1);
	}

	ctx = &entropy->m_contextState[OFF_QT_ROOT_CBF_CTX];

	estBitsSbac->blockRootCbpBits[0] = sbacGetEntropyBits(ctx[0], 0);
	estBitsSbac->blockRootCbpBits[1] = sbacGetEntropyBits(ctx[0], 1);*/
}
void  estSignificantCoeffGroupMapBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, char bIsLuma)
{/*
	int firstCtx = 0, numCtx = NUM_SIG_CG_FLAG_CTX, ctxIdx;
	uint32_t bin;
	for (ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
		for (bin = 0; bin < 2; bin++)
			estBitsSbac->significantCoeffGroupBits[ctxIdx][bin] = sbacGetEntropyBits(entroy->m_contextState[OFF_SIG_CG_FLAG_CTX + ((bIsLuma ? 0 : NUM_SIG_CG_FLAG_CTX) + ctxIdx)], bin);
*/
}

void estSignificantMapBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, uint32_t log2TrSize, char bIsLuma)
{/*
	int firstCtx = 1, numCtx = 8, ctxIdx, i;
	uint32_t bin;
	if (log2TrSize >= 4)
	{
		firstCtx = bIsLuma ? 21 : 12;
		numCtx = bIsLuma ? 6 : 3;
	}
	else if (log2TrSize == 3)
	{
		firstCtx = 9;
		numCtx = bIsLuma ? 12 : 3;
	}

	if (bIsLuma)
	{
		for (bin = 0; bin < 2; bin++)
			estBitsSbac->significantBits[bin][0] = sbacGetEntropyBits(entroy->m_contextState[OFF_SIG_FLAG_CTX], bin);

		for (ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
			for (bin = 0; bin < 2; bin++)
				estBitsSbac->significantBits[bin][ctxIdx] = sbacGetEntropyBits(entroy->m_contextState[OFF_SIG_FLAG_CTX + ctxIdx], bin);
	}
	else
	{
		for (bin = 0; bin < 2; bin++)
			estBitsSbac->significantBits[bin][0] = sbacGetEntropyBits(entroy->m_contextState[OFF_SIG_FLAG_CTX + (NUM_SIG_FLAG_CTX_LUMA + 0)], bin);

		for (ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++)
			for (bin = 0; bin < 2; bin++)
				estBitsSbac->significantBits[bin][ctxIdx] = sbacGetEntropyBits(entroy->m_contextState[OFF_SIG_FLAG_CTX + (NUM_SIG_FLAG_CTX_LUMA + ctxIdx)], bin);
	}

	int blkSizeOffset = bIsLuma ? ((log2TrSize - 2) * 3 + ((log2TrSize - 1) >> 2)) : NUM_CTX_LAST_FLAG_XY_LUMA;
	int ctxShift = bIsLuma ? ((log2TrSize + 1) >> 2) : log2TrSize - 2;
	uint32_t maxGroupIdx = log2TrSize * 2 - 1;

	uint32_t ctx;
	for (i = 0, ctxIdx = 0; i < 2; i++, ctxIdx += NUM_CTX_LAST_FLAG_XY)
	{
		int bits = 0;
		const uint8_t *ctxState = &entroy->m_contextState[OFF_CTX_LAST_FLAG_X + ctxIdx];

		for (ctx = 0; ctx < maxGroupIdx; ctx++)
		{
			int ctxOffset = blkSizeOffset + (ctx >> ctxShift);
			estBitsSbac->lastBits[i][ctx] = bits + sbacGetEntropyBits(ctxState[ctxOffset], 0);
			bits += sbacGetEntropyBits(ctxState[ctxOffset], 1);
		}

		estBitsSbac->lastBits[i][ctx] = bits;
	}*/
}

void estSignificantCoefficientsBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, char bIsLuma)
{/*
	int ctxIdx;
	if (bIsLuma)
	{
		const uint8_t *ctxOne = &entroy->m_contextState[OFF_ONE_FLAG_CTX];
		const uint8_t *ctxAbs = &entroy->m_contextState[OFF_ABS_FLAG_CTX];

		for (ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_LUMA; ctxIdx++)
		{
			estBitsSbac->greaterOneBits[ctxIdx][0] = sbacGetEntropyBits(ctxOne[ctxIdx], 0);
			estBitsSbac->greaterOneBits[ctxIdx][1] = sbacGetEntropyBits(ctxOne[ctxIdx], 1);
		}

		for (ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_LUMA; ctxIdx++)
		{
			estBitsSbac->levelAbsBits[ctxIdx][0] = sbacGetEntropyBits(ctxAbs[ctxIdx], 0);
			estBitsSbac->levelAbsBits[ctxIdx][1] = sbacGetEntropyBits(ctxAbs[ctxIdx], 1);
		}
	}
	else
	{
		const uint8_t *ctxOne = &entroy->m_contextState[OFF_ONE_FLAG_CTX + NUM_ONE_FLAG_CTX_LUMA];
		const uint8_t *ctxAbs = &entroy->m_contextState[OFF_ABS_FLAG_CTX + NUM_ABS_FLAG_CTX_LUMA];

		for (ctxIdx = 0; ctxIdx < NUM_ONE_FLAG_CTX_CHROMA; ctxIdx++)
		{
			estBitsSbac->greaterOneBits[ctxIdx][0] = sbacGetEntropyBits(ctxOne[ctxIdx], 0);
			estBitsSbac->greaterOneBits[ctxIdx][1] = sbacGetEntropyBits(ctxOne[ctxIdx], 1);
		}

		for (ctxIdx = 0; ctxIdx < NUM_ABS_FLAG_CTX_CHROMA; ctxIdx++)
		{
			estBitsSbac->levelAbsBits[ctxIdx][0] = sbacGetEntropyBits(ctxAbs[ctxIdx], 0);
			estBitsSbac->levelAbsBits[ctxIdx][1] = sbacGetEntropyBits(ctxAbs[ctxIdx], 1);
		}
	}*/
}

void estBit(const Entropy* entroy, struct EstBitsSbac* estBitsSbac, uint32_t log2TrSize, char bIsLuma)
{/*
	estCBFBit(entroy, estBitsSbac);

	estSignificantCoeffGroupMapBit(entroy, estBitsSbac, bIsLuma);

	// encode significance map
	estSignificantMapBit(entroy, estBitsSbac, log2TrSize, bIsLuma);

	// encode significant coefficients
	estSignificantCoefficientsBit(entroy, estBitsSbac, bIsLuma);*/
}

uint32_t bitsIntraModeNonMPM(const Entropy* entroy)  
{ /*
  return bitsCodeBin(entroy, 0, entroy->m_contextState[OFF_ADI_CTX]) + 5; */return 0;
}
uint32_t bitsIntraModeMPM(Entropy* entroy, const uint32_t preds[3], uint32_t dir)  
{ /*
  return bitsCodeBin(entroy, 1, entroy->m_contextState[OFF_ADI_CTX]) + (dir == preds[0] ? 1 : 2);*/ return 0;
}
uint32_t estimateCbfBits(Entropy* entroy, uint32_t cbf, enum TextType ttype, uint32_t tuDepth)  
{ /*
  return bitsCodeBin(entroy, cbf, entroy->m_contextState[OFF_QT_CBF_CTX + ctxCbf[ttype][tuDepth]]); */return 0;
}
uint32_t bitsIntraMode(Entropy* entroy, CUData* cu, uint32_t absPartIdx)
{/*
	return bitsCodeBin(entroy, 0, entroy->m_contextState[OFF_SKIP_FLAG_CTX + CUData_getCtxSkipFlag(cu, absPartIdx)]) + bitsCodeBin(entroy, 1, entroy->m_contextState[OFF_PRED_MODE_CTX]); // intra //
	*/return 0;
}

uint32_t bitsInterMode(Entropy* entroy, CUData* cu, uint32_t absPartIdx, uint32_t depth)
{/*
	uint32_t bits;
	bits = bitsCodeBin(entroy, 0, entroy->m_contextState[OFF_SKIP_FLAG_CTX + CUData_getCtxSkipFlag(cu, absPartIdx)]); // not skip //
	bits += bitsCodeBin(entroy, 0, entroy->m_contextState[OFF_PRED_MODE_CTX]); // inter //
	enum PartSize partSize = (PartSize)cu->m_partSize[absPartIdx];
	switch (partSize)
	{
	case SIZE_2Nx2N:
	bits += bitsCodeBin(entroy, 1, entroy->m_contextState[OFF_PART_SIZE_CTX]);
	break;

	case SIZE_2NxN:
	case SIZE_2NxnU:
	case SIZE_2NxnD:
	bits += bitsCodeBin(entroy, 0, entroy->m_contextState[OFF_PART_SIZE_CTX + 0]);
	bits += bitsCodeBin(entroy, 1, entroy->m_contextState[OFF_PART_SIZE_CTX + 1]);
	if (cu->m_slice->m_sps->maxAMPDepth > depth)
	{
	bits += bitsCodeBin(entroy, (partSize == SIZE_2NxN) ? 1 : 0, entroy->m_contextState[OFF_PART_SIZE_CTX + 3]);
	if (partSize != SIZE_2NxN)
	bits++; // encodeBinEP((partSize == SIZE_2NxnU ? 0 : 1));
	}
	break;

	case SIZE_Nx2N:
	case SIZE_nLx2N:
	case SIZE_nRx2N:
	bits += bitsCodeBin(entroy, 0, entroy->m_contextState[OFF_PART_SIZE_CTX + 0]);
	bits += bitsCodeBin(entroy, 0, entroy->m_contextState[OFF_PART_SIZE_CTX + 1]);
	if (depth == g_maxCUDepth && !(cu->m_log2CUSize[absPartIdx] == 3))
	bits += bitsCodeBin(entroy, 1, entroy->m_contextState[OFF_PART_SIZE_CTX + 2]);
	if (cu->m_slice->m_sps->maxAMPDepth > depth)
	{
	bits += bitsCodeBin(entroy, (partSize == SIZE_Nx2N) ? 1 : 0, entroy->m_contextState[OFF_PART_SIZE_CTX + 3]);
	if (partSize != SIZE_Nx2N)
	bits++; // encodeBinEP((partSize == SIZE_nLx2N ? 0 : 1));
	}
	break;
	default:
	if (0)
	printf("invalid CU partition\n");
	break;
	}
	return bits; */return 0;
}

void start(Entropy* entropy)
{
	entropy->m_low = 0;
	entropy->m_range = 510;
	entropy->m_bitsLeft = -12;
	entropy->m_numBufferedBytes = 0;
	entropy->m_bufferedByte = 0xff;
}

void copyState(Entropy* dst, const Entropy* other)
{/*
	dst->m_low = other->m_low;
	dst->m_range = other->m_range;
	dst->m_bitsLeft = other->m_bitsLeft;
	dst->m_bufferedByte = other->m_bufferedByte;
	dst->m_numBufferedBytes = other->m_numBufferedBytes;
	dst->m_fracBits = other->m_fracBits;*/
}

void markInvalid(Entropy* entropy)                 
{ /*
	entropy->m_valid = FALSE; */
}
void markValid(Entropy* entropy)                   
{
	entropy->m_valid = TRUE; 
}
void zeroFract(Entropy* entropy)                   
{/*
	entropy->m_fracBits = 0; */
}
void copyFrom(Entropy* dst, const Entropy* src)
{/*
	X265_CHECK(src->m_valid, "invalid copy source context\n");
	copyState(dst, src);
	memcpy(dst->m_contextState, src->m_contextState, MAX_OFF_CTX_MOD * sizeof(uint8_t));
	markValid(dst);*/
}

/* Initialize our context information from the nominated source */
void copyContextsFrom(Entropy* dst, const Entropy* src)
{/*
	X265_CHECK(src->m_valid, "invalid copy source context\n");

	memcpy(dst->m_contextState, src->m_contextState, MAX_OFF_CTX_MOD * sizeof(dst->m_contextState[0]));
	markValid(dst);*/
}
// SBAC RD
void loadIntraDirModeLuma(Entropy* dst, const Entropy* src)
{/*
	dst->m_fracBits = src->m_fracBits;
	dst->m_contextState[OFF_ADI_CTX] = src->m_contextState[OFF_ADI_CTX];*/
}
void load(Entropy* dst, const Entropy* src)            
{/*
	copyFrom(dst, src);*/ 
}
void store(const Entropy* src, Entropy* dst)          
{/*
	copyFrom(dst, src); */
}

uint8_t sbacInit(int qp, int initValue)
{/*
	qp = x265_clip3_int(QP_MIN, QP_MAX_SPEC, qp);

	int  slope = (initValue >> 4) * 5 - 45;
	int  offset = ((initValue & 15) << 3) - 16;
	int  initState = X265_MIN(X265_MAX(1, (((slope * qp) >> 4) + offset)), 126);
	uint32_t mpState = (initState >= 64);
	uint32_t state = ((mpState ? (initState - 64) : (63 - initState)) << 1) + mpState;

	return (state & 0xff);*/return 0;
}
void initBuffer(uint8_t* contextModel, enum SliceType sliceType, int qp, uint8_t* ctxModel, int size)
{/*
	int n;
	ctxModel += sliceType * size;
	for (n = 0; n < size; n++)
		contextModel[n] = sbacInit(qp, ctxModel[n]);*/
}
void resetEntropy(Entropy* entropy, Slice *slice)
{/*
	int  qp = slice->m_sliceQp;
	SliceType sliceType = slice->m_sliceType;

	initBuffer(&entropy->m_contextState[OFF_SPLIT_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SPLIT_FLAG, NUM_SPLIT_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_SKIP_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SKIP_FLAG, NUM_SKIP_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_MERGE_FLAG_EXT_CTX], sliceType, qp, (uint8_t*)INIT_MERGE_FLAG_EXT, NUM_MERGE_FLAG_EXT_CTX);
	initBuffer(&entropy->m_contextState[OFF_MERGE_IDX_EXT_CTX], sliceType, qp, (uint8_t*)INIT_MERGE_IDX_EXT, NUM_MERGE_IDX_EXT_CTX);
	initBuffer(&entropy->m_contextState[OFF_PART_SIZE_CTX], sliceType, qp, (uint8_t*)INIT_PART_SIZE, NUM_PART_SIZE_CTX);
	initBuffer(&entropy->m_contextState[OFF_PRED_MODE_CTX], sliceType, qp, (uint8_t*)INIT_PRED_MODE, NUM_PRED_MODE_CTX);
	initBuffer(&entropy->m_contextState[OFF_ADI_CTX], sliceType, qp, (uint8_t*)INIT_INTRA_PRED_MODE, NUM_ADI_CTX);
	initBuffer(&entropy->m_contextState[OFF_CHROMA_PRED_CTX], sliceType, qp, (uint8_t*)INIT_CHROMA_PRED_MODE, NUM_CHROMA_PRED_CTX);
	initBuffer(&entropy->m_contextState[OFF_DELTA_QP_CTX], sliceType, qp, (uint8_t*)INIT_DQP, NUM_DELTA_QP_CTX);
	initBuffer(&entropy->m_contextState[OFF_INTER_DIR_CTX], sliceType, qp, (uint8_t*)INIT_INTER_DIR, NUM_INTER_DIR_CTX);
	initBuffer(&entropy->m_contextState[OFF_REF_NO_CTX], sliceType, qp, (uint8_t*)INIT_REF_PIC, NUM_REF_NO_CTX);
	initBuffer(&entropy->m_contextState[OFF_MV_RES_CTX], sliceType, qp, (uint8_t*)INIT_MVD, NUM_MV_RES_CTX);
	initBuffer(&entropy->m_contextState[OFF_QT_CBF_CTX], sliceType, qp, (uint8_t*)INIT_QT_CBF, NUM_QT_CBF_CTX);
	initBuffer(&entropy->m_contextState[OFF_TRANS_SUBDIV_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_TRANS_SUBDIV_FLAG, NUM_TRANS_SUBDIV_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_QT_ROOT_CBF_CTX], sliceType, qp, (uint8_t*)INIT_QT_ROOT_CBF, NUM_QT_ROOT_CBF_CTX);
	initBuffer(&entropy->m_contextState[OFF_SIG_CG_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SIG_CG_FLAG, 2 * NUM_SIG_CG_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_SIG_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SIG_FLAG, NUM_SIG_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_CTX_LAST_FLAG_X], sliceType, qp, (uint8_t*)INIT_LAST, NUM_CTX_LAST_FLAG_XY);
	initBuffer(&entropy->m_contextState[OFF_CTX_LAST_FLAG_Y], sliceType, qp, (uint8_t*)INIT_LAST, NUM_CTX_LAST_FLAG_XY);
	initBuffer(&entropy->m_contextState[OFF_ONE_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_ONE_FLAG, NUM_ONE_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_ABS_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_ABS_FLAG, NUM_ABS_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_MVP_IDX_CTX], sliceType, qp, (uint8_t*)INIT_MVP_IDX, NUM_MVP_IDX_CTX);
	initBuffer(&entropy->m_contextState[OFF_SAO_MERGE_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_SAO_MERGE_FLAG, NUM_SAO_MERGE_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_SAO_TYPE_IDX_CTX], sliceType, qp, (uint8_t*)INIT_SAO_TYPE_IDX, NUM_SAO_TYPE_IDX_CTX);
	initBuffer(&entropy->m_contextState[OFF_TRANSFORMSKIP_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_TRANSFORMSKIP_FLAG, 2 * NUM_TRANSFORMSKIP_FLAG_CTX);
	initBuffer(&entropy->m_contextState[OFF_TQUANT_BYPASS_FLAG_CTX], sliceType, qp, (uint8_t*)INIT_CU_TRANSQUANT_BYPASS_FLAG, NUM_TQUANT_BYPASS_FLAG_CTX);
	// new structure
	start(entropy);*/
}
/** Move bits from register into bitstream */
void writeOut(Entropy* entropy)
{/*
	uint32_t leadByte = entropy->m_low >> (13 + entropy->m_bitsLeft);
	uint32_t low_mask = (uint32_t)(~0) >> (11 + 8 - entropy->m_bitsLeft);

	entropy->m_bitsLeft -= 8;
	entropy->m_low &= low_mask;

	if (leadByte == 0xff)
		entropy->m_numBufferedBytes++;
	else
	{
		uint32_t numBufferedBytes = entropy->m_numBufferedBytes;
		if (numBufferedBytes > 0)
		{
			uint32_t carry = leadByte >> 8;
			uint32_t byteTowrite = entropy->m_bufferedByte + carry;
			writeByte(entropy->syn.m_bitIf, byteTowrite);
			byteTowrite = (0xff + carry) & 0xff;
			while (numBufferedBytes > 1)
			{
				writeByte(entropy->syn.m_bitIf, byteTowrite);
				numBufferedBytes--;
			}
		}
		entropy->m_numBufferedBytes = 1;
		entropy->m_bufferedByte = leadByte & 0xff;
	}*/
}
void finish(Entropy* entropy)
{/*
	if (entropy->m_low >> (21 + entropy->m_bitsLeft))
	{
		writeByte(entropy->syn.m_bitIf, entropy->m_bufferedByte + 1);
		while (entropy->m_numBufferedBytes > 1)
		{
			writeByte(entropy->syn.m_bitIf, 0x00);
			entropy->m_numBufferedBytes--;
		}
		entropy->m_low -= 1 << (21 + entropy->m_bitsLeft);
	}
	else
	{
		if (entropy->m_numBufferedBytes > 0)
			writeByte(entropy->syn.m_bitIf, entropy->m_bufferedByte);
		while (entropy->m_numBufferedBytes > 1)
		{
			writeByte(entropy->syn.m_bitIf, 0xff);
			entropy->m_numBufferedBytes--;
		}
	}
	enc_write(entropy->syn.m_bitIf, entropy->m_low >> 8, 13 + entropy->m_bitsLeft);*/
}
/** Encode bin */
void encodeBin(Entropy* entropy, uint32_t binValue, uint8_t *ctxModel)
{/*
	uint32_t mstate = *ctxModel;

	*ctxModel = sbacNext(mstate, binValue);

	if (!entropy->syn.m_bitIf)
	{
		entropy->m_fracBits += sbacGetEntropyBits(mstate, binValue);
		return;
	}//对该位编码采用熵编码估计值（即以查表的方式）而不是直接进行编码，从而在不增加码率的条件下增加编码速度。

	uint32_t range = entropy->m_range;
	uint32_t state = sbacGetState(mstate);
	uint32_t lps = g_lpsTable[state][(range & 0xff) >> 6];
	range -= lps;

	if (lps < 2)
	{
		printf("lps is too small\n");
		return;
	}

	int numBits = (uint32_t)(range - 256) >> 31;

	uint32_t low = entropy->m_low;

	// NOTE: MPS must be LOWEST bit in mstate
	if ((uint32_t)((binValue ^ mstate) & 1) != (uint32_t)(binValue != sbacGetMps(mstate)))
	{
		printf("binValue failure\n");
		return;
	}
	if ((binValue ^ mstate) & 1)
	{
		// NOTE: lps is non-zero and the maximum of idx is 8 because lps less than 256
		//numBits = g_renormTable[lps >> 3];
		unsigned long idx;
		//CLZ(idx, lps);
		idx = clz(lps);
		if (state == 63 && idx != 1)
		{
			printf("state failure\n");
			return;
		}

		numBits = 8 - idx;
		if (state >= 63)
			numBits = 6;
		if (numBits > 6)
		{
			printf("numBits failure\n");
			return;
		}

		low += range;
		range = lps;
	}
	entropy->m_low = (low << numBits);
	entropy->m_range = (range << numBits);
	entropy->m_bitsLeft += numBits;

	if (entropy->m_bitsLeft >= 0)
		writeOut(entropy);*/
}
int clz(unsigned int a)
{/*
	int i, count = 0;
	for (i = 0; i<32; i++)
	{
		if (((a << i) & 0x80000000) != 0)
			break;
		count++;
	}
	return count ^ 31;*/return 0;
}
int ctz(unsigned int a)
{/*
	int i, count = 0;
	for (i = 0; i<32; i++)
	{
		if (((a >> i) & 0x00000001) != 0)
			break;
		count++;
	}
	return count;*/return 0;
}

void codePPS(Entropy *entropy, PPS *pps)
{/*
	WRITE_UVLC(entropy->syn.m_bitIf, 0, "pps_pic_parameter_set_id");
	WRITE_UVLC(entropy->syn.m_bitIf, 0, "pps_seq_parameter_set_id");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "dependent_slice_segments_enabled_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "output_flag_present_flag");
	WRITE_CODE(entropy->syn.m_bitIf, 0, 3, "num_extra_slice_header_bits");
	WRITE_FLAG(entropy->syn.m_bitIf, pps->bSignHideEnabled, "sign_data_hiding_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "cabac_init_present_flag");
	WRITE_UVLC(entropy->syn.m_bitIf, 0, "num_ref_idx_l0_default_active_minus1");
	WRITE_UVLC(entropy->syn.m_bitIf, 0, "num_ref_idx_l1_default_active_minus1");

	WRITE_SVLC(entropy->syn.m_bitIf, 0, "init_qp_minus26");
	WRITE_FLAG(entropy->syn.m_bitIf, pps->bConstrainedIntraPred, "constrained_intra_pred_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, pps->bTransformSkipEnabled, "transform_skip_enabled_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, pps->bUseDQP, "cu_qp_delta_enabled_flag");
	if (pps->bUseDQP)
		WRITE_UVLC(entropy->syn.m_bitIf, pps->maxCuDQPDepth, "diff_cu_qp_delta_depth");

	WRITE_SVLC(entropy->syn.m_bitIf, pps->chromaQpOffset[0], "pps_cb_qp_offset");
	WRITE_SVLC(entropy->syn.m_bitIf, pps->chromaQpOffset[1], "pps_cr_qp_offset");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "pps_slice_chroma_qp_offsets_present_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, pps->bUseWeightPred, "weighted_pred_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, pps->bUseWeightedBiPred, "weighted_bipred_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, pps->bTransquantBypassEnabled, "transquant_bypass_enable_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "tiles_enabled_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, pps->bEntropyCodingSyncEnabled, "entropy_coding_sync_enabled_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 1, "loop_filter_across_slices_enabled_flag");

	WRITE_FLAG(entropy->syn.m_bitIf, pps->bDeblockingFilterControlPresent, "deblocking_filter_control_present_flag");
	if (pps->bDeblockingFilterControlPresent)
	{
		WRITE_FLAG(entropy->syn.m_bitIf, 0, "deblocking_filter_override_enabled_flag");
		WRITE_FLAG(entropy->syn.m_bitIf, pps->bPicDisableDeblockingFilter, "pps_disable_deblocking_filter_flag");
		if (!pps->bPicDisableDeblockingFilter)
		{
			WRITE_SVLC(entropy->syn.m_bitIf, pps->deblockingFilterBetaOffsetDiv2, "pps_beta_offset_div2");
			WRITE_SVLC(entropy->syn.m_bitIf, pps->deblockingFilterTcOffsetDiv2, "pps_tc_offset_div2");
		}
	}

	WRITE_FLAG(entropy->syn.m_bitIf, 0, "pps_scaling_list_data_present_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "lists_modification_present_flag");
	WRITE_UVLC(entropy->syn.m_bitIf, 0, "log2_parallel_merge_level_minus2");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "slice_segment_header_extension_present_flag");
	WRITE_FLAG(entropy->syn.m_bitIf, 0, "pps_extension_flag");*/
}

/** Encode equiprobable bin */
void encodeBinEP(Entropy* entropy, uint32_t binValue)
{/*
	if (!entropy->syn.m_bitIf)
	{
		entropy->m_fracBits += 32768;
		return;
	}
	entropy->m_low <<= 1;
	if (binValue)
		entropy->m_low += entropy->m_range;
	entropy->m_bitsLeft++;

	if (entropy->m_bitsLeft >= 0)
		writeOut(entropy);*/
}
/** Encode equiprobable bins */
void encodeBinsEP(Entropy* entropy, uint32_t binValues, int numBins)
{/*
	if (!entropy->syn.m_bitIf)
	{
		entropy->m_fracBits += 32768 * numBins;
		return;
	}

	while (numBins > 8)
	{
		numBins -= 8;
		uint32_t pattern = binValues >> numBins;
		entropy->m_low <<= 8;
		entropy->m_low += entropy->m_range * pattern;
		binValues -= pattern << numBins;
		entropy->m_bitsLeft += 8;

		if (entropy->m_bitsLeft >= 0)
			writeOut(entropy);
	}

	entropy->m_low <<= numBins;
	entropy->m_low += entropy->m_range * binValues;
	entropy->m_bitsLeft += numBins;

	if (entropy->m_bitsLeft >= 0)
		writeOut(entropy);*/
}
/** Encode terminating bin */
void encodeBinTrm(Entropy* entropy, uint32_t binValue)
{/*
	if (!entropy->syn.m_bitIf)
	{
		entropy->m_fracBits += sbacGetEntropyBitsTrm(binValue);
		return;
	}

	entropy->m_range -= 2;
	if (binValue)
	{
		entropy->m_low += entropy->m_range;
		entropy->m_low <<= 7;
		entropy->m_range = 2 << 7;
		entropy->m_bitsLeft += 7;
	}
	else if (entropy->m_range >= 256)
		return;
	else
	{
		entropy->m_low <<= 1;
		entropy->m_range <<= 1;
		entropy->m_bitsLeft++;
	}

	if (entropy->m_bitsLeft >= 0)
		writeOut(entropy);*/
}
void codeTransformSubdivFlag(Entropy* entropy, uint32_t symbol, uint32_t ctx)    
{/*
	encodeBin(entropy, symbol, &entropy->m_contextState[OFF_TRANS_SUBDIV_FLAG_CTX + ctx]); */
}
void codeCUTransquantBypassFlag(Entropy* entropy, uint32_t symbol)               
{/*
	encodeBin(entropy, symbol, &entropy->m_contextState[OFF_TQUANT_BYPASS_FLAG_CTX]); */
}
void codeQtCbfLuma(Entropy* entropy, uint32_t cbf, uint32_t tuDepth)             
{/*
	encodeBin(entropy, cbf, &entropy->m_contextState[OFF_QT_CBF_CTX + !tuDepth]); */
}
void codeQtCbfChroma(Entropy* entropy, uint32_t cbf, uint32_t tuDepth)           
{/*
	encodeBin(entropy, cbf, &entropy->m_contextState[OFF_QT_CBF_CTX + 2 + tuDepth]); */
}
void codeQtRootCbf(Entropy* entropy, uint32_t cbf)                               
{/*
	encodeBin(entropy, cbf, &entropy->m_contextState[OFF_QT_ROOT_CBF_CTX]); */
}
void codeSplitFlag(Entropy* entropy, CUData *cu, uint32_t absPartIdx, uint32_t depth)
{/*
	printf("isSpilit");
	encodeBin(entropy, cu->m_cuDepth[absPartIdx] > depth, &entropy->m_contextState[OFF_SPLIT_FLAG_CTX + CUData_getCtxSplitFlag(cu, absPartIdx, depth)]);
	*/
}
void codeSkipFlag(Entropy* entropy, CUData* cu, uint32_t absPartIdx)
{/*
	encodeBin(entropy, isSkipped(cu, absPartIdx), &entropy->m_contextState[OFF_SKIP_FLAG_CTX + CUData_getCtxSkipFlag(cu, absPartIdx)]);
	*/
}
void codePredMode(Entropy* entropy, int predMode)
{/*
	encodeBin(entropy, predMode == MODE_INTRA ? 1 : 0, &entropy->m_contextState[OFF_PRED_MODE_CTX]);*/
}
void codeTransformSkipFlags(Entropy* entropy, uint32_t transformSkip, enum TextType ttype) 
{/*
	encodeBin(entropy, transformSkip, &entropy->m_contextState[OFF_TRANSFORMSKIP_FLAG_CTX + (ttype ? NUM_TRANSFORMSKIP_FLAG_CTX : 0)]); 
	*/
}
/* these functions are only used to estimate the bits when cbf is 0 and will never be called when writing the bistream. */
void codeQtRootCbfZero(Entropy* entropy) 
{/*
	encodeBin(entropy, 0, &entropy->m_contextState[OFF_QT_ROOT_CBF_CTX]); */
}
void  writeUnaryMaxSymbol(Entropy* entropy, uint32_t symbol, uint8_t* scmModel, int offset, uint32_t maxSymbol)//使用截断一元码（TU）二元化方法将前缀转换为二元码，在对每一位比特进行常规编码。
{/*
	if (maxSymbol <= 0)
	{
		printf("maxSymbol too small\n");
		return;
	}

	encodeBin(entropy, symbol ? 1 : 0, &scmModel[0]);

	if (!symbol)
		return;

	char bCodeLast = (maxSymbol > symbol);

	while (--symbol)
		encodeBin(entropy, 1, &scmModel[offset]);

	if (bCodeLast)
		encodeBin(entropy, 0, &scmModel[offset]);*/
}
void writeEpExGolomb(Entropy* entropy, uint32_t symbol, uint32_t count)//使用EGK二元化方法将后缀转换为二元码，在对每一位比特进行旁路编码。
{/*
	uint32_t bins = 0;
	int numBins = 0;

	while (symbol >= (uint32_t)(1 << count))
	{
		bins = 2 * bins + 1;
		numBins++;
		symbol -= 1 << count;
		count++;
	}

	bins = 2 * bins + 0;
	numBins++;

	bins = (bins << count) | symbol;//二元化化后的二进制串
	numBins += count;//二元化化后的二进制串比特个数

	if (numBins > 32)
	{
		printf("numBins too large\n");
		return;
	}
	encodeBinsEP(entropy, bins, numBins);*/
}
void codeDeltaQP(Entropy* entropy, CUData* cu, uint32_t absPartIdx)//对语法元素cu_qp_delta_abs进行编码。
{/*
	int dqp = 0;//cu->m_qp[absPartIdx] - getRefQP(cu,absPartIdx);

	int qpBdOffsetY = QP_BD_OFFSET;

	dqp = (dqp + 78 + qpBdOffsetY + (qpBdOffsetY / 2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY / 2);//语法元素cu_qp_delta_abs的值。

	uint32_t absDQp = (uint32_t)((dqp > 0) ? dqp : (-dqp));
	uint32_t TUValue = X265_MIN((int)absDQp, CU_DQP_TU_CMAX);//语法元素cu_qp_delta_abs前缀值。
	writeUnaryMaxSymbol(entropy, TUValue, &entropy->m_contextState[OFF_DELTA_QP_CTX], 1, CU_DQP_TU_CMAX);//使用截断一元码（TU）二元化方法将前缀转换为二元码，在对每一位比特进行常规编码。
	if (absDQp >= CU_DQP_TU_CMAX)
		writeEpExGolomb(entropy, absDQp - CU_DQP_TU_CMAX, CU_DQP_EG_k);//使用EGK二元化方法将后缀转换为二元码，在对每一位比特进行旁路编码。

	if (absDQp > 0)
	{
		uint32_t sign = (dqp > 0 ? 0 : 1);
		encodeBinEP(entropy, sign);//对符号进行旁路编码。
	}*/
}
/** Coding of coeff_abs_level_minus3 */
void writeCoefRemainExGolomb(Entropy* entropy, uint32_t codeNumber, uint32_t absGoRice)
{/*
	uint32_t length;
	const uint32_t codeRemain = codeNumber & ((1 << absGoRice) - 1);

	if ((codeNumber >> absGoRice) < COEF_REMAIN_BIN_REDUCTION)
	{
		length = codeNumber >> absGoRice;

		if (codeNumber - (length << absGoRice) != (codeNumber & ((1 << absGoRice) - 1)))
		{
			printf("codeNumber failure\n");
			return;
		}
		if (length + 1 + absGoRice >= 32)
		{
			printf("length failure\n");
			return;
		}
		encodeBinsEP(entropy, (((1 << (length + 1)) - 2) << absGoRice) + codeRemain, length + 1 + absGoRice);
	}
	else
	{
		length = 0;
		codeNumber = (codeNumber >> absGoRice) - COEF_REMAIN_BIN_REDUCTION;
		{
			unsigned long idx;
			// CLZ(idx, codeNumber + 1);
			idx = clz(codeNumber + 1);
			length = idx;
			if ((codeNumber == 0) && (length != 0))
			{
				printf("length check failure\n");
				return;
			}
			codeNumber -= (1 << idx) - 1;
		}
		codeNumber = (codeNumber << absGoRice) + codeRemain;

		encodeBinsEP(entropy, (1 << (COEF_REMAIN_BIN_REDUCTION + length + 1)) - 2, COEF_REMAIN_BIN_REDUCTION + length + 1);
		encodeBinsEP(entropy, codeNumber, length + absGoRice);
	}*/
}
void codeMergeIndex(Entropy* entropy, CUData* cu, uint32_t absPartIdx)
{/*
	uint32_t numCand = cu->m_slice->m_maxNumMergeCand;

	if (numCand > 1)
	{
		uint32_t unaryIdx = cu->m_mvpIdx[0][absPartIdx]; // merge candidate index was stored in L0 MVP idx
		//encodeBin(entropy,(unaryIdx != 0), &entropy->m_contextState[OFF_MERGE_IDX_EXT_CTX]);

		if (unaryIdx >= numCand)
		{
			printf("unaryIdx out of range\n");
			return;
		}
		if (unaryIdx != 0)
		{
			uint32_t mask = (1 << unaryIdx) - 2;
			mask >>= (unaryIdx == numCand - 1) ? 1 : 0;
			encodeBinsEP(entropy, mask, unaryIdx - (unaryIdx == numCand - 1));
		}
	}*/
}

void entropy_resetBits(Entropy* entropy)
{/*
	entropy->m_low = 0;
	entropy->m_bitsLeft = -12;
	entropy->m_numBufferedBytes = 0;
	entropy->m_bufferedByte = 0xff;
	entropy->m_fracBits &= 32767;
	if (entropy->syn.m_bitIf)
		resetBits(entropy->syn.m_bitIf);*/
}

void setBitstream(Entropy* entropy, Bitstream* pbit)
{/*
	entropy->syn.m_bitIf = pbit;*/
}

/* finish encoding a cu and handle end-of-slice conditions */
void finishCU(Entropy* entropy, const CUData* cu, uint32_t absPartIdx, uint32_t depth, bool bCodeDQP)
{/*
	const Slice* slice = cu->m_slice;
	uint32_t realEndAddress = slice->m_endCUAddr;//真正的结束地址
	uint32_t cuAddr = getSCUAddr(cu) + absPartIdx;//CU的地址
	if (realEndAddress != Slice_realEndAddress(slice, slice->m_endCUAddr))
	{
		printf("real end address expected\n");
		return;
	}
	uint32_t granularityMask = g_maxCUSize - 1;
	uint32_t cuSize = 1 << cu->m_log2CUSize[absPartIdx];
	uint32_t rpelx = cu->m_cuPelX + g_zscanToPelX[absPartIdx] + cuSize;
	uint32_t bpely = cu->m_cuPelY + g_zscanToPelY[absPartIdx] + cuSize;
	char granularityBoundary = (((rpelx & granularityMask) == 0 || (rpelx == slice->m_sps->picWidthInLumaSamples)) &&
		((bpely & granularityMask) == 0 || (bpely == slice->m_sps->picHeightInLumaSamples)));

	if (slice->m_pps->bUseDQP)
		setQPSubParts((CUData*)cu, bCodeDQP ? getRefQP(cu, absPartIdx) : cu->m_qp[absPartIdx], absPartIdx, depth);

	if (granularityBoundary)
	{
		// Encode slice finish
		bool bTerminateSlice = FALSE;//初始化bTerminateSlice为false
		if (cuAddr + (NUM_4x4_PARTITIONS >> (depth << 1)) == realEndAddress)//若条件成立，则Slice结束
			bTerminateSlice = TRUE;

		// The 1-terminating bit is added to all streams, so don't add it here when it's 1.
		if (!bTerminateSlice)
			encodeBinTrm(entropy, 0);

		if (!entropy->syn.m_bitIf)
			entropy_resetBits(entropy); // TODO: most likely unnecessary
	}*/
}

void codePartSize(Entropy* entropy, CUData* cu, uint32_t absPartIdx, uint32_t depth)
{/*
	enum PartSize partSize = (enum PartSize)(cu->m_partSize[absPartIdx]);

	if (isIntra_cudata(cu, absPartIdx))
	{
		if (depth == g_maxCUDepth)
			encodeBin(entropy, 1, &entropy->m_contextState[OFF_PART_SIZE_CTX]);//partSize == SIZE_2Nx2N ? 1 : 0
		return;
	}

	switch (partSize)
	{
	case SIZE_2Nx2N:
		encodeBin(entropy, 1, &entropy->m_contextState[OFF_PART_SIZE_CTX]);
		break;

	case SIZE_2NxN:
	case SIZE_2NxnU:
	case SIZE_2NxnD:
		encodeBin(entropy, 0, &entropy->m_contextState[OFF_PART_SIZE_CTX + 0]);
		encodeBin(entropy, 1, &entropy->m_contextState[OFF_PART_SIZE_CTX + 1]);
		if (cu->m_slice->m_sps->maxAMPDepth > depth)
		{
			encodeBin(entropy, (partSize == SIZE_2NxN) ? 1 : 0, &entropy->m_contextState[OFF_PART_SIZE_CTX + 3]);
			if (partSize != SIZE_2NxN)
				encodeBinEP(entropy, (partSize == SIZE_2NxnU ? 0 : 1));
		}
		break;

	case SIZE_Nx2N:
	case SIZE_nLx2N:
	case SIZE_nRx2N:
		encodeBin(entropy, 0, &entropy->m_contextState[OFF_PART_SIZE_CTX + 0]);
		encodeBin(entropy, 0, &entropy->m_contextState[OFF_PART_SIZE_CTX + 1]);
		if (depth == g_maxCUDepth && !(cu->m_log2CUSize[absPartIdx] == 3))
			encodeBin(entropy, 1, &entropy->m_contextState[OFF_PART_SIZE_CTX + 2]);
		if (cu->m_slice->m_sps->maxAMPDepth > depth)
		{
			encodeBin(entropy, (partSize == SIZE_Nx2N) ? 1 : 0, &entropy->m_contextState[OFF_PART_SIZE_CTX + 3]);
			if (partSize != SIZE_Nx2N)
				encodeBinEP(entropy, (partSize == SIZE_nLx2N ? 0 : 1));
		}
		break;
	default:
		if (1)
		{
			printf("invalid CU partition\n");
			return;
		}
		break;
	}*/
}

void swap(unsigned int *x,unsigned int *y)//使用指针传递地址
{/*
	int temp;
	temp = *x;
	*x = *y;
	*y = temp;*/
}

unsigned int rightshift(unsigned int x[2], unsigned int shift)
{/*
	unsigned int b[2] = { 0 };
	unsigned int temp;
	b[1] = x[1] >> shift;
	if (shift >= 32)
	{
		temp = x[0] >> (shift - 32);
		return temp;
	}
	else
	{
		temp = x[0];
		temp &= (unsigned int)pow(2.0, (double)shift) - 1;
		temp = temp << (32 - shift);
		b[1] = temp + b[1];
		return b[1];
	}*/return 0;
}
/* Pattern decision for context derivation process of significant_coeff_flag */
uint32_t calc_sigPattern(/*uint64_t sigCoeffGroupFlag64*/uint32_t x[2], uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG)
{/*
	uint32_t p[2]={x[1],x[0]};
	if (trSizeCG == 1)
	return 0;

	X265_CHECK(trSizeCG <= 8, "transform CG is too large\n");
	X265_CHECK(cgBlkPos < 64, "cgBlkPos is too large\n");
	// NOTE: cgBlkPos+1 may more than 63, it is invalid for shift,
	//       but in this case, both cgPosX and cgPosY equal to (trSizeCG - 1),
	//       the sigRight and sigLower will clear value to zero, the final result will be correct

	const uint32_t sigPos = rightshift(p,cgBlkPos + 1);
	// TODO: instruction BT is faster, but _bittest64 still generate instruction 'BT m, r' in VS2012
	const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & (sigPos & 1);
	const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 2)) & 2;
	return sigRight + sigLower;*/return 0;
}

/* Context derivation process of coeff_abs_significant_flag */
uint32_t get_Sigctx(/*uint64_t cgGroupMask*/uint32_t x[2], uint32_t cgPosX, uint32_t cgPosY, uint32_t cgBlkPos, uint32_t trSizeCG)
{/*
	uint32_t p[2]={x[1],x[0]};
	X265_CHECK(cgBlkPos < 64, "cgBlkPos is too large\n");
	// NOTE: unsafe shift operator, see NOTE in calcPatternSigCtx
	const uint32_t sigPos = rightshift(p,cgBlkPos + 1);
	const uint32_t sigRight = ((int32_t)(cgPosX - (trSizeCG - 1)) >> 31) & sigPos;
	const uint32_t sigLower = ((int32_t)(cgPosY - (trSizeCG - 1)) >> 31) & (sigPos >> (trSizeCG - 1));

	return (sigRight | sigLower) & 1;*/return 0;
}

void codeCoeffNxN(Entropy* entropy, CUData* cu, coeff_t* coeff, uint32_t absPartIdx, uint32_t log2TrSize, enum TextType ttype)
{/*
	uint32_t blkPos, sig, ctxSig;
	uint32_t trSize = 1 << log2TrSize;
	uint32_t tqBypass = cu->m_tqBypass[absPartIdx];
	// compute number of significant coefficients
	uint32_t numSig = count_nonzero_c(trSize, coeff);//当前TB中非零系数的个数
	if (!(numSig > 0))
	{
		printf("cbf check fail\n");
		return;
	}
	char bHideFirstSign = cu->m_slice->m_pps->bSignHideEnabled && !tqBypass;//用来判断编码器是否允许使用SDH技术

	if (log2TrSize <= MAX_LOG2_TS_SIZE && !tqBypass && cu->m_slice->m_pps->bTransformSkipEnabled)
		codeTransformSkipFlags(entropy, cu->m_transformSkip[ttype][absPartIdx], ttype);//对语法元素transform_skip_flag编码，transform_skip_flag规定当前TU的残差是否进行了变换，该技术称为Transform skip。

	char bIsLuma = ttype == TEXT_LUMA;//判断该TB是亮度块还是色度块。
	// set the scan orders
	// select scans
	TUEntropyCodingParameters codingParameters;
	getTUEntropyCodingParameters(cu, &codingParameters, absPartIdx, log2TrSize, bIsLuma);

	uint8_t coeffNum[MLS_GRP_NUM] = { 0 };      // value range[0, 16]
	uint16_t coeffSign[MLS_GRP_NUM] = { 0 };    // bit mask map for non-zero coeff sign//为后面CG内非零系数的符号coeff_sign_flag旁路编码而设置的。
	uint16_t coeffFlag[MLS_GRP_NUM] = { 0 };    // bit mask map for non-zero coeff//为后面CG内每个系数的sig_coeff_flag常规编码而设置的。

	//----- encode significance map -----

	// Find position of last coefficient
	int scanPosLast = 0;
	uint32_t posLast;
	uint32_t ch1[2] = { 0 };

	if ((uint32_t)((1 << (log2TrSize - MLS_CG_LOG2_SIZE)) - 1) != (((uint32_t)~0 >> (31 - log2TrSize + MLS_CG_LOG2_SIZE)) >> 1))
	{
		printf("maskPosXY fault\n");
		return;
	}

	scanPosLast = scanPosLast_c(codingParameters.scan, coeff, coeffSign, coeffFlag, coeffNum, numSig);//, g_scan4x4[codingParameters.scanType], trSize//
	posLast = codingParameters.scan[scanPosLast];//扫描后第一个非零系数的位置，即TB中最后一个非零系数的位置。

	const int lastScanSet = scanPosLast >> MLS_CG_SIZE;//确定扫描后第一个非零系数的位置落在第几个CG

	// Calculate CG block non-zero mask, the latest CG always flag as non-zero in CG scan loop
	int idx;
	for (idx = 0; idx < lastScanSet; idx++)
	{
		uint8_t subSet = codingParameters.scanCG[idx] & 0xff;
		const uint8_t nonZero = (coeffNum[idx] != 0);

		if (nonZero == 1)
		{
			if (subSet<32)
				ch1[0] |= 1 << subSet;
			else
				ch1[1] |= 1 << (subSet - 32);
		}
	}

	// Code position of last coefficient//对TB中最后一个非零系数位置，即扫描后第一个非零系数在TB中的位置进行编码。
	{
		// The last position is composed of a prefix and suffix.
		// The prefix is context coded truncated unary bins. The suffix is bypass coded fixed length bins.
		// The bypass coded bins for both the x and y components are grouped together.
		uint32_t packedSuffixBits = 0, packedSuffixLen = 0;
		uint32_t pos[2] = { (posLast & (trSize - 1)), (posLast >> log2TrSize) };
		// swap
		if (codingParameters.scanType == SCAN_VER)
			swap(&pos[0], &pos[1]);

		int ctxIdx = bIsLuma ? (3 * (log2TrSize - 2) + ((log2TrSize - 1) >> 2)) : NUM_CTX_LAST_FLAG_XY_LUMA;
		int ctxShift = bIsLuma ? ((log2TrSize + 1) >> 2) : log2TrSize - 2;
		uint32_t maxGroupIdx = (log2TrSize << 1) - 1;

		uint8_t *ctx = &entropy->m_contextState[OFF_CTX_LAST_FLAG_X];
		uint32_t i;
		for (i = 0; i < 2; i++, ctxIdx += NUM_CTX_LAST_FLAG_XY)
		{
			uint32_t temp = g_lastCoeffTable[pos[i]];
			uint32_t prefixOnes = temp & 15;
			uint32_t suffixLen = temp >> 4;
			uint32_t ctxLast;
			for (ctxLast = 0; ctxLast < prefixOnes; ctxLast++)
				encodeBin(entropy, 1, ctx + ctxIdx + (ctxLast >> ctxShift));

			if (prefixOnes < maxGroupIdx)
				encodeBin(entropy, 0, ctx + ctxIdx + (prefixOnes >> ctxShift));

			packedSuffixBits <<= suffixLen;
			packedSuffixBits |= (pos[i] & ((1 << suffixLen) - 1));
			packedSuffixLen += suffixLen;
		}

		encodeBinsEP(entropy, packedSuffixBits, packedSuffixLen);
	}
	//对于CSBF和sig_flag都是按照扫描的顺序进行编码的
	// code significance flag
	uint8_t * const baseCoeffGroupCtx = &entropy->m_contextState[OFF_SIG_CG_FLAG_CTX + (bIsLuma ? 0 : NUM_SIG_CG_FLAG_CTX)];//CSBF的上下文模型的起始地址（亮度和色度各两个上下文模型）
	uint8_t * const baseCtx = bIsLuma ? &entropy->m_contextState[OFF_SIG_FLAG_CTX] : &entropy->m_contextState[OFF_SIG_FLAG_CTX + NUM_SIG_FLAG_CTX_LUMA];//sig_flag的上下文模型的起始地址（亮度有27，色度有15）
	uint32_t c1 = 1;
	int scanPosSigOff = scanPosLast - (lastScanSet << MLS_CG_SIZE) - 1;//按照扫描顺序，最后一个非零系数的前一个系数的坐标。
	int absCoeff[1 << MLS_CG_SIZE];//存放当前编码的CG中的非零系数的绝对值.
	int numNonZero = 1;
	unsigned long lastNZPosInCG;
	unsigned long firstNZPosInCG;

	absCoeff[0] = (int)abs(coeff[posLast]);//最后一个非零系数的绝对值赋值给absCoeff[0]
	int subSet = lastScanSet;
	for (; subSet >= 0; subSet--)//其余系数的位置信息以及非零系数的幅值信息编码开始。
	{
		uint32_t ch2[2] = { 0 };
		const uint32_t subCoeffFlag = coeffFlag[subSet];//为了求当前CG的每个系数对应的sig_flag的值而设置的。
		uint32_t scanFlagMask = subCoeffFlag;
		int subPosBase = subSet << MLS_CG_SIZE;//为了找到当前CG的扫描开始的第一个位置。

		if (subSet == lastScanSet)
		{
			if (scanPosSigOff != scanPosLast - (lastScanSet << MLS_CG_SIZE) - 1)
			{
				printf("scanPos mistake\n");
				return;
			}
			scanFlagMask >>= 1;//默认最后一个非零系数位置对应的sig_flag=1，因此从它前面一个系数位置开始sig_flag的编码（按照扫描顺序）
		}

		// encode significant_coeffgroup_flag//按照CG的扫描顺序从含有最后一个非零系数位置CG的前一个CG开始逐个对CG的CSBF的值进行常规编码。
		int cgBlkPos = codingParameters.scanCG[subSet];//当前CG在TB中的位置
		const int cgPosY = cgBlkPos >> (log2TrSize - MLS_CG_LOG2_SIZE);//当前CG在TB中的行号(0,1,2...)
		const int cgPosX = cgBlkPos & ((1 << (log2TrSize - MLS_CG_LOG2_SIZE)) - 1);//当前CG在TB中的列号(0,1,2...)
		//if(cgBlkPos>31)
		//	cgBlkPos=31;
		if (cgBlkPos<32)
			ch2[0] = 1 << cgBlkPos;
		else
			ch2[1] = 1 << (cgBlkPos - 32);

		if (subSet == lastScanSet || !subSet)//判断当前CG是否含有最后一个非零系数，如果含有，则CSBF=1，不进行编码，否则进行CSBF的编码，第一个CG（含有DC系数）的CSBF默认为1，也不进行编码。
			// sigCoeffGroupFlag64 |= cgBlkPosMask;
		{
			ch1[0] |= ch2[0];
			ch1[1] |= ch2[1];
		}
		else
		{
			uint32_t sigCoeffGroup = (((ch1[0] & ch2[0]) || (ch1[1] & ch2[1])) != 0);//当前CG的CSBF的值（0/1）
			//当前CG的CSBF的上下文模型选取与其下、右相邻CG的CSBF的取值有关，公式：ctx=min(1,Sr+Sl)。
			uint32_t ctxSig = get_Sigctx(ch1, cgPosX, cgPosY, cgBlkPos, (trSize >> MLS_CG_LOG2_SIZE));//当前CG的CSBF的ctxIdx（0/1）
			encodeBin(entropy, sigCoeffGroup, &baseCoeffGroupCtx[ctxSig]);//当前CG的CSBF的算术编码
		}

		// encode significant_coeff_flag//当前CG的CSBF=1时，按照系数的扫描顺序逐一编码当前CG内每一个位置上非零系数标识sig_coeff_flag,然后再编码所有非零系数的幅值信息。
		if ((ch1[0] & ch2[0]) || (ch1[1] & ch2[1]))//判断当前CG的CSBF是否为1。
		{
			if ((log2TrSize == 2) && (log2TrSize != 2 || subSet != 0))//当前TB大小不为4，或者为4且就是当前CG。
			{
				printf("log2TrSize and subSet mistake!\n");
				return;
			}
			//对于8×8、16×16和32×32的TBs，sig_flag的上下文模型索引的选择不仅跟当前CG的下，右相邻CG的CSBF取值有关，还与当前CG的系数的位置有关，根据当前CG的下，右相邻CG的
			//CSBF的取值分成4种模式，在当前CG中，每一种模式对于不同的位置对应着不同的上下文索引。
			const int patternSigCtx = calc_sigPattern(ch1, cgPosX, cgPosY, cgBlkPos, (trSize >> MLS_CG_LOG2_SIZE));//确定模式号（0,1,2,3）
			const uint32_t posOffset = (bIsLuma && subSet) ? 3 : 0;
			//对于4×4的TBs，sig_flag的上下文索引只与该TB的位置有关。
			uint8_t ctxIndMap4x4[16] =
			{
				0, 1, 4, 5,//对应位置的每个数值表示sig_flag上下文模型的索引,并行的为一个CG内16个sig_flag确定了上下文模型索引。
				2, 3, 4, 5,
				6, 6, 8, 8,
				7, 7, 8, 8
			};
			// NOTE: [patternSigCtx][posXinSubset][posYinSubset]
			const uint8_t table_cnt[4][SCAN_SET_SIZE] =
			{
				// patternSigCtx = 0
				{
					2, 1, 1, 0,
					1, 1, 0, 0,
					1, 0, 0, 0,
					0, 0, 0, 0,
				},
				// patternSigCtx = 1
				{
					2, 2, 2, 2,
					1, 1, 1, 1,
					0, 0, 0, 0,
					0, 0, 0, 0,
				},
				// patternSigCtx = 2
				{
					2, 1, 0, 0,
					2, 1, 0, 0,
					2, 1, 0, 0,
					2, 1, 0, 0,
				},
				// patternSigCtx = 3
				{
					2, 2, 2, 2,
					2, 2, 2, 2,
					2, 2, 2, 2,
					2, 2, 2, 2,
				}
			};

			const int offset = codingParameters.firstSignificanceMapContext;
			//ALIGN_VAR_32(uint16_t, tmpCoeff[SCAN_SET_SIZE]);//为数组tmpCoeff[16]开辟一个32字节对齐方式的内存空间。
			uint16_t tmpCoeff[SCAN_SET_SIZE];
			// TODO: accelerate by PABSW
			const uint32_t blkPosBase = codingParameters.scan[subPosBase];//当前CG的第一个系数的位置(右下角)。（扫描顺序）
			//将当前CG按照从右下角系数位置开始从右→左顺序依次将对应位置系数赋值给数组tmpCoeff[16].
			int i;
			for (i = 0; i < MLS_CG_SIZE; i++)
			{
				tmpCoeff[i * MLS_CG_SIZE + 0] = (uint16_t)abs(coeff[blkPosBase + i * trSize + 0]);//当前CG右下角系数位置对应的系数的绝对值赋值给tmpCoeff[0]
				tmpCoeff[i * MLS_CG_SIZE + 1] = (uint16_t)abs(coeff[blkPosBase + i * trSize + 1]);
				tmpCoeff[i * MLS_CG_SIZE + 2] = (uint16_t)abs(coeff[blkPosBase + i * trSize + 2]);
				tmpCoeff[i * MLS_CG_SIZE + 3] = (uint16_t)abs(coeff[blkPosBase + i * trSize + 3]);
			}

			if (entropy->syn.m_bitIf)//判断指针m_bitIf是否为空指针
			{
				if (log2TrSize == 2)//判断当前TB的大小是否为4×4.
				{
					uint32_t blkPos, sig, ctxSig;
					//从最后一个非零系数位置前一个系数位置开始进行每个位置系数对应的sig_flag的常规编码。
					for (; scanPosSigOff >= 0; scanPosSigOff--)
					{
						blkPos = g_scan4x4[codingParameters.scanType][scanPosSigOff];//当前坐标对应的系数的位置。
						sig = scanFlagMask & 1;//当前位置系数对应的sig_flag的值（0/1）
						scanFlagMask >>= 1;
						if ((uint32_t)(tmpCoeff[blkPos] != 0) != sig)//判断该位置上系数的值与该位置上系数对应的sig_flag的值是否“对应”，如果不对应，出现错误。
						{
							printf("sign bit mistake\n");
							return;
						}
						{
							ctxSig = ctxIndMap4x4[blkPos];//取出对应位置上系数的sig_flag的上下文模型索引。
							if (ctxSig != getSigCtxInc(patternSigCtx, log2TrSize, trSize, blkPos, bIsLuma, codingParameters.firstSignificanceMapContext))
							{
								printf("sigCtx mistake!\n");
								return;
							}
							encodeBin(entropy, sig, &baseCtx[ctxSig]);
						}
						absCoeff[numNonZero] = tmpCoeff[blkPos];//将该CG中非零系数提取赋给数组absCoeff[16]
						numNonZero += sig;
					}
				}
				else//当前TB不是4×4大小的。
				{
					if ((log2TrSize <= 2))
					{
						printf("log2TrSize must be more than 2 in this path!\n");
						return;
					}

					const uint8_t *tabSigCtx = table_cnt[(uint32_t)patternSigCtx];//根据模式号选取对应的上下文模型索引表。

					//uint32_t blkPos, sig, ctxSig;
					for (; scanPosSigOff >= 0; scanPosSigOff--)
					{
						blkPos = g_scan4x4[codingParameters.scanType][scanPosSigOff];
						const uint32_t posZeroMask = (subPosBase + scanPosSigOff) ? ~0 : 0;
						sig = scanFlagMask & 1;
						scanFlagMask >>= 1;
						if ((uint32_t)(tmpCoeff[blkPos] != 0) != sig)
						{
							printf("sign bit mistake\n");
							return;
						}
						if (scanPosSigOff != 0 || subSet == 0 || numNonZero)
						{
							const uint32_t cnt = tabSigCtx[blkPos] + offset;
							ctxSig = (cnt + posOffset) & posZeroMask;

							if (ctxSig != getSigCtxInc(patternSigCtx, log2TrSize, trSize, codingParameters.scan[subPosBase + scanPosSigOff], bIsLuma, codingParameters.firstSignificanceMapContext))
							{
								printf("sigCtx mistake!\n");
								return;
							}
							encodeBin(entropy, sig, &baseCtx[ctxSig]);
						}
						absCoeff[numNonZero] = tmpCoeff[blkPos];
						numNonZero += sig;
					}
				}
			}
			else // fast RD path//下面这个提案为了在一开始模式选择时熵编码使用估计值（以查表方式）而不是直接进行编码，依次在不增加码率的情况下提高编码速度。
			{
				// maximum g_entropyBits are 18-bits and maximum of count are 16, so intermedia of sum are 22-bits
				uint32_t sum = 0;
				if (log2TrSize == 2)
				{
					//uint32_t blkPos, sig, ctxSig;
					for (; scanPosSigOff >= 0; scanPosSigOff--)
					{
						blkPos = g_scan4x4[codingParameters.scanType][scanPosSigOff];
						sig = scanFlagMask & 1;
						scanFlagMask >>= 1;
						if ((uint32_t)(tmpCoeff[blkPos] != 0) != sig)
						{
							printf("sign bit mistake\n");
							return;
						}
						{
							ctxSig = ctxIndMap4x4[blkPos];
							if (ctxSig != getSigCtxInc(patternSigCtx, log2TrSize, trSize, codingParameters.scan[subPosBase + scanPosSigOff], bIsLuma, codingParameters.firstSignificanceMapContext))
							{
								printf("sigCtx mistake!\n");
								return;
							}
							//encodeBin(sig, baseCtx[ctxSig]);
							const uint32_t mstate = baseCtx[ctxSig];//取出State+MPS
							const uint32_t mps = mstate & 1;//mstate最低一位是MPS
							const uint32_t stateBits = g_entropyStateBits[mstate ^ sig];//该位的熵编码的估计值，查表得到，从而避免了对其算术编码。
							uint32_t nextState = (stateBits >> 23) + mps;//概率更新（实际就是概率状态索引的更新）
							if ((mstate ^ sig) == 1)//当编码的sig=LPS时，若此时state=0,则先互换MPS和LPS的值，再进行概率更新，否则，就不互换MPS和LPS的值，只进行概率更新。
								nextState = sig;
							if (sbacNext(mstate, sig) != nextState)//验证概率更新的结果与查表的结果是否一致，若不一致，计算错误。
							{
								printf("nextState check failure\n");
								return;
							}
							if (sbacGetEntropyBits(mstate, sig) != (stateBits & 0xFFFFFF))//验证得到的熵编码的结果和实际中查表的结果是否一致。
							{
								printf("entropyBits check failure\n");
								return;
							}
							baseCtx[ctxSig] = nextState & 0xff;//把更新过的状态存入当前的上下文模型中。
							sum += stateBits;//熵编码的结果进行累加。
						}
						absCoeff[numNonZero] = tmpCoeff[blkPos];//从当前CG按照扫描的顺序第一个非零系数开始放入数组中absCoeff[16]
						numNonZero += sig;
					}
				} // end of 4x4//当前4x4TB的每一个系数位置的sig_flag的编码结束。
				else
				{
					if ((log2TrSize <= 2))
					{
						printf("log2TrSize must be more than 2 in this path!\n");
						return;
					}

					const uint8_t *tabSigCtx = table_cnt[(uint32_t)patternSigCtx];

					for (; scanPosSigOff >= 0; scanPosSigOff--)
					{
						blkPos = g_scan4x4[codingParameters.scanType][scanPosSigOff];
						const uint32_t posZeroMask = (subPosBase + scanPosSigOff) ? ~0 : 0;
						sig = scanFlagMask & 1;
						scanFlagMask >>= 1;
						if ((uint32_t)(tmpCoeff[blkPos] != 0) != sig)
						{
							printf("sign bit mistake\n");
							return;
						}
						if (scanPosSigOff != 0 || subSet == 0 || numNonZero)
						{
							const uint32_t cnt = tabSigCtx[blkPos] + offset;
							ctxSig = (cnt + posOffset) & posZeroMask;

							if (ctxSig != getSigCtxInc(patternSigCtx, log2TrSize, trSize, codingParameters.scan[subPosBase + scanPosSigOff], bIsLuma, codingParameters.firstSignificanceMapContext))
							{
								printf("sigCtx mistake!\n");
								return;
							}
							//encodeBin(sig, baseCtx[ctxSig]);
							const uint32_t mstate = baseCtx[ctxSig];
							const uint32_t mps = mstate & 1;
							const uint32_t stateBits = g_entropyStateBits[mstate ^ sig];
							uint32_t nextState = (stateBits >> 23) + mps;
							if ((mstate ^ sig) == 1)
								nextState = sig;
							if (sbacNext(mstate, sig) != nextState)
							{
								printf("nextState check failure\n");
								return;
							}
							if (sbacGetEntropyBits(mstate, sig) != (stateBits & 0xFFFFFF))
							{
								printf("entropyBits check failure\n");
								return;
							}
							baseCtx[ctxSig] = nextState & 0xff;
							sum += stateBits;
						}
						absCoeff[numNonZero] = tmpCoeff[blkPos];
						numNonZero += sig;
					}
				} // end of non 4x4 path//当前TB不是4x4的，当前CG的每个位置系数的sig_flag编码结束。
				sum &= 0xFFFFFF;//去掉前面的状态位（stateMPS或stateLPS，8位）只留下熵编码的最终结果。

				// update RD cost//直接查表的比特串最终通过变量m_fracBits累加。
				entropy->m_fracBits += sum;
			} // end of fast RD path -- !m_bitIf
		}
		if (coeffNum[subSet] != numNonZero)//其余非零系数的位置信息编码结束
		{
			printf("coefNum mistake\n");
			return;
		}

		uint32_t coeffSigns = coeffSign[subSet];//确定当前CG非零系数的符号
		numNonZero = coeffNum[subSet];//确定当前CG非零系数的个数
		if (numNonZero > 0)//非零系数幅值信息编码开始。
		{
			if (subCoeffFlag <= 0)//若subCoeffFlag=0，表示当前CG没有非零系数。
			{
				printf("subCoeffFlag is zero\n");
				return;
			}
			//CLZ(lastNZPosInCG, subCoeffFlag);//subCoeffFlag的前缀零的个数与31异或的结果赋给lastNZPosInCG
			lastNZPosInCG = (unsigned long)clz(subCoeffFlag);
			//CTZ(firstNZPosInCG, subCoeffFlag);//subCoeffFlag的拖尾零的个数赋给lastNZPosInCG
			firstNZPosInCG = (unsigned long)ctz(subCoeffFlag);
			char signHidden = (lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD);//判断当前编码CG中第一个非零系数和最后一个非零系数之间的间隔是否大于等于4。
			uint32_t ctxSet = (subSet > 0 && bIsLuma) ? 2 : 0;

			if (c1 == 0)
				ctxSet++;

			c1 = 1;
			uint8_t *baseCtxMod = bIsLuma ? &entropy->m_contextState[OFF_ONE_FLAG_CTX + 4 * ctxSet] : &entropy->m_contextState[OFF_ONE_FLAG_CTX + NUM_ONE_FLAG_CTX_LUMA + 4 * ctxSet];//greater than 1 flag的上下文模型偏移。

			int numC1Flag = X265_MIN(numNonZero, C1FLAG_NUMBER);//按照扫描的顺序逐一编码CG内前8个非零系数的语法元素coeff_abs_level_greater1_flag,后续的非零系数不在编码，默认为1.
			int firstC2FlagIdx = -1;
			//对coeff_abs_level_greater1_flag进行常规编码
			int idx;
			for (idx = 0; idx < numC1Flag; idx++)
			{
				uint32_t symbol = absCoeff[idx] > 1;//求coeff_abs_level_greater1_flag得值，大于1，取1，小于等于1，取0.
				encodeBin(entropy, symbol, &baseCtxMod[c1]);
				if (symbol)//用来找到当前CG内第一个非零系数绝对值大于1的坐标。
				{
					c1 = 0;

					if (firstC2FlagIdx == -1)
						firstC2FlagIdx = idx;
				}
				else if ((c1 < 3) && (c1 > 0))
					c1++;
			}

			if (!c1)//如果在当前CG中找到第一个非零系数绝对值大于1的坐标，即c1 = 0，那么就对该系数的coeff_abs_level_greater2_flag进行常规编码。
			{
				baseCtxMod = bIsLuma ? &entropy->m_contextState[OFF_ABS_FLAG_CTX + ctxSet] : &entropy->m_contextState[OFF_ABS_FLAG_CTX + NUM_ABS_FLAG_CTX_LUMA + ctxSet];

				if ((firstC2FlagIdx == -1))//此时firstC2FlagIdx一定不等于-1
				{
					printf("firstC2FlagIdx check failure\n");
					return;
				}
				uint32_t symbol = absCoeff[firstC2FlagIdx] > 2;//coeff_abs_level_greater2_flag的值
				encodeBin(entropy, symbol, &baseCtxMod[0]);
			}

			const int hiddenShift = (bHideFirstSign && signHidden) ? 1 : 0;//当编码器允许使用SDH技术且当前编码CG中第一个非零系数和最后一个非零系数之间的间隔大于等于4时，则当前CG省略最后一个最后一个非零系数符号的熵编码。
			encodeBinsEP(entropy, (coeffSigns >> hiddenShift), numNonZero - hiddenShift);//对当前CG内每个非零系数的符号coeff_sign_flag进行旁路编码。

			if (!c1 || numNonZero > C1FLAG_NUMBER)//如果当前CG中非零系数的个数少于8个且在该非零系数中不存在非零系数的绝对值>1，则不对语法元素coeff_abs_level_remaining进行编码。
			{
				uint32_t goRiceParam = 0;
				int firstCoeff2 = 1;
				uint32_t baseLevelN = 0x5555AAAA; // 2-bits encode format baseLevel

				if (!entropy->syn.m_bitIf)
				{
					// FastRd path
					int idx;
					for (idx = 0; idx < numNonZero; idx++)
					{
						int baseLevel = (baseLevelN & 3) | firstCoeff2;
						if (baseLevel != ((idx < C1FLAG_NUMBER) ? (2 + firstCoeff2) : 1))
						{
							printf("baseLevel check failurr\n");
							return;
						}
						baseLevelN >>= 2;
						int codeNumber = absCoeff[idx] - baseLevel;

						if (codeNumber >= 0)
						{
							//writeCoefRemainExGolomb(absCoeff[idx] - baseLevel, goRiceParam);
							uint32_t length = 0;

							codeNumber = ((uint32_t)codeNumber >> goRiceParam) - COEF_REMAIN_BIN_REDUCTION;
							if (codeNumber >= 0)
							{
								{
									unsigned long cidx;
									//CLZ(cidx, codeNumber + 1);
									cidx = (unsigned long)clz(codeNumber + 1);
									length = cidx;
								}
								if ((codeNumber == 0) && (length != 0))
								{
									printf("length check failure\n");
									return;
								}

								codeNumber = (length + length);
							}
							entropy->m_fracBits += (COEF_REMAIN_BIN_REDUCTION + 1 + goRiceParam + codeNumber) << 15;

							if (absCoeff[idx] > (COEF_REMAIN_BIN_REDUCTION << goRiceParam))
								goRiceParam = (goRiceParam + 1) - (goRiceParam >> 2);
							if (goRiceParam > 4)
							{
								printf("goRiceParam check failure\n");
								return;
							}
						}
						if (absCoeff[idx] >= 2)
							firstCoeff2 = 0;
					}
				}
				else//按照扫描顺序对当前C内的每一个非零系数对应的语法元素coeff_abs_level_remaining进行编码，标准通道。
				{
					// Standard path
					int idx;
					for (idx = 0; idx < numNonZero; idx++)
					{
						int baseLevel = (baseLevelN & 3) | firstCoeff2;//确定每个非零系数的baseLevel=sig_coeff_flag+coeff_abs_level_greater1_flag+coeff_abs_level_greater2_flag
						if (baseLevel != ((idx < C1FLAG_NUMBER) ? (2 + firstCoeff2) : 1))//在当前CG内前8个非零系数内，每个非零系数的baseLevel=2 + firstCoeff2；后续的非零系数baseLevel=1.
						{
							printf("baseLevel check failurr\n");
							return;
						}
						baseLevelN >>= 2;

						if (absCoeff[idx] >= baseLevel)//coeff_abs_level_remaining=absCoeff[idx]-baseLevel>0.
						{
							writeCoefRemainExGolomb(entropy, absCoeff[idx] - baseLevel, goRiceParam);
							if (absCoeff[idx] >(COEF_REMAIN_BIN_REDUCTION << goRiceParam))
								goRiceParam = (goRiceParam + 1) - (goRiceParam >> 2);
							if (goRiceParam > 4)
							{
								printf("goRiceParam check failure\n");
								return;
							}
						}
						if (absCoeff[idx] >= 2)
							firstCoeff2 = 0;
					}
				}
			}
		}//非零系数的幅值信息编码结束
		// Initialize value for next loop
		numNonZero = 0;
		scanPosSigOff = (1 << MLS_CG_SIZE) - 1;//15
	}//其余系数的位置信息以及非零系数的幅值信息编码结束。
	//printf("finish codeCoeffNxN!\n");*/
}
void codeSaoMaxUvlc(Entropy* entropy, uint32_t code, uint32_t maxSymbol)
{/*
	X265_CHECK(maxSymbol > 0, "maxSymbol too small\n");

	uint32_t isCodeNonZero = !!code;

	encodeBinEP(entropy, isCodeNonZero);
	if (isCodeNonZero)
	{
		uint32_t isCodeLast = (maxSymbol > code);
		uint32_t mask = (1 << (code - 1)) - 1;
		uint32_t len = code - 1 + isCodeLast;
		mask <<= isCodeLast;

		encodeBinsEP(entropy, mask, len);
	}*/
}
void codeSaoOffset(Entropy* entropy, const SaoCtuParam* ctuParam, int plane)
{/*
	int typeIdx = ctuParam->typeIdx;

	if (plane != 2)
	{
		encodeBin(entropy, typeIdx >= 0, &entropy->m_contextState[OFF_SAO_TYPE_IDX_CTX]);
		if (typeIdx >= 0)
			encodeBinEP(entropy, typeIdx < SAO_BO ? 1 : 0);
	}

	if (typeIdx >= 0)
	{
		enum { OFFSET_THRESH = 1 << X265_MIN(X265_DEPTH - 5, 5) };
		if (typeIdx == SAO_BO)
		{
			for (int i = 0; i < SAO_BO_LEN; i++)
				codeSaoMaxUvlc(entropy, abs(ctuParam->offset[i]), OFFSET_THRESH - 1);

			for (int i = 0; i < SAO_BO_LEN; i++)
				if (ctuParam->offset[i] != 0)
					encodeBinEP(entropy, ctuParam->offset[i] < 0);

			encodeBinsEP(entropy, ctuParam->bandPos, 5);
		}
		else // if (typeIdx < SAO_BO)
		{
			codeSaoMaxUvlc(entropy, ctuParam->offset[0], OFFSET_THRESH - 1);
			codeSaoMaxUvlc(entropy, ctuParam->offset[1], OFFSET_THRESH - 1);
			codeSaoMaxUvlc(entropy, -ctuParam->offset[2], OFFSET_THRESH - 1);
			codeSaoMaxUvlc(entropy, -ctuParam->offset[3], OFFSET_THRESH - 1);
			if (plane != 2)
				encodeBinsEP(entropy, (uint32_t)(typeIdx), 2);
		}
	}*/
}
void Entropy_codeQtCbfChroma(Entropy* entropy, CUData* cu, uint32_t absPartIdx, enum TextType ttype, uint32_t tuDepth, bool lowestLevel)
{/*
	uint32_t ctx = tuDepth + 2;

	uint32_t log2TrSize = cu->m_log2CUSize[absPartIdx] - tuDepth;
	bool canQuadSplit = (log2TrSize - cu->m_hChromaShift > 2);
	uint32_t lowestTUDepth = tuDepth + ((!lowestLevel && !canQuadSplit) ? 1 : 0); // unsplittable TUs inherit their parent's CBF

	if (cu->m_chromaFormat == X265_CSP_I422 && (lowestLevel || !canQuadSplit)) // if sub-TUs are present
	{
		uint32_t subTUDepth = lowestTUDepth + 1;   // if this is the lowest level of the TU-tree, the sub-TUs are directly below.
		// Otherwise, this must be the level above the lowest level (as specified above)
		uint32_t tuNumParts = 1 << ((log2TrSize - LOG2_UNIT_SIZE) * 2 - 1);

		encodeBin(entropy, getCbf(cu, absPartIdx, ttype, subTUDepth), &entropy->m_contextState[OFF_QT_CBF_CTX + ctx]);
		encodeBin(entropy, getCbf(cu, absPartIdx + tuNumParts, ttype, subTUDepth), &entropy->m_contextState[OFF_QT_CBF_CTX + ctx]);
	}
	else
		encodeBin(entropy, getCbf(cu, absPartIdx, ttype, lowestTUDepth), &entropy->m_contextState[OFF_QT_CBF_CTX + ctx]);*/
}

void Entropy_codeQtCbfLuma(Entropy* entropy, CUData* cu, uint32_t absPartIdx, uint32_t tuDepth)
{/*
	codeQtCbfLuma(entropy, getCbf(cu, absPartIdx, TEXT_LUMA, tuDepth), tuDepth*/
}

void TURecurse_init(TURecurse *tURecurse, enum SplitType splitType, uint32_t _absPartIdxStep, uint32_t _absPartIdxTU)
{/*
	uint32_t partIdxStepShift[NUMBER_OF_SPLIT_MODES] = { 0, 1, 2 };
	tURecurse->section = 0;
	tURecurse->absPartIdxTURelCU = _absPartIdxTU;
	tURecurse->splitMode = (uint32_t)splitType;
	tURecurse->absPartIdxStep = _absPartIdxStep >> partIdxStepShift[tURecurse->splitMode];*/
}

char isNextSection(TURecurse *tURecurse)
{/*
	if (tURecurse->splitMode == DONT_SPLIT)
	{
	tURecurse->section++;
	return 0;
	}
	else
	{
	tURecurse->absPartIdxTURelCU += tURecurse->absPartIdxStep;

	tURecurse->section++;
	return tURecurse->section < (uint32_t)(1 << tURecurse->splitMode);
	}*/return 0;
}
char isLastSection(TURecurse *tURecurse)
{/*
	return (tURecurse->section + 1) >= (uint32_t)(1 << tURecurse->splitMode);*/return 0;
}
void encodeTransform(Entropy* entropy, CUData* cu, uint32_t absPartIdx, uint32_t curDepth, uint32_t log2CurSize, bool bCodeDQP, uint32_t depthRange[2])
{/*
	bool subdiv = cu->m_tuDepth[absPartIdx] > curDepth;
	// in each of these conditions, the subdiv flag is implied and not signaled,
	// so we have checks to make sure the implied value matches our intentions //
	if (isIntra_cudata(cu, absPartIdx) && cu->m_partSize[absPartIdx] != SIZE_2Nx2N && log2CurSize == MIN_LOG2_CU_SIZE)
	{
		if (!subdiv)
		{

			printf("intra NxN requires TU depth below CU depth\n");
			return;
		}
	}
	else
		if (isInter_cudata(cu, absPartIdx) && cu->m_partSize[absPartIdx] != SIZE_2Nx2N &&!curDepth && cu->m_slice->m_sps->quadtreeTUMaxDepthInter == 1)
		{
			if (!subdiv)
			{
				printf("inter TU must be smaller than CU when not 2Nx2N part size: log2CurSize %d, depthRange[0] %d\n", log2CurSize, depthRange[0]);
				return;
			}
		}
		else
			if (log2CurSize > depthRange[1])
			{
				if (!subdiv)
				{
					printf("TU is larger than the max allowed, it should have been split\n");
					return;
				}
			}
			else if (log2CurSize == cu->m_slice->m_sps->quadtreeTULog2MinSize || log2CurSize == depthRange[0])
			{
				if (subdiv)
				{
					printf("min sized TU cannot be subdivided\n");
					return;
				}
			}
			else
			{
				if (log2CurSize <= depthRange[0])
				{
					printf("transform size failure\n");
					return;
				}

				codeTransformSubdivFlag(entropy, subdiv, 5 - log2CurSize);
			}

			uint32_t hChromaShift = cu->m_hChromaShift;
			uint32_t vChromaShift = cu->m_vChromaShift;
			bool bSmallChroma = (log2CurSize - hChromaShift) < 2;
			if (!curDepth || !bSmallChroma)
			{
				if (!curDepth || getCbf(cu, absPartIdx, TEXT_CHROMA_U, curDepth - 1))
					Entropy_codeQtCbfChroma(entropy, cu, absPartIdx, TEXT_CHROMA_U, curDepth, !subdiv);
				if (!curDepth || getCbf(cu, absPartIdx, TEXT_CHROMA_V, curDepth - 1))
					Entropy_codeQtCbfChroma(entropy, cu, absPartIdx, TEXT_CHROMA_V, curDepth, !subdiv);
			}
			else
			{
				if (getCbf(cu, absPartIdx, TEXT_CHROMA_U, curDepth) != getCbf(cu, absPartIdx, TEXT_CHROMA_U, curDepth - 1))
				{
					printf("chroma xform size match failure\n");
					return;
				}
				if (getCbf(cu, absPartIdx, TEXT_CHROMA_V, curDepth) != getCbf(cu, absPartIdx, TEXT_CHROMA_V, curDepth - 1))
				{
					printf("chroma xform size match failure\n");
					return;
				}
			}

			if (subdiv)
			{
				--log2CurSize;
				++curDepth;

				uint32_t qNumParts = 1 << (log2CurSize - LOG2_UNIT_SIZE) * 2;

				encodeTransform(entropy, cu, absPartIdx + 0 * qNumParts, curDepth, log2CurSize, bCodeDQP, depthRange);
				encodeTransform(entropy, cu, absPartIdx + 1 * qNumParts, curDepth, log2CurSize, bCodeDQP, depthRange);
				encodeTransform(entropy, cu, absPartIdx + 2 * qNumParts, curDepth, log2CurSize, bCodeDQP, depthRange);
				encodeTransform(entropy, cu, absPartIdx + 3 * qNumParts, curDepth, log2CurSize, bCodeDQP, depthRange);
				return;
			}

			uint32_t absPartIdxC = bSmallChroma ? absPartIdx & 0xFC : absPartIdx;

			TURecurse tURecurse;

			if (isInter_cudata(cu, absPartIdxC) && !curDepth && !getCbf(cu, absPartIdxC, TEXT_CHROMA_U, 0) && !getCbf(cu, absPartIdxC, TEXT_CHROMA_V, 0))
			{
				if (!getCbf(cu, absPartIdxC, TEXT_LUMA, 0))
				{
					printf("CBF should have been set\n");
					return;
				}
			}
			else
				Entropy_codeQtCbfLuma(entropy, cu, absPartIdx, curDepth);

			uint32_t cbfY = getCbf(cu, absPartIdx, TEXT_LUMA, curDepth);
			uint32_t cbfU = getCbf(cu, absPartIdxC, TEXT_CHROMA_U, curDepth);
			uint32_t cbfV = getCbf(cu, absPartIdxC, TEXT_CHROMA_V, curDepth);
			if (!(cbfY || cbfU || cbfV))
				return;

			// dQP: only for CTU once
			if (cu->m_slice->m_pps->bUseDQP && bCodeDQP)
			{
				uint32_t log2CUSize = cu->m_log2CUSize[absPartIdx];
				uint32_t absPartIdxLT = absPartIdx & (0xFF << (log2CUSize - LOG2_UNIT_SIZE) * 2);
				codeDeltaQP(entropy, cu, absPartIdxLT);
				bCodeDQP = 0;
			}

			if (cbfY)
			{
				uint32_t coeffOffset = absPartIdx << (LOG2_UNIT_SIZE * 2);
				codeCoeffNxN(entropy, cu, cu->m_trCoeff[0] + coeffOffset, absPartIdx, log2CurSize, TEXT_LUMA);
				if (!(cbfU || cbfV))
					return;
			}

			if (bSmallChroma)
			{
				if ((absPartIdx & 3) != 3)
					return;

				uint32_t log2CurSizeC = 2;
				bool splitIntoSubTUs = (cu->m_chromaFormat == X265_CSP_I422);
				uint32_t curPartNum = 4;
				uint32_t coeffOffsetC = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (hChromaShift + vChromaShift));
				uint32_t chromaId = TEXT_CHROMA_U;
				for (; chromaId <= TEXT_CHROMA_V; chromaId++)
				{
					TURecurse_init(&tURecurse, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdxC);
					coeff_t* coeffChroma = cu->m_trCoeff[chromaId];
					do
					{
						if (getCbf(cu, tURecurse.absPartIdxTURelCU, (TextType)chromaId, curDepth + splitIntoSubTUs))
						{
							uint32_t subTUOffset = tURecurse.section << (log2CurSizeC * 2);
							codeCoeffNxN(entropy, cu, coeffChroma + coeffOffsetC + subTUOffset, tURecurse.absPartIdxTURelCU, log2CurSizeC, (TextType)chromaId);
						}
					} while (isNextSection(&tURecurse));
				}
			}
			else
			{
				uint32_t log2CurSizeC = log2CurSize - hChromaShift;
				bool splitIntoSubTUs = (cu->m_chromaFormat == X265_CSP_I422);
				uint32_t curPartNum = 1 << (log2CurSize - LOG2_UNIT_SIZE) * 2;
				uint32_t coeffOffsetC = absPartIdxC << (LOG2_UNIT_SIZE * 2 - (hChromaShift + vChromaShift));
				uint32_t chromaId = TEXT_CHROMA_U;
				for (; chromaId <= TEXT_CHROMA_V; chromaId++)
				{
					TURecurse_init(&tURecurse, splitIntoSubTUs ? VERTICAL_SPLIT : DONT_SPLIT, curPartNum, absPartIdxC);
					coeff_t* coeffChroma = cu->m_trCoeff[chromaId];
					do
					{
						if (getCbf(cu, tURecurse.absPartIdxTURelCU, (TextType)chromaId, curDepth + splitIntoSubTUs))
						{
							uint32_t subTUOffset = tURecurse.section << (log2CurSizeC * 2);
							codeCoeffNxN(entropy, cu, coeffChroma + coeffOffsetC + subTUOffset, tURecurse.absPartIdxTURelCU, log2CurSizeC, (TextType)chromaId);
						}
					} while (isNextSection(&tURecurse));
				}
			}*/
}
void codeCoeff(Entropy* entropy, CUData* cu, uint32_t absPartIdx, bool bCodeDQP, uint32_t depthRange[2])
{/*
	if (!isIntra_cudata(cu, absPartIdx))
	{
		if (!(cu->m_mergeFlag[absPartIdx] && cu->m_partSize[absPartIdx] == SIZE_2Nx2N))
			codeQtRootCbf(entropy, getQtRootCbf(cu, absPartIdx));
		if (!getQtRootCbf(cu, absPartIdx))
			return;
	}

	uint32_t log2CUSize = cu->m_log2CUSize[absPartIdx];
	encodeTransform(entropy, cu, absPartIdx, 0, log2CUSize, bCodeDQP, depthRange);*/
}
void codeIntraDirLumaAng(Entropy *entropy, CUData *cu, uint32_t absPartIdx, bool isMultiple)//编码帧内亮度
{/*
	uint32_t dir[4], j, i;
	uint32_t preds[4][3];
	int predIdx[4];
	uint32_t partNum = isMultiple && cu->m_partSize[absPartIdx] != SIZE_2Nx2N ? 4 : 1;//先!=
	uint32_t qNumParts = 1 << (cu->m_log2CUSize[absPartIdx] - 1 - LOG2_UNIT_SIZE) * 2;

	for (j = 0; j < partNum; j++, absPartIdx += qNumParts)
	{
		dir[j] = cu->m_lumaIntraDir[absPartIdx];
		CUData_getIntraDirLumaPredictor(cu, absPartIdx, preds[j]);
		predIdx[j] = -1;
		for (i = 0; i < 3; i++)
			if (dir[j] == preds[j][i])
				predIdx[j] = i;

		encodeBin(entropy, (predIdx[j] != -1) ? 1 : 0, &entropy->m_contextState[OFF_ADI_CTX]);
	}
	for (j = 0; j < partNum; j++)
	{
		if (predIdx[j] != -1)
		{
			if ((predIdx[j] <0) || (predIdx[j] > 2))
			{
				printf("predIdx out of range\n");
				return;
			}
			// NOTE: Mapping
			//       0 = 0
			//       1 = 10
			//       2 = 11
			int nonzero = (!!predIdx[j]);
			encodeBinsEP(entropy, predIdx[j] + nonzero, 1 + nonzero);
		}
		else//排序
		{
			if (preds[j][0] > preds[j][1])
				swap(&preds[j][0], &preds[j][1]);

			if (preds[j][0] > preds[j][2])
				swap(&preds[j][0], &preds[j][2]);

			if (preds[j][1] > preds[j][2])
				swap(&preds[j][1], &preds[j][2]);

			dir[j] += (dir[j] > preds[j][2]) ? -1 : 0;
			dir[j] += (dir[j] > preds[j][1]) ? -1 : 0;
			dir[j] += (dir[j] > preds[j][0]) ? -1 : 0;
			encodeBinsEP(entropy, dir[j], 5);
		}
	}*/
}


void codeIntraDirChroma(Entropy *entropy, const CUData* cu, uint32_t absPartIdx, uint32_t *chromaDirMode)
{/*
	int i;
	uint32_t intraDirChroma = cu->m_chromaIntraDir[absPartIdx];

	if (intraDirChroma == DM_CHROMA_IDX)
		encodeBin(entropy, 0, &entropy->m_contextState[OFF_CHROMA_PRED_CTX]);
	else
	{
		for (i = 0; i < NUM_CHROMA_MODE - 1; i++)
		{
			if (intraDirChroma == chromaDirMode[i])
			{
				intraDirChroma = i;
				break;
			}
		}

		encodeBin(entropy, 1, &entropy->m_contextState[OFF_CHROMA_PRED_CTX]);
		encodeBinsEP(entropy, intraDirChroma, 2);
	}*/
}
void codeInterDir(Entropy *entropy, const CUData* cu, uint32_t absPartIdx)//帧间
{/*
	const uint32_t interDir = cu->m_interDir[absPartIdx] - 1;
	const uint32_t ctx = cu->m_cuDepth[absPartIdx]; // the context of the inter dir is the depth of the CU

	if (cu->m_partSize[absPartIdx] == SIZE_2Nx2N || cu->m_log2CUSize[absPartIdx] != 3)
		encodeBin(entropy, interDir == 2 ? 1 : 0, &entropy->m_contextState[OFF_INTER_DIR_CTX + ctx]);
	if (interDir < 2)
		encodeBin(entropy, interDir, &entropy->m_contextState[OFF_INTER_DIR_CTX + 4]);*/
}
void codeRefFrmIdx(Entropy *entropy, CUData* cu, uint32_t absPartIdx, int list)
{/*
	uint32_t refFrame = cu->m_refIdx[list][absPartIdx];

	encodeBin(entropy, refFrame > 0, &entropy->m_contextState[OFF_REF_NO_CTX]);

	if (refFrame > 0)
	{
		uint32_t refNum = cu->m_slice->m_numRefIdx[list] - 2;
		if (refNum == 0)
			return;

		refFrame--;
		encodeBin(entropy, refFrame > 0, &entropy->m_contextState[OFF_REF_NO_CTX + 1]);
		if (refFrame > 0)
		{
			uint32_t mask = (1 << refFrame) - 2;
			mask >>= (refFrame == refNum) ? 1 : 0;
			encodeBinsEP(entropy, mask, refFrame - (refFrame == refNum));
		}
	}*/
}
/** encode reference frame index for a PU block */
void codeRefFrmIdxPU(Entropy *entropy, CUData* cu, uint32_t absPartIdx, int list)
{/*
	X265_CHECK(!isIntra_cudata(cu, absPartIdx), "intra block not expected\n");

	if (cu->m_slice->m_numRefIdx[list] > 1)
		codeRefFrmIdx(entropy, cu, absPartIdx, list);*/
}
void codeMvd(Entropy *entropy, const CUData* cu, uint32_t absPartIdx, int list)//帧间
{/*
	const MV* mvd = &cu->m_mvd[list][absPartIdx];
	const int hor = mvd->x;
	const int ver = mvd->y;

	encodeBin(entropy, hor != 0 ? 1 : 0, &entropy->m_contextState[OFF_MV_RES_CTX]);
	encodeBin(entropy, ver != 0 ? 1 : 0, &entropy->m_contextState[OFF_MV_RES_CTX]);

	const bool bHorAbsGr0 = hor != 0;
	const bool bVerAbsGr0 = ver != 0;
	const uint32_t horAbs = 0 > hor ? -hor : hor;
	const uint32_t verAbs = 0 > ver ? -ver : ver;

	if (bHorAbsGr0)
		encodeBin(entropy, horAbs > 1 ? 1 : 0, &entropy->m_contextState[OFF_MV_RES_CTX + 1]);

	if (bVerAbsGr0)
		encodeBin(entropy, verAbs > 1 ? 1 : 0, &entropy->m_contextState[OFF_MV_RES_CTX + 1]);

	if (bHorAbsGr0)
	{
		if (horAbs > 1)
			writeEpExGolomb(entropy, horAbs - 2, 1);

		encodeBinEP(entropy, 0 > hor ? 1 : 0);
	}

	if (bVerAbsGr0)
	{
		if (verAbs > 1)
			writeEpExGolomb(entropy, verAbs - 2, 1);

		encodeBinEP(entropy, 0 > ver ? 1 : 0);
	}*/
}
void codeMVPIdx(Entropy *entropy, uint32_t symbol)
{/*
	encodeBin(entropy, symbol, &entropy->m_contextState[OFF_MVP_IDX_CTX]);*/
}
void codeMergeFlag(Entropy *entropy, const CUData* cu, uint32_t absPartIdx)
{/*
	//cu->m_mergeFlag[absPartIdx]
	encodeBin(entropy, 0, &entropy->m_contextState[OFF_MERGE_FLAG_EXT_CTX]);*/
}
/** encode motion information for every PU block */
void codePUWise(Entropy *entropy, CUData* cu, uint32_t absPartIdx)
{/*
	X265_CHECK(!isIntra_cudata(cu, absPartIdx), "intra block not expected\n");
	PartSize partSize = (PartSize)cu->m_partSize[absPartIdx];
	uint32_t numPU = (partSize == SIZE_2Nx2N ? 1 : (partSize == SIZE_NxN ? 4 : 2));
	uint32_t depth = cu->m_cuDepth[absPartIdx];
	uint32_t puOffset = (g_puOffset[(uint32_t)(partSize)] << (g_unitSizeDepth - depth) * 2) >> 4;

	for (uint32_t puIdx = 0, subPartIdx = absPartIdx; puIdx < numPU; puIdx++, subPartIdx += puOffset)
	{
		codeMergeFlag(entropy, cu, subPartIdx);
		if (cu->m_mergeFlag[subPartIdx])
			codeMergeIndex(entropy, cu, subPartIdx);
		else
		{
			if (isInterB(cu->m_slice))
				codeInterDir(entropy, cu, subPartIdx);

			uint32_t interDir = 1;//cu->m_interDir[subPartIdx];
			for (uint32_t list = 0; list < 2; list++)
			{
				if (interDir & (1 << list))
				{
					X265_CHECK(cu->m_slice->m_numRefIdx[list] > 0, "numRefs should have been > 0\n");

					codeRefFrmIdxPU(entropy, cu, subPartIdx, list);
					codeMvd(entropy, cu, subPartIdx, list);
					codeMVPIdx(entropy, cu->m_mvpIdx[list][subPartIdx]);
				}
			}
		}
	}*/
}

void codePredInfo(Entropy *entropy, CUData *cu, uint32_t absPartIdx)
{/*
	if (isIntra_cudata(cu, absPartIdx)) // If it is intra mode, encode intra prediction mode.
	{
		codeIntraDirLumaAng(entropy, cu, absPartIdx, TRUE);
		if (cu->m_chromaFormat != X265_CSP_I400)
		{
			uint32_t chromaDirMode[NUM_CHROMA_MODE];
			CUData_getAllowedChromaDir(cu, absPartIdx, chromaDirMode);

			codeIntraDirChroma(entropy, cu, absPartIdx, chromaDirMode);

			if (cu->m_chromaFormat == X265_CSP_I444 && cu->m_partSize[absPartIdx] != SIZE_2Nx2N)
			{
				uint32_t qNumParts = 1 << (cu->m_log2CUSize[absPartIdx] - 1 - LOG2_UNIT_SIZE) * 2;
				uint32_t qIdx;
				for (qIdx = 1; qIdx < 4; ++qIdx)
				{
					absPartIdx += qNumParts;
					CUData_getAllowedChromaDir(cu, absPartIdx, chromaDirMode);
					codeIntraDirChroma(entropy, cu, absPartIdx, chromaDirMode);
				}
			}
		}
	}
	else // if it is inter mode, encode motion vector and reference index
		codePUWise(entropy, cu, absPartIdx);//帧间部分先不用考虑
	*/
}
void finishSlice(Entropy* entropy)
{/*
	encodeBinTrm(entropy, 1);
	finish(entropy);
	writeByteAlignment(entropy->syn.m_bitIf);*/
}
/* encode a CU block recursively */
void encodeCU(Entropy* entropy, CUData* cu, CUGeom* cuGeom, uint32_t absPartIdx, uint32_t depth, bool bEncodeDQP)
{/*
	const Slice* slice = cu->m_slice;

	int cuSplitFlag = 0;//!(cuGeom->flags & LEAF);//分割标志
	int cuUnsplitFlag =1; //!(cuGeom->flags & SPLIT_MANDATORY)//不分割标志

	//CU不继续分割时
	if (!cuUnsplitFlag)
	{
		uint32_t qNumParts = cuGeom->numPartitions >> 2;
		if (depth == slice->m_pps->maxCuDQPDepth && slice->m_pps->bUseDQP)//获取深度
			bEncodeDQP = 1;
		uint32_t qIdx;
		for (qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)//当前深度的4个CU
		{
			CUGeom *childGeom = cuGeom + cuGeom->childOffset + qIdx;
			if (childGeom->flags & PRESENT)
				encodeCU(entropy, cu, childGeom, absPartIdx, depth + 1, bEncodeDQP);//递归调用
		}
		return;
	}

	if (cuSplitFlag)
		codeSplitFlag(entropy, cu, absPartIdx, depth);

	if (depth < cu->m_cuDepth[absPartIdx] && depth < g_maxCUDepth)
	{
		uint32_t qNumParts = cuGeom->numPartitions >> 2;
		if (depth == slice->m_pps->maxCuDQPDepth && slice->m_pps->bUseDQP)
			bEncodeDQP = 1;
		uint32_t qIdx;
		for (qIdx = 0; qIdx < 4; ++qIdx, absPartIdx += qNumParts)
		{
			struct CUGeom* childGeom = cuGeom + cuGeom->childOffset + qIdx;
			encodeCU(entropy, cu, childGeom, absPartIdx, depth + 1, bEncodeDQP);
		}
		return;
	}

	if (depth <= slice->m_pps->maxCuDQPDepth && slice->m_pps->bUseDQP)
		bEncodeDQP = 1;

	if (slice->m_pps->bTransquantBypassEnabled)
		codeCUTransquantBypassFlag(entropy, cu->m_tqBypass[absPartIdx]);

	if (!Slice_isIntra(slice))
	{
		codeSkipFlag(entropy, cu, absPartIdx);
		if (isSkipped(cu, absPartIdx))
		{
			codeMergeIndex(entropy, cu, absPartIdx);
			finishCU(entropy, cu, absPartIdx, depth, bEncodeDQP);
			return;
		}
		codePredMode(entropy, cu->m_predMode[absPartIdx]);//===============================编码预测模式
	}

	codePartSize(entropy, cu, absPartIdx, depth);//===============================编码分割大小

	// prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
	codePredInfo(entropy, cu, absPartIdx);//===============================编码预测信息

	uint32_t tuDepthRange[2];
	if (isIntra_cudata(cu, absPartIdx))
		CUData_getIntraTUQtDepthRange(cu, tuDepthRange, absPartIdx);
	else
		CUData_getInterTUQtDepthRange(cu, tuDepthRange, absPartIdx);

	// Encode Coefficients, allow codeCoeff() to modify bEncodeDQP
	codeCoeff(entropy, cu, absPartIdx, bEncodeDQP, tuDepthRange); //===============================编码系数

	// --- write terminating bit ---
	finishCU(entropy, cu, absPartIdx, depth, bEncodeDQP);//===============================调用finishCU()，完成Bit的最终写入
*/}

void encodeCTU(Entropy* entropy, CUData* cu, CUGeom* cuGeom)
{/*
	bool bEncodeDQP = cu->m_slice->m_pps->bUseDQP;
	encodeCU(entropy, cu, cuGeom, 0, 0, bEncodeDQP);*/
}

uint32_t entropy_getNumberOfWrittenBits(Entropy* entropy)
{/*
	return (uint32_t)(entropy->m_fracBits >> 15);*/return 0;
}

const uint32_t g_entropyBits[128] =
{
	// Corrected table, most notably for last state
	0x07b23, 0x085f9, 0x074a0, 0x08cbc, 0x06ee4, 0x09354, 0x067f4, 0x09c1b, 0x060b0, 0x0a62a, 0x05a9c, 0x0af5b, 0x0548d, 0x0b955, 0x04f56, 0x0c2a9,
	0x04a87, 0x0cbf7, 0x045d6, 0x0d5c3, 0x04144, 0x0e01b, 0x03d88, 0x0e937, 0x039e0, 0x0f2cd, 0x03663, 0x0fc9e, 0x03347, 0x10600, 0x03050, 0x10f95,
	0x02d4d, 0x11a02, 0x02ad3, 0x12333, 0x0286e, 0x12cad, 0x02604, 0x136df, 0x02425, 0x13f48, 0x021f4, 0x149c4, 0x0203e, 0x1527b, 0x01e4d, 0x15d00,
	0x01c99, 0x166de, 0x01b18, 0x17017, 0x019a5, 0x17988, 0x01841, 0x18327, 0x016df, 0x18d50, 0x015d9, 0x19547, 0x0147c, 0x1a083, 0x0138e, 0x1a8a3,
	0x01251, 0x1b418, 0x01166, 0x1bd27, 0x01068, 0x1c77b, 0x00f7f, 0x1d18e, 0x00eda, 0x1d91a, 0x00e19, 0x1e254, 0x00d4f, 0x1ec9a, 0x00c90, 0x1f6e0,
	0x00c01, 0x1fef8, 0x00b5f, 0x208b1, 0x00ab6, 0x21362, 0x00a15, 0x21e46, 0x00988, 0x2285d, 0x00934, 0x22ea8, 0x008a8, 0x239b2, 0x0081d, 0x24577,
	0x007c9, 0x24ce6, 0x00763, 0x25663, 0x00710, 0x25e8f, 0x006a0, 0x26a26, 0x00672, 0x26f23, 0x005e8, 0x27ef8, 0x005ba, 0x284b5, 0x0055e, 0x29057,
	0x0050c, 0x29bab, 0x004c1, 0x2a674, 0x004a7, 0x2aa5e, 0x0046f, 0x2b32f, 0x0041f, 0x2c0ad, 0x003e7, 0x2ca8d, 0x003ba, 0x2d323, 0x0010c, 0x3bfbb
};//采用查表熵编码方式的表，其中每一个值都是一个估计值（24位）。
const uint8_t g_nextState[128][2] =
{
	{ 2, 1 }, { 0, 3 }, { 4, 0 }, { 1, 5 }, { 6, 2 }, { 3, 7 }, { 8, 4 }, { 5, 9 },
	{ 10, 4 }, { 5, 11 }, { 12, 8 }, { 9, 13 }, { 14, 8 }, { 9, 15 }, { 16, 10 }, { 11, 17 },
	{ 18, 12 }, { 13, 19 }, { 20, 14 }, { 15, 21 }, { 22, 16 }, { 17, 23 }, { 24, 18 }, { 19, 25 },
	{ 26, 18 }, { 19, 27 }, { 28, 22 }, { 23, 29 }, { 30, 22 }, { 23, 31 }, { 32, 24 }, { 25, 33 },
	{ 34, 26 }, { 27, 35 }, { 36, 26 }, { 27, 37 }, { 38, 30 }, { 31, 39 }, { 40, 30 }, { 31, 41 },
	{ 42, 32 }, { 33, 43 }, { 44, 32 }, { 33, 45 }, { 46, 36 }, { 37, 47 }, { 48, 36 }, { 37, 49 },
	{ 50, 38 }, { 39, 51 }, { 52, 38 }, { 39, 53 }, { 54, 42 }, { 43, 55 }, { 56, 42 }, { 43, 57 },
	{ 58, 44 }, { 45, 59 }, { 60, 44 }, { 45, 61 }, { 62, 46 }, { 47, 63 }, { 64, 48 }, { 49, 65 },
	{ 66, 48 }, { 49, 67 }, { 68, 50 }, { 51, 69 }, { 70, 52 }, { 53, 71 }, { 72, 52 }, { 53, 73 },
	{ 74, 54 }, { 55, 75 }, { 76, 54 }, { 55, 77 }, { 78, 56 }, { 57, 79 }, { 80, 58 }, { 59, 81 },
	{ 82, 58 }, { 59, 83 }, { 84, 60 }, { 61, 85 }, { 86, 60 }, { 61, 87 }, { 88, 60 }, { 61, 89 },
	{ 90, 62 }, { 63, 91 }, { 92, 64 }, { 65, 93 }, { 94, 64 }, { 65, 95 }, { 96, 66 }, { 67, 97 },
	{ 98, 66 }, { 67, 99 }, { 100, 66 }, { 67, 101 }, { 102, 68 }, { 69, 103 }, { 104, 68 }, { 69, 105 },
	{ 106, 70 }, { 71, 107 }, { 108, 70 }, { 71, 109 }, { 110, 70 }, { 71, 111 }, { 112, 72 }, { 73, 113 },
	{ 114, 72 }, { 73, 115 }, { 116, 72 }, { 73, 117 }, { 118, 74 }, { 75, 119 }, { 120, 74 }, { 75, 121 },
	{ 122, 74 }, { 75, 123 }, { 124, 76 }, { 77, 125 }, { 124, 76 }, { 77, 125 }, { 126, 126 }, { 127, 127 }
};
// [8 24] --> [stateMPS BitCost], [stateLPS BitCost]
const uint32_t g_entropyStateBits[128] =
{
	// Corrected table, most notably for last state
	0x01007b23, 0x000085f9, 0x020074a0, 0x00008cbc, 0x03006ee4, 0x01009354, 0x040067f4, 0x02009c1b,
	0x050060b0, 0x0200a62a, 0x06005a9c, 0x0400af5b, 0x0700548d, 0x0400b955, 0x08004f56, 0x0500c2a9,
	0x09004a87, 0x0600cbf7, 0x0a0045d6, 0x0700d5c3, 0x0b004144, 0x0800e01b, 0x0c003d88, 0x0900e937,
	0x0d0039e0, 0x0900f2cd, 0x0e003663, 0x0b00fc9e, 0x0f003347, 0x0b010600, 0x10003050, 0x0c010f95,
	0x11002d4d, 0x0d011a02, 0x12002ad3, 0x0d012333, 0x1300286e, 0x0f012cad, 0x14002604, 0x0f0136df,
	0x15002425, 0x10013f48, 0x160021f4, 0x100149c4, 0x1700203e, 0x1201527b, 0x18001e4d, 0x12015d00,
	0x19001c99, 0x130166de, 0x1a001b18, 0x13017017, 0x1b0019a5, 0x15017988, 0x1c001841, 0x15018327,
	0x1d0016df, 0x16018d50, 0x1e0015d9, 0x16019547, 0x1f00147c, 0x1701a083, 0x2000138e, 0x1801a8a3,
	0x21001251, 0x1801b418, 0x22001166, 0x1901bd27, 0x23001068, 0x1a01c77b, 0x24000f7f, 0x1a01d18e,
	0x25000eda, 0x1b01d91a, 0x26000e19, 0x1b01e254, 0x27000d4f, 0x1c01ec9a, 0x28000c90, 0x1d01f6e0,
	0x29000c01, 0x1d01fef8, 0x2a000b5f, 0x1e0208b1, 0x2b000ab6, 0x1e021362, 0x2c000a15, 0x1e021e46,
	0x2d000988, 0x1f02285d, 0x2e000934, 0x20022ea8, 0x2f0008a8, 0x200239b2, 0x3000081d, 0x21024577,
	0x310007c9, 0x21024ce6, 0x32000763, 0x21025663, 0x33000710, 0x22025e8f, 0x340006a0, 0x22026a26,
	0x35000672, 0x23026f23, 0x360005e8, 0x23027ef8, 0x370005ba, 0x230284b5, 0x3800055e, 0x24029057,
	0x3900050c, 0x24029bab, 0x3a0004c1, 0x2402a674, 0x3b0004a7, 0x2502aa5e, 0x3c00046f, 0x2502b32f,
	0x3d00041f, 0x2502c0ad, 0x3e0003e7, 0x2602ca8d, 0x3e0003ba, 0x2602d323, 0x3f00010c, 0x3f03bfbb
};//采用查表熵编码方式的表，其中每一个值都是一个估计值（32位，前8位是为了概率更新，后24位才是真正的估计值）。一般用在自适应编码函数中。
