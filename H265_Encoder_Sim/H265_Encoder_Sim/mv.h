/*
* mv.h
*
*  Created on: 2015-10-11
*      Author: adminster
*/
#include "common.h"
#include "primitives.h"

#ifndef MV_H_
#define MV_H_

typedef struct MV
{

	int16_t x, y;
	int32_t word;
}MV;

void init_MV(MV *mv, int a, int b, int c);
MV* mvzero(MV *mv, int16_t a, int16_t b);
bool notZero(MV *mv);
int16_t toQPel(int16_t toQPel);
int16_t clipped(int16_t xy, int16_t _min, int16_t _max);
int16_t roundToFPel(int16_t xy);
bool isSubpel(MV *isSubpel);
bool checkRange(int16_t x, int16_t y, const MV* _min, const MV* _max);

#endif /* MV_H_ */
