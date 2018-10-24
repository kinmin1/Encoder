/*
* sao.h
*
*  Created on: 2016-7-19
*      Author: Administrator
*/

#ifndef SAO_H_
#define SAO_H_

#include "common.h"
#include "frame.h"
#include "entropy.h"

enum SAOTypeLen
{
	SAO_EO_LEN = 4,
	SAO_BO_LEN = 4,
	SAO_NUM_BO_CLASSES = 32
};

enum SAOType
{
	SAO_EO_0 = 0,
	SAO_EO_1,
	SAO_EO_2,
	SAO_EO_3,
	SAO_BO,
	MAX_NUM_SAO_TYPE
};

#endif /* SAO_H_ */


