/*
* common.h
*
*  Created on: 2015-10-11
*      Author: adminster
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "x265.h"
#include <stdio.h>

#ifndef COMMON_H_
#define COMMON_H_

#define NUM_OF_I_FRAME 1

#ifndef _SIZE_T
#define _SIZE_T
//typedef unsigned int size_t;
#endif

#ifndef int32_t
#define int32_t int
#endif

#ifndef uint32_t
#define uint32_t unsigned int
#endif

#define COPY1_IF_LT(x, y) if ((y) < (x)) (x) = (y);
#define COPY2_IF_LT(x, y, a, b) \
    if ((y) < (x)) \
	    { \
        (x) = (y); \
        (a) = (b); \
	    }
#define COPY3_IF_LT(x, y, a, b, c, d) \
    if ((y) < (x)) \
	    { \
        (x) = (y); \
        (a) = (b); \
        (c) = (d); \
	    }
#define COPY4_IF_LT(x, y, a, b, c, d, e, f) \
    if ((y) < (x)) \
	    { \
        (x) = (y); \
        (a) = (b); \
        (c) = (d); \
        (e) = (f); \
	    }
/* Types for `void *' pointers.  */

#define CHROMA_H_SHIFT(x) (x == X265_CSP_I420 || x == X265_CSP_I422)
#define CHROMA_V_SHIFT(x) (x == X265_CSP_I420)
#define X265_MAX_PRED_MODE_PER_CTU 85 * 2 * 8
#define X265_CHECK(expr,input) if(!(expr)){\
    printf("%s",input);\
    exit(0);\
}\

#define CHECKED_MALLOC(var, type, count) \
	    { \
        var = (type*)malloc(sizeof(type) * (count)); \
        if (!var) \
		        { \
            printf( "malloc of size %d failed\n", sizeof(type) * (count)); \
            goto fail; \
		        } \
	    }

#define X265_MALLOC(type, count)    (type*)malloc(sizeof(type) * count)

#define X265_MIN(a, b) ((a) < (b) ? (a) : (b))
#define X265_MAX(a, b) ((a) > (b) ? (a) : (b))

#define QP_BD_OFFSET (6 * (X265_DEPTH - 8))
#define COEF_REMAIN_BIN_REDUCTION   3 // indicates the level at which the VLC
// transitions from Golomb-Rice to TU+EG(k)
#define NUM_CU_DEPTH            4                           // maximum number of CU depths（CU深度的最大数目）
#define NUM_FULL_DEPTH          5                           // maximum number of full depths
#define MIN_LOG2_CU_SIZE        3                           // log2(minCUSize)
#define MAX_LOG2_CU_SIZE        6                           // log2(maxCUSize)
#define MIN_CU_SIZE             (1 << MIN_LOG2_CU_SIZE)     // minimum allowable size of CU
#define MAX_CU_SIZE             (1 << MAX_LOG2_CU_SIZE)     // maximum allowable size of CU

#define LOG2_UNIT_SIZE          2                           // log2(unitSize)
#define UNIT_SIZE               (1 << LOG2_UNIT_SIZE)       // unit size of CU partition

#define MAX_NUM_PARTITIONS      256
#define NUM_4x4_PARTITIONS      (1U << (g_unitSizeDepth << 1)) // number of 4x4 units in max CU size

#define QP_MIN          0
#define QP_MAX_SPEC     51 /* max allowed signaled QP in HEVC */
#define QP_MAX_MAX      69 /* max allowed QP to be output by rate control */

#define MAX_LOG2_TR_SIZE 5
#define MAX_LOG2_TS_SIZE 2 // TODO: RExt
#define MAX_TR_SIZE (1 << MAX_LOG2_TR_SIZE)
#define MAX_TS_SIZE (1 << MAX_LOG2_TS_SIZE)

#define MLS_GRP_NUM                 64 // Max number of coefficient groups, max(16, 64)
#define MLS_CG_SIZE                 4  // Coefficient group size of 4x4
#define MLS_CG_BLK_SIZE             (MLS_CG_SIZE * MLS_CG_SIZE)
#define MLS_CG_LOG2_SIZE            2

#define SBH_THRESHOLD               4 // fixed sign bit hiding controlling threshold
#define C1FLAG_NUMBER               8 // maximum number of largerThan1 flag coded in one chunk:  16 in HM5
#define C2FLAG_NUMBER               1 // maximum number of largerThan2 flag coded in one chunk:  16 in HM5

#define MAX_CHROMA_LAMBDA_OFFSET 36

#ifndef NULL
#define NULL 0
#endif

typedef signed char int8_t;
typedef short int int16_t;
typedef signed long   int64_t;

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned long long uint64_t;

typedef int16_t  coeff_t;      // transform coefficient
//typedef long intptr_t;

#ifndef bool
#define bool int
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif
/*enum  bool
{
FALSE,
TRUE
};*/

#define NUM_INTRA_MODE 35

#define SCAN_SET_SIZE               16
#define LOG2_SCAN_SET_SIZE          4

#define ALL_IDX                     -1
#define PLANAR_IDX                  0
#define VER_IDX                     26 // index for intra VERTICAL   mode
#define HOR_IDX                     10 // index for intra HORIZONTAL mode
#define DC_IDX                      1  // index for intra DC mode
#define NUM_CHROMA_MODE             5  // total number of chroma modes
#define DM_CHROMA_IDX               36 // chroma mode index for derived from luma intra mode

#define MDCS_ANGLE_LIMIT            4 // distance from true angle that horiz or vertical scan is allowed
#define MDCS_LOG2_MAX_SIZE          3 // TUs with log2 of size greater than this can only use diagonal scan

#define MAX_NUM_REF_PICS            16 // max. number of pictures used for reference
#define MAX_NUM_REF                 16 // max. number of entries in picture reference list

#define REF_NOT_VALID               -1

#define AMVP_NUM_CANDS              2 // number of AMVP candidates
#define MRG_MAX_NUM_CANDS           5 // max number of final merge candidates

#define MIN_PU_SIZE             4
#define MIN_TU_SIZE             4
#define MAX_NUM_SPU_W           (MAX_CU_SIZE / MIN_PU_SIZE) // maximum number of SPU in horizontal line
#define X265_CSP_COUNT          4  /* Number of supported internal color spaces */

#define FENC_STRIDE 64
#define NUM_INTRA_MODE 35

typedef uint8_t  pixel;
typedef uint16_t sum_t;
typedef unsigned int sum2_t;
typedef unsigned int pixel4;
typedef int  ssum2_t;      //Signed sum

#define  X265_DEPTH   8           // compile time configurable bit depth

#define MAX_UINT        0xFFFFFFFFU // max. value of unsigned 32-bit integer
#define MAX_INT         2147483647  // max. value of signed 32-bit integer
#define MAX_INT64       0x7FFFFFFFLL  // max. value of signed 64-bit integer
#define MAX_DOUBLE      1.7e+308    // max. value of double-type value

#define QP_MIN          0
#define QP_MAX_SPEC     51 /* max allowed signaled QP in HEVC */
#define QP_MAX_MAX      69 /* max allowed QP to be output by rate control */

#define MIN_QPSCALE     0.21249999999999999
#define MAX_MAX_QPSCALE 615.46574234477100

#define BITS_FOR_POC 8

#ifndef UINT32_MAX
#define UINT32_MAX	0xffffffff
#endif

#define X265_LOOKAHEAD_MAX 250

inline int x265_min(int a, int b);
inline int x265_max(int a, int b);
inline int x265_clip3(int minVal, int maxVal, int a);
inline pixel x265_clip(pixel x);

/* Stores inter analysis data for a single frame */
typedef struct analysis_inter_data
{
	int32_t*    ref;
	uint8_t*    depth;
	uint8_t*    modes;
	uint32_t*   bestMergeCand;
}analysis_inter_data;

/* Stores intra analysis data for a single frame. This struct needs better packing */
typedef struct analysis_intra_data
{
	uint8_t*  depth;
	uint8_t*  modes;
	char*     partSizes;
	uint8_t*  chromaModes;
}analysis_intra_data;

typedef enum TextType
{
	TEXT_LUMA = 0,  // luma
	TEXT_CHROMA_U = 1,  // chroma U
	TEXT_CHROMA_V = 2,  // chroma V
	MAX_NUM_COMPONENT = 3
}TextType;

// coefficient scanning type used in ACS
enum ScanType
{
	SCAN_DIAG = 0,     // up-right diagonal scan
	SCAN_HOR = 1,      // horizontal first scan
	SCAN_VER = 2,      // vertical first scan
	NUM_SCAN_TYPE = 3
};

enum SignificanceMapContextType
{
	CONTEXT_TYPE_4x4 = 0,
	CONTEXT_TYPE_8x8 = 1,
	CONTEXT_TYPE_NxN = 2,
	CONTEXT_NUMBER_OF_TYPES = 3
};

enum { SAO_NUM_OFFSET = 4 };

typedef enum SaoMergeMode
{
	SAO_MERGE_NONE,
	SAO_MERGE_LEFT,
	SAO_MERGE_UP
}SaoMergeMode;

typedef struct SaoCtuParam
{
	SaoMergeMode mergeMode;
	int  typeIdx;
	unsigned int bandPos;    // BO band position
	int  offset[SAO_NUM_OFFSET];

}SaoCtuParam;

typedef struct SAOParam
{
	SaoCtuParam* ctuParam[3];
	bool         bSaoFlag[2];
	int          numCuInWidth;
}SAOParam;

int x265_min_int(int a, int b);

int x265_max_int(int a, int b);

int x265_clip3_int(int minVal, int maxVal, int a);

uint32_t x265_min_uint32_t(uint32_t a, uint32_t b);

uint32_t x265_max_uint32_t(uint32_t a, uint32_t b);

uint32_t x265_clip3_uint32_t(uint32_t minVal, uint32_t maxVal, uint32_t a);

void ECS_memcpy(void *dst, void *src, int count);
#endif /* COMMON_H_ */
