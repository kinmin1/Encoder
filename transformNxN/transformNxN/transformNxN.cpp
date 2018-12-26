#include<stdio.h>
#include<math.h>
#include<malloc.h>
#include"transformNxN.h"

//变换(反变换)声明
void partialButterfly4_1(int (*a)[4],int** b,int** c,int shift,int line);
void partialButterfly4_2(int** a,int (*b)[4],int** c,int shift,int line);

void partialButterfly8_1(int (*a)[8],int** b,int** c,int shift,int line);
void partialButterfly8_2(int** a,int (*b)[8],int** c,int shift,int line);

void partialButterfly16_1(int (*a)[16],int** b,int** c,int shift,int line);
void partialButterfly16_2(int** a,int (*b)[16],int** c,int shift,int line);

void partialButterfly32_1(int (*a)[32],int** b,int** c,int shift,int line);
void partialButterfly32_2(int** a,int (*b)[32],int** c,int shift,int line);

//量化声明
int prescale_quantize(int **W,int **Z,int num,int log2Trsize);
//反量化声明
void rescale(int **Z,int **Wi,int num,int log2Trsize);
void sum_diff(int **W,int Z,int num);
int numSig=0;

	
	/*int blo[4][4]=
	{
		{1,2,3,4},
		{5,6,7,8},
		{9,10,11,12},
		{13,14,15,16}
		
		{-256 , -256, -256, -256},
		{-256 , -256, -256, -256},
		{-256 , -256, -256, -256},
		{-256 , -256, -256, -256}
		
	};*/
	/*
	int blo[8][8]=
	{
		{69,       124,       124,       124,       123,       123,       122,       122}, 
		{68,       123,       122,       123,       122,       122,       122,       121},
		{68,       123,       122,       122,       122,       121,       120,       120},
		{68,       122,       120,       121,       121,       120,       120,       119},
		{67,       121,       121,       120,       119,       119,       119,       119},
		{67,       121,       118,       119,       119,       118,       118,       117},
		{66,       118,       119,       118,       117,       116,       117,       117},
		{66,       119,       116,       116,       115,       114,       115,       114}
	};*/
	/*
	int blo[8][8]=
	{
		{-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256},

		{-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256}

	};*/
	
	/*
	int blo[16][16]=
	{
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256}
	};
	*/
	
	int blo[32][32]=
	{
		{ -80, -80, -80, -80, -80, -77, -73, -73, -73, -73, -75, -80, -81, -81, -81, -80, -81, -81, -80, -81, -80, -80, -80, -80, -80, -80, -79, -81, -87, -88, -86, -86 },
		{ -93, -85, -85, -85, -85, -85, -85, -85, -85, -85, -85, -84, -84, -84, -84, -85, -84, -84, -85, -84, -84, -85, -84, -84, -84, -84, -84, -83, -87, -87, -86, -86 },
		{ -93, -89, -88, -88, -88, -88, -88, -88, -87, -87, -87, -87, -86, -86, -85, -86, -86, -86, -86, -87, -87, -87, -87, -87, -87, -88, -88, -86, -87, -87, -86, -86 },
		{ -93, -70, -71, -71, -71, -72, -72, -72, -72, -71, -72, -71, -71, -70, -70, -70, -70, -72, -72, -72, -72, -72, -74, -75, -75, -79, -86, -85, -87, -88, -86, -86 },
		{ -92, -52, -53, -55, -55, -55, -56, -56, -56, -55, -55, -55, -55, -54, -54, -54, -53, -56, -56, -56, -57, -56, -57, -58, -59, -69, -84, -85, -87, -88, -86, -86 },
		{ -92, -48, -50, -52, -52, -53, -53, -53, -53, -52, -52, -51, -52, -50, -50, -50, -49, -51, -52, -51, -51, -51, -52, -54, -54, -65, -83, -85, -87, -88, -85, -85 },
		{ -92, -40, -48, -52, -51, -52, -52, -52, -53, -52, -48, -41, -40, -38, -37, -37, -37, -38, -38, -38, -38, -39, -43, -50, -53, -64, -82, -85, -86, -87, -86, -86 },
		{ -92, -33, -47, -50, -50, -52, -52, -51, -51, -50, -40, -29, -27, -26, -25, -26, -25, -26, -27, -27, -28, -28, -33, -47, -53, -64, -83, -85, -87, -87, -86, -85 },
		{ -92, -30, -47, -49, -49, -50, -50, -50, -50, -47, -35, -23, -21, -19, -19, -19, -19, -21, -23, -24, -25, -22, -28, -45, -52, -64, -83, -85, -86, -87, -85, -85 },
		{ -92, -29, -46, -48, -48, -49, -49, -49, -49, -46, -31, -19, -15, -10, -6, -6, -8, -12, -15, -21, -22, -17, -25, -45, -52, -63, -82, -85, -86, -87, -85, -85 },
		{ -92, -29, -44, -47, -47, -47, -47, -47, -47, -43, -28, -16, -12, -6, -1, -2, -1, -5, -9, -17, -22, -21, -29, -46, -51, -64, -83, -85, -86, -87, -86, -85 },
		{ -92, -31, -43, -46, -46, -46, -46, -46, -46, -42, -24, -10, -9, -5, -1, +0, +3, -2, -7, -12, -18, -21, -29, -45, -51, -63, -82, -84, -86, -87, -85, -85 },
		{ -91, -32, -41, -44, -44, -45, -45, -45, -45, -40, -21, -5, -4, -3, +0, +2, +3, -1, -5, -8, -15, -17, -26, -44, -50, -63, -82, -84, -86, -86, -85, -84 },
		{ -91, -30, -40, -43, -42, -43, -43, -43, -43, -38, -19, -4, -2, -2, +0, +0, -1, -4, -6, -9, -13, -15, -25, -44, -49, -62, -82, -84, -86, -86, -84, -84 },
		{ -91, -28, -37, -40, -41, -42, -42, -41, -41, -37, -19, -5, -3, -1, +0, +0, -1, -4, -6, -8, -11, -13, -24, -42, -48, -62, -82, -84, -86, -86, -85, -84 },
		{ -91, -26, -36, -39, -39, -41, -40, -40, -39, -35, -18, -5, +2, +0, +0, +0, -1, -4, -6, -8, -11, -13, -23, -42, -47, -62, -83, -84, -86, -86, -84, -84 },
		{ -91, -28, -35, -38, -38, -39, -39, -39, -39, -35, -21, +1, +19, +7, -2, -1, -2, -4, -7, -9, -11, -14, -25, -41, -46, -61, -82, -83, -85, -86, -84, -84 },
		{ -91, -32, -35, -36, -37, -38, -38, -38, -38, -36, -28, -6, +15, -1, -11, -8, -9, -11, -12, -14, -16, -19, -29, -41, -45, -60, -82, -83, -85, -86, -84, -83 },
		{ -91, -32, -34, -34, -35, -36, -36, -36, -36, -36, -34, -29, -24, -30, -29, -27, -28, -30, -29, -30, -31, -33, -37, -41, -43, -59, -82, -83, -85, -85, -84, -83 },
		{ -91, -31, -33, -34, -35, -35, -35, -34, -35, -34, -34, -32, -33, -34, -32, -32, -35, -36, -36, -36, -37, -37, -38, -41, -42, -60, -82, -83, -84, -85, -84, -83 },
		{ -90, -31, -32, -33, -33, -34, -34, -34, -34, -33, -32, -32, -32, -34, -31, -31, -34, -35, -34, -34, -35, -35, -36, -39, -41, -59, -82, -83, -84, -85, -83, -83 },
		{ -91, -31, -32, -33, -32, -33, -33, -33, -32, -32, -32, -32, -33, -34, -33, -32, -33, -34, -34, -34, -35, -35, -36, -38, -40, -59, -82, -83, -84, -84, -83, -82 },
		{ -90, -31, -31, -32, -31, -32, -32, -32, -31, -31, -31, -32, -33, -33, -33, -32, -33, -34, -33, -34, -34, -35, -35, -36, -38, -59, -82, -82, -83, -84, -82, -82 },
		{ 90, -30, -31, -31, -31, -31, -31, -31, -30, -30, -31, -31, -32, -32, -32, -32, -32, -33, -33, -33, -33, -34, -34, -34, -37, -58, -82, -82, -83, -84, -82, -81 },
		{ -90, -30, -30, -31, -30, -31, -30, -30, -30, -30, -30, -30, -32, -32, -32, -31, -32, -33, -33, -33, -32, -32, -32, -33, -36, -57, -81, -81, -83, -83, -82, -81 },
		{ -90, -30, -30, -30, -30, -31, -30, -29, -30, -28, -26, -26, -27, -27, -27, -27, -28, -30, -30, -30, -30, -31, -31, -32, -35, -57, -82, -81, -82, -83, -81, -81 },
		{ -90, -30, -29, -29, -29, -31, -30, -29, -30, -26, -19, -20, -21, -20, -19, -18, -18, -19, -19, -19, -21, -27, -31, -31, -34, -57, -82, -80, -82, -83, -82, -81 },
		{ -89, -30, -29, -28, -29, -30, -30, -29, -29, -22, -17, -23, -26, -25, -24, -22, -20, -19, -17, -14, -11, -22, -29, -29, -33, -56, -82, -80, -81, -83, -81, -81 },
		{ -89, -29, -29, -29, -29, -29, -29, -29, -29, -20, -16, -26, -28, -27, -27, -25, -22, -23, -24, -18, -11, -21, -28, -28, -31, -56, -82, -79, -80, -82, -81, -80 },
		{ -89, -29, -29, -28, -28, -28, -28, -28, -27, -17, -16, -26, -27, -25, -25, -24, -22, -23, -24, -17, -10, -21, -27, -26, -29, -55, -82, -79, -80, -82, -80, -80 },
		{ -89, -28, -27, -27, -27, -27, -27, -28, -26, -14, -15, -24, -24, -23, -22, -23, -23, -24, -23, -14, -8, -21, -25, -25, -28, -55, -82, -78, -80, -81, -80, -79 },
		{ -88, -28, -27, -27, -27, -27, -27, -27, -25, -13, -16, -23, -23, -22, -22, -22, -22, -23, -21, -10, -7, -21, -24, -23, -27, -55, -83, -77, -79, -82, -80, -79 }

		/*
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
		{-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256,-256 , -256, -256, -256},
	*/
	};

void transform1()
{
	int num=(int)sqrt((double)sizeof(blo)/(double)sizeof(int));
	int i,j;
	int **block;
	int **mid;
	int **dct;
	int **quant;
	int **iquant;
	int **imid;
	int **idct;
/*
	for(i=0;i<=32;i++)
		for(j=1;j<=32;j++)
		{
			blo[i][j-1]=j+32*i;
		}*/
	//分配内存
	block = (int **)malloc(sizeof(int *)*num);//分配一维的大小
	for (i=0; i<num; i++)
	{
		block[i] = (int *)malloc(sizeof(int)*num);//分配二维的大小
	}

	mid = (int **)malloc(sizeof(int *)*num);//分配一维的大小
	for (i=0; i<num; i++)
	{
		mid[i] = (int *)malloc(sizeof(int)*num);//分配二维的大小
	}
	dct = (int **)malloc(sizeof(int *)*num);//分配一维的大小
	for (i=0; i<num; i++)
	{
		dct[i] = (int *)malloc(sizeof(int)*num);//分配二维的大小
	}

	quant = (int **)malloc(sizeof(int *)*num);//分配一维的大小
	for (i=0; i<num; i++)
	{
		quant[i] = (int *)malloc(sizeof(int)*num);//分配二维的大小
	}
	iquant = (int **)malloc(sizeof(int *)*num);//分配一维的大小
	for (i=0; i<num; i++)
	{
		iquant[i] = (int *)malloc(sizeof(int)*num);//分配二维的大小
	}

	imid = (int **)malloc(sizeof(int *)*num);//分配一维的大小
	for (i=0; i<num; i++)
	{
		imid[i] = (int *)malloc(sizeof(int)*num);//分配二维的大小
	}
	idct = (int **)malloc(sizeof(int *)*num);//分配一维的大小
	for (i=0; i<num; i++)
	{
		idct[i] = (int *)malloc(sizeof(int)*num);//分配二维的大小
	}
	int diff[32][32]={0};
	int sumdiff=0;

		printf("====================================block=================================\n");
	for(j=0; j<num; j++)
	for(i=0; i<num;i++)
	{
		block[j][i]=blo[j][i];
	}
	for(j=0; j<num; j++)
	{
		for(i=0; i<num;i++)
		{
			printf("%d\t",block[j][i]);
		}
		printf("\n");
	}

	if(num==4)
	{
		int shift1=1 + dep -8;
		int shift2=8;
		partialButterfly4_1(g_t4,block,mid,shift1,4);
		
		partialButterfly4_2(mid,ig_t4,dct,shift2,4);
	
		printf("====================================dct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",dct[j][i]);
			}
			printf("\n");
		}
		//量化
		prescale_quantize(dct,quant,num,2);

		printf("====================================quant=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",quant[j][i]);
			}
			printf("\n");
		}
		//反量化
		rescale(quant,iquant,num,2);
	
		//反变换
		partialButterfly4_1(ig_t4,iquant,imid,7,4);

		partialButterfly4_2(imid,g_t4,idct,12,4);
		printf("====================================idct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",idct[j][i]);
			}
			printf("\n");
		}
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				diff[j][i]=idct[j][i]-blo[j][i];
			}
		}
		printf("====================================diff=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",diff[j][i]);
			}
			printf("\n");
		}
	}
	
	if(num==8)
	{
		int shift1=2 + dep -8;
		int shift2=9;
		partialButterfly8_1(g_t8,block,mid,shift1,8);

		printf("====================================semi_dct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",mid[j][i]);
			}
			printf("\n");
		}
		partialButterfly8_2(mid,ig_t8,dct,shift2,8);

		printf("====================================dct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",dct[j][i]);
			}
			printf("\n");
		}
		//量化
		numSig = prescale_quantize(dct,quant,num,3);


		printf("====================================quant=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",quant[j][i]);
			}
			printf("\n");
		}
		//sum_diff(quant,sumdiff,8);
		
		//反量化
		rescale(quant,iquant,num,3);

		printf("====================================iquant=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",iquant[j][i]);
			}
			printf("\n");
		}
		//反变换
		partialButterfly8_1(ig_t8,iquant,imid,7,8);
		
		printf("====================================imid=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",imid[j][i]);
			}
			printf("\n");
		}
		partialButterfly8_2(imid,g_t8,idct,12,8);
		printf("====================================idct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",idct[j][i]);
			}
			printf("\n");
		}

		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				diff[j][i]=idct[j][i]-blo[j][i];
			}
		}
		printf("====================================diff=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",diff[j][i]);
			}
			printf("\n");
		}

		for(i=0;i<num;i++)
			for(j=0;j<num;j++)
			{
				sumdiff+=abs(diff[i][j]);
			}  
		printf("====================================sumdiff=====================================\n");
		printf("sumdiff=%d\n",sumdiff);
	

	}
	if(num==16)
	{
		int shift1=3 + dep -8;
		int shift2=10;
		partialButterfly16_1(g_t16,block,mid,shift1,16);

		partialButterfly16_2(mid,ig_t16,dct,shift2,16);
		
		//量化
		prescale_quantize(dct,quant,num,4);
		printf("====================================dct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",quant[j][i]);
			}
			printf("\n");
		}

		//反量化
		rescale(quant,iquant,num,4);

		//反变换
		partialButterfly16_1(ig_t16,iquant,imid,7,16);

		partialButterfly16_2(imid,g_t16,idct,12,16);
		printf("====================================idct=====================================\n\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",idct[j][i]);
			}
			printf("\n");
		}
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				diff[j][i]=idct[j][i]-blo[j][i];
			}
		}
		printf("====================================diff=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",diff[j][i]);
			}
			printf("\n");
		}
	}

	if(num==32)
	{
		//转置
		int ig_t32[32][32];
		for(i=0;i<32;i++)
			for(j=0; j<32; j++)
			{  
				ig_t32[i][j]=g_t32[j][i];
			}
		int shift1=4 + dep -8;
		int shift2=11;
		partialButterfly32_1(g_t32,block,mid,shift1,32);
		
		printf("====================================semi_dct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",mid[j][i]);
			}
			printf("\n");
		}


		partialButterfly32_2(mid,ig_t32,dct,shift2,32);
		printf("====================================dct=====================================\n");
		for (j = 0; j<num; j++)
		{
			for (i = 0; i<num; i++)
			{
				printf("%d\t", dct[j][i]);
			}
			printf("\n");
		}
		//量化
		prescale_quantize(dct,quant,num,5);
		printf("====================================quant=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",quant[j][i]);
			}
			printf("\n");
		}

		//反量化
		rescale(quant,iquant,num,5);
		//反变换
		partialButterfly32_1(ig_t32,iquant,imid,7,32);

		partialButterfly32_2(imid,g_t32,idct,12,32);
		printf("====================================idct=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",idct[j][i]);
			}
			printf("\n");
		}
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				diff[j][i]=idct[j][i]-blo[j][i];
			}
		}
		printf("====================================diff=====================================\n");
		for(j=0; j<num; j++)
		{
			for(i=0; i<num;i++)
			{
				printf("%d\t",diff[j][i]);
			}
			printf("\n");
		}

	}
	printf("numSig=%d\n",numSig);
	//释放内存
	for(i=0;i<num;i++)   
		free(block[i]);   //free一维
	free(block);  //free二维的

	getchar();
}

//变换(反变换)
void partialButterfly4_1(int (*a)[4],int** b,int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}

void partialButterfly4_2(int** a,int (*b)[4],int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}

void partialButterfly8_1(int (*a)[8],int** b,int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}

void partialButterfly8_2(int** a,int (*b)[8],int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}
void partialButterfly16_1(int (*a)[16],int** b,int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}

void partialButterfly16_2(int** a,int (*b)[16],int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}

void partialButterfly32_1(int (*a)[32],int** b,int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}

void partialButterfly32_2(int** a,int (*b)[32],int** c,int shift,int line)
{
	int i,j,k;
	for(i=0;i<line;i++)
		for(j=0;j<line;j++)
		{
			c[i][j]=0;
			for(k=0;k<line;k++)
			{
				c[i][j]+=a[i][k]*b[k][j];
			}
			c[i][j]=c[i][j]>>shift;
		}
}
//量化
int prescale_quantize(int **W,int **Z,int num,int log2Trsize)
{
	numSig=0;
	int i,j;
	for(i=0;i<num;i++)
		for(j=0;j<num;j++)
		{
			int sign=1;
			int qbits=QUANT_SHIFT +floor(QP/6.0);//QP=27,qbits=18;
			int T_shift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH -log2Trsize;
			int mf;

			int f=(int)(pow(2.0,qbits+T_shift)/3);
			
			if(W[i][j]<0)
			{
				sign=-1;
				W[i][j]*=-1;
			}
			
			mf=MF[QP%6];

			Z[i][j]=(W[i][j]*mf+f)>>(qbits + T_shift);
			if (Z[i][j])
				++numSig;
			Z[i][j]*=sign;
		}
		return numSig;
}
//反量化
void rescale(int **Z,int **Wi,int num,int log2Trsize)
{
	int i,j;
	for(i=0;i<num;i++)
		for(j=0;j<num;j++)
		{
			int scale,shift;
			int IT_shift = MAX_TR_DYNAMIC_RANGE - X265_DEPTH -log2Trsize;

			shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT -IT_shift;

			scale=V[QP%6] << int(floor(QP/6.0));
			Wi[i][j]=( Z[i][j]*scale +(1<<shift-1)) >> shift;
		}

}

void sum_diff(int **W,int Z,int num)
{	
	int i,j;
	for(i=0;i<num;i++)
		for(j=0;j<num;j++)
		{
			Z+=abs(W[i][j]);
		}        
}

//x265 transform==>>

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
void inversedst(const int16_t* tmp, int16_t* block, int shift)  // input tmp, output block
{
	int i, c[4];
	int rnd_factor = 1 << (shift - 1);

	for (i = 0; i < 4; i++)
	{
		// Intermediate Variables
		c[0] = tmp[i] + tmp[8 + i];
		c[1] = tmp[8 + i] + tmp[12 + i];
		c[2] = tmp[i] - tmp[12 + i];
		c[3] = 74 * tmp[4 + i];

		block[4 * i + 0] = (int16_t)x265_clip3(-32768, 32767, (29 * c[0] + 55 * c[1] + c[3] + rnd_factor) >> shift);
		block[4 * i + 1] = (int16_t)x265_clip3(-32768, 32767, (55 * c[2] - 29 * c[1] + c[3] + rnd_factor) >> shift);
		block[4 * i + 2] = (int16_t)x265_clip3(-32768, 32767, (74 * (tmp[i] - tmp[8 + i] + tmp[12 + i]) + rnd_factor) >> shift);
		block[4 * i + 3] = (int16_t)x265_clip3(-32768, 32767, (55 * c[0] + 29 * c[2] - c[3] + rnd_factor) >> shift);
	}
}

void partialButterflyInverse4(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j;
	int E[2], O[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		// Utilizing symmetry properties to the maximum to minimize the number of multiplications //
		O[0] = g_t4[1][0] * src[line] + g_t4[3][0] * src[3 * line];
		O[1] = g_t4[1][1] * src[line] + g_t4[3][1] * src[3 * line];
		E[0] = g_t4[0][0] * src[0] + g_t4[2][0] * src[2 * line];
		E[1] = g_t4[0][1] * src[0] + g_t4[2][1] * src[2 * line];

		// Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector //
		dst[0] = (int16_t)(x265_clip3(-32768, 32767, (E[0] + O[0] + add) >> shift));
		dst[1] = (int16_t)(x265_clip3(-32768, 32767, (E[1] + O[1] + add) >> shift));
		dst[2] = (int16_t)(x265_clip3(-32768, 32767, (E[1] - O[1] + add) >> shift));
		dst[3] = (int16_t)(x265_clip3(-32768, 32767, (E[0] - O[0] + add) >> shift));

		src++;
		dst += 4;
	}
}

void partialButterflyInverse8(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j, k;
	int E[4], O[4];
	int EE[2], EO[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		// Utilizing symmetry properties to the maximum to minimize the number of multiplications //
		for (k = 0; k < 4; k++)
		{
			O[k] = g_t8[1][k] * src[line] + g_t8[3][k] * src[3 * line] + g_t8[5][k] * src[5 * line] + g_t8[7][k] * src[7 * line];
		}

		EO[0] = g_t8[2][0] * src[2 * line] + g_t8[6][0] * src[6 * line];
		EO[1] = g_t8[2][1] * src[2 * line] + g_t8[6][1] * src[6 * line];
		EE[0] = g_t8[0][0] * src[0] + g_t8[4][0] * src[4 * line];
		EE[1] = g_t8[0][1] * src[0] + g_t8[4][1] * src[4 * line];

		// Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector //
		E[0] = EE[0] + EO[0];
		E[3] = EE[0] - EO[0];
		E[1] = EE[1] + EO[1];
		E[2] = EE[1] - EO[1];
		for (k = 0; k < 4; k++)
		{
			dst[k] = (int16_t)x265_clip3(-32768, 32767, (E[k] + O[k] + add) >> shift);
			dst[k + 4] = (int16_t)x265_clip3(-32768, 32767, (E[3 - k] - O[3 - k] + add) >> shift);
		}

		src++;
		dst += 8;
	}
}

void partialButterflyInverse16(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j, k;
	int E[8], O[8];
	int EE[4], EO[4];
	int EEE[2], EEO[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		// Utilizing symmetry properties to the maximum to minimize the number of multiplications //
		for (k = 0; k < 8; k++)
		{
			O[k] = g_t16[1][k] * src[line] + g_t16[3][k] * src[3 * line] + g_t16[5][k] * src[5 * line] + g_t16[7][k] * src[7 * line] +
				g_t16[9][k] * src[9 * line] + g_t16[11][k] * src[11 * line] + g_t16[13][k] * src[13 * line] + g_t16[15][k] * src[15 * line];
		}

		for (k = 0; k < 4; k++)
		{
			EO[k] = g_t16[2][k] * src[2 * line] + g_t16[6][k] * src[6 * line] + g_t16[10][k] * src[10 * line] + g_t16[14][k] * src[14 * line];
		}

		EEO[0] = g_t16[4][0] * src[4 * line] + g_t16[12][0] * src[12 * line];
		EEE[0] = g_t16[0][0] * src[0] + g_t16[8][0] * src[8 * line];
		EEO[1] = g_t16[4][1] * src[4 * line] + g_t16[12][1] * src[12 * line];
		EEE[1] = g_t16[0][1] * src[0] + g_t16[8][1] * src[8 * line];

		// Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector //
		for (k = 0; k < 2; k++)
		{
			EE[k] = EEE[k] + EEO[k];
			EE[k + 2] = EEE[1 - k] - EEO[1 - k];
		}

		for (k = 0; k < 4; k++)
		{
			E[k] = EE[k] + EO[k];
			E[k + 4] = EE[3 - k] - EO[3 - k];
		}

		for (k = 0; k < 8; k++)
		{
			dst[k] = (int16_t)x265_clip3(-32768, 32767, (E[k] + O[k] + add) >> shift);
			dst[k + 8] = (int16_t)x265_clip3(-32768, 32767, (E[7 - k] - O[7 - k] + add) >> shift);
		}

		src++;
		dst += 16;
	}
}

void partialButterflyInverse32(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j, k;
	int E[16], O[16];
	int EE[8], EO[8];
	int EEE[4], EEO[4];
	int EEEE[2], EEEO[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		// Utilizing symmetry properties to the maximum to minimize the number of multiplications //
		for (k = 0; k < 16; k++)
		{
			O[k] = g_t32[1][k] * src[line] + g_t32[3][k] * src[3 * line] + g_t32[5][k] * src[5 * line] + g_t32[7][k] * src[7 * line] +
				g_t32[9][k] * src[9 * line] + g_t32[11][k] * src[11 * line] + g_t32[13][k] * src[13 * line] + g_t32[15][k] * src[15 * line] +
				g_t32[17][k] * src[17 * line] + g_t32[19][k] * src[19 * line] + g_t32[21][k] * src[21 * line] + g_t32[23][k] * src[23 * line] +
				g_t32[25][k] * src[25 * line] + g_t32[27][k] * src[27 * line] + g_t32[29][k] * src[29 * line] + g_t32[31][k] * src[31 * line];
		}

		for (k = 0; k < 8; k++)
		{
			EO[k] = g_t32[2][k] * src[2 * line] + g_t32[6][k] * src[6 * line] + g_t32[10][k] * src[10 * line] + g_t32[14][k] * src[14 * line] +
				g_t32[18][k] * src[18 * line] + g_t32[22][k] * src[22 * line] + g_t32[26][k] * src[26 * line] + g_t32[30][k] * src[30 * line];
		}

		for (k = 0; k < 4; k++)
		{
			EEO[k] = g_t32[4][k] * src[4 * line] + g_t32[12][k] * src[12 * line] + g_t32[20][k] * src[20 * line] + g_t32[28][k] * src[28 * line];
		}

		EEEO[0] = g_t32[8][0] * src[8 * line] + g_t32[24][0] * src[24 * line];
		EEEO[1] = g_t32[8][1] * src[8 * line] + g_t32[24][1] * src[24 * line];
		EEEE[0] = g_t32[0][0] * src[0] + g_t32[16][0] * src[16 * line];
		EEEE[1] = g_t32[0][1] * src[0] + g_t32[16][1] * src[16 * line];

		// Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector //
		EEE[0] = EEEE[0] + EEEO[0];
		EEE[3] = EEEE[0] - EEEO[0];
		EEE[1] = EEEE[1] + EEEO[1];
		EEE[2] = EEEE[1] - EEEO[1];
		for (k = 0; k < 4; k++)
		{
			EE[k] = EEE[k] + EEO[k];
			EE[k + 4] = EEE[3 - k] - EEO[3 - k];
		}

		for (k = 0; k < 8; k++)
		{
			E[k] = EE[k] + EO[k];
			E[k + 8] = EE[7 - k] - EO[7 - k];
		}

		for (k = 0; k < 16; k++)
		{
			dst[k] = (int16_t)x265_clip3(-32768, 32767, (E[k] + O[k] + add) >> shift);
			dst[k + 16] = (int16_t)x265_clip3(-32768, 32767, (E[15 - k] - O[15 - k] + add) >> shift);
		}

		src++;
		dst += 32;
	}
}

void partialButterfly4(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j;
	int E[2], O[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		/* E and O */
		E[0] = src[0] + src[3];
		O[0] = src[0] - src[3];
		E[1] = src[1] + src[2];
		O[1] = src[1] - src[2];

		dst[0] = (int16_t)((g_t4[0][0] * E[0] + g_t4[0][1] * E[1] + add) >> shift);
		dst[2 * line] = (int16_t)((g_t4[2][0] * E[0] + g_t4[2][1] * E[1] + add) >> shift);
		dst[line] = (int16_t)((g_t4[1][0] * O[0] + g_t4[1][1] * O[1] + add) >> shift);
		dst[3 * line] = (int16_t)((g_t4[3][0] * O[0] + g_t4[3][1] * O[1] + add) >> shift);

		src += 4;
		dst++;
	}
}
void partialButterfly8(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j, k;
	int E[4], O[4];
	int EE[2], EO[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		// E and O //
		for (k = 0; k < 4; k++)
		{
			E[k] = src[k] + src[7 - k];
			O[k] = src[k] - src[7 - k];
		}

		// EE and EO //
		EE[0] = E[0] + E[3];
		EO[0] = E[0] - E[3];
		EE[1] = E[1] + E[2];
		EO[1] = E[1] - E[2];

		dst[0] = (int16_t)((g_t8[0][0] * EE[0] + g_t8[0][1] * EE[1] + add) >> shift);
		dst[4 * line] = (int16_t)((g_t8[4][0] * EE[0] + g_t8[4][1] * EE[1] + add) >> shift);
		dst[2 * line] = (int16_t)((g_t8[2][0] * EO[0] + g_t8[2][1] * EO[1] + add) >> shift);
		dst[6 * line] = (int16_t)((g_t8[6][0] * EO[0] + g_t8[6][1] * EO[1] + add) >> shift);

		dst[line] = (int16_t)((g_t8[1][0] * O[0] + g_t8[1][1] * O[1] + g_t8[1][2] * O[2] + g_t8[1][3] * O[3] + add) >> shift);
		dst[3 * line] = (int16_t)((g_t8[3][0] * O[0] + g_t8[3][1] * O[1] + g_t8[3][2] * O[2] + g_t8[3][3] * O[3] + add) >> shift);
		dst[5 * line] = (int16_t)((g_t8[5][0] * O[0] + g_t8[5][1] * O[1] + g_t8[5][2] * O[2] + g_t8[5][3] * O[3] + add) >> shift);
		dst[7 * line] = (int16_t)((g_t8[7][0] * O[0] + g_t8[7][1] * O[1] + g_t8[7][2] * O[2] + g_t8[7][3] * O[3] + add) >> shift);

		src += 8;
		dst++;
	}
}
void partialButterfly16(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j, k;
	int E[8], O[8];
	int EE[4], EO[4];
	int EEE[2], EEO[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		// E and O //
		for (k = 0; k < 8; k++)
		{
			E[k] = src[k] + src[15 - k];
			O[k] = src[k] - src[15 - k];
		}

		// EE and EO //
		for (k = 0; k < 4; k++)
		{
			EE[k] = E[k] + E[7 - k];
			EO[k] = E[k] - E[7 - k];
		}

		// EEE and EEO //
		EEE[0] = EE[0] + EE[3];
		EEO[0] = EE[0] - EE[3];
		EEE[1] = EE[1] + EE[2];
		EEO[1] = EE[1] - EE[2];

		dst[0] = (int16_t)((g_t16[0][0] * EEE[0] + g_t16[0][1] * EEE[1] + add) >> shift);
		dst[8 * line] = (int16_t)((g_t16[8][0] * EEE[0] + g_t16[8][1] * EEE[1] + add) >> shift);
		dst[4 * line] = (int16_t)((g_t16[4][0] * EEO[0] + g_t16[4][1] * EEO[1] + add) >> shift);
		dst[12 * line] = (int16_t)((g_t16[12][0] * EEO[0] + g_t16[12][1] * EEO[1] + add) >> shift);

		for (k = 2; k < 16; k += 4)
		{
			dst[k * line] = (int16_t)((g_t16[k][0] * EO[0] + g_t16[k][1] * EO[1] + g_t16[k][2] * EO[2] +
				g_t16[k][3] * EO[3] + add) >> shift);
		}

		for (k = 1; k < 16; k += 2)
		{
			dst[k * line] = (int16_t)((g_t16[k][0] * O[0] + g_t16[k][1] * O[1] + g_t16[k][2] * O[2] + g_t16[k][3] * O[3] +
				g_t16[k][4] * O[4] + g_t16[k][5] * O[5] + g_t16[k][6] * O[6] + g_t16[k][7] * O[7] +
				add) >> shift);
		}

		src += 16;
		dst++;
	}
}

void partialButterfly32(const int16_t* src, int16_t* dst, int shift, int line)
{
	int j, k;
	int E[16], O[16];
	int EE[8], EO[8];
	int EEE[4], EEO[4];
	int EEEE[2], EEEO[2];
	int add = 1 << (shift - 1);

	for (j = 0; j < line; j++)
	{
		// E and O//
		for (k = 0; k < 16; k++)
		{
			E[k] = src[k] + src[31 - k];
			O[k] = src[k] - src[31 - k];
		}

		// EE and EO //
		for (k = 0; k < 8; k++)
		{
			EE[k] = E[k] + E[15 - k];
			EO[k] = E[k] - E[15 - k];
		}

		// EEE and EEO //
		for (k = 0; k < 4; k++)
		{
			EEE[k] = EE[k] + EE[7 - k];
			EEO[k] = EE[k] - EE[7 - k];
		}

		// EEEE and EEEO //
		EEEE[0] = EEE[0] + EEE[3];
		EEEO[0] = EEE[0] - EEE[3];
		EEEE[1] = EEE[1] + EEE[2];
		EEEO[1] = EEE[1] - EEE[2];

		dst[0] = (int16_t)((g_t32[0][0] * EEEE[0] + g_t32[0][1] * EEEE[1] + add) >> shift);
		dst[16 * line] = (int16_t)((g_t32[16][0] * EEEE[0] + g_t32[16][1] * EEEE[1] + add) >> shift);
		dst[8 * line] = (int16_t)((g_t32[8][0] * EEEO[0] + g_t32[8][1] * EEEO[1] + add) >> shift);
		dst[24 * line] = (int16_t)((g_t32[24][0] * EEEO[0] + g_t32[24][1] * EEEO[1] + add) >> shift);
		for (k = 4; k < 32; k += 8)
		{
			dst[k * line] = (int16_t)((g_t32[k][0] * EEO[0] + g_t32[k][1] * EEO[1] + g_t32[k][2] * EEO[2] +
				g_t32[k][3] * EEO[3] + add) >> shift);
		}

		for (k = 2; k < 32; k += 4)
		{
			dst[k * line] = (int16_t)((g_t32[k][0] * EO[0] + g_t32[k][1] * EO[1] + g_t32[k][2] * EO[2] +
				g_t32[k][3] * EO[3] + g_t32[k][4] * EO[4] + g_t32[k][5] * EO[5] +
				g_t32[k][6] * EO[6] + g_t32[k][7] * EO[7] + add) >> shift);
		}

		for (k = 1; k < 32; k += 2)
		{
			dst[k * line] = (int16_t)((g_t32[k][0] * O[0] + g_t32[k][1] * O[1] + g_t32[k][2] * O[2] + g_t32[k][3] * O[3] +
				g_t32[k][4] * O[4] + g_t32[k][5] * O[5] + g_t32[k][6] * O[6] + g_t32[k][7] * O[7] +
				g_t32[k][8] * O[8] + g_t32[k][9] * O[9] + g_t32[k][10] * O[10] + g_t32[k][11] *
				O[11] + g_t32[k][12] * O[12] + g_t32[k][13] * O[13] + g_t32[k][14] * O[14] +
				g_t32[k][15] * O[15] + add) >> shift);
		}

		src += 32;
		dst++;
	}
}

void dct4_c(int16_t* src, int16_t* dst, intptr_t srcStride)
{
	const int shift_1st = 1 + X265_DEPTH - 8;
	const int shift_2nd = 8;

	int16_t coef[4 * 4];
	int16_t block[4 * 4];

	for (int i = 0; i < 4; i++)
	{
		memcpy(&block[i * 4], src, 4);
		src = src + srcStride;
	}
	src = src - 4 * srcStride;

	partialButterfly4(block, coef, shift_1st, 4);
	partialButterfly4(coef, dst, shift_2nd, 4);
}

void dct8_c(int16_t* src, int16_t* dst, intptr_t srcStride)
{
	const int shift_1st = 2 + X265_DEPTH - 8;
	const int shift_2nd = 9;

	int16_t coef[8 * 8];
	int16_t block[8 * 8];

	for (int i = 0; i < 8; i++)
	{
		memcpy(&block[i * 8], src, 8);
		src = src + srcStride;
	}
	src = src - 8 * srcStride;

	partialButterfly8(block, coef, shift_1st, 8);
	partialButterfly8(coef, dst, shift_2nd, 8);
}

void dct16_c(int16_t* src, int16_t* dst, intptr_t srcStride)
{
	const int shift_1st = 3 + X265_DEPTH - 8;
	const int shift_2nd = 10;

	int16_t coef[16 * 16];
	int16_t block[16 * 16];

	for (int i = 0; i < 16; i++)
	{
		memcpy(&block[i * 16], src, 16);
		src = src + srcStride;
	}
	src = src - 16 * srcStride;

	partialButterfly16(block, coef, shift_1st, 16);
	partialButterfly16(coef, dst, shift_2nd, 16);

}

void dct32_c(int16_t* src, int16_t* dst, intptr_t srcStride)
{
	const int shift_1st = 4 + X265_DEPTH - 8;
	const int shift_2nd = 11;

	int16_t coef[32 * 32];
	int16_t block[32 * 32];

	for (int i = 0; i < 32; i++)
	{
		memcpy(&block[i * 32], src, 32);
		src = src + srcStride;
	}
	src = src - 32 * srcStride;

	partialButterfly32(block, coef, shift_1st, 32);
	partialButterfly32(coef, dst, shift_2nd, 32);

}

void idst4_c(int16_t* src, int16_t* dst, intptr_t dstStride)
{
	const int shift_1st = 7;
	const int shift_2nd = 12 - (X265_DEPTH - 8);

	int16_t coef[4 * 4];
	int16_t block[4 * 4];


	inversedst(src, coef, shift_1st); // Forward DST BY FAST ALGORITHM, block input, coef output
	inversedst(coef, block, shift_2nd); // Forward DST BY FAST ALGORITHM, coef input, coeff output

	for (int i = 0; i < 4; i++)
	{
		memcpy(dst, &block[i * 4], 4);
		dst = dst + dstStride;
	}
	dst = dst - 4 * dstStride;
}

void idct4_c(int16_t* src, int16_t* dst, intptr_t dstStride)
{
	const int shift_1st = 7;
	const int shift_2nd = 12 - (X265_DEPTH - 8);

	int16_t coef[4 * 4];
	int16_t block[4 * 4];

	partialButterflyInverse4(src, coef, shift_1st, 4); // Forward DST BY FAST ALGORITHM, block input, coef output
	partialButterflyInverse4(coef, block, shift_2nd, 4); // Forward DST BY FAST ALGORITHM, coef input, coeff output

	for (int i = 0; i < 4; i++)
	{
		memcpy(&dst[i * dstStride], &block[i * 4], 4 * sizeof(int16_t));
	}
}

void idct8_c(int16_t* src, int16_t* dst, intptr_t dstStride)
{
	const int shift_1st = 7;
	const int shift_2nd = 12 - (X265_DEPTH - 8);

	int16_t coef[8 * 8];
	int16_t block[8 * 8];

	partialButterflyInverse8(src, coef, shift_1st, 8);
	partialButterflyInverse8(coef, block, shift_2nd, 8);

	for (int i = 0; i < 8; i++)
	{
		memcpy(&dst[i * dstStride], &block[i * 8], 8 * sizeof(int16_t));
	}
}

void idct16_c(int16_t* src, int16_t* dst, intptr_t dstStride)
{
	const int shift_1st = 7;
	const int shift_2nd = 12 - (X265_DEPTH - 8);

	int16_t coef[16 * 16];
	int16_t block[16 * 16];

	partialButterflyInverse16(src, coef, shift_1st, 16);
	partialButterflyInverse16(coef, block, shift_2nd, 16);

	for (int i = 0; i < 16; i++)
	{
		memcpy(&dst[i * dstStride], &block[i * 16], 16 * sizeof(int16_t));
	}
}

void idct32_c(int16_t* src, int16_t* dst, intptr_t dstStride)
{
	const int shift_1st = 7;
	const int shift_2nd = 12 - (X265_DEPTH - 8);

	int16_t coef[32 * 32];
	int16_t block[32 * 32];

	partialButterflyInverse32(src, coef, shift_1st, 32);
	partialButterflyInverse32(coef, block, shift_2nd, 32);

	for (int i = 0; i < 32; i++)
	{
		memcpy(&dst[i * dstStride], &block[i * 32], 32 * sizeof(int16_t));
	}
}

void dequant_normal_c(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift)
{
#if HIGH_BIT_DEPTH
	X265_CHECK(scale < 32768 || ((scale & 3) == 0 && shift > 2), "dequant invalid scale %d\n", scale);
#else
	// NOTE: maximum of scale is (72 * 256)
	X265_CHECK(scale < 32768, "dequant invalid scale %d\n");
#endif
	X265_CHECK(num <= 32 * 32, "dequant num %d too large\n");
	X265_CHECK((num % 8) == 0, "dequant num %d not multiple of 8\n");
	X265_CHECK(shift <= 10, "shift too large %d\n");

	int add, coeffQ;

	add = 1 << (shift - 1);

	for (int n = 0; n < num; n++)
	{
		coeffQ = (quantCoef[n] * scale + add) >> shift;
		coef[n] = (int16_t)x265_clip3(-32768, 32767, coeffQ);
	}
}

void dequant_scaling_c(const int16_t* quantCoef, const int32_t* deQuantCoef, int16_t* coef, int num, int per, int shift)
{
	X265_CHECK(num <= 32 * 32, "dequant num %d too large\n");

	int add, coeffQ;

	shift += 4;

	if (shift > per)
	{
		add = 1 << (shift - per - 1);

		for (int n = 0; n < num; n++)
		{
			coeffQ = ((quantCoef[n] * deQuantCoef[n]) + add) >> (shift - per);
			coef[n] = (int16_t)x265_clip3(-32768, 32767, coeffQ);
		}
	}
	else
	{
		for (int n = 0; n < num; n++)
		{
			coeffQ = x265_clip3(-32768, 32767, quantCoef[n] * deQuantCoef[n]);
			coef[n] = (int16_t)x265_clip3(-32768, 32767, coeffQ << (per - shift));
		}
	}
}

uint32_t quant_c(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff)
{
	X265_CHECK(qBits >= 8, "qBits less than 8\n");
	X265_CHECK((numCoeff % 16) == 0, "numCoeff must be multiple of 16\n");
	int qBits8 = qBits - 8;
	uint32_t numSig = 0;
	for (int blockpos = 0; blockpos < numCoeff; blockpos++)
	{
		int level = coef[blockpos];
		int sign = (level < 0 ? -1 : 1);

		int tmplevel = abs(level) * quantCoeff[blockpos];
		level = ((tmplevel + add) >> qBits);
		deltaU[blockpos] = ((tmplevel - (level << qBits)) >> qBits8);
		if (level)
			++numSig;
		level *= sign;
		qCoef[blockpos] = (int16_t)x265_clip3(-32768, 32767, level);
	}
	return numSig;
}

uint32_t nquant_c(const int16_t* coef, const int32_t* quantCoeff, int16_t* qCoef, int qBits, int add, int numCoeff)
{
	X265_CHECK((numCoeff % 16) == 0, "number of quant coeff is not multiple of 4x4\n");
	X265_CHECK((uint32_t)add < ((uint32_t)1 << qBits), "2 ^ qBits less than add\n");
	X265_CHECK(((intptr_t)quantCoeff & 31) == 0, "quantCoeff buffer not aligned\n");

	uint32_t numSig = 0;

	for (int blockpos = 0; blockpos < numCoeff; blockpos++)
	{
		int level = coef[blockpos];
		int sign = (level < 0 ? -1 : 1);

		int tmplevel = abs(level) * quantCoeff[blockpos];
		level = ((tmplevel + add) >> qBits);
		if (level)
			++numSig;
		level *= sign;
		qCoef[blockpos] = (int16_t)x265_clip3(-32768, 32767, level);
	}

	return numSig;
}

int  count_nonzero_c(uint32_t trSize, const int16_t* quantCoeff)
{
	int count = 0;
	int numCoeff = trSize * trSize;
	for (int i = 0; i < numCoeff; i++)
	{
		count += quantCoeff[i] != 0;
	}
	return count;
}

uint32_t copy_count(int16_t* coeff, int16_t* residual, intptr_t resiStride, int trSize)
{
	uint32_t numSig = 0;
	for (int k = 0; k < trSize; k++)
	{
		for (int j = 0; j < trSize; j++)
		{
			coeff[k * trSize + j] = residual[k * resiStride + j];
			numSig += (residual[k * resiStride + j] != 0);
		}
	}

	return numSig;
}
// Fast DST Algorithm. Full matrix multiplication for DST and Fast DST algorithm
// give identical results
void fastForwardDst(int16_t* block, int16_t* coeff, int shift)  // input block, output coeff
{
	int c[4];
	int rnd_factor = 1 << (shift - 1);

	for (int i = 0; i < 4; i++)
	{
		// Intermediate Variables
		c[0] = block[4 * i + 0] + block[4 * i + 3];
		c[1] = block[4 * i + 1] + block[4 * i + 3];
		c[2] = block[4 * i + 0] - block[4 * i + 1];
		c[3] = 74 * block[4 * i + 2];

		coeff[i] = (int16_t)((29 * c[0] + 55 * c[1] + c[3] + rnd_factor) >> shift);
		coeff[4 + i] = (int16_t)((74 * (block[4 * i + 0] + block[4 * i + 1] - block[4 * i + 3]) + rnd_factor) >> shift);
		coeff[8 + i] = (int16_t)((29 * c[2] + 55 * c[0] - c[3] + rnd_factor) >> shift);
		coeff[12 + i] = (int16_t)((55 * c[2] - 29 * c[1] + c[3] + rnd_factor) >> shift);
	}
}

void dst4_c(int16_t* src, int16_t* dst, intptr_t srcStride)
{
	const int shift_1st = 1 + X265_DEPTH - 8;
	const int shift_2nd = 8;

	int16_t coef[4 * 4];
	int16_t block[4 * 4];

	for (int i = 0; i < 4; i++)
	{
		memcpy(&block[i * 4], src, 4);
		src = src + srcStride;
	}
	src = src - 4 * srcStride;

	fastForwardDst(block, coef, shift_1st);
	fastForwardDst(coef, dst, shift_2nd);
}
int16_t src[32 * 32] = {
	-80, -80, -80, -80, -80, -77, -73, -73, -73, -73, -75, -80, -81, -81, -81, -80, -81, -81, -80, -81, -80, -80, -80, -80, -80, -80, -79, -81, -87, -88, -86, -86,
	-93, -85, -85, -85, -85, -85, -85, -85, -85, -85, -85, -84, -84, -84, -84, -85, -84, -84, -85, -84, -84, -85, -84, -84, -84, -84, -84, -83, -87, -87, -86, -86,
	-93, -89, -88, -88, -88, -88, -88, -88, -87, -87, -87, -87, -86, -86, -85, -86, -86, -86, -86, -87, -87, -87, -87, -87, -87, -88, -88, -86, -87, -87, -86, -86,
	-93, -70, -71, -71, -71, -72, -72, -72, -72, -71, -72, -71, -71, -70, -70, -70, -70, -72, -72, -72, -72, -72, -74, -75, -75, -79, -86, -85, -87, -88, -86, -86,
	-92, -52, -53, -55, -55, -55, -56, -56, -56, -55, -55, -55, -55, -54, -54, -54, -53, -56, -56, -56, -57, -56, -57, -58, -59, -69, -84, -85, -87, -88, -86, -86,
	-92, -48, -50, -52, -52, -53, -53, -53, -53, -52, -52, -51, -52, -50, -50, -50, -49, -51, -52, -51, -51, -51, -52, -54, -54, -65, -83, -85, -87, -88, -85, -85,
	-92, -40, -48, -52, -51, -52, -52, -52, -53, -52, -48, -41, -40, -38, -37, -37, -37, -38, -38, -38, -38, -39, -43, -50, -53, -64, -82, -85, -86, -87, -86, -86,
	-92, -33, -47, -50, -50, -52, -52, -51, -51, -50, -40, -29, -27, -26, -25, -26, -25, -26, -27, -27, -28, -28, -33, -47, -53, -64, -83, -85, -87, -87, -86, -85,
	-92, -30, -47, -49, -49, -50, -50, -50, -50, -47, -35, -23, -21, -19, -19, -19, -19, -21, -23, -24, -25, -22, -28, -45, -52, -64, -83, -85, -86, -87, -85, -85,
	-92, -29, -46, -48, -48, -49, -49, -49, -49, -46, -31, -19, -15, -10, -6, -6, -8, -12, -15, -21, -22, -17, -25, -45, -52, -63, -82, -85, -86, -87, -85, -85,
	-92, -29, -44, -47, -47, -47, -47, -47, -47, -43, -28, -16, -12, -6, -1, -2, -1, -5, -9, -17, -22, -21, -29, -46, -51, -64, -83, -85, -86, -87, -86, -85,
	-92, -31, -43, -46, -46, -46, -46, -46, -46, -42, -24, -10, -9, -5, -1, +0, +3, -2, -7, -12, -18, -21, -29, -45, -51, -63, -82, -84, -86, -87, -85, -85,
	-91, -32, -41, -44, -44, -45, -45, -45, -45, -40, -21, -5, -4, -3, +0, +2, +3, -1, -5, -8, -15, -17, -26, -44, -50, -63, -82, -84, -86, -86, -85, -84,
	-91, -30, -40, -43, -42, -43, -43, -43, -43, -38, -19, -4, -2, -2, +0, +0, -1, -4, -6, -9, -13, -15, -25, -44, -49, -62, -82, -84, -86, -86, -84, -84,
	-91, -28, -37, -40, -41, -42, -42, -41, -41, -37, -19, -5, -3, -1, +0, +0, -1, -4, -6, -8, -11, -13, -24, -42, -48, -62, -82, -84, -86, -86, -85, -84,
	-91, -26, -36, -39, -39, -41, -40, -40, -39, -35, -18, -5, +2, +0, +0, +0, -1, -4, -6, -8, -11, -13, -23, -42, -47, -62, -83, -84, -86, -86, -84, -84,
	-91, -28, -35, -38, -38, -39, -39, -39, -39, -35, -21, +1, +19, +7, -2, -1, -2, -4, -7, -9, -11, -14, -25, -41, -46, -61, -82, -83, -85, -86, -84, -84,
	-91, -32, -35, -36, -37, -38, -38, -38, -38, -36, -28, -6, +15, -1, -11, -8, -9, -11, -12, -14, -16, -19, -29, -41, -45, -60, -82, -83, -85, -86, -84, -83,
	-91, -32, -34, -34, -35, -36, -36, -36, -36, -36, -34, -29, -24, -30, -29, -27, -28, -30, -29, -30, -31, -33, -37, -41, -43, -59, -82, -83, -85, -85, -84, -83,
	-91, -31, -33, -34, -35, -35, -35, -34, -35, -34, -34, -32, -33, -34, -32, -32, -35, -36, -36, -36, -37, -37, -38, -41, -42, -60, -82, -83, -84, -85, -84, -83,
	-90, -31, -32, -33, -33, -34, -34, -34, -34, -33, -32, -32, -32, -34, -31, -31, -34, -35, -34, -34, -35, -35, -36, -39, -41, -59, -82, -83, -84, -85, -83, -83,
	-91, -31, -32, -33, -32, -33, -33, -33, -32, -32, -32, -32, -33, -34, -33, -32, -33, -34, -34, -34, -35, -35, -36, -38, -40, -59, -82, -83, -84, -84, -83, -82,
	-90, -31, -31, -32, -31, -32, -32, -32, -31, -31, -31, -32, -33, -33, -33, -32, -33, -34, -33, -34, -34, -35, -35, -36, -38, -59, -82, -82, -83, -84, -82, -82,
	90, -30, -31, -31, -31, -31, -31, -31, -30, -30, -31, -31, -32, -32, -32, -32, -32, -33, -33, -33, -33, -34, -34, -34, -37, -58, -82, -82, -83, -84, -82, -81,
	-90, -30, -30, -31, -30, -31, -30, -30, -30, -30, -30, -30, -32, -32, -32, -31, -32, -33, -33, -33, -32, -32, -32, -33, -36, -57, -81, -81, -83, -83, -82, -81,
	-90, -30, -30, -30, -30, -31, -30, -29, -30, -28, -26, -26, -27, -27, -27, -27, -28, -30, -30, -30, -30, -31, -31, -32, -35, -57, -82, -81, -82, -83, -81, -81,
	-90, -30, -29, -29, -29, -31, -30, -29, -30, -26, -19, -20, -21, -20, -19, -18, -18, -19, -19, -19, -21, -27, -31, -31, -34, -57, -82, -80, -82, -83, -82, -81,
	-89, -30, -29, -28, -29, -30, -30, -29, -29, -22, -17, -23, -26, -25, -24, -22, -20, -19, -17, -14, -11, -22, -29, -29, -33, -56, -82, -80, -81, -83, -81, -81,
	-89, -29, -29, -29, -29, -29, -29, -29, -29, -20, -16, -26, -28, -27, -27, -25, -22, -23, -24, -18, -11, -21, -28, -28, -31, -56, -82, -79, -80, -82, -81, -80,
	-89, -29, -29, -28, -28, -28, -28, -28, -27, -17, -16, -26, -27, -25, -25, -24, -22, -23, -24, -17, -10, -21, -27, -26, -29, -55, -82, -79, -80, -82, -80, -80,
	-89, -28, -27, -27, -27, -27, -27, -28, -26, -14, -15, -24, -24, -23, -22, -23, -23, -24, -23, -14, -8, -21, -25, -25, -28, -55, -82, -78, -80, -81, -80, -79,
	-88, -28, -27, -27, -27, -27, -27, -27, -25, -13, -16, -23, -23, -22, -22, -22, -22, -23, -21, -10, -7, -21, -24, -23, -27, -55, -83, -77, -79, -82, -80, -79
};
void transform2()
{
	int i, j;
	int16_t dct[32 * 32] = { 0 };
	int16_t residual[32 * 32] = { 0 };
	int16_t coeff[32 * 32] = { 0 };
	int32_t deltaU[32 * 32] = { 0 };
	int16_t coef[32 * 32] = { 0 };

	dct32_c(src,dct,32);
	printf("====================================dct=====================================\n");
	for (j = 0; j<32; j++)
	{
		for (i = 0; i<32; i++)
		{
			printf("%d\t", dct[j*32+i]);
		}
		printf("\n");
	}
	
	quant_c((const int16_t*)dct, (const int32_t*)&MF[2], deltaU, coeff, 20, 350208, 1024);

	printf("====================================quant=====================================\n");
	for (j = 0; j<32; j++)
	{
		for (i = 0; i<32; i++)
		{
			printf("%d\t", coeff[j * 32 + i]);
		}
		printf("\n");
	}

	dequant_normal_c(coeff,coef,1024,816,4);
	printf("====================================dequant=====================================\n");
	for (j = 0; j<32; j++)
	{
		for (i = 0; i<32; i++)
		{
			printf("%d\t", coef[j * 32 + i]);
		}
		printf("\n");
	}
	//void dequant_normal_c(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift)
	idct32_c(dct, residual,32);

	printf("====================================idct=====================================\n");
	for (j = 0; j<32; j++)
	{
		for (i = 0; i<32; i++)
		{
			printf("%d\t", residual[j * 32 + i]);
		}
		printf("\n");
	}

	
	//void dequant_scaling_c(const int16_t* quantCoef, const int32_t* deQuantCoef, int16_t* coef, int num, int per, int shift)
	//uint32_t quant_c(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff)
	int aa = 0;
	//void idct32_c(int16_t* src, int16_t* dst, intptr_t dstStride)
}
//<<==x265 transform

int main()
{
	//transform1();
	transform2();
	return 0;
}