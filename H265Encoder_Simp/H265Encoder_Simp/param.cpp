#include "x265.h"
#include "string.h"
#include "constants.h"

const int x265_max_bit_depth = 8;
void x265_param_default(x265_param *param)
{
	memset(param, 0, sizeof(x265_param));

	param->decodedPictureHashSEI = 0;

	param->frameNumThreads = 1;

	param->internalBitDepth = x265_max_bit_depth;
	param->internalCsp = X265_CSP_I420;

	param->sourceWidth = 88;
	param->sourceHeight = 72;
	param->levelIdc = 0;
	param->bHighTier = 0;
	param->interlaceMode = 0;
	param->bAnnexB = 1;
	param->bRepeatHeaders = 0;
	param->bEnableAccessUnitDelimiters = 0;
	param->bEmitHRDSEI = 0;
	param->bEmitInfoSEI = 0;

	/* CU definitions */
	param->maxCUSize = 32;
	param->minCUSize = 32;
	param->tuQTMaxInterDepth = 1;
	param->tuQTMaxIntraDepth = 1;
	param->maxTUSize = 32;

	/* Coding Structure */
	param->keyframeMin = 0;
	param->keyframeMax = 5;
	param->bOpenGOP = 8;
	param->bframes = 0;
	param->lookaheadDepth = 10;
	param->bFrameAdaptive = 0;//X265_B_ADAPT_TRELLIS;
	param->bBPyramid = 0;
	param->scenecutThreshold = 40; /* Magic number pulled in from x264 */
	param->lookaheadSlices = 0;

	/* Intra Coding Tools */
	param->bEnableConstrainedIntra = 0;
	param->bEnableStrongIntraSmoothing = 1;
	param->bEnableFastIntra = 1;

	/* Inter Coding tools */
	param->searchMethod = X265_DIA_SEARCH;//X265_HEX_SEARCH;
	param->subpelRefine = 1;
	param->searchRange = 57;
	param->maxNumMergeCand = 2;
	param->bEnableWeightedPred = 0;
	param->bEnableWeightedBiPred = 0;
	param->bEnableEarlySkip = 0;
	param->bEnableAMP = 0;
	param->bEnableRectInter = 0;
	param->rdLevel = 2;
	param->rdoqLevel = 0;
	param->bEnableSignHiding = 0;
	param->bEnableTransformSkip = 0;
	param->bEnableTSkipFast = 0;
	param->maxNumReferences = 1;
	param->bEnableTemporalMvp = 0;

	/* Loop Filter */
	param->bEnableLoopFilter = 1;
	param->deblockingFilterBetaOffset = 0;
	param->deblockingFilterTCOffset = 0;

	/* SAO Loop Filter */
	param->bEnableSAO = 0;
	param->bSaoNonDeblocked = 0;

	/* Coding Quality */
	param->cbQpOffset = 0;
	param->crQpOffset = 0;
	param->rdPenalty = 0;
	param->psyRd = 0.3;
	param->psyRdoq = 0.0;
	param->analysisMode = 0;
	param->analysisFileName = NULL;
	param->bIntraInBFrames = 0;
	param->bLossless = 0;
	param->bCULossless = 0;
	param->bEnableTemporalSubLayers = 0;

	/* Rate control options */
	param->rc.vbvMaxBitrate = 0;
	param->rc.vbvBufferSize = 0;
	param->rc.vbvBufferInit = 0.9;
	param->rc.rfConstant = 28;
	param->rc.bitrate = 0;
	param->rc.qCompress = 0.6;
	param->rc.ipFactor = 1.4f;
	param->rc.pbFactor = 1.3f;
	param->rc.qpStep = 4;
	param->rc.rateControlMode = X265_RC_CRF;
	param->rc.qp = 32;
	param->rc.aqMode = 0;//X265_AQ_VARIANCE;
	param->rc.aqStrength = 0.0;
	param->rc.cuTree = 0;
	param->rc.rfConstantMax = 0;
	param->rc.rfConstantMin = 0;
	param->rc.bStatRead = 0;
	param->rc.bStatWrite = 0;
	param->rc.statFileName = NULL;
	param->rc.complexityBlur = 20;
	param->rc.qblur = 0.5;
	param->rc.zoneCount = 0;
	param->rc.zones = NULL;
	param->rc.bEnableSlowFirstPass = 0;
	param->rc.bStrictCbr = 0;
	param->rc.qgSize = 32; /* Same as maxCUSize */

	/* Video Usability Information (VUI) */
	param->vui.aspectRatioIdc = 0;
	param->vui.sarWidth = 0;
	param->vui.sarHeight = 0;
	param->vui.bEnableOverscanAppropriateFlag = 0;
	param->vui.bEnableVideoSignalTypePresentFlag = 0;
	param->vui.videoFormat = 5;
	param->vui.bEnableVideoFullRangeFlag = 0;
	param->vui.bEnableColorDescriptionPresentFlag = 0;
	param->vui.colorPrimaries = 2;
	param->vui.transferCharacteristics = 2;
	param->vui.matrixCoeffs = 2;
	param->vui.bEnableChromaLocInfoPresentFlag = 0;
	param->vui.chromaSampleLocTypeTopField = 0;
	param->vui.chromaSampleLocTypeBottomField = 0;
	param->vui.bEnableDefaultDisplayWindowFlag = 0;
	param->vui.defDispWinLeftOffset = 0;
	param->vui.defDispWinRightOffset = 0;
	param->vui.defDispWinTopOffset = 0;
	param->vui.defDispWinBottomOffset = 0;
	param->vui.bEnableOverscanInfoPresentFlag = 0;
}


int x265_set_globals(x265_param *param)
{
	uint32_t maxLog2CUSize = (uint32_t)g_log2Size[param->maxCUSize];
	uint32_t minLog2CUSize = (uint32_t)g_log2Size[param->minCUSize];
	
	// set max CU width & height
	g_maxCUSize = param->maxCUSize;
	g_maxLog2CUSize = maxLog2CUSize;

	// compute actual CU depth with respect to config depth and max transform size
	g_maxCUDepth = maxLog2CUSize - minLog2CUSize;
	g_unitSizeDepth = maxLog2CUSize - LOG2_UNIT_SIZE;

	// initialize partition order
	uint32_t* tmp = &g_zscanToRaster[0];
	uint32_t** ptmp = &tmp;
	initZscanToRaster(g_unitSizeDepth, 1, 0, ptmp);
	initRasterToZscan(g_unitSizeDepth);
	return 0;
}
