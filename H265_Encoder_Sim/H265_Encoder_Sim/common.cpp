/*
* common.c
*
*  Created on: 2015-10-23
*      Author: adminster
*/
#include"common.h"
#include <time.h>

int x265_mdate(void)
{
	clock_t t = clock();
	long sec = t / CLOCKS_PER_SEC;
	return sec;
}
int x265_min(int a, int b)
{
	return a < b ? a : b;
}
int x265_max(int a, int b)
{
	return a > b ? a : b;
}

int x265_clip3(int minVal, int maxVal, int a)
{
	return x265_min(x265_max(minVal, a), maxVal);
}

pixel x265_clip(pixel x)
{
	return (pixel)x265_min(((1 << X265_DEPTH) - 1), x265_max((0), x));
}

int x265_min_int(int a, int b) { return a < b ? a : b; }

int x265_max_int(int a, int b) { return a > b ? a : b; }

int x265_clip3_int(int minVal, int maxVal, int a) { return x265_min_int(x265_max_int(minVal, a), maxVal); }

uint32_t x265_min_uint32_t(uint32_t a, uint32_t b) { return a < b ? a : b; }

uint32_t x265_max_uint32_t(uint32_t a, uint32_t b) { return a > b ? a : b; }

uint32_t x265_clip3_uint32_t(uint32_t minVal, uint32_t maxVal, uint32_t a) { return x265_min_uint32_t(x265_max_uint32_t(minVal, a), maxVal); }

