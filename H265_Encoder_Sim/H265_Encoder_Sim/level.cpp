/*
* level.c
*
*  Created on: 2016-8-10
*      Author: Administrator
*/

#include "level.h"
#include <math.h>

typedef struct
{
	uint32_t maxLumaSamples;
	uint32_t maxLumaSamplesPerSecond;
	uint32_t maxBitrateMain;
	uint32_t maxBitrateHigh;
	uint32_t maxCpbSizeMain;
	uint32_t maxCpbSizeHigh;
	uint32_t minCompressionRatio;
	Name_name levelEnum;
	const char* name;
	int levelIdc;
} LevelSpec;

LevelSpec levels[] =
{
	{ 36864, 552960, 128, MAX_UINT, 350, MAX_UINT, 2, Name_LEVEL1, "1", 10 },
	{ 122880, 3686400, 1500, MAX_UINT, 1500, MAX_UINT, 2, Name_LEVEL2, "2", 20 },
	{ 245760, 7372800, 3000, MAX_UINT, 3000, MAX_UINT, 2, Name_LEVEL2_1, "2.1", 21 },
	{ 552960, 16588800, 6000, MAX_UINT, 6000, MAX_UINT, 2, Name_LEVEL3, "3", 30 },
	{ 983040, 33177600, 10000, MAX_UINT, 10000, MAX_UINT, 2, Name_LEVEL3_1, "3.1", 31 },
	{ 2228224, 66846720, 12000, 30000, 12000, 30000, 4, Name_LEVEL4, "4", 40 },
	{ 2228224, 133693440, 20000, 50000, 20000, 50000, 4, Name_LEVEL4_1, "4.1", 41 },
	{ 8912896, 267386880, 25000, 100000, 25000, 100000, 6, Name_LEVEL5, "5", 50 },
	{ 8912896, 534773760, 40000, 160000, 40000, 160000, 8, Name_LEVEL5_1, "5.1", 51 },
	{ 8912896, 1069547520, 60000, 240000, 60000, 240000, 8, Name_LEVEL5_2, "5.2", 52 },
	{ 35651584, 1069547520, 60000, 240000, 60000, 240000, 8, Name_LEVEL6, "6", 60 },
	{ 35651584, 2139095040, 120000, 480000, 120000, 480000, 8, Name_LEVEL6_1, "6.1", 61 },
	{ 35651584, 4278190080U, 240000, 800000, 240000, 800000, 6, Name_LEVEL6_2, "6.2", 62 },
	{ MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT, 1, Name_LEVEL8_5, "8.5", 85 },
};

/* determine minimum decoder level required to decode the described video */
void determineLevel(x265_param *param, VPS*vps)
{/*
	vps->maxTempSubLayers = param->bEnableTemporalSubLayers ? 2 : 1;
	if (param->internalCsp == X265_CSP_I420)
	{
		if (param->internalBitDepth == 8)
		{
			if (param->keyframeMax == 1 && param->maxNumReferences == 1)
				vps->ptl.profileIdc = MAINSTILLPICTURE;
			else
				vps->ptl.profileIdc = Profile_MAIN;
		}
		else if (param->internalBitDepth == 10)
			vps->ptl.profileIdc = MAIN10;
	}
	else
		vps->ptl.profileIdc = MAINREXT;

	// determine which profiles are compatible with this stream //

	memset(vps->ptl.profileCompatibilityFlag, 0, sizeof(vps->ptl.profileCompatibilityFlag));
	vps->ptl.profileCompatibilityFlag[vps->ptl.profileIdc] = TRUE;

	if (vps->ptl.profileIdc == MAIN10&&param->internalBitDepth == 8)
		vps->ptl.profileCompatibilityFlag[Profile_MAIN] = TRUE;
	else if (vps->ptl.profileIdc == Profile_MAIN)
		vps->ptl.profileCompatibilityFlag[MAIN10] = TRUE;
	else if (vps->ptl.profileIdc == MAINSTILLPICTURE)
	{
		vps->ptl.profileCompatibilityFlag[Profile_MAIN] = TRUE;
		vps->ptl.profileCompatibilityFlag[MAIN10] = TRUE;
	}
	else if (vps->ptl.profileIdc == MAINREXT)
		vps->ptl.profileCompatibilityFlag[MAINREXT] = TRUE;

	uint32_t lumaSamples = param->sourceWidth * param->sourceHeight;
	uint32_t bitrate = param->rc.vbvMaxBitrate ? param->rc.vbvMaxBitrate : param->rc.bitrate;

	const uint32_t MaxDpbPicBuf = 6;
	vps->ptl.levelIdc = Name_NONE;
	vps->ptl.tierFlag = Tier_MAIN;

	const unsigned int NumLevels = sizeof(levels) / sizeof(levels[0]);
	uint32_t i;
	if (param->bLossless)
	{
		i = 13;
		vps->ptl.minCrForLevel = 1;
		vps->ptl.maxLumaSrForLevel = MAX_UINT;
		vps->ptl.levelIdc = Name_LEVEL8_5;
		vps->ptl.tierFlag = Tier_MAIN;
	}
	else for (i = 0; i < NumLevels; i++)
	{
		if (lumaSamples > levels[i].maxLumaSamples)
			continue;
		// else if (samplesPerSec > levels[i].maxLumaSamplesPerSecond)
		//continue;
		else if (bitrate > levels[i].maxBitrateMain && levels[i].maxBitrateHigh == MAX_UINT)
			continue;
		else if (bitrate > levels[i].maxBitrateHigh)
			continue;
		else if (param->sourceWidth > sqrt(levels[i].maxLumaSamples * 8.0f))
			continue;
		else if (param->sourceHeight > sqrt(levels[i].maxLumaSamples * 8.0f))
			continue;

		uint32_t maxDpbSize = MaxDpbPicBuf;
		if (lumaSamples <= (levels[i].maxLumaSamples >> 2))
			maxDpbSize = X265_MIN(4 * MaxDpbPicBuf, 16);
		else if (lumaSamples <= (levels[i].maxLumaSamples >> 1))
			maxDpbSize = X265_MIN(2 * MaxDpbPicBuf, 16);
		else if (lumaSamples <= ((3 * levels[i].maxLumaSamples) >> 2))
			maxDpbSize = X265_MIN((4 * MaxDpbPicBuf) / 3, 16);

		// The value of sps_max_dec_pic_buffering_minus1[ HighestTid ] + 1 shall be less than
		// or equal to MaxDpbSize //
		if (vps->maxDecPicBuffering > maxDpbSize)
			continue;

		// For level 5 and higher levels, the value of CtbSizeY shall be equal to 32 or 64 //
		if (levels[i].levelEnum >= Name_LEVEL5 && param->maxCUSize < 32)
		{
			vps->ptl.profileIdc = Profile_NONE;
			vps->ptl.levelIdc = Name_NONE;
			vps->ptl.tierFlag = Tier_MAIN;
			return;
		}

		// The value of NumPocTotalCurr shall be less than or equal to 8 //
		int numPocTotalCurr = param->maxNumReferences + vps->numReorderPics;
		if (numPocTotalCurr > 8)
		{
			//x265_log(&param, X265_LOG_WARNING, "level %s detected, but NumPocTotalCurr (total references) is non-compliant\n", levels[i].name);
			vps->ptl.profileIdc = Profile_NONE;
			vps->ptl.levelIdc = Name_NONE;
			vps->ptl.tierFlag = Tier_MAIN;
			//x265_log(&param, X265_LOG_INFO, "NONE profile, Level-NONE (Main tier)\n");
			return;
		}

		vps->ptl.levelIdc = levels[i].levelEnum;
		vps->ptl.minCrForLevel = levels[i].minCompressionRatio;
		vps->ptl.maxLumaSrForLevel = levels[i].maxLumaSamplesPerSecond;
		break;
	}

	vps->ptl.intraConstraintFlag = FALSE;
	vps->ptl.lowerBitRateConstraintFlag = TRUE;
	vps->ptl.bitDepthConstraint = param->internalBitDepth;
	vps->ptl.chromaFormatConstraint = param->internalCsp;

	static const char *profiles[] = { "None", "Main", "Main 10", "Main Still Picture", "RExt" };
	static const char *tiers[] = { "Main", "High" };

	const char *profile = profiles[vps->ptl.profileIdc];
	if (vps->ptl.profileIdc == MAINREXT)
	{
		if (param->internalCsp == X265_CSP_I422)
			profile = "Main 4:2:2 10";
		if (param->internalCsp == X265_CSP_I444)
		{
			if (vps->ptl.bitDepthConstraint <= 8)
				profile = "Main 4:4:4 8";
			else if (vps->ptl.bitDepthConstraint <= 10)
				profile = "Main 4:4:4 10";
		}
	}*/
}

bool enforceLevel(x265_param* param, VPS* vps)
{/*
	vps->numReorderPics = (param->bBPyramid && param->bframes > 1) ? 2 : !!param->bframes;
	vps->maxDecPicBuffering = X265_MIN(MAX_NUM_REF, X265_MAX(vps->numReorderPics + 2, (uint32_t)param->maxNumReferences) + vps->numReorderPics);

	// no level specified by user, just auto-detect from the configuration //
	if (param->levelIdc <= 0)*/
		return TRUE;
}
