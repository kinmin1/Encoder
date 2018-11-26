/*
* cudata.c
*
*  Created on: 2015-10-13
*      Author: adminster
*/
#include "common.h"
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"
#include "mv.h"
#include "cudata.h"
#include <string.h>
#include "constants.h"
/* for all bcast* and copy* functions, dst and src are aligned to MIN(size, 32) */

const uint32_t nbPartsTable[8] = { 1, 2, 2, 4, 2, 2, 2, 2 };

void bcast1(uint8_t* dst, uint8_t val)
{
	dst[0] = val;
}

void copy4(uint8_t* dst, uint8_t* src)
{
	((uint32_t*)dst)[0] = ((uint32_t*)src)[0];
}

void bcast4(uint8_t* dst, uint8_t val)
{
	((uint32_t*)dst)[0] = 0x01010101 * val;
}

void copy16(uint8_t* dst, uint8_t* src)
{
	((uint64_t*)dst)[0] = ((uint64_t*)src)[0];
	((uint64_t*)dst)[1] = ((uint64_t*)src)[1];
}

void bcast16(uint8_t* dst, uint8_t val)
{
	uint64_t bval = val;
	((uint64_t*)dst)[0] = bval;
	((uint64_t*)dst)[1] = bval;
}

void copy64(uint8_t* dst, uint8_t* src)
{
((uint64_t*)dst)[0] = ((uint64_t*)src)[0];
((uint64_t*)dst)[1] = ((uint64_t*)src)[1];
((uint64_t*)dst)[2] = ((uint64_t*)src)[2];
((uint64_t*)dst)[3] = ((uint64_t*)src)[3];
((uint64_t*)dst)[4] = ((uint64_t*)src)[4];
((uint64_t*)dst)[5] = ((uint64_t*)src)[5];
((uint64_t*)dst)[6] = ((uint64_t*)src)[6];
((uint64_t*)dst)[7] = ((uint64_t*)src)[7];
((uint64_t*)dst)[8] = ((uint64_t*)src)[8];
((uint64_t*)dst)[9] = ((uint64_t*)src)[9];
((uint64_t*)dst)[10] = ((uint64_t*)src)[10];
((uint64_t*)dst)[11] = ((uint64_t*)src)[11];
((uint64_t*)dst)[12] = ((uint64_t*)src)[12];
((uint64_t*)dst)[13] = ((uint64_t*)src)[13];
((uint64_t*)dst)[14] = ((uint64_t*)src)[14];
((uint64_t*)dst)[15] = ((uint64_t*)src)[15];
((uint64_t*)dst)[16] = ((uint64_t*)src)[16];
((uint64_t*)dst)[17] = ((uint64_t*)src)[17];
((uint64_t*)dst)[18] = ((uint64_t*)src)[18];
((uint64_t*)dst)[19] = ((uint64_t*)src)[19];

((uint64_t*)dst)[20] = ((uint64_t*)src)[20];
((uint64_t*)dst)[21] = ((uint64_t*)src)[21];
((uint64_t*)dst)[22] = ((uint64_t*)src)[22];
((uint64_t*)dst)[23] = ((uint64_t*)src)[23];
((uint64_t*)dst)[24] = ((uint64_t*)src)[24];
((uint64_t*)dst)[25] = ((uint64_t*)src)[25];
((uint64_t*)dst)[26] = ((uint64_t*)src)[26];
((uint64_t*)dst)[27] = ((uint64_t*)src)[27];
((uint64_t*)dst)[28] = ((uint64_t*)src)[28];
((uint64_t*)dst)[29] = ((uint64_t*)src)[29];

((uint64_t*)dst)[30] = ((uint64_t*)src)[30];
((uint64_t*)dst)[31] = ((uint64_t*)src)[31];

}

void bcast64(uint8_t* dst, uint8_t val)
{
dst[0] = val; dst[1] = val;  dst[2] = val;
dst[3] = val; dst[4] = val;  dst[5] = val;
dst[6] = val; dst[7] = val;
dst[8] = val;  dst[9] = val; dst[10] = val;
dst[11] = val; dst[12] = val;dst[13] = val;
dst[14] = val; dst[15] = val;
dst[16] = val; dst[17] = val;dst[18] = val;
dst[19] = val; dst[20] = val;dst[21] = val;
dst[22] = val; dst[23] = val;
dst[24] = val; dst[25] = val;dst[26] = val;
dst[27] = val; dst[28] = val;dst[29] = val;
dst[30] = val; dst[31] = val;
}

/* at 256 bytes, memset/memcpy will probably use SIMD more effectively than our uint64_t hack,
* but hand-written assembly would beat it. */
void copy256(uint8_t* dst, uint8_t* src)
{
	memcpy(dst, src, 256);
}
void bcast256(uint8_t* dst, uint8_t val)
{
	memset(dst, val, 256);
}

/* Check whether 2 addresses point to the same column */
int isEqualCol(int addrA, int addrB, int numUnits)
{
	// addrA % numUnits == addrB % numUnits
	return ((addrA ^ addrB) &  (numUnits - 1)) == 0;
}

/* Check whether 2 addresses point to the same row */
int isEqualRow(int addrA, int addrB, int numUnits)
{
	// addrA / numUnits == addrB / numUnits
	return ((addrA ^ addrB) & ~(numUnits - 1)) == 0;
}

/* Check whether 2 addresses point to the same row or column */
int isEqualRowOrCol(int addrA, int addrB, int numUnits)
{
	return isEqualCol(addrA, addrB, numUnits) | isEqualRow(addrA, addrB, numUnits);
}

/* Check whether one address points to the first column */
int isZeroCol(int addr, int numUnits)
{
	// addr % numUnits == 0
	return (addr & (numUnits - 1)) == 0;
}

/* Check whether one address points to the first row */
int isZeroRow(int addr, int numUnits)
{
	// addr / numUnits == 0
	return (addr & ~(numUnits - 1)) == 0;
}

/* Check whether one address points to a column whose index is smaller than a given value */
int lessThanCol(int addr, int val, int numUnits)
{
	// addr % numUnits < val
	return (addr & (numUnits - 1)) < val;
}

/* Check whether one address points to a row whose index is smaller than a given value */
int lessThanRow(int addr, int val, int numUnits)
{
	// addr / numUnits < val
	return addr < val * numUnits;
}

void scaleMv(MV *mv, int scale)
{
	int x = x265_clip3(-32768, 32767, (scale * mv->x + 127 + (scale * mv->x < 0)) >> 8);
	int y = x265_clip3(-32768, 32767, (scale * mv->y + 127 + (scale * mv->y < 0)) >> 8);
	mv->x = x;
	mv->y = y;
	//return MV(mvx,mvy);
}

// Partition table.
// First index is partitioning mode. Second index is partition index.
// Third index is 0 for partition sizes, 1 for partition offsets. The
// sizes and offsets are encoded as two packed 4-bit values (X,Y).
// X and Y represent 1/4 fractions of the block size.
const uint32_t partTable[8][4][2] =
{
	//        XY
	{ { 0x44, 0x00 }, { 0x00, 0x00 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2Nx2N.
	{ { 0x42, 0x00 }, { 0x42, 0x02 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2NxN.
	{ { 0x24, 0x00 }, { 0x24, 0x20 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_Nx2N.
	{ { 0x22, 0x00 }, { 0x22, 0x20 }, { 0x22, 0x02 }, { 0x22, 0x22 } }, // SIZE_NxN.
	{ { 0x41, 0x00 }, { 0x43, 0x01 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2NxnU.
	{ { 0x43, 0x00 }, { 0x41, 0x03 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_2NxnD.
	{ { 0x14, 0x00 }, { 0x34, 0x10 }, { 0x00, 0x00 }, { 0x00, 0x00 } }, // SIZE_nLx2N.
	{ { 0x34, 0x00 }, { 0x14, 0x30 }, { 0x00, 0x00 }, { 0x00, 0x00 } }  // SIZE_nRx2N.
};

// Partition Address table.
// First index is partitioning mode. Second index is partition address.
const uint32_t partAddrTable[8][4] =
{
	{ 0x00, 0x00, 0x00, 0x00 }, // SIZE_2Nx2N.
	{ 0x00, 0x08, 0x08, 0x08 }, // SIZE_2NxN.
	{ 0x00, 0x04, 0x04, 0x04 }, // SIZE_Nx2N.
	{ 0x00, 0x04, 0x08, 0x0C }, // SIZE_NxN.
	{ 0x00, 0x02, 0x02, 0x02 }, // SIZE_2NxnU.
	{ 0x00, 0x0A, 0x0A, 0x0A }, // SIZE_2NxnD.
	{ 0x00, 0x01, 0x01, 0x01 }, // SIZE_nLx2N.
	{ 0x00, 0x05, 0x05, 0x05 }  // SIZE_nRx2N.
};


void CUData_CUData(struct CUData *cu)
{
	memset(cu, 0, sizeof(cu));
}

int CUDataMemPool_create_analysis(CUDataMemPool *MemPool, uint32_t depth, uint32_t numInstances)
{
	/*
	uint32_t numPartition = NUM_4x4_PARTITIONS >> (depth * 2);
	uint32_t cuSize = g_maxCUSize >> depth;
	uint32_t sizeL = cuSize * cuSize;
	uint32_t sizeC = sizeL >> 1;

	MemPool->trCoeffMemBlock = (coeff_t *)malloc((sizeL + sizeC * 2) * numInstances);
	if (!MemPool->trCoeffMemBlock)
	{
		printf("malloc of size %d failed\n", sizeof(coeff_t) * (sizeL + sizeC * 2)* numInstances);
		goto fail;
	}

	MemPool->charMemBlock = (uint8_t *)malloc(numPartition * numInstances * BytesPerPartition);
	if (!MemPool->charMemBlock)
	{
		printf("malloc of size %d failed\n", sizeof(uint8_t) * (numPartition * numInstances * BytesPerPartition));
		goto fail;
	}

	MemPool->mvMemBlock = (MV *)malloc(numPartition * 4 * numInstances);
	if (!MemPool->mvMemBlock)
	{
		printf("malloc of size %d failed\n", sizeof(MV) * (numPartition * 4 * numInstances));
		goto fail;
	}

	return TRUE;

fail:
	return FALSE;*/
	return 0;
}

int CUDataMemPool_create_frame(CUDataMemPool *MemPool, uint32_t depth, uint32_t numInstances)
{/*

	uint32_t numPartition = NUM_4x4_PARTITIONS >> (depth * 2);
	uint32_t cuSize = g_maxCUSize >> depth;
	uint32_t sizeL = cuSize * cuSize;
	uint32_t sizeC = sizeL >> 1;

	MemPool->trCoeffMemBlock = (coeff_t *)malloc((sizeL + sizeC * 2) * numInstances);
	//CHECKED_MALLOC(trCoeffMemBlock, coeff_t, (sizeL + sizeC * 2) * numInstances);
	//CHECKED_MALLOC(charMemBlock, uint8_t, numPartition * numInstances * CUData::BytesPerPartition);
	//CHECKED_MALLOC(mvMemBlock, MV, numPartition * 4 * numInstances);
	if (!MemPool->trCoeffMemBlock)
	{
	printf("malloc of size %d failed\n", sizeof(coeff_t) * (sizeL + sizeC * 2));
	goto fail;
	}

	MemPool->charMemBlock = (uint8_t *)malloc(numPartition * numInstances * BytesPerPartition);
	if (!MemPool->charMemBlock)
	{
	printf("malloc of size %d failed\n", sizeof(uint8_t) * (numPartition * numInstances * BytesPerPartition));
	goto fail;
	}

	MemPool->mvMemBlock = (MV *)malloc(numPartition * 4 * numInstances);
	if (!MemPool->mvMemBlock)
	{
	printf("malloc of size %d failed\n", sizeof(MV) * (numPartition * 4 * numInstances));
	goto fail;
	}

	return TRUE;

	fail:
	return FALSE;*/return 0;
}

void CUData_initialize(struct CUData *cu, struct CUDataMemPool *dataPool, uint32_t depth, int instance)
{/*
	cu->m_chromaFormat = 1;
	cu->m_hChromaShift = 1;
	cu->m_vChromaShift = 1;
	cu->s_numPartInCUSize = 1 << g_unitSizeDepth;
	cu->m_numPartitions = NUM_4x4_PARTITIONS >> (depth * 2);
	if (!cu->s_partSet[0])
	{
		switch (g_maxLog2CUSize)
		{
		case 6:
			cu->s_partSet[0] = bcast256;
			cu->s_partSet[1] = bcast64;
			cu->s_partSet[2] = bcast16;
			cu->s_partSet[3] = bcast4;
			cu->s_partSet[4] = bcast1;
			break;
		case 5:
			cu->s_partSet[0] = bcast64;
			cu->s_partSet[1] = bcast16;
			cu->s_partSet[2] = bcast4;
			cu->s_partSet[3] = bcast1;
			cu->s_partSet[4] = NULL;
			break;
		case 4:
			cu->s_partSet[0] = bcast16;
			cu->s_partSet[1] = bcast4;
			cu->s_partSet[2] = bcast1;
			cu->s_partSet[3] = NULL;
			cu->s_partSet[4] = NULL;
			break;
		default:
			printf("unexpected CTU size\n");
			break;
		}
	}

	switch (cu->m_numPartitions)
	{
	case 256: // 64x64 CU
		cu->m_partCopy = copy256;
		cu->m_partSet = bcast256;
		cu->m_subPartCopy = copy64;
		cu->m_subPartSet = bcast64;
		break;
	case 64:  // 32x32 CU
		cu->m_partCopy = copy64;
		cu->m_partSet = bcast64;
		cu->m_subPartCopy = copy16;
		cu->m_subPartSet = bcast16;
		break;
	case 16:  // 16x16 CU
		cu->m_partCopy = copy16;
		cu->m_partSet = bcast16;
		cu->m_subPartCopy = copy4;
		cu->m_subPartSet = bcast4;
		break;
	case 4:   // 8x8 CU
		cu->m_partCopy = copy4;
		cu->m_partSet = bcast4;
		cu->m_subPartCopy = NULL;
		cu->m_subPartSet = NULL;
		break;
	default:
		printf("unexpected CU partition count\n");
		break;
	}

	uint8_t *charBuf = dataPool->charMemBlock + (cu->m_numPartitions * 21) * instance;
	cu->m_qp = (int8_t*)charBuf; charBuf += cu->m_numPartitions;
	cu->m_log2CUSize = charBuf; charBuf += cu->m_numPartitions;
	cu->m_lumaIntraDir = charBuf; charBuf += cu->m_numPartitions;
	cu->m_tqBypass = charBuf; charBuf += cu->m_numPartitions;
	cu->m_refIdx[0] = (int8_t*)charBuf; charBuf += cu->m_numPartitions;
	cu->m_refIdx[1] = (int8_t*)charBuf; charBuf += cu->m_numPartitions;
	cu->m_cuDepth = charBuf; charBuf += cu->m_numPartitions;
	cu->m_predMode = charBuf; charBuf += cu->m_numPartitions; // the order up to here is important in initCTU() and initSubCU() //
	cu->m_partSize = charBuf; charBuf += cu->m_numPartitions;
	cu->m_mergeFlag = charBuf; charBuf += cu->m_numPartitions;
	cu->m_interDir = charBuf; charBuf += cu->m_numPartitions;
	cu->m_mvpIdx[0] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_mvpIdx[1] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_tuDepth = charBuf; charBuf += cu->m_numPartitions;
	cu->m_transformSkip[0] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_transformSkip[1] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_transformSkip[2] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_cbf[0] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_cbf[1] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_cbf[2] = charBuf; charBuf += cu->m_numPartitions;
	cu->m_chromaIntraDir = charBuf; charBuf += cu->m_numPartitions;

	if (!(charBuf == dataPool->charMemBlock + (cu->m_numPartitions * BytesPerPartition) * (instance + 1)))
		printf("CU data layout is broken\n");

	cu->m_mv[0] = dataPool->mvMemBlock + (instance * 4) * cu->m_numPartitions;
	cu->m_mv[1] = cu->m_mv[0] + cu->m_numPartitions;
	cu->m_mvd[0] = cu->m_mv[1] + cu->m_numPartitions;
	cu->m_mvd[1] = cu->m_mvd[0] + cu->m_numPartitions;

	uint32_t cuSize = g_maxCUSize >> depth;
	uint32_t sizeL = cuSize * cuSize;
	uint32_t sizeC = sizeL >> (cu->m_hChromaShift + cu->m_vChromaShift);
	cu->m_trCoeff[0] = dataPool->trCoeffMemBlock + instance * (sizeL + sizeC * 2);
	cu->m_trCoeff[1] = cu->m_trCoeff[0] + sizeL;
	cu->m_trCoeff[2] = cu->m_trCoeff[0] + sizeL + sizeC;*/
}

void CUData_initCTU(CUData* cu, struct Frame* frame, uint32_t cuAddr, int qp)
{
	/*
	cu->m_encData = frame->m_encData;
	cu->m_slice = cu->m_encData->m_slice;
	cu->m_cuAddr = cuAddr;
	cu->m_cuPelX = (cuAddr %cu->m_slice->m_sps->numCuInWidth) << g_maxLog2CUSize;
	cu->m_cuPelY = (cuAddr / cu->m_slice->m_sps->numCuInWidth) << g_maxLog2CUSize;
	cu->m_absIdxInCTU = 0;
	cu->m_numPartitions = NUM_4x4_PARTITIONS;

	// sequential memsets //
	cu->m_partSet((uint8_t*)cu->m_qp, (uint8_t)qp);
	cu->m_partSet(cu->m_log2CUSize, (uint8_t)g_maxLog2CUSize);
	cu->m_partSet(cu->m_lumaIntraDir, (uint8_t)DC_IDX);
	cu->m_partSet(cu->m_tqBypass, (uint8_t)frame->m_encData->m_param->bLossless);
	if (cu->m_slice->m_sliceType != I_SLICE)
	{
		cu->m_partSet((uint8_t*)cu->m_refIdx[0], (uint8_t)REF_NOT_VALID);
		cu->m_partSet((uint8_t*)cu->m_refIdx[1], (uint8_t)REF_NOT_VALID);
	}

	if (!(!(frame->m_encData->m_param->bLossless && !cu->m_slice->m_pps->bTransquantBypassEnabled)))
		printf("lossless enabled without TQbypass in PPS\n");

	// initialize the remaining CU data in one memset //
	memset(cu->m_cuDepth, 0, (BytesPerPartition - 6) * cu->m_numPartitions);

	uint32_t widthInCU = cu->m_slice->m_sps->numCuInWidth;
	cu->m_cuLeft = (cu->m_cuAddr % widthInCU) ? framedata_getPicCTU(cu->m_encData, cu->m_cuAddr - 1) : NULL;
	cu->m_cuAbove = (cu->m_cuAddr / widthInCU) ? framedata_getPicCTU(cu->m_encData, cu->m_cuAddr - widthInCU) : NULL;
	cu->m_cuAboveLeft = (cu->m_cuLeft && cu->m_cuAbove) ? framedata_getPicCTU(cu->m_encData, cu->m_cuAddr - widthInCU - 1) : NULL;
	cu->m_cuAboveRight = (cu->m_cuAbove && ((cu->m_cuAddr % widthInCU) < (widthInCU - 1))) ? framedata_getPicCTU(cu->m_encData, cu->m_cuAddr - widthInCU + 1) : NULL;
	*/
}


// initialize Sub partition
void CUData_initSubCU(struct CUData *cu, struct CUData* ctu, struct CUGeom* cuGeom, int qp)
{
	/*
	cu->m_absIdxInCTU = cuGeom->absPartIdx;
	cu->m_encData = ctu->m_encData;
	cu->m_slice = ctu->m_slice;
	cu->m_cuAddr = ctu->m_cuAddr;
	cu->m_cuPelX = ctu->m_cuPelX + g_zscanToPelX[cuGeom->absPartIdx];
	cu->m_cuPelY = ctu->m_cuPelY + g_zscanToPelY[cuGeom->absPartIdx];
	cu->m_cuLeft = ctu->m_cuLeft;
	cu->m_cuAbove = ctu->m_cuAbove;
	cu->m_cuAboveLeft = ctu->m_cuAboveLeft;
	cu->m_cuAboveRight = ctu->m_cuAboveRight;

	X265_CHECK(cu->m_numPartitions == cuGeom->numPartitions, "initSubCU() size mismatch\n");

	cu->m_partSet((uint8_t*)cu->m_qp, (uint8_t)qp);

	cu->m_partSet(cu->m_log2CUSize, (uint8_t)cuGeom->log2CUSize);
	cu->m_partSet(cu->m_lumaIntraDir, (uint8_t)DC_IDX);
	cu->m_partSet(cu->m_tqBypass, (uint8_t)ctu->m_encData->m_param->bLossless);
	cu->m_partSet((uint8_t*)cu->m_refIdx[0], (uint8_t)REF_NOT_VALID);
	cu->m_partSet((uint8_t*)cu->m_refIdx[1], (uint8_t)REF_NOT_VALID);
	cu->m_partSet(cu->m_cuDepth, (uint8_t)cuGeom->depth);

	// initialize the remaining CU data in one memset //
	memset(cu->m_predMode, 0, (BytesPerPartition - 7) * cu->m_numPartitions);*/
}

/* Copy the results of a sub-part (split) CU to the parent CU */
void CUData_copyPartFrom(struct CUData *cu, const struct CUData* subCU, const struct CUGeom* childGeom, uint32_t subPartIdx)
{
	/*
	if (subPartIdx < 4)
		printf("part unit should be less than 4\n");

	uint32_t offset = childGeom->numPartitions * subPartIdx;

	cu->m_subPartCopy((uint8_t*)cu->m_qp + offset, (uint8_t*)subCU->m_qp);
	cu->m_subPartCopy(cu->m_log2CUSize + offset, subCU->m_log2CUSize);
	cu->m_subPartCopy(cu->m_lumaIntraDir + offset, subCU->m_lumaIntraDir);
	cu->m_subPartCopy(cu->m_tqBypass + offset, subCU->m_tqBypass);
	cu->m_subPartCopy((uint8_t*)cu->m_refIdx[0] + offset, (uint8_t*)subCU->m_refIdx[0]);
	cu->m_subPartCopy((uint8_t*)cu->m_refIdx[1] + offset, (uint8_t*)subCU->m_refIdx[1]);
	cu->m_subPartCopy(cu->m_cuDepth + offset, subCU->m_cuDepth);
	cu->m_subPartCopy(cu->m_predMode + offset, subCU->m_predMode);
	cu->m_subPartCopy(cu->m_partSize + offset, subCU->m_partSize);
	cu->m_subPartCopy(cu->m_mergeFlag + offset, subCU->m_mergeFlag);
	cu->m_subPartCopy(cu->m_interDir + offset, subCU->m_interDir);
	cu->m_subPartCopy(cu->m_mvpIdx[0] + offset, subCU->m_mvpIdx[0]);
	cu->m_subPartCopy(cu->m_mvpIdx[1] + offset, subCU->m_mvpIdx[1]);
	cu->m_subPartCopy(cu->m_tuDepth + offset, subCU->m_tuDepth);
	cu->m_subPartCopy(cu->m_transformSkip[0] + offset, subCU->m_transformSkip[0]);
	cu->m_subPartCopy(cu->m_transformSkip[1] + offset, subCU->m_transformSkip[1]);
	cu->m_subPartCopy(cu->m_transformSkip[2] + offset, subCU->m_transformSkip[2]);
	cu->m_subPartCopy(cu->m_cbf[0] + offset, subCU->m_cbf[0]);
	cu->m_subPartCopy(cu->m_cbf[1] + offset, subCU->m_cbf[1]);
	cu->m_subPartCopy(cu->m_cbf[2] + offset, subCU->m_cbf[2]);
	cu->m_subPartCopy(cu->m_chromaIntraDir + offset, subCU->m_chromaIntraDir);

	memcpy(cu->m_mv[0] + offset, subCU->m_mv[0], childGeom->numPartitions * sizeof(struct MV));
	memcpy(cu->m_mv[1] + offset, subCU->m_mv[1], childGeom->numPartitions * sizeof(struct MV));
	memcpy(cu->m_mvd[0] + offset, subCU->m_mvd[0], childGeom->numPartitions * sizeof(struct MV));
	memcpy(cu->m_mvd[1] + offset, subCU->m_mvd[1], childGeom->numPartitions * sizeof(struct MV));

	uint32_t tmp = 1 << ((g_maxLog2CUSize - childGeom->depth) * 2);
	uint32_t tmp2 = subPartIdx * tmp;
	memcpy(cu->m_trCoeff[0] + tmp2, subCU->m_trCoeff[0], sizeof(coeff_t) * tmp);

	uint32_t tmpC = tmp >> ((cu->m_hChromaShift = 1) + (cu->m_vChromaShift = 1));
	uint32_t tmpC2 = tmp2 >> ((cu->m_hChromaShift = 1) + (cu->m_vChromaShift = 1));
	memcpy(cu->m_trCoeff[1] + tmpC2, subCU->m_trCoeff[1], sizeof(coeff_t) * tmpC);
	memcpy(cu->m_trCoeff[2] + tmpC2, subCU->m_trCoeff[2], sizeof(coeff_t) * tmpC);*/
}

/* If a sub-CU part is not present (off the edge of the picture) its depth and
* log2size should still be configured */
void CUData_setEmptyPart(struct CUData *cu, const struct CUGeom* childGeom, uint32_t subPartIdx)
{/*
	uint32_t offset = childGeom->numPartitions * subPartIdx;
	cu->m_subPartSet(cu->m_cuDepth + offset, (uint8_t)childGeom->depth);
	cu->m_subPartSet(cu->m_log2CUSize + offset, (uint8_t)childGeom->log2CUSize);*/
}

/* Copy all CU data from one instance to the next, except set lossless flag
* This will only get used when --cu-lossless is enabled but --lossless is not. */
void CUData_initLosslessCU(struct CUData *ctu, const struct CUData* cu, const struct CUGeom* cuGeom)
{
	/*
	// Start by making an exact copy //
	ctu->m_encData = cu->m_encData;
	ctu->m_slice = cu->m_slice;
	ctu->m_cuAddr = cu->m_cuAddr;
	ctu->m_cuPelX = cu->m_cuPelX;
	ctu->m_cuPelY = cu->m_cuPelY;
	ctu->m_cuLeft = cu->m_cuLeft;
	ctu->m_cuAbove = cu->m_cuAbove;
	ctu->m_cuAboveLeft = cu->m_cuAboveLeft;
	ctu->m_cuAboveRight = cu->m_cuAboveRight;
	ctu->m_absIdxInCTU = cuGeom->absPartIdx;
	ctu->m_numPartitions = cuGeom->numPartitions;
	memcpy(ctu->m_qp, cu->m_qp, BytesPerPartition *ctu->m_numPartitions);
	memcpy(ctu->m_mv[0], cu->m_mv[0], ctu->m_numPartitions * sizeof(struct MV));
	memcpy(ctu->m_mv[1], cu->m_mv[1], ctu->m_numPartitions * sizeof(struct MV));
	memcpy(ctu->m_mvd[0], cu->m_mvd[0], ctu->m_numPartitions * sizeof(struct MV));
	memcpy(ctu->m_mvd[1], cu->m_mvd[1], ctu->m_numPartitions * sizeof(struct MV));

	// force TQBypass to true //
	cu->m_partSet(ctu->m_tqBypass, 1);

	// clear residual coding flags //
	cu->m_partSet(ctu->m_predMode, cu->m_predMode[0] & (MODE_INTRA | MODE_INTER));
	cu->m_partSet(ctu->m_tuDepth, 0);
	cu->m_partSet(ctu->m_transformSkip[0], 0);
	cu->m_partSet(ctu->m_transformSkip[1], 0);
	cu->m_partSet(ctu->m_transformSkip[2], 0);
	cu->m_partSet(ctu->m_cbf[0], 0);
	cu->m_partSet(ctu->m_cbf[1], 0);
	cu->m_partSet(ctu->m_cbf[2], 0);*/
}

void x265_memcpy(void *to, const void *from, unsigned int count);
/* Copy completed predicted CU to CTU in picture */
void CUData_copyToPic(CUData *cu, uint32_t depth)
{/*
	CUData* ctu = framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);

	cu->m_partCopy((uint8_t*)ctu->m_qp + cu->m_absIdxInCTU, (uint8_t*)cu->m_qp);
	cu->m_partCopy(ctu->m_log2CUSize + cu->m_absIdxInCTU, cu->m_log2CUSize);
	cu->m_partCopy(ctu->m_lumaIntraDir + cu->m_absIdxInCTU, cu->m_lumaIntraDir);
	cu->m_partCopy(ctu->m_tqBypass + cu->m_absIdxInCTU, cu->m_tqBypass);
	cu->m_partCopy((uint8_t*)ctu->m_refIdx[0] + cu->m_absIdxInCTU, (uint8_t*)cu->m_refIdx[0]);
	cu->m_partCopy((uint8_t*)ctu->m_refIdx[1] + cu->m_absIdxInCTU, (uint8_t*)cu->m_refIdx[1]);
	cu->m_partCopy(ctu->m_cuDepth + cu->m_absIdxInCTU, cu->m_cuDepth);
	cu->m_partCopy(ctu->m_predMode + cu->m_absIdxInCTU, cu->m_predMode);
	cu->m_partCopy(ctu->m_partSize + cu->m_absIdxInCTU, cu->m_partSize);
	cu->m_partCopy(ctu->m_mergeFlag + cu->m_absIdxInCTU, cu->m_mergeFlag);
	cu->m_partCopy(ctu->m_interDir + cu->m_absIdxInCTU, cu->m_interDir);
	cu->m_partCopy(ctu->m_mvpIdx[0] + cu->m_absIdxInCTU, cu->m_mvpIdx[0]);
	cu->m_partCopy(ctu->m_mvpIdx[1] + cu->m_absIdxInCTU, cu->m_mvpIdx[1]);
	cu->m_partCopy(ctu->m_tuDepth + cu->m_absIdxInCTU, cu->m_tuDepth);
	cu->m_partCopy(ctu->m_transformSkip[0] + cu->m_absIdxInCTU, cu->m_transformSkip[0]);
	cu->m_partCopy(ctu->m_transformSkip[1] + cu->m_absIdxInCTU, cu->m_transformSkip[1]);
	cu->m_partCopy(ctu->m_transformSkip[2] + cu->m_absIdxInCTU, cu->m_transformSkip[2]);
	cu->m_partCopy(ctu->m_cbf[0] + cu->m_absIdxInCTU, cu->m_cbf[0]);
	cu->m_partCopy(ctu->m_cbf[1] + cu->m_absIdxInCTU, cu->m_cbf[1]);
	cu->m_partCopy(ctu->m_cbf[2] + cu->m_absIdxInCTU, cu->m_cbf[2]);
	cu->m_partCopy(ctu->m_chromaIntraDir + cu->m_absIdxInCTU, cu->m_chromaIntraDir);

	memcpy(ctu->m_mv[0] + cu->m_absIdxInCTU, cu->m_mv[0], cu->m_numPartitions * sizeof(struct MV));
	memcpy(ctu->m_mv[1] + cu->m_absIdxInCTU, cu->m_mv[1], cu->m_numPartitions * sizeof(struct MV));
	memcpy(ctu->m_mvd[0] + cu->m_absIdxInCTU, cu->m_mvd[0], cu->m_numPartitions * sizeof(struct MV));
	memcpy(ctu->m_mvd[1] + cu->m_absIdxInCTU, cu->m_mvd[1], cu->m_numPartitions * sizeof(struct MV));

	uint32_t tmpY = 1 << ((g_maxLog2CUSize - depth) * 2);
	uint32_t tmpY2 = cu->m_absIdxInCTU << (LOG2_UNIT_SIZE * 2);
	memcpy(ctu->m_trCoeff[0] + tmpY2, cu->m_trCoeff[0], sizeof(coeff_t) * tmpY);

	uint32_t tmpC = tmpY >> ((cu->m_hChromaShift = 1) + (cu->m_vChromaShift = 1));
	uint32_t tmpC2 = tmpY2 >> ((cu->m_hChromaShift = 1) + (cu->m_vChromaShift = 1));
	memcpy(ctu->m_trCoeff[1] + tmpC2, cu->m_trCoeff[1], sizeof(coeff_t) * tmpC);
	memcpy(ctu->m_trCoeff[2] + tmpC2, cu->m_trCoeff[2], sizeof(coeff_t) * tmpC);*/
}

/* The reverse of copyToPic, called only by encodeResidue */
void CUData_copyFromPic(struct CUData *cu, const struct CUData* ctu, const struct CUGeom* cuGeom)
{/*
	cu->m_encData = ctu->m_encData;
	cu->m_slice = ctu->m_slice;
	cu->m_cuAddr = ctu->m_cuAddr;
	cu->m_cuPelX = ctu->m_cuPelX + g_zscanToPelX[cuGeom->absPartIdx];
	cu->m_cuPelY = ctu->m_cuPelY + g_zscanToPelY[cuGeom->absPartIdx];
	cu->m_absIdxInCTU = cuGeom->absPartIdx;
	cu->m_numPartitions = cuGeom->numPartitions;

	// copy out all prediction info for this part //
	cu->m_partCopy((uint8_t*)cu->m_qp, (uint8_t*)ctu->m_qp + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_log2CUSize, ctu->m_log2CUSize + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_lumaIntraDir, ctu->m_lumaIntraDir + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_tqBypass, ctu->m_tqBypass + cu->m_absIdxInCTU);
	cu->m_partCopy((uint8_t*)cu->m_refIdx[0], (uint8_t*)ctu->m_refIdx[0] + cu->m_absIdxInCTU);
	cu->m_partCopy((uint8_t*)cu->m_refIdx[1], (uint8_t*)ctu->m_refIdx[1] + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_cuDepth, ctu->m_cuDepth + cu->m_absIdxInCTU);
	cu->m_partSet(cu->m_predMode, ctu->m_predMode[cu->m_absIdxInCTU] & (MODE_INTRA | MODE_INTER)); // clear skip flag //
	cu->m_partCopy(cu->m_partSize, ctu->m_partSize + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_mergeFlag, ctu->m_mergeFlag + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_interDir, ctu->m_interDir + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_mvpIdx[0], ctu->m_mvpIdx[0] + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_mvpIdx[1], ctu->m_mvpIdx[1] + cu->m_absIdxInCTU);
	cu->m_partCopy(cu->m_chromaIntraDir, ctu->m_chromaIntraDir + cu->m_absIdxInCTU);

	memcpy(cu->m_mv[0], ctu->m_mv[0] + cu->m_absIdxInCTU, cu->m_numPartitions * sizeof(struct MV));
	memcpy(cu->m_mv[1], ctu->m_mv[1] + cu->m_absIdxInCTU, cu->m_numPartitions * sizeof(struct MV));
	memcpy(cu->m_mvd[0], ctu->m_mvd[0] + cu->m_absIdxInCTU, cu->m_numPartitions * sizeof(struct MV));
	memcpy(cu->m_mvd[1], ctu->m_mvd[1] + cu->m_absIdxInCTU, cu->m_numPartitions * sizeof(struct MV));

	// clear residual coding flags //
	cu->m_partSet(cu->m_tuDepth, 0);
	cu->m_partSet(cu->m_transformSkip[0], 0);
	cu->m_partSet(cu->m_transformSkip[1], 0);
	cu->m_partSet(cu->m_transformSkip[2], 0);
	cu->m_partSet(cu->m_cbf[0], 0);
	cu->m_partSet(cu->m_cbf[1], 0);
	cu->m_partSet(cu->m_cbf[2], 0); */
}

/* Only called by encodeResidue, these fields can be modified during inter/intra coding */
void CUData_updatePic(struct CUData *cu, uint32_t depth)
{/*
	CUData* ctu = framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
	cu->m_partCopy((uint8_t*)ctu->m_qp + cu->m_absIdxInCTU, (uint8_t*)cu->m_qp);
	cu->m_partCopy(ctu->m_transformSkip[0] + cu->m_absIdxInCTU, cu->m_transformSkip[0]);
	cu->m_partCopy(ctu->m_transformSkip[1] + cu->m_absIdxInCTU, cu->m_transformSkip[1]);
	cu->m_partCopy(ctu->m_transformSkip[2] + cu->m_absIdxInCTU, cu->m_transformSkip[2]);
	cu->m_partCopy(ctu->m_predMode + cu->m_absIdxInCTU, cu->m_predMode);
	cu->m_partCopy(ctu->m_tuDepth + cu->m_absIdxInCTU, cu->m_tuDepth);
	cu->m_partCopy(ctu->m_cbf[0] + cu->m_absIdxInCTU, cu->m_cbf[0]);
	cu->m_partCopy(ctu->m_cbf[1] + cu->m_absIdxInCTU, cu->m_cbf[1]);
	cu->m_partCopy(ctu->m_cbf[2] + cu->m_absIdxInCTU, cu->m_cbf[2]);
	cu->m_partCopy(ctu->m_chromaIntraDir + cu->m_absIdxInCTU, cu->m_chromaIntraDir);

	uint32_t tmpY = 1 << ((g_maxLog2CUSize - depth) * 2);
	uint32_t tmpY2 = cu->m_absIdxInCTU << (LOG2_UNIT_SIZE * 2);
	memcpy(ctu->m_trCoeff[0] + tmpY2, cu->m_trCoeff[0], sizeof(coeff_t) * tmpY);
	tmpY >>= (cu->m_hChromaShift = 1) + (cu->m_vChromaShift = 1);
	tmpY2 >>= (cu->m_hChromaShift = 1) + (cu->m_vChromaShift = 1);
	memcpy(ctu->m_trCoeff[1] + tmpY2, cu->m_trCoeff[1], sizeof(coeff_t) * tmpY);
	memcpy(ctu->m_trCoeff[2] + tmpY2, cu->m_trCoeff[2], sizeof(coeff_t) * tmpY);*/
}

struct CUData *CUData_getPULeft(struct CUData *cu, uint32_t *lPartUnitIdx, uint32_t curPartUnitIdx)
{/*
	uint32_t absPartIdx = g_zscanToRaster[curPartUnitIdx];
	if (!isZeroCol(absPartIdx, cu->s_numPartInCUSize))
	{
	uint32_t absZorderCUIdx = g_zscanToRaster[cu->m_absIdxInCTU];
	*lPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
	if (isEqualCol(absPartIdx, absZorderCUIdx, cu->s_numPartInCUSize))
	return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
	else
	{
	*lPartUnitIdx -= cu->m_absIdxInCTU;
	return cu;
	}
	}

	*lPartUnitIdx = g_rasterToZscan[absPartIdx + cu->s_numPartInCUSize - 1];
	return cu->m_cuLeft;*/return 0;
}

struct CUData* CUData_getPUAbove(struct CUData *cu, uint32_t *aPartUnitIdx, uint32_t curPartUnitIdx)
{/*
	uint32_t absPartIdx = g_zscanToRaster[curPartUnitIdx];
	if (!isZeroRow(absPartIdx, cu->s_numPartInCUSize))
	{
	uint32_t absZorderCUIdx = g_zscanToRaster[cu->m_absIdxInCTU];
	*aPartUnitIdx = g_rasterToZscan[absPartIdx - cu->s_numPartInCUSize];
	if (isEqualRow(absPartIdx, absZorderCUIdx, cu->s_numPartInCUSize))
	return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
	else
	*aPartUnitIdx -= cu->m_absIdxInCTU;
	return cu;
	}

	*aPartUnitIdx = g_rasterToZscan[absPartIdx + NUM_4x4_PARTITIONS - cu->s_numPartInCUSize];
	return cu->m_cuAbove;*/return 0;
}

struct CUData* CUData_getPUAboveLeft(CUData *cu, uint32_t *alPartUnitIdx, uint32_t curPartUnitIdx)
{/*
	uint32_t absPartIdx = g_zscanToRaster[curPartUnitIdx];

	if (!isZeroCol(absPartIdx, cu->s_numPartInCUSize))
	{
	if (!isZeroRow(absPartIdx, cu->s_numPartInCUSize))
	{
	uint32_t absZorderCUIdx = g_zscanToRaster[cu->m_absIdxInCTU];
	*alPartUnitIdx = g_rasterToZscan[absPartIdx - cu->s_numPartInCUSize - 1];
	if (isEqualRowOrCol(absPartIdx, absZorderCUIdx, cu->s_numPartInCUSize))
	return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
	else
	{
	*alPartUnitIdx -= cu->m_absIdxInCTU;
	return cu;
	}
	}
	*alPartUnitIdx = g_rasterToZscan[absPartIdx + NUM_4x4_PARTITIONS - cu->s_numPartInCUSize - 1];
	return cu->m_cuAbove;
	}

	if (!isZeroRow(absPartIdx, cu->s_numPartInCUSize))
	{
	*alPartUnitIdx = g_rasterToZscan[absPartIdx - 1];
	return cu->m_cuLeft;
	}

	*alPartUnitIdx = g_rasterToZscan[NUM_4x4_PARTITIONS - 1];
	return cu->m_cuAboveLeft;*/return 0;
}

CUData* CUData_getPUAboveRight(CUData *cu, uint32_t *arPartUnitIdx, uint32_t curPartUnitIdx)
{/*
	if ((framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelX + g_zscanToPelX[curPartUnitIdx] + UNIT_SIZE) >= cu->m_slice->m_sps->picWidthInLumaSamples)
	return NULL;

	uint32_t absPartIdxRT = g_zscanToRaster[curPartUnitIdx];

	if (lessThanCol(absPartIdxRT, cu->s_numPartInCUSize - 1, cu->s_numPartInCUSize))
	{
	if (!isZeroRow(absPartIdxRT, cu->s_numPartInCUSize))
	{
	if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - cu->s_numPartInCUSize + 1])
	{
	uint32_t absZorderCUIdx = g_zscanToRaster[cu->m_absIdxInCTU] + (1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1;
	*arPartUnitIdx = g_rasterToZscan[absPartIdxRT - cu->s_numPartInCUSize + 1];
	if (isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, cu->s_numPartInCUSize))
	return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
	else
	{
	*arPartUnitIdx -= cu->m_absIdxInCTU;
	return cu;
	}
	}
	return NULL;
	}
	*arPartUnitIdx = g_rasterToZscan[absPartIdxRT + NUM_4x4_PARTITIONS - cu->s_numPartInCUSize + 1];
	return cu->m_cuAbove;
	}

	if (!isZeroRow(absPartIdxRT, cu->s_numPartInCUSize))
	return NULL;

	*arPartUnitIdx = g_rasterToZscan[NUM_4x4_PARTITIONS - cu->s_numPartInCUSize];
	return cu->m_cuAboveRight;*/return 0;
}

struct CUData* CUData_getPUBelowLeft(struct CUData *cu, uint32_t *blPartUnitIdx, uint32_t curPartUnitIdx)
{/*
	if ((framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelY + g_zscanToPelY[curPartUnitIdx] + UNIT_SIZE) >= cu->m_slice->m_sps->picHeightInLumaSamples)
		return NULL;

	uint32_t absPartIdxLB = g_zscanToRaster[curPartUnitIdx];

	if (lessThanRow(absPartIdxLB, cu->s_numPartInCUSize - 1, cu->s_numPartInCUSize))
	{
		if (!isZeroCol(absPartIdxLB, cu->s_numPartInCUSize))
		{
			if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + cu->s_numPartInCUSize - 1])
			{
				uint32_t absZorderCUIdxLB = g_zscanToRaster[cu->m_absIdxInCTU] + ((1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1) * cu->s_numPartInCUSize;
				*blPartUnitIdx = g_rasterToZscan[absPartIdxLB + cu->s_numPartInCUSize - 1];
				if (isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, cu->s_numPartInCUSize))
					return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
				else
				{
					*blPartUnitIdx -= cu->m_absIdxInCTU;
					return cu;
				}
			}
			return NULL;
		}
		*blPartUnitIdx = g_rasterToZscan[absPartIdxLB + cu->s_numPartInCUSize * 2 - 1];
		return cu->m_cuLeft;
	}
	*/
	return NULL;
}

struct CUData* CUData_getPUBelowLeftAdi(struct CUData *cu, uint32_t *blPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset)
{/*
	if ((framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelY + g_zscanToPelY[curPartUnitIdx] + (partUnitOffset << LOG2_UNIT_SIZE)) >= cu->m_slice->m_sps->picHeightInLumaSamples)
		return NULL;

	uint32_t absPartIdxLB = g_zscanToRaster[curPartUnitIdx];

	if (lessThanRow(absPartIdxLB, cu->s_numPartInCUSize - partUnitOffset, cu->s_numPartInCUSize))
	{
		if (!isZeroCol(absPartIdxLB, cu->s_numPartInCUSize))
		{
			if (curPartUnitIdx > g_rasterToZscan[absPartIdxLB + partUnitOffset * cu->s_numPartInCUSize - 1])
			{
				uint32_t absZorderCUIdxLB = g_zscanToRaster[cu->m_absIdxInCTU] + ((1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1) * cu->s_numPartInCUSize;
				*blPartUnitIdx = g_rasterToZscan[absPartIdxLB + partUnitOffset * cu->s_numPartInCUSize - 1];
				if (isEqualRowOrCol(absPartIdxLB, absZorderCUIdxLB, cu->s_numPartInCUSize))
					return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
				else
				{
					*blPartUnitIdx -= cu->m_absIdxInCTU;
					return cu;
				}
			}
			return NULL;
		}
		*blPartUnitIdx = g_rasterToZscan[absPartIdxLB + (1 + partUnitOffset) * cu->s_numPartInCUSize - 1];
		return cu->m_cuLeft;
	}
	*/
	return NULL;
}

struct CUData* CUData_getPUAboveRightAdi(struct CUData *cu, uint32_t *arPartUnitIdx, uint32_t curPartUnitIdx, uint32_t partUnitOffset)
{/*
	if ((framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelX + g_zscanToPelX[curPartUnitIdx] + (partUnitOffset << LOG2_UNIT_SIZE)) >= cu->m_slice->m_sps->picWidthInLumaSamples)
	return NULL;

	uint32_t absPartIdxRT = g_zscanToRaster[curPartUnitIdx];

	if (lessThanCol(absPartIdxRT, cu->s_numPartInCUSize - partUnitOffset, cu->s_numPartInCUSize))
	{
	if (!isZeroRow(absPartIdxRT, cu->s_numPartInCUSize))
	{
	if (curPartUnitIdx > g_rasterToZscan[absPartIdxRT - cu->s_numPartInCUSize + partUnitOffset])
	{
	uint32_t absZorderCUIdx = g_zscanToRaster[cu->m_absIdxInCTU] + (1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1;
	*arPartUnitIdx = g_rasterToZscan[absPartIdxRT - cu->s_numPartInCUSize + partUnitOffset];
	if (isEqualRowOrCol(absPartIdxRT, absZorderCUIdx, cu->s_numPartInCUSize))
	return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);
	else
	{
	*arPartUnitIdx -= cu->m_absIdxInCTU;
	return cu;
	}
	}
	return NULL;
	}
	*arPartUnitIdx = g_rasterToZscan[absPartIdxRT + NUM_4x4_PARTITIONS - cu->s_numPartInCUSize + partUnitOffset];
	return cu->m_cuAbove;
	}

	if (!isZeroRow(absPartIdxRT, cu->s_numPartInCUSize))
	return NULL;

	*arPartUnitIdx = g_rasterToZscan[NUM_4x4_PARTITIONS - cu->s_numPartInCUSize + partUnitOffset - 1];
	return cu->m_cuAboveRight;*/return 0;
}

/* Get left QpMinCu */
const struct CUData* CUData_getQpMinCuLeft(const struct CUData *cu, uint32_t *lPartUnitIdx, uint32_t curAbsIdxInCTU)
{/*

	uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_unitSizeDepth - cu->m_slice->m_pps->maxCuDQPDepth) * 2);
	uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

	// check for left CTU boundary
	if (isZeroCol(absRorderQpMinCUIdx, cu->s_numPartInCUSize))
	return NULL;

	// get index of left-CU relative to top-left corner of current quantization group
	*lPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - 1];

	// return pointer to current CTU
	return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);*/return 0;
}

/* Get above QpMinCu */
const struct CUData* CUData_getQpMinCuAbove(const struct CUData *cu, uint32_t *aPartUnitIdx, uint32_t curAbsIdxInCTU)
{/*
	uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_unitSizeDepth - cu->m_slice->m_pps->maxCuDQPDepth) * 2);
	uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

	// check for top CTU boundary
	if (isZeroRow(absRorderQpMinCUIdx, cu->s_numPartInCUSize))
	return NULL;

	// get index of top-CU relative to top-left corner of current quantization group
	*aPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - cu->s_numPartInCUSize];

	// return pointer to current CTU
	return framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);*/return 0;
}

/* Get reference QP from left QpMinCu or latest coded QP */
const int8_t CUData_getRefQP(const struct CUData *cu, uint32_t curAbsIdxInCTU)
{/*
	uint32_t lPartIdx = 0, aPartIdx = 0;
	const struct CUData* cULeft = CUData_getQpMinCuLeft(cu, &lPartIdx, cu->m_absIdxInCTU + curAbsIdxInCTU);
	const struct CUData* cUAbove = CUData_getQpMinCuAbove(cu, &aPartIdx, cu->m_absIdxInCTU + curAbsIdxInCTU);

	return ((cULeft ? cULeft->m_qp[lPartIdx] : CUData_getLastCodedQP(cu, curAbsIdxInCTU)) + (cUAbove ? cUAbove->m_qp[aPartIdx] : CUData_getLastCodedQP(cu, curAbsIdxInCTU)) + 1) >> 1;
	*/return 0;
}

const int CUData_getLastValidPartIdx(const struct CUData *cu, int absPartIdx)
{/*
	int lastValidPartIdx = absPartIdx - 1;

	while (lastValidPartIdx >= 0 && cu->m_predMode[lastValidPartIdx] == MODE_NONE)
	{
	uint32_t depth = cu->m_cuDepth[lastValidPartIdx];
	lastValidPartIdx -= cu->m_numPartitions >> (depth << 1);
	}

	return lastValidPartIdx;*/return 0;
}

const int8_t CUData_getLastCodedQP(const CUData *cu, uint32_t absPartIdx)
{/*
	uint32_t quPartIdxMask = 0xFF << (g_unitSizeDepth - cu->m_slice->m_pps->maxCuDQPDepth) * 2;
	int lastValidPartIdx = CUData_getLastValidPartIdx(cu, absPartIdx & quPartIdxMask);

	if (lastValidPartIdx >= 0)
	return cu->m_qp[lastValidPartIdx];
	else
	{
	if (cu->m_absIdxInCTU)
	return CUData_getLastCodedQP(cu, cu->m_absIdxInCTU);
	else if (cu->m_cuAddr > 0 && !(cu->m_slice->m_pps->bEntropyCodingSyncEnabled && !(cu->m_cuAddr % cu->m_slice->m_sps->numCuInWidth)))
	return CUData_getLastCodedQP(cu, NUM_4x4_PARTITIONS);
	else
	return (int8_t)cu->m_slice->m_sliceQp;
	}*/return 0;
}

/* Get allowed chroma intra modes */
void CUData_getAllowedChromaDir(struct CUData *cu, uint32_t absPartIdx, uint32_t *modeList)
{/*
	int i;
	modeList[0] = PLANAR_IDX;
	modeList[1] = VER_IDX;
	modeList[2] = HOR_IDX;
	modeList[3] = DC_IDX;
	modeList[4] = DM_CHROMA_IDX;

	uint32_t lumaMode = cu->m_lumaIntraDir[absPartIdx];

	for (i = 0; i < NUM_CHROMA_MODE - 1; i++)
	{
		if (lumaMode == modeList[i])
		{
			modeList[i] = 34; // VER+8 mode
			break;
		}
	}*/
}

/* Get most probable intra modes */
int CUData_getIntraDirLumaPredictor(struct CUData *cu, uint32_t absPartIdx, uint32_t *intraDirPred)
{/*
	const struct CUData* tempCU;
	uint32_t tempPartIdx;
	uint32_t leftIntraDir, aboveIntraDir;

	// Get intra direction of left PU
	tempCU = CUData_getPULeft(cu, &tempPartIdx, cu->m_absIdxInCTU + absPartIdx);

	leftIntraDir = (tempCU && isIntra_cudata(cu, tempPartIdx)) ? tempCU->m_lumaIntraDir[tempPartIdx] : DC_IDX;

	// Get intra direction of above PU
	tempCU = g_zscanToPelY[cu->m_absIdxInCTU + absPartIdx] > 0 ? CUData_getPUAbove(cu, &tempPartIdx, cu->m_absIdxInCTU + absPartIdx) : NULL;

	aboveIntraDir = (tempCU && isIntra_cudata(cu, tempPartIdx)) ? tempCU->m_lumaIntraDir[tempPartIdx] : DC_IDX;

	if (leftIntraDir == aboveIntraDir)
	{
	if (leftIntraDir >= 2) // angular modes
	{
	intraDirPred[0] = leftIntraDir;
	intraDirPred[1] = ((leftIntraDir - 2 + 31) & 31) + 2;
	intraDirPred[2] = ((leftIntraDir - 2 + 1) & 31) + 2;
	}
	else //non-angular
	{
	intraDirPred[0] = PLANAR_IDX;
	intraDirPred[1] = DC_IDX;
	intraDirPred[2] = VER_IDX;
	}
	return 1;
	}
	else
	{
	intraDirPred[0] = leftIntraDir;
	intraDirPred[1] = aboveIntraDir;

	if (leftIntraDir && aboveIntraDir) //both modes are non-planar
	intraDirPred[2] = PLANAR_IDX;
	else
	intraDirPred[2] = (leftIntraDir + aboveIntraDir) < 2 ? VER_IDX : DC_IDX;
	return 2;
	}*/return 0;
}

const uint32_t CUData_getCtxSplitFlag(struct CUData *cu, uint32_t absPartIdx, uint32_t depth)
{/*
	const struct CUData* tempCU;
	uint32_t    tempPartIdx;
	uint32_t    ctx;

	// Get left split flag
	tempCU = CUData_getPULeft(cu, &tempPartIdx, cu->m_absIdxInCTU + absPartIdx);
	ctx = (tempCU) ? ((tempCU->m_cuDepth[tempPartIdx] > depth) ? 1 : 0) : 0;

	// Get above split flag
	tempCU = CUData_getPUAbove(cu, &tempPartIdx, cu->m_absIdxInCTU + absPartIdx);
	ctx += (tempCU) ? ((tempCU->m_cuDepth[tempPartIdx] > depth) ? 1 : 0) : 0;

	return ctx;*/return 0;
}
int8_t getRefQP(const CUData* cu, const uint32_t curAbsIdxInCTU)
{/*
	uint32_t lPartIdx = 0, aPartIdx = 0;
	const CUData* cULeft = getQpMinCuLeft(cu, lPartIdx, cu->m_absIdxInCTU + curAbsIdxInCTU);
	const CUData* cUAbove = getQpMinCuAbove(cu, aPartIdx, cu->m_absIdxInCTU + curAbsIdxInCTU);

	return ((cULeft ? cULeft->m_qp[lPartIdx] : CUData_getLastCodedQP(cu, curAbsIdxInCTU)) + (cUAbove ? cUAbove->m_qp[aPartIdx] : CUData_getLastCodedQP(cu, curAbsIdxInCTU)) + 1) >> 1;
	*/return 0;
}

void CUData_getIntraTUQtDepthRange(struct CUData *cu, uint32_t tuDepthRange[2], uint32_t absPartIdx)
{/*
	uint32_t log2CUSize = cu->m_log2CUSize[absPartIdx];
	uint32_t splitFlag = cu->m_partSize[absPartIdx] != SIZE_2Nx2N;

	tuDepthRange[0] = cu->m_slice->m_sps->quadtreeTULog2MinSize;
	tuDepthRange[1] = cu->m_slice->m_sps->quadtreeTULog2MaxSize;

	tuDepthRange[0] = x265_clip3(tuDepthRange[0], tuDepthRange[1], log2CUSize - (cu->m_slice->m_sps->quadtreeTUMaxDepthIntra - 1 + splitFlag));
	*/
}

void CUData_getInterTUQtDepthRange(struct CUData *cu, uint32_t tuDepthRange[2], uint32_t absPartIdx)
{/*
	uint32_t log2CUSize = cu->m_log2CUSize[absPartIdx];
	uint32_t quadtreeTUMaxDepth = cu->m_slice->m_sps->quadtreeTUMaxDepthInter;
	uint32_t splitFlag = quadtreeTUMaxDepth == 1 && cu->m_partSize[absPartIdx] != SIZE_2Nx2N;

	tuDepthRange[0] = cu->m_slice->m_sps->quadtreeTULog2MinSize;
	tuDepthRange[1] = cu->m_slice->m_sps->quadtreeTULog2MaxSize;

	tuDepthRange[0] = x265_clip3(tuDepthRange[0], tuDepthRange[1], log2CUSize - (quadtreeTUMaxDepth - 1 + splitFlag));
	*/
}

/* Get left QpMinCu */
const CUData* getQpMinCuLeft(const CUData *cu, uint32_t lPartUnitIdx, uint32_t curAbsIdxInCTU)
{/*
	uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_unitSizeDepth - cu->m_slice->m_pps->maxCuDQPDepth) * 2);
	uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

	// check for left CTU boundary
	if (isZeroCol(absRorderQpMinCUIdx, cu->s_numPartInCUSize))
	return NULL;

	// get index of left-CU relative to top-left corner of current quantization group
	lPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - 1];

	// return pointer to current CTU
	return (const CUData*)(framedata_getPicCTU(cu->m_encData, cu->m_cuAddr));*/return 0;
}

/* Get above QpMinCu */
const CUData* getQpMinCuAbove(const CUData *cu, uint32_t aPartUnitIdx, uint32_t curAbsIdxInCTU)
{/*
	uint32_t absZorderQpMinCUIdx = curAbsIdxInCTU & (0xFF << (g_unitSizeDepth - cu->m_slice->m_pps->maxCuDQPDepth) * 2);
	uint32_t absRorderQpMinCUIdx = g_zscanToRaster[absZorderQpMinCUIdx];

	// check for top CTU boundary
	if (isZeroRow(absRorderQpMinCUIdx, cu->s_numPartInCUSize))
	return NULL;

	// get index of top-CU relative to top-left corner of current quantization group
	aPartUnitIdx = g_rasterToZscan[absRorderQpMinCUIdx - cu->s_numPartInCUSize];

	// return pointer to current CTU
	return (const CUData*)framedata_getPicCTU(cu->m_encData, cu->m_cuAddr);*/return 0;
}

const uint32_t CUData_getCtxSkipFlag(struct CUData *cu, uint32_t absPartIdx)
{/*
	const struct CUData* tempCU;
	uint32_t tempPartIdx;
	uint32_t ctx;

	// Get BCBP of left PU
	tempCU = CUData_getPULeft(cu, &tempPartIdx, cu->m_absIdxInCTU + absPartIdx);
	ctx = tempCU ? isSkipped(cu, tempPartIdx) : 0;

	// Get BCBP of above PU
	tempCU = CUData_getPUAbove(cu, &tempPartIdx, cu->m_absIdxInCTU + absPartIdx);
	ctx += tempCU ? isSkipped(cu, tempPartIdx) : 0;

	return ctx;*/return 0;
}

int CUData_setQPSubCUs(struct CUData *cu, int8_t qp, uint32_t absPartIdx, uint32_t depth)
{/*
	uint32_t curPartNumb = NUM_4x4_PARTITIONS >> (depth << 1);
	uint32_t curPartNumQ = curPartNumb >> 2;

	if (cu->m_cuDepth[absPartIdx] > depth)
	{
		uint32_t subPartIdx;
		for (subPartIdx = 0; subPartIdx < 4; subPartIdx++)
			if (CUData_setQPSubCUs(cu, qp, absPartIdx + subPartIdx * curPartNumQ, depth + 1))
				return TRUE;
	}
	else
	{
		if (getQtRootCbf(cu, absPartIdx))
			return TRUE;
		else
			setQPSubParts(cu, qp, absPartIdx, depth);
	}
	*/
	return FALSE;
}

void CUData_setPUInterDir(struct CUData *cu, uint8_t dir, uint32_t absPartIdx, uint32_t puIdx)
{/*
	uint32_t curPartNumQ = cu->m_numPartitions >> 2;
	if (!(puIdx < 2))
		printf("unexpected part unit index\n");

	switch (cu->m_partSize[absPartIdx])
	{
	case SIZE_2Nx2N:
		memset(cu->m_interDir + absPartIdx, dir, 4 * curPartNumQ);
		break;
	case SIZE_2NxN:
		memset(cu->m_interDir + absPartIdx, dir, 2 * curPartNumQ);
		break;
	case SIZE_Nx2N:
		memset(cu->m_interDir + absPartIdx, dir, curPartNumQ);
		memset(cu->m_interDir + absPartIdx + 2 * curPartNumQ, dir, curPartNumQ);
		break;
	case SIZE_NxN:
		memset(cu->m_interDir + absPartIdx, dir, curPartNumQ);
		break;
	case SIZE_2NxnU:
		if (!puIdx)
		{
			memset(cu->m_interDir + absPartIdx, dir, (curPartNumQ >> 1));
			memset(cu->m_interDir + absPartIdx + curPartNumQ, dir, (curPartNumQ >> 1));
		}
		else
		{
			memset(cu->m_interDir + absPartIdx, dir, (curPartNumQ >> 1));
			memset(cu->m_interDir + absPartIdx + curPartNumQ, dir, ((curPartNumQ >> 1) + (curPartNumQ << 1)));
		}
		break;
	case SIZE_2NxnD:
		if (!puIdx)
		{
			memset(cu->m_interDir + absPartIdx, dir, ((curPartNumQ << 1) + (curPartNumQ >> 1)));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1) + curPartNumQ, dir, (curPartNumQ >> 1));
		}
		else
		{
			memset(cu->m_interDir + absPartIdx, dir, (curPartNumQ >> 1));
			memset(cu->m_interDir + absPartIdx + curPartNumQ, dir, (curPartNumQ >> 1));
		}
		break;
	case SIZE_nLx2N:
		if (!puIdx)
		{
			memset(cu->m_interDir + absPartIdx, dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1) + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
		}
		else
		{
			memset(cu->m_interDir + absPartIdx, dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ >> 1), dir, (curPartNumQ + (curPartNumQ >> 2)));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1) + (curPartNumQ >> 1), dir, (curPartNumQ + (curPartNumQ >> 2)));
		}
		break;
	case SIZE_nRx2N:
		if (!puIdx)
		{
			memset(cu->m_interDir + absPartIdx, dir, (curPartNumQ + (curPartNumQ >> 2)));
			memset(cu->m_interDir + absPartIdx + curPartNumQ + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ + (curPartNumQ >> 2)));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1) + curPartNumQ + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
		}
		else
		{
			memset(cu->m_interDir + absPartIdx, dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1), dir, (curPartNumQ >> 2));
			memset(cu->m_interDir + absPartIdx + (curPartNumQ << 1) + (curPartNumQ >> 1), dir, (curPartNumQ >> 2));
		}
		break;
	default:
		printf("unexpected part type\n");
		break;
	}*/
}


void getTUEntropyCodingParameters(CUData* cu, TUEntropyCodingParameters *result, uint32_t absPartIdx, uint32_t log2TrSize, char bIsLuma)
{/*
	bool bIsIntra = isIntra_cudata(cu, absPartIdx);

	// set the group layout
	result->log2TrSizeCG = log2TrSize - 2;

	// set the scan orders
	if (bIsIntra)
	{
		uint32_t dirMode;

		if (bIsLuma)
			dirMode = cu->m_lumaIntraDir[absPartIdx];
		else
		{
			dirMode = cu->m_chromaIntraDir[absPartIdx];
			if (dirMode == DM_CHROMA_IDX)
			{
				dirMode = cu->m_lumaIntraDir[(cu->m_chromaFormat == X265_CSP_I444) ? absPartIdx : absPartIdx & 0xFC];
				dirMode = (cu->m_chromaFormat == X265_CSP_I422) ? g_chroma422IntraAngleMappingTable[dirMode] : dirMode;
			}
		}

		if (log2TrSize <= (MDCS_LOG2_MAX_SIZE - cu->m_hChromaShift) || (bIsLuma && log2TrSize == MDCS_LOG2_MAX_SIZE))
			result->scanType = dirMode >= 22 && dirMode <= 30 ? SCAN_HOR : dirMode >= 6 && dirMode <= 14 ? SCAN_VER : SCAN_DIAG;
		else
			result->scanType = SCAN_DIAG;
	}
	else
		result->scanType = SCAN_DIAG;

	result->scan = g_scanOrder[result->scanType][log2TrSize - 2];//确定子块内部系数扫描顺序（方式），TB都是最终分割成4×4大小的子块。
	result->scanCG = g_scanOrderCG[result->scanType][result->log2TrSizeCG];//确定子块的扫描顺序（方式）

	if (log2TrSize == 2)
		result->firstSignificanceMapContext = 0;
	else if (log2TrSize == 3)
		result->firstSignificanceMapContext = (result->scanType != SCAN_DIAG && bIsLuma) ? 15 : 9;
	else
		result->firstSignificanceMapContext = bIsLuma ? 21 : 12;*/
}

//template<typename T>
void CUData_setAllPU(struct CUData *cu, struct MV *p, const struct MV *val, int absPartIdx, int puIdx)
{/*
	int i;

	p += absPartIdx;
	int numElements = cu->m_numPartitions;

	switch (cu->m_partSize[absPartIdx])

	{
	case SIZE_2Nx2N:
		for (i = 0; i < numElements; i++)
			p[i] = *val;
		break;
	case SIZE_2NxN:
		numElements >>= 1;
		for (i = 0; i < numElements; i++)
			p[i] = *val;
		break;

	case SIZE_Nx2N:
		numElements >>= 2;
		for (i = 0; i < numElements; i++)
		{
			p[i] = *val;
			p[i + 2 * numElements] = *val;
		}
		break;

	case SIZE_2NxnU:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			struct MV *pT = p;
			struct MV *pT2 = p + curPartNumQ;
			for (i = 0; i < (curPartNumQ >> 1); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		else
		{
			struct MV *pT = p;
			for (i = 0; i < (curPartNumQ >> 1); i++)
				pT[i] = *val;

			pT = p + curPartNumQ;
			for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
				pT[i] = *val;
		}
		break;
	}

	case SIZE_2NxnD:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			struct MV *pT = p;
			for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
				pT[i] = *val;

			pT = p + (numElements - curPartNumQ);
			for (i = 0; i < (curPartNumQ >> 1); i++)
				pT[i] = *val;
		}
		else
		{
			struct MV *pT = p;
			struct MV *pT2 = p + curPartNumQ;
			for (i = 0; i < (curPartNumQ >> 1); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		break;
	}

	case SIZE_nLx2N:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			struct MV *pT = p;
			struct MV *pT2 = p + (curPartNumQ << 1);
			struct MV *pT3 = p + (curPartNumQ >> 1);
			struct MV *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);

			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
				pT3[i] = *val;
				pT4[i] = *val;
			}
		}
		else
		{
			struct MV *pT = p;
			struct MV *pT2 = p + (curPartNumQ << 1);
			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}

			pT = p + (curPartNumQ >> 1);
			pT2 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
			for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		break;
	}

	case SIZE_nRx2N:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			struct MV *pT = p;
			struct MV *pT2 = p + (curPartNumQ << 1);
			for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}

			pT = p + curPartNumQ + (curPartNumQ >> 1);
			pT2 = p + numElements - curPartNumQ + (curPartNumQ >> 1);
			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		else
		{
			struct MV *pT = p;
			struct MV *pT2 = p + (curPartNumQ >> 1);
			struct MV *pT3 = p + (curPartNumQ << 1);
			struct MV *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
				pT3[i] = *val;
				pT4[i] = *val;
			}
		}
		break;
	}

	case SIZE_NxN:
	default:
		printf("unknown partition type\n");
		break;
	}*/
}
//void CUData::setAllPU(T* p, const T& val, int absPartIdx, int puIdx)
void setAllPU(struct CUData *cu, int8_t *p, int8_t *val, int absPartIdx, int puIdx)
{/*
	int i;

	p += absPartIdx;
	int numElements = cu->m_numPartitions;

	switch (cu->m_partSize[absPartIdx])

	{
	case SIZE_2Nx2N:
		for (i = 0; i < numElements; i++)
			p[i] = *val;
		break;
	case SIZE_2NxN:
		numElements >>= 1;
		for (i = 0; i < numElements; i++)
			p[i] = *val;
		break;

	case SIZE_Nx2N:
		numElements >>= 2;
		for (i = 0; i < numElements; i++)
		{
			p[i] = *val;
			p[i + 2 * numElements] = *val;
		}
		break;

	case SIZE_2NxnU:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			int8_t *pT = p;
			int8_t *pT2 = p + curPartNumQ;
			for (i = 0; i < (curPartNumQ >> 1); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		else
		{
			int8_t *pT = p;
			for (i = 0; i < (curPartNumQ >> 1); i++)
				pT[i] = *val;

			pT = p + curPartNumQ;
			for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
				pT[i] = *val;
		}
		break;
	}

	case SIZE_2NxnD:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			int8_t *pT = p;
			for (i = 0; i < ((curPartNumQ >> 1) + (curPartNumQ << 1)); i++)
				pT[i] = *val;

			pT = p + (numElements - curPartNumQ);
			for (i = 0; i < (curPartNumQ >> 1); i++)
				pT[i] = *val;
		}
		else
		{
			int8_t *pT = p;
			int8_t *pT2 = p + curPartNumQ;
			for (i = 0; i < (curPartNumQ >> 1); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		break;
	}

	case SIZE_nLx2N:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			int8_t *pT = p;
			int8_t *pT2 = p + (curPartNumQ << 1);
			int8_t *pT3 = p + (curPartNumQ >> 1);
			int8_t *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);

			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
				pT3[i] = *val;
				pT4[i] = *val;
			}
		}
		else
		{
			int8_t *pT = p;
			int8_t *pT2 = p + (curPartNumQ << 1);
			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}

			pT = p + (curPartNumQ >> 1);
			pT2 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
			for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		break;
	}

	case SIZE_nRx2N:
	{
		int curPartNumQ = numElements >> 2;
		if (!puIdx)
		{
			int8_t *pT = p;
			int8_t *pT2 = p + (curPartNumQ << 1);
			for (i = 0; i < ((curPartNumQ >> 2) + curPartNumQ); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}

			pT = p + curPartNumQ + (curPartNumQ >> 1);
			pT2 = p + numElements - curPartNumQ + (curPartNumQ >> 1);
			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
			}
		}
		else
		{
			int8_t *pT = p;
			int8_t *pT2 = p + (curPartNumQ >> 1);
			int8_t *pT3 = p + (curPartNumQ << 1);
			int8_t *pT4 = p + (curPartNumQ << 1) + (curPartNumQ >> 1);
			for (i = 0; i < (curPartNumQ >> 2); i++)
			{
				pT[i] = *val;
				pT2[i] = *val;
				pT3[i] = *val;
				pT4[i] = *val;
			}
		}
		break;
	}

	case SIZE_NxN:
	default:
		printf("unknown partition type\n");
		break;
	}*/
}

void CUData_setPUMv(struct CUData *cu, int list, MV *mv, int absPartIdx, int puIdx)
{/*
	CUData_setAllPU(cu, cu->m_mv[list], mv, absPartIdx, puIdx);*/
}

void CUData_setPURefIdx(struct CUData *cu, int list, int8_t refIdx, int absPartIdx, int puIdx)
{/*
	setAllPU(cu, cu->m_refIdx[list], &refIdx, absPartIdx, puIdx);*/
}

#define CU_SET_FLAG(bitfield, flag, value) (bitfield) = ((bitfield) & (~(flag))) | ((~((value) - 1)) & (flag))

/** 函数功能       ： 计算CU的几何信息
/*  调用范围       ： 只在FrameEncoder CUData_initializeGeoms()函数中被调用
* \参数 ctuWidth   ： 宽度的余数
* \参数 ctuHeight  ： 高度的余数
* \参数 maxCUSize  ： 最大CU
* \参数 minCUSize  ： 最小CU
* \参数 cuDataArray： 存储位置
*   返回值         ： null
**/

void CUData_calcCTUGeoms(uint32_t ctuWidth, uint32_t ctuHeight, uint32_t maxCUSize, uint32_t minCUSize, CUGeom cuDataArray[85])
{
	uint32_t sbY;
	uint32_t sbX;
	uint32_t log2CUSize, rangeCUIdx;

	// Initialize the coding blocks inside the CTB
	for (log2CUSize = g_log2Size[maxCUSize], rangeCUIdx = 0; log2CUSize >= g_log2Size[minCUSize]; log2CUSize--)
		//从最大CU遍历到最小CU
	{
		uint32_t blockSize = 1 << log2CUSize;//当前的块大小
		uint32_t sbWidth = 1 << (g_log2Size[maxCUSize] - log2CUSize);//最大CU下有宽度上有几个当前块大小 如：64 ：1 32 ：2 16:4 8:8
		int32_t lastLevelFlag = log2CUSize == g_log2Size[minCUSize]; //判断当前位置是否为最小CU

		for (sbY = 0; sbY < sbWidth; sbY++) //按行遍历
		{
			for (sbX = 0; sbX < sbWidth; sbX++) //遍历当前行的每个块
			{
				uint32_t depthIdx = g_depthScanIdx[sbY][sbX]; //获取当前的zigzag号
				uint32_t cuIdx = rangeCUIdx + depthIdx; //当前在geom的存储位置
				uint32_t childIdx = rangeCUIdx + sbWidth * sbWidth + (depthIdx << 2); //对应位置子块在geom的存储位置
				uint32_t px = sbX * blockSize; //在CTU中的pixel地址
				uint32_t py = sbY * blockSize; //在CTU中的pixel地址
				int32_t presentFlag = px < ctuWidth && py < ctuHeight; //判断当前块左上角像素是否在图像内部
				int32_t splitMandatoryFlag = presentFlag && !lastLevelFlag && (px + blockSize > ctuWidth || py + blockSize > ctuHeight);
				// 在图像内部 不是最小CU CU超过边界
				// Offset of the luma CU in the X, Y direction in terms of pixels from the CTU origin //
				uint32_t xOffset = (sbX * blockSize) >> 3;
				uint32_t yOffset = (sbY * blockSize) >> 3;
				if (cuIdx > MAX_GEOMS)
					printf("CU geom index bug\n");

				CUGeom *cu = cuDataArray + cuIdx;  //获取存储位置
				cu->log2CUSize = log2CUSize; //记录当前块大小
				cu->childOffset = childIdx - cuIdx; //示从当前位置到第一个子cU的偏移量
				cu->absPartIdx = g_depthScanIdx[yOffset][xOffset] * 4; // 当前CU在LCU中4x4 zizag地址
				cu->numPartitions = (NUM_4x4_PARTITIONS >> ((g_maxLog2CUSize - cu->log2CUSize) * 2)); // 当前CU有多少4x4块
				cu->depth = g_log2Size[maxCUSize] - log2CUSize; // 当前CU的深度

				cu->flags = 0;
				CU_SET_FLAG(cu->flags, cuDataArray->type = PRESENT, presentFlag); //记录是否有PRESENT
				CU_SET_FLAG(cu->flags, (cuDataArray->type = SPLIT_MANDATORY) | (cuDataArray->type = SPLIT), splitMandatoryFlag); //记录 SPLIT_MANDATORY SPLIT
				CU_SET_FLAG(cu->flags, cuDataArray->type = LEAF, lastLevelFlag); //记录LEAF
			}
		}
		rangeCUIdx += sbWidth * sbWidth; //用于计算每个块在geom的位置
	}
}

bool isDiffMER(int xN, int yN, int xP, int yP)
{
	return ((xN >> 2) != (xP >> 2)) || ((yN >> 2) != (yP >> 2));
}

uint32_t getNumPartInter(struct CUData *cu)
{
	return nbPartsTable[(int)cu->m_partSize[0]];
}
int  isIntra_cudata(struct CUData *cu, uint32_t absPartIdx)
{
	return cu->m_predMode[absPartIdx] == MODE_INTRA;
}
int  isInter_cudata(const struct CUData *cu, uint32_t absPartIdx)
{
	return !!(cu->m_predMode[absPartIdx] & MODE_INTER);
}
int  isSkipped(struct CUData *cu, uint32_t absPartIdx)
{
	return cu->m_predMode[absPartIdx] == MODE_SKIP;
}
int  isBipredRestriction(struct CUData *cu)
{
	return cu->m_log2CUSize[0] == 3 && cu->m_partSize[0] != SIZE_2Nx2N;
}

/* these functions all take depth as an absolute depth from CTU, it is used to calculate the number of parts to copy */
void     setQPSubParts(struct CUData *cu, int8_t qp, uint32_t absPartIdx, uint32_t depth)
{
	bcast64((uint8_t*)cu->m_qp + absPartIdx, qp & 0xff);
}

void     setTUDepthSubParts(struct CUData *cu, uint8_t tuDepth, uint32_t absPartIdx, uint32_t depth)
{
	cu->s_partSet[depth](cu->m_tuDepth + absPartIdx, tuDepth);
}

void     setLumaIntraDirSubParts(struct CUData *cu, uint8_t dir, uint32_t absPartIdx, uint32_t depth)
{
	cu->s_partSet[depth](cu->m_lumaIntraDir + absPartIdx, dir);
}

void     setChromIntraDirSubParts(struct CUData *cu, uint8_t dir, uint32_t absPartIdx, uint32_t depth)
{
	cu->s_partSet[depth](cu->m_chromaIntraDir + absPartIdx, dir);
}

void     setCbfSubParts(struct CUData *cu, uint8_t cbf, enum TextType ttype, uint32_t absPartIdx, uint32_t depth)
{
	cu->s_partSet[depth](cu->m_cbf[ttype] + absPartIdx, cbf);
}

void     setCbfPartRange(struct CUData *cu, uint8_t cbf, enum TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes)
{
	memset(cu->m_cbf[ttype] + absPartIdx, cbf, coveredPartIdxes);
}

void     setTransformSkipSubParts(struct CUData *cu, uint8_t tskip, enum TextType ttype, uint32_t absPartIdx, uint32_t depth)
{
	cu->s_partSet[depth](cu->m_transformSkip[ttype] + absPartIdx, tskip);
}

void     setTransformSkipPartRange(struct CUData *cu, uint8_t tskip, enum TextType ttype, uint32_t absPartIdx, uint32_t coveredPartIdxes)
{
	memset(cu->m_transformSkip[ttype] + absPartIdx, tskip, coveredPartIdxes >> 1);
}

const uint8_t  getQtRootCbf(struct CUData *cu, uint32_t absPartIdx)
{
	return cu->m_cbf[0][absPartIdx] || cu->m_cbf[1][absPartIdx] || cu->m_cbf[2][absPartIdx];
}

uint32_t getSCUAddr(const CUData *cu)
{
	return (cu->m_cuAddr << g_unitSizeDepth * 2) + cu->m_absIdxInCTU;
}

uint8_t  getCbf(CUData* cu, uint32_t absPartIdx, enum TextType ttype, uint32_t tuDepth)
{
	int count = (cu->m_cbf[ttype][absPartIdx] >> tuDepth) & 0x1;
	return count;
}

void     setPartSizeSubParts(CUData *cu, enum PartSize size)    { cu->m_partSet(cu->m_partSize, 0xff & size); }
void     setPredModeSubParts(CUData *cu, enum PredMode mode)    { bcast64(cu->m_predMode, 0xff & mode); }
void     clearCbf(CUData *cu)                            { cu->m_partSet(cu->m_cbf[0], 0); cu->m_partSet(cu->m_cbf[1], 0); cu->m_partSet(cu->m_cbf[2], 0); }

void CUMemPool_destory(CUDataMemPool *Pool)
{
	free(Pool->charMemBlock); Pool->charMemBlock = NULL;
	free(Pool->mvMemBlock); Pool->mvMemBlock = NULL;
	free(Pool->trCoeffMemBlock); Pool->trCoeffMemBlock = NULL;
}

void CUDataMemPool_init(CUDataMemPool* cumem)
{
	cumem->charMemBlock = NULL;
	cumem->trCoeffMemBlock = NULL;
	cumem->mvMemBlock = NULL;
}
bool CUData_hasEqualMotion(const CUData *cu, uint32_t absPartIdx, const CUData* candCU, uint32_t candAbsPartIdx)
{
	if (cu->m_interDir[absPartIdx] != candCU->m_interDir[candAbsPartIdx])
		return FALSE;

	for (uint32_t refListIdx = 0; refListIdx < 2; refListIdx++)
	{
		if (cu->m_interDir[absPartIdx] & (1 << refListIdx))
		{
			if ((cu->m_mv[refListIdx][absPartIdx].x != candCU->m_mv[refListIdx][candAbsPartIdx].x &&
				cu->m_mv[refListIdx][absPartIdx].y != candCU->m_mv[refListIdx][candAbsPartIdx].y) ||
				cu->m_refIdx[refListIdx][absPartIdx] != candCU->m_refIdx[refListIdx][candAbsPartIdx])
				return FALSE;
		}
	}

	return TRUE;
}

/* Construct list of merging candidates, returns count. */
/*
uint32_t CUData_getInterMergeCandidates(struct CUData *cu, uint32_t absPartIdx, uint32_t puIdx, struct MVField(*candMvField)[2], uint8_t* candDir)
{
	uint32_t absPartAddr = cu->m_absIdxInCTU + absPartIdx;
	const bool isInter_B = isInterB(cu->m_slice);

	const uint32_t maxNumMergeCand = cu->m_slice->m_maxNumMergeCand;
	for (uint32_t i = 0; i < maxNumMergeCand; ++i)
	{
		candMvField[i][0].mv.x = 0;
		candMvField[i][0].mv.y = 0;
		candMvField[i][1].mv.x = 0;
		candMvField[i][1].mv.y = 0;
		candMvField[i][0].refIdx = REF_NOT_VALID;
		candMvField[i][1].refIdx = REF_NOT_VALID;
	}

	// calculate the location of upper-left corner pixel and size of the current PU //
	int xP, yP, nPSW, nPSH;

	int cuSize = 1 << cu->m_log2CUSize[0];
	int partMode = cu->m_partSize[0];

	int tmp = partTable[partMode][puIdx][0];
	nPSW = ((tmp >> 4) * cuSize) >> 2;
	nPSH = ((tmp & 0xF) * cuSize) >> 2;

	tmp = partTable[partMode][puIdx][1];
	xP = ((tmp >> 4) * cuSize) >> 2;
	yP = ((tmp & 0xF) * cuSize) >> 2;

	uint32_t count = 0;

	uint32_t partIdxLT, partIdxRT, partIdxLB = CUData_deriveLeftBottomIdx(cu, puIdx);
	PartSize curPS = (PartSize)cu->m_partSize[absPartIdx];

	// left
	uint32_t leftPartIdx = 0;
	const CUData* cuLeft = CUData_getPULeft(cu, &leftPartIdx, partIdxLB);
	bool isAvailableA1 = cuLeft &&
		isDiffMER(xP - 1, yP + nPSH - 1, xP, yP) &&
		!(puIdx == 1 && (curPS == SIZE_Nx2N || curPS == SIZE_nLx2N || curPS == SIZE_nRx2N)) &&
		isInter_cudata(cuLeft, leftPartIdx);
	if (isAvailableA1)
	{
		// get Inter Dir
		candDir[count] = cuLeft->m_interDir[leftPartIdx];
		// get Mv from Left
		CUData_getMvField(cuLeft, leftPartIdx, 0, &candMvField[count][0]);
		if (isInterB)
			CUData_getMvField(cuLeft, leftPartIdx, 1, &candMvField[count][1]);

		if (++count == maxNumMergeCand)
			return maxNumMergeCand;
	}
	CUData_deriveLeftRightTopIdx(cu, puIdx, &partIdxLT, &partIdxRT);

	// above
	uint32_t abovePartIdx = 0;
	const CUData* cuAbove = CUData_getPUAbove(cu, &abovePartIdx, partIdxRT);
	bool isAvailableB1 = cuAbove &&
		isDiffMER(xP + nPSW - 1, yP - 1, xP, yP) &&
		!(puIdx == 1 && (curPS == SIZE_2NxN || curPS == SIZE_2NxnU || curPS == SIZE_2NxnD)) &&
		isInter_cudata(cuAbove, abovePartIdx);

	if (isAvailableB1 && (!isAvailableA1 || !CUData_hasEqualMotion(cuLeft, leftPartIdx, cuAbove, abovePartIdx)))
	{
		// get Inter Dir
		candDir[count] = cuAbove->m_interDir[abovePartIdx];
		// get Mv from Left
		CUData_getMvField(cuAbove, abovePartIdx, 0, &candMvField[count][0]);
		if (isInterB)
			CUData_getMvField(cuAbove, abovePartIdx, 1, &candMvField[count][1]);

		if (++count == maxNumMergeCand)
			return maxNumMergeCand;
	}

	// above right
	uint32_t aboveRightPartIdx = 0;
	const CUData* cuAboveRight = CUData_getPUAboveRight(cu, &aboveRightPartIdx, partIdxRT);
	bool isAvailableB0 = cuAboveRight &&
		isDiffMER(xP + nPSW, yP - 1, xP, yP) &&
		isInter_cudata(cuAboveRight, aboveRightPartIdx);
	if (isAvailableB0 && (!isAvailableB1 || !CUData_hasEqualMotion(cuAbove, abovePartIdx, cuAboveRight, aboveRightPartIdx)))
	{
		// get Inter Dir
		candDir[count] = cuAboveRight->m_interDir[aboveRightPartIdx];
		// get Mv from Left
		CUData_getMvField(cuAboveRight, aboveRightPartIdx, 0, &candMvField[count][0]);
		if (isInterB)
			CUData_getMvField(cuAboveRight, aboveRightPartIdx, 1, &candMvField[count][1]);

		if (++count == maxNumMergeCand)
			return maxNumMergeCand;
	}

	// left bottom
	uint32_t leftBottomPartIdx = 0;
	const CUData* cuLeftBottom = CUData_getPUBelowLeft(cu, &leftBottomPartIdx, partIdxLB);
	bool isAvailableA0 = cuLeftBottom &&
		isDiffMER(xP - 1, yP + nPSH, xP, yP) &&
		isInter_cudata(cuLeftBottom, leftBottomPartIdx);
	if (isAvailableA0 && (!isAvailableA1 || !CUData_hasEqualMotion(cuLeft, leftPartIdx, cuLeftBottom, leftBottomPartIdx)))
	{
		// get Inter Dir
		candDir[count] = cuLeftBottom->m_interDir[leftBottomPartIdx];
		// get Mv from Left
		CUData_getMvField(cuLeftBottom, leftBottomPartIdx, 0, &candMvField[count][0]);
		if (isInterB)
			CUData_getMvField(cuLeftBottom, leftBottomPartIdx, 1, &candMvField[count][1]);

		if (++count == maxNumMergeCand)
			return maxNumMergeCand;
	}

	// above left
	if (count < 4)
	{
		uint32_t aboveLeftPartIdx = 0;
		const CUData* cuAboveLeft = CUData_getPUAboveLeft(cu, &aboveLeftPartIdx, absPartAddr);
		bool isAvailableB2 = cuAboveLeft &&
			isDiffMER(xP - 1, yP - 1, xP, yP) &&
			isInter_cudata(cuAboveLeft, aboveLeftPartIdx);
		if (isAvailableB2 && (!isAvailableA1 || !CUData_hasEqualMotion(cuLeft, leftPartIdx, cuAboveLeft, aboveLeftPartIdx))
			&& (!isAvailableB1 || !CUData_hasEqualMotion(cuAbove, abovePartIdx, cuAboveLeft, aboveLeftPartIdx)))
		{
			// get Inter Dir
			candDir[count] = cuAboveLeft->m_interDir[aboveLeftPartIdx];
			// get Mv from Left
			CUData_getMvField(cuAboveLeft, aboveLeftPartIdx, 0, &candMvField[count][0]);
			if (isInterB)
				CUData_getMvField(cuAboveLeft, aboveLeftPartIdx, 1, &candMvField[count][1]);

			if (++count == maxNumMergeCand)
				return maxNumMergeCand;
		}
	}
	if (cu->m_slice->m_sps->bTemporalMVPEnabled)
	{
		uint32_t partIdxRB = CUData_deriveRightBottomIdx(cu, puIdx);
		MV *colmv;
		int ctuIdx = -1;

		// image boundary check
		if (framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelX + g_zscanToPelX[partIdxRB] + UNIT_SIZE < cu->m_slice->m_sps->picWidthInLumaSamples &&
			framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelY + g_zscanToPelY[partIdxRB] + UNIT_SIZE < cu->m_slice->m_sps->picHeightInLumaSamples)
		{
			uint32_t absPartIdxRB = g_zscanToRaster[partIdxRB];
			uint32_t numUnits = cu->s_numPartInCUSize;
			bool bNotLastCol = lessThanCol(absPartIdxRB, numUnits - 1, numUnits); // is not at the last column of CTU
			bool bNotLastRow = lessThanRow(absPartIdxRB, numUnits - 1, numUnits); // is not at the last row    of CTU

			if (bNotLastCol && bNotLastRow)
			{
				absPartAddr = g_rasterToZscan[absPartIdxRB + numUnits + 1];
				ctuIdx = cu->m_cuAddr;
			}
			else if (bNotLastCol)
				absPartAddr = g_rasterToZscan[(absPartIdxRB + numUnits + 1) & (numUnits - 1)];
			else if (bNotLastRow)
			{
				absPartAddr = g_rasterToZscan[absPartIdxRB + 1];
				ctuIdx = cu->m_cuAddr + 1;
			}
			else // is the right bottom corner of CTU
				absPartAddr = 0;
		}

		int maxList = isInterB ? 2 : 1;
		int dir = 0, refIdx = 0;
		for (int list = 0; list < maxList; list++)
		{
			bool bExistMV = ctuIdx >= 0 && CUData_getColMVP(cu, colmv, &refIdx, list, ctuIdx, absPartAddr);
			if (!bExistMV)
			{
				uint32_t partIdxCenter = CUData_deriveCenterIdx(cu, puIdx);
				bExistMV = CUData_getColMVP(cu, colmv, &refIdx, list, cu->m_cuAddr, partIdxCenter);
			}
			if (bExistMV)
			{
				dir |= (1 << list);
				candMvField[count][list].mv.x = colmv->x;
				candMvField[count][list].mv.y = colmv->y;
				candMvField[count][list].refIdx = refIdx;
			}
		}

		if (dir != 0)
		{
			candDir[count] = (uint8_t)dir;

			if (++count == maxNumMergeCand)
				return maxNumMergeCand;
		}
	}

	int numRefIdx = (isInterB) ? X265_MIN(cu->m_slice->m_numRefIdx[0], cu->m_slice->m_numRefIdx[1]) : cu->m_slice->m_numRefIdx[0];
	int r = 0;
	int refcnt = 0;
	while (count < maxNumMergeCand)
	{
		candDir[count] = 1;
		candMvField[count][0].mv.word = 0;
		candMvField[count][0].refIdx = r;

		if (isInterB)
		{
			candDir[count] = 3;
			candMvField[count][1].mv.word = 0;
			candMvField[count][1].refIdx = r;
		}

		count++;

		if (refcnt == numRefIdx - 1)
			r = 0;
		else
		{
			++r;
			++refcnt;
		}
	}

	return count;
}*/
void CUData_deriveLeftRightTopIdx(struct CUData *cu, uint32_t partIdx, uint32_t* partIdxLT, uint32_t* partIdxRT)
{
	*partIdxLT = cu->m_absIdxInCTU;
	*partIdxRT = g_rasterToZscan[g_zscanToRaster[*partIdxLT] + (1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1];

	switch (cu->m_partSize[0])
	{
	case SIZE_2Nx2N: break;
	case SIZE_2NxN:
		partIdxLT += (partIdx == 0) ? 0 : cu->m_numPartitions >> 1;
		partIdxRT += (partIdx == 0) ? 0 : cu->m_numPartitions >> 1;
		break;
	case SIZE_Nx2N:
		partIdxLT += (partIdx == 0) ? 0 : cu->m_numPartitions >> 2;
		partIdxRT -= (partIdx == 1) ? 0 : cu->m_numPartitions >> 2;
		break;
	case SIZE_NxN:
		partIdxLT += (cu->m_numPartitions >> 2) * partIdx;
		partIdxRT += (cu->m_numPartitions >> 2) * (partIdx - 1);
		break;
	case SIZE_2NxnU:
		partIdxLT += (partIdx == 0) ? 0 : cu->m_numPartitions >> 3;
		partIdxRT += (partIdx == 0) ? 0 : cu->m_numPartitions >> 3;
		break;
	case SIZE_2NxnD:
		partIdxLT += (partIdx == 0) ? 0 : (cu->m_numPartitions >> 1) + (cu->m_numPartitions >> 3);
		partIdxRT += (partIdx == 0) ? 0 : (cu->m_numPartitions >> 1) + (cu->m_numPartitions >> 3);
		break;
	case SIZE_nLx2N:
		partIdxLT += (partIdx == 0) ? 0 : cu->m_numPartitions >> 4;
		partIdxRT -= (partIdx == 1) ? 0 : (cu->m_numPartitions >> 2) + (cu->m_numPartitions >> 4);
		break;
	case SIZE_nRx2N:
		partIdxLT += (partIdx == 0) ? 0 : (cu->m_numPartitions >> 2) + (cu->m_numPartitions >> 4);
		partIdxRT -= (partIdx == 1) ? 0 : cu->m_numPartitions >> 4;
		break;
	default:
		X265_CHECK(0, "unexpected part index\n");
		break;
	}
}

uint32_t CUData_deriveLeftBottomIdx(struct CUData *cu, uint32_t puIdx)
{
	uint32_t outPartIdxLB;
	outPartIdxLB = g_rasterToZscan[g_zscanToRaster[cu->m_absIdxInCTU] + ((1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE - 1)) - 1) * cu->s_numPartInCUSize];

	switch (cu->m_partSize[0])
	{
	case SIZE_2Nx2N:
		outPartIdxLB += cu->m_numPartitions >> 1;
		break;
	case SIZE_2NxN:
		outPartIdxLB += puIdx ? cu->m_numPartitions >> 1 : 0;
		break;
	case SIZE_Nx2N:
		outPartIdxLB += puIdx ? (cu->m_numPartitions >> 2) * 3 : cu->m_numPartitions >> 1;
		break;
	case SIZE_NxN:
		outPartIdxLB += (cu->m_numPartitions >> 2) * puIdx;
		break;
	case SIZE_2NxnU:
		outPartIdxLB += puIdx ? cu->m_numPartitions >> 1 : -((int)cu->m_numPartitions >> 3);
		break;
	case SIZE_2NxnD:
		outPartIdxLB += puIdx ? cu->m_numPartitions >> 1 : (cu->m_numPartitions >> 2) + (cu->m_numPartitions >> 3);
		break;
	case SIZE_nLx2N:
		outPartIdxLB += puIdx ? (cu->m_numPartitions >> 1) + (cu->m_numPartitions >> 4) : cu->m_numPartitions >> 1;
		break;
	case SIZE_nRx2N:
		outPartIdxLB += puIdx ? (cu->m_numPartitions >> 1) + (cu->m_numPartitions >> 2) + (cu->m_numPartitions >> 4) : cu->m_numPartitions >> 1;
		break;
	default:
		X265_CHECK(0, "unexpected part index\n");
		break;
	}
	return outPartIdxLB;
}
/* Derives the partition index of neighboring bottom right block */
uint32_t CUData_deriveRightBottomIdx(struct CUData *cu, uint32_t puIdx)
{
	uint32_t outPartIdxRB;
	outPartIdxRB = g_rasterToZscan[g_zscanToRaster[cu->m_absIdxInCTU] +
		((1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE - 1)) - 1) * cu->s_numPartInCUSize +
		(1 << (cu->m_log2CUSize[0] - LOG2_UNIT_SIZE)) - 1];

	switch (cu->m_partSize[0])
	{
	case SIZE_2Nx2N:
		outPartIdxRB += cu->m_numPartitions >> 1;
		break;
	case SIZE_2NxN:
		outPartIdxRB += puIdx ? cu->m_numPartitions >> 1 : 0;
		break;
	case SIZE_Nx2N:
		outPartIdxRB += puIdx ? cu->m_numPartitions >> 1 : cu->m_numPartitions >> 2;
		break;
	case SIZE_NxN:
		outPartIdxRB += (cu->m_numPartitions >> 2) * (puIdx - 1);
		break;
	case SIZE_2NxnU:
		outPartIdxRB += puIdx ? cu->m_numPartitions >> 1 : -((int)cu->m_numPartitions >> 3);
		break;
	case SIZE_2NxnD:
		outPartIdxRB += puIdx ? cu->m_numPartitions >> 1 : (cu->m_numPartitions >> 2) + (cu->m_numPartitions >> 3);
		break;
	case SIZE_nLx2N:
		outPartIdxRB += puIdx ? cu->m_numPartitions >> 1 : (cu->m_numPartitions >> 3) + (cu->m_numPartitions >> 4);
		break;
	case SIZE_nRx2N:
		outPartIdxRB += puIdx ? cu->m_numPartitions >> 1 : (cu->m_numPartitions >> 2) + (cu->m_numPartitions >> 3) + (cu->m_numPartitions >> 4);
		break;
	default:
		X265_CHECK(0, "unexpected part index\n");
		break;
	}
	return outPartIdxRB;
}

void CUData_getMvField(const CUData* cu, uint32_t absPartIdx, int picList, struct MVField *outMvField)
{
	if (cu)
	{
		outMvField->mv = cu->m_mv[picList][absPartIdx];
		outMvField->refIdx = cu->m_refIdx[picList][absPartIdx];
	}
	else
	{
		// OUT OF BOUNDARY
		outMvField->mv.x = 0;
		outMvField->mv.y = 0;
		outMvField->refIdx = REF_NOT_VALID;
	}
}
bool CUData_getColMVP(struct CUData *cu, MV* outMV, int* outRefIdx, int picList, int cuAddr, int partUnitIdx)
{
	Frame* colPic = cu->m_slice->m_refPicList[isInterB(cu->m_slice) && !cu->m_slice->m_colFromL0Flag][cu->m_slice->m_colRefIdx];
	CUData* colCU = framedata_getPicCTU(colPic->m_encData, cuAddr);

	uint32_t absPartAddr = partUnitIdx & TMVP_UNIT_MASK;
	if (colCU->m_predMode[partUnitIdx] == MODE_NONE || isIntra_cudata(colCU, absPartAddr))
		return FALSE;

	int colRefPicList = cu->m_slice->m_bCheckLDC ? picList : cu->m_slice->m_colFromL0Flag;

	int colRefIdx = colCU->m_refIdx[colRefPicList][absPartAddr];

	if (colRefIdx < 0)
	{
		colRefPicList = !colRefPicList;
		colRefIdx = colCU->m_refIdx[colRefPicList][absPartAddr];

		if (colRefIdx < 0)
			return FALSE;
	}

	// Scale the vector
	int colRefPOC = colCU->m_slice->m_refPOCList[colRefPicList][colRefIdx];
	int colPOC = colCU->m_slice->m_poc;
	MV colmv = colCU->m_mv[colRefPicList][absPartAddr];

	int curRefPOC = cu->m_slice->m_refPOCList[picList][*outRefIdx];
	int curPOC = cu->m_slice->m_poc;

	*outMV = CUData_scaleMvByPOCDist(&colmv, curPOC, curRefPOC, colPOC, colRefPOC);
	return TRUE;
}

MV CUData_scaleMvByPOCDist(MV *inMV, int curPOC, int curRefPOC, int colPOC, int colRefPOC)
{
	int diffPocD = colPOC - colRefPOC;
	int diffPocB = curPOC - curRefPOC;

	if (diffPocD == diffPocB)
		return *inMV;
	else
	{
		int tdb = x265_clip3(-128, 127, diffPocB);
		int tdd = x265_clip3(-128, 127, diffPocD);
		int x = (0x4000 + abs(tdd / 2)) / tdd;
		int scale = x265_clip3(-4096, 4095, (tdb * x + 32) >> 6);

		int mvx = x265_clip3(-32768, 32767, (scale * inMV->x + 127 + (scale * inMV->x < 0)) >> 8);
		int mvy = x265_clip3(-32768, 32767, (scale * inMV->y + 127 + (scale * inMV->y < 0)) >> 8);
		inMV->x = mvx;
		inMV->y = mvy;
		return *inMV;
	}
}
void CUData_getPartIndexAndSize(struct CUData *cu, uint32_t partIdx, uint32_t *outPartAddr, int *outWidth, int *outHeight)
{
	int cuSize = 1 << cu->m_log2CUSize[0];
	int partType = cu->m_partSize[0];

	int tmp = partTable[partType][partIdx][0];
	*outWidth = ((tmp >> 4) * cuSize) >> 2;
	*outHeight = ((tmp & 0xF) * cuSize) >> 2;
	*outPartAddr = (partAddrTable[partType][partIdx] * cu->m_numPartitions) >> 4;
}
uint32_t CUData_deriveCenterIdx(struct CUData *cu, uint32_t puIdx)
{
	uint32_t absPartIdx;
	int puWidth, puHeight;

	CUData_getPartIndexAndSize(cu, puIdx, &absPartIdx, &puWidth, &puHeight);

	return g_rasterToZscan[g_zscanToRaster[cu->m_absIdxInCTU + absPartIdx]
		+ (puHeight >> (LOG2_UNIT_SIZE + 1)) * cu->s_numPartInCUSize
		+ (puWidth >> (LOG2_UNIT_SIZE + 1))];
}

//创建AMVP列表
/* Constructs a list of candidates for AMVP, and a larger list of motion candidates */
void CUData_getNeighbourMV(CUData *cu, uint32_t puIdx, uint32_t absPartIdx, struct InterNeighbourMV* neighbours)
{
	// Set the temporal neighbour to unavailable by default.
	neighbours[MD_COLLOCATED].unifiedRef = -1;

	uint32_t partIdxLT, partIdxRT, partIdxLB = CUData_deriveLeftBottomIdx(cu, puIdx);
	CUData_deriveLeftRightTopIdx(cu, puIdx, &partIdxLT, &partIdxRT);

	// Load the spatial MVs.
	CUData_getInterNeighbourMV(cu, neighbours + MD_BELOW_LEFT, partIdxLB, MD_BELOW_LEFT);
	CUData_getInterNeighbourMV(cu, neighbours + MD_LEFT, partIdxLB, MD_LEFT);
	CUData_getInterNeighbourMV(cu, neighbours + MD_ABOVE_RIGHT, partIdxRT, MD_ABOVE_RIGHT);
	CUData_getInterNeighbourMV(cu, neighbours + MD_ABOVE, partIdxRT, MD_ABOVE);
	CUData_getInterNeighbourMV(cu, neighbours + MD_ABOVE_LEFT, partIdxLT, MD_ABOVE_LEFT);

	if (cu->m_slice->m_sps->bTemporalMVPEnabled)
	{
		uint32_t absPartAddr = cu->m_absIdxInCTU + absPartIdx;
		uint32_t partIdxRB = CUData_deriveRightBottomIdx(cu, puIdx);

		// co-located RightBottom temporal predictor (H)
		int ctuIdx = -1;

		// image boundary check
		if (framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelX + g_zscanToPelX[partIdxRB] + UNIT_SIZE < cu->m_slice->m_sps->picWidthInLumaSamples &&
			framedata_getPicCTU(cu->m_encData, cu->m_cuAddr)->m_cuPelY + g_zscanToPelY[partIdxRB] + UNIT_SIZE < cu->m_slice->m_sps->picHeightInLumaSamples)
		{
			uint32_t absPartIdxRB = g_zscanToRaster[partIdxRB];
			uint32_t numUnits = cu->s_numPartInCUSize;
			bool bNotLastCol = lessThanCol(absPartIdxRB, numUnits - 1, numUnits); // is not at the last column of CTU
			bool bNotLastRow = lessThanRow(absPartIdxRB, numUnits - 1, numUnits); // is not at the last row    of CTU

			if (bNotLastCol && bNotLastRow)
			{
				absPartAddr = g_rasterToZscan[absPartIdxRB + numUnits + 1];
				ctuIdx = cu->m_cuAddr;
			}
			else if (bNotLastCol)
				absPartAddr = g_rasterToZscan[(absPartIdxRB + numUnits + 1) & (numUnits - 1)];
			else if (bNotLastRow)
			{
				absPartAddr = g_rasterToZscan[absPartIdxRB + 1];
				ctuIdx = cu->m_cuAddr + 1;
			}
			else // is the right bottom corner of CTU
				absPartAddr = 0;
		}

		if (!(ctuIdx >= 0 && CUData_getCollocatedMV(cu, ctuIdx, absPartAddr, neighbours + MD_COLLOCATED)))
		{
			uint32_t partIdxCenter = CUData_deriveCenterIdx(cu, puIdx);
			uint32_t curCTUIdx = cu->m_cuAddr;
			CUData_getCollocatedMV(cu, curCTUIdx, partIdxCenter, neighbours + MD_COLLOCATED);
		}
	}
}

void CUData_getInterNeighbourMV(CUData *cu, struct InterNeighbourMV *neighbour, uint32_t partUnitIdx, MVP_DIR dir)
{
	CUData* tmpCU = NULL;
	uint32_t idx = 0;

	switch (dir)
	{
	case MD_LEFT:
		tmpCU = CUData_getPULeft(cu, &idx, partUnitIdx);
		break;
	case MD_ABOVE:
		tmpCU = CUData_getPUAbove(cu, &idx, partUnitIdx);
		break;
	case MD_ABOVE_RIGHT:
		tmpCU = CUData_getPUAboveRight(cu, &idx, partUnitIdx);
		break;
	case MD_BELOW_LEFT:
		tmpCU = CUData_getPUBelowLeft(cu, &idx, partUnitIdx);
		break;
	case MD_ABOVE_LEFT:
		tmpCU = CUData_getPUAboveLeft(cu, &idx, partUnitIdx);
		break;
	default:
		break;
	}

	if (!tmpCU)
	{
		// Mark the PMV as unavailable.
		for (int i = 0; i < 2; i++)
			neighbour->refIdx[i] = -1;
		return;
	}

	for (int i = 0; i < 2; i++)
	{
		// Get the MV.
		neighbour->mv[i] = tmpCU->m_mv[i][idx];

		// Get the reference idx.
		neighbour->refIdx[i] = tmpCU->m_refIdx[i][idx];
	}
}
// Cache the collocated MV.
bool CUData_getCollocatedMV(CUData *cu, int cuAddr, int partUnitIdx, struct InterNeighbourMV *neighbour)
{
	const Frame* colPic = cu->m_slice->m_refPicList[isInterB(cu->m_slice) && !cu->m_slice->m_colFromL0Flag][cu->m_slice->m_colRefIdx];
	CUData* colCU = framedata_getPicCTU(colPic->m_encData, cuAddr);

	uint32_t absPartAddr = partUnitIdx & TMVP_UNIT_MASK;
	if (colCU->m_predMode[partUnitIdx] == MODE_NONE || isIntra_cudata(colCU, absPartAddr))
		return FALSE;

	for (int list = 0; list < 2; list++)
	{
		neighbour->cuAddr[list] = cuAddr;
		int colRefPicList = cu->m_slice->m_bCheckLDC ? list : cu->m_slice->m_colFromL0Flag;
		int colRefIdx = colCU->m_refIdx[colRefPicList][absPartAddr];

		if (colRefIdx < 0)
			colRefPicList = !colRefPicList;

		neighbour->refIdx[list] = colCU->m_refIdx[colRefPicList][absPartAddr];
		neighbour->refIdx[list] |= colRefPicList << 4;

		neighbour->mv[list] = colCU->m_mv[colRefPicList][absPartAddr];
	}

	return neighbour->unifiedRef != -1;
}

/* Clip motion vector to within slightly padded boundary of picture (the
* MV may reference a block that is completely within the padded area).
* Note this function is unaware of how much of this picture is actually
* available for use (re: frame parallelism) */
void CUData_clipMv(const CUData *cu, MV* outMV)
{
	const uint32_t mvshift = 2;
	uint32_t offset = 8;

	int16_t xmax = (int16_t)((cu->m_slice->m_sps->picWidthInLumaSamples + offset - cu->m_cuPelX - 1) << mvshift);
	int16_t xmin = -(int16_t)((g_maxCUSize + offset + cu->m_cuPelX - 1) << mvshift);

	int16_t ymax = (int16_t)((cu->m_slice->m_sps->picHeightInLumaSamples + offset - cu->m_cuPelY - 1) << mvshift);
	int16_t ymin = -(int16_t)((g_maxCUSize + offset + cu->m_cuPelY - 1) << mvshift);

	outMV->x = X265_MIN(xmax, X265_MAX(xmin, outMV->x));
	outMV->y = X265_MIN(ymax, X265_MAX(ymin, outMV->y));
}

// Load direct spatial MV if available.
bool CUData_getDirectPMV(CUData *cu, MV* pmv, struct InterNeighbourMV *neighbours, uint32_t picList, uint32_t refIdx)
{
	int curRefPOC = cu->m_slice->m_refPOCList[picList][refIdx];
	for (int i = 0; i < 2; i++, picList = !picList)
	{
		int partRefIdx = neighbours->refIdx[picList];
		if (partRefIdx >= 0 && curRefPOC == cu->m_slice->m_refPOCList[picList][partRefIdx])
		{
			*pmv = neighbours->mv[picList];
			//pmv->x=0;pmv->y=0;pmv->word=0;
			return TRUE;
		}
	}
	return FALSE;
}

// Load indirect spatial MV if available. An indirect MV has to be scaled.
bool CUData_getIndirectPMV(CUData *cu, MV* outMV, struct InterNeighbourMV *neighbours, uint32_t picList, uint32_t refIdx)
{
	int curPOC = cu->m_slice->m_poc;
	int neibPOC = curPOC;
	int curRefPOC = cu->m_slice->m_refPOCList[picList][refIdx];

	for (int i = 0; i < 2; i++, picList = !picList)
	{
		int partRefIdx = neighbours->refIdx[picList];
		if (partRefIdx >= 0)
		{
			int neibRefPOC = cu->m_slice->m_refPOCList[picList][partRefIdx];
			MV mvp = neighbours->mv[picList];
			//*outMV = CUData_scaleMvByPOCDist(&mvp, curPOC, curRefPOC, neibPOC, neibRefPOC);
			outMV->x = 0; outMV->y = 0; outMV->word = 0;
			return TRUE;
		}
	}
	return FALSE;
}

// Create the PMV list. Called for each reference index.
int CUData_getPMV(CUData *cu, struct InterNeighbourMV *neighbours, uint32_t picList, uint32_t refIdx, MV* amvpCand, MV* pmv)
{
	MV directMV[MD_ABOVE_LEFT + 1] = { 0 };
	MV indirectMV[MD_ABOVE_LEFT + 1] = { 0 };
	bool validDirect[MD_ABOVE_LEFT + 1] = { 0 };
	bool validIndirect[MD_ABOVE_LEFT + 1] = { 0 };

	// Left candidate.
	validDirect[MD_BELOW_LEFT] = CUData_getDirectPMV(cu, &directMV[MD_BELOW_LEFT], neighbours + MD_BELOW_LEFT, picList, refIdx);
	validDirect[MD_LEFT] = CUData_getDirectPMV(cu, &directMV[MD_LEFT], neighbours + MD_LEFT, picList, refIdx);
	// Top candidate.
	validDirect[MD_ABOVE_RIGHT] = CUData_getDirectPMV(cu, &directMV[MD_ABOVE_RIGHT], neighbours + MD_ABOVE_RIGHT, picList, refIdx);
	validDirect[MD_ABOVE] = CUData_getDirectPMV(cu, &directMV[MD_ABOVE], neighbours + MD_ABOVE, picList, refIdx);
	validDirect[MD_ABOVE_LEFT] = CUData_getDirectPMV(cu, &directMV[MD_ABOVE_LEFT], neighbours + MD_ABOVE_LEFT, picList, refIdx);

	int num = 0;
	// Left predictor search
	if (validDirect[MD_BELOW_LEFT])
		amvpCand[num++] = directMV[MD_BELOW_LEFT];
	else if (validDirect[MD_LEFT])
		amvpCand[num++] = directMV[MD_LEFT];

	bool bAddedSmvp = num > 0;

	// Above predictor search
	if (validDirect[MD_ABOVE_RIGHT])
		amvpCand[num++] = directMV[MD_ABOVE_RIGHT];
	else if (validDirect[MD_ABOVE])
		amvpCand[num++] = directMV[MD_ABOVE];
	else if (validDirect[MD_ABOVE_LEFT])
		amvpCand[num++] = directMV[MD_ABOVE_LEFT];

	int numMvc = 0;
	if (num == 2)
		if (amvpCand[0].x == amvpCand[1].x && amvpCand[0].y == amvpCand[1].y)
			num -= 1;

	// Get the collocated candidate. At this step, either the first candidate
	// was found or its value is 0.
	if (cu->m_slice->m_sps->bTemporalMVPEnabled && num < 2)
	{
		int tempRefIdx = neighbours[MD_COLLOCATED].refIdx[picList];
		if (tempRefIdx != -1)
		{
			uint32_t cuAddr = neighbours[MD_COLLOCATED].cuAddr[picList];
			const Frame* colPic = cu->m_slice->m_refPicList[isInterB(cu->m_slice) && !cu->m_slice->m_colFromL0Flag][cu->m_slice->m_colRefIdx];
			const CUData* colCU = framedata_getPicCTU(colPic->m_encData, cuAddr);

			// Scale the vector
			int colRefPOC = colCU->m_slice->m_refPOCList[tempRefIdx >> 4][tempRefIdx & 0xf];
			int colPOC = colCU->m_slice->m_poc;

			int curRefPOC = cu->m_slice->m_refPOCList[picList][refIdx];
			int curPOC = cu->m_slice->m_poc;

			pmv[numMvc++] = amvpCand[num++] = CUData_scaleMvByPOCDist(&neighbours[MD_COLLOCATED].mv[picList], curPOC, curRefPOC, colPOC, colRefPOC);
		}
	}

	while (num < AMVP_NUM_CANDS)
	{
		amvpCand[num].x = 0;
		amvpCand[num].y = 0;
		amvpCand[num].word = 0;
		num++;
	}
	neighbours = NULL;
	return numMvc;
}
