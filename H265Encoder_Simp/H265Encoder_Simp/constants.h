
#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#include "common.h"

extern int g_ctuSizeConfigured;

void initZscanToRaster(uint32_t maxFullDepth, uint32_t depth, uint32_t startVal, uint32_t** curIdx);
void initRasterToZscan(uint32_t maxFullDepth);

extern double x265_lambda_tab[QP_MAX_MAX + 1];
extern double x265_lambda2_tab[QP_MAX_MAX + 1];
extern const uint16_t x265_chroma_lambda2_offset_tab[MAX_CHROMA_LAMBDA_OFFSET + 1];

enum { ChromaQPMappingTableSize = 70 };
enum { AngleMapping422TableSize = 36 };

extern const uint8_t g_chromaScale[ChromaQPMappingTableSize];
extern const uint8_t g_chroma422IntraAngleMappingTable[AngleMapping422TableSize];

// flexible conversion from relative to absolute index
extern uint32_t g_zscanToRaster[MAX_NUM_PARTITIONS];
extern uint32_t g_rasterToZscan[MAX_NUM_PARTITIONS];

// conversion of partition index to picture pel position
extern const uint8_t g_zscanToPelX[MAX_NUM_PARTITIONS];
extern const uint8_t g_zscanToPelY[MAX_NUM_PARTITIONS];
extern const uint8_t g_log2Size[MAX_CU_SIZE + 1]; // from size to log2(size)

// global variable (CTU width/height, max. CU depth)
extern uint32_t g_maxLog2CUSize;
extern uint32_t g_maxCUSize;
extern uint32_t g_maxCUDepth;
extern uint32_t g_unitSizeDepth; // Depth at which 4x4 unit occurs from max CU size(从最大CU尺寸到4x4单元的深度)

extern int g_t4[4][4];
extern int g_t8[8][8];
extern int g_t16[16][16];
extern int g_t32[32][32];

extern int ig_t4[4][4];
extern int ig_t8[8][8];
extern int ig_t16[16][16];
extern int ig_t32[32][32];
// Subpel interpolation defines and constants

#define NTAPS_LUMA        8                            // Number of taps for luma
#define NTAPS_CHROMA      4                            // Number of taps for chroma
#define IF_INTERNAL_PREC 14                            // Number of bits for internal precision
#define IF_FILTER_PREC    6                            // Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) // Offset used internally
#define SLFASE_CONSTANT  0x5f4e4a53

extern int16_t g_lumaFilter[4][NTAPS_LUMA];      // Luma filter taps
extern int16_t g_chromaFilter[8][NTAPS_CHROMA];  // Chroma filter taps

// Scanning order & context mapping table

#define NUM_SCAN_SIZE 4

extern  uint16_t*  g_scanOrder[NUM_SCAN_TYPE][NUM_SCAN_SIZE];
extern  uint16_t*  g_scanOrderCG[NUM_SCAN_TYPE][NUM_SCAN_SIZE];
extern  uint16_t g_scan8x8diag[8 * 8];
extern  uint16_t g_scan4x4[NUM_SCAN_TYPE][4 * 4];

extern const uint8_t g_lastCoeffTable[32];
extern const uint8_t g_goRiceRange[5]; // maximum value coded with Rice codes

// CABAC tables
extern const uint8_t g_lpsTable[64][4];
extern const uint8_t x265_exp2_lut[64];

// Intra tables
extern const uint8_t g_intraFilterFlags[NUM_INTRA_MODE];
extern const uint32_t g_depthInc[3][4];
extern const uint32_t g_depthScanIdx[8][8];

#endif /* CONSTANTS_H_ */
