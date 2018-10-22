/*
* param.h
*
*  Created on: 2015-11-5
*      Author: adminster
*/

#ifndef PARAM_H_
#define PARAM_H_
#include"x265.h"

int   x265_check_params(x265_param *param);
int   x265_set_globals(x265_param *param);
void  x265_print_params(x265_param *param);
void  x265_print_reconfigured_params(x265_param* param, x265_param* reconfiguredParam);
void  x265_param_apply_fastfirstpass(x265_param *p);
char* x265_param2string(x265_param *param);
int   x265_atoi(const char *str, int* bError);
int   parseCpuName(const char *value, int *bError);
void  setParamAspectRatio(x265_param *p, int width, int height);
void  getParamAspectRatio(x265_param *p, int *width, int *height);
int  parseLambdaFile(x265_param *param);

void x265_param_default(x265_param *param);

#define MAXPARAMSIZE 2000
#endif /* PARAM_H_ */
