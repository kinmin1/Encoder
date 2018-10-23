/*
* reference.h
*
*  Created on: 2015-11-7
*      Author: adminster
*/

#ifndef REFERENCE_H_
#define REFERENCE_H_

#include "primitives.h"
#include "picyuv.h"
#include "lowres.h"
#include "mv.h"

typedef struct MotionReference
{
	//struct PicYuv;
	ReferencePlanes *referenceplanes;

	pixel*  weightBuffer[3];
	int     numInterpPlanes;
	int     numWeightedRows;
}MotionReference;

int MotionReference_init(MotionReference *motionref, PicYuv* recPic, /*struct WeightParam *wp,*/ x265_param* p);

#endif /* REFERENCE_H_ */
