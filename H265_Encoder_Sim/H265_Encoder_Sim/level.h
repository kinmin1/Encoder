/*
* level.h
*
*  Created on: 2016-8-10
*      Author: Administrator
*/

#ifndef LEVEL_H_
#define LEVEL_H_

#include "common.h"
#include "x265.h"
#include "slice.h"

// encoder private namespace
void determineLevel(x265_param *param, VPS* vps);
bool enforceLevel(x265_param* param, VPS* vps);


#endif /* LEVEL_H_ */
