/*
* deblock.h
*
*  Created on: 2017-8-22
*      Author: Administrator
*/

#ifndef DEBLOCK_H_
#define DEBLOCK_H_

#include "common.h"
#include "cudata.h"

struct CUData;
struct CUGeom;

enum EDGE{ EDGE_VER, EDGE_HOR };


void deblockCTU(CUData* ctu, CUGeom* cuGeom, int32_t dir);


// CU-level deblocking function
void deblockCU(CUData* cu, CUGeom* cuGeom, const int32_t dir, uint8_t blockStrength[]);

// set filtering functions
void setEdgefilterTU(CUData* cu, uint32_t absPartIdx, uint32_t tuDepth, int32_t dir, uint8_t blockStrength[]);
void setEdgefilterPU(CUData* cu, uint32_t absPartIdx, int32_t dir, uint8_t blockStrength[], uint32_t numUnits);
void setEdgefilterMultiple(CUData* cu, uint32_t absPartIdx, int32_t dir, int32_t edgeIdx, uint8_t value, uint8_t blockStrength[], uint32_t numUnits);

// get filtering functions
uint8_t getBoundaryStrength(CUData* cuQ, int32_t dir, uint32_t partQ, const uint8_t blockStrength[]);

// filter luma/chroma functions
void edgeFilterLuma(CUData* cuQ, uint32_t absPartIdx, uint32_t depth, int32_t dir, int32_t edge, const uint8_t blockStrength[]);
void edgeFilterChroma(CUData* cuQ, uint32_t absPartIdx, uint32_t depth, int32_t dir, int32_t edge, const uint8_t blockStrength[]);


#endif /* DEBLOCK_H_ */
