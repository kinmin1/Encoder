/*
* yuv.c
*
*  Created on: 2015-10-29
*      Author: adminster
*/

#include"yuv.h"
#include "pixel.h"
#include"malloc.h"
#include <math.h>

pixel Yuv_create_buf_32[1536] = { 1 };
pixel Yuv_create_buf_16[384] = { 1 };
pixel Yuv_create_buf_8[96] = { 1 };

pixel predYuv_md_0[1536] = { 1 };
pixel predYuv_md_1[384] = { 1 };
pixel predYuv_md_2[96] = { 1 };

pixel reconYuv_md_0[1536] = { 1 };
pixel reconYuv_md_1[384] = { 1 };
pixel reconYuv_md_2[96] = { 1 };

pixel predYuv_pred2Nx2N_0[1536] = { 1 };
pixel predYuv_pred2Nx2N_1[384] = { 1 };
pixel predYuv_pred2Nx2N_2[96] = { 1 };

pixel reconYuv_pred2Nx2N0[1536] = { 1 };
pixel reconYuv_pred2Nx2N1[384] = { 1 };
pixel reconYuv_pred2Nx2N2[96] = { 1 };

pixel reconQtYuv_0[1536] = { 1 };
pixel reconQtYuv_1[1536] = { 1 };
pixel reconQtYuv_2[1536] = { 1 };
pixel reconQtYuv_3[1536] = { 1 };

pixel tmpPredYuv_0[1536] = { 1 };
pixel tmpPredYuv_1[1536] = { 1 };
pixel tmpPredYuv_2[1536] = { 1 };

pixel bidirPredYuv_0[1536] = { 1 };
pixel bidirPredYuv_1[1536] = { 1 };
pixel bidirPredYuv_2[1536] = { 1 };

pixel bidirPredYuv0[1536] = { 1 };
pixel bidirPredYuv1[1536] = { 1 };
pixel bidirPredYuv2[1536] = { 1 };

pixel fencYuv_md_32[1536] = { 1 };
pixel fencYuv_md_16[384] = { 1 };
pixel fencYuv_md_8[96] = { 1 };

pixel predYuv[7720] = { 1 };
pixel reconYuv[7720] = { 1 };
void Yuv_init(Yuv *yuv)
{
	yuv->m_buf[0] = NULL;
	yuv->m_buf[1] = NULL;
	yuv->m_buf[2] = NULL;
}

int Yuv_create(Yuv*yuv, uint32_t size, int csp)
{
	if (size == 64)
	{
		yuv->m_csp = csp;
		yuv->m_hChromaShift = 1;
		yuv->m_vChromaShift = 1;

		yuv->m_size = size;
		yuv->m_part = partitionFromSizes(size, size);

		yuv->m_csize = size >> yuv->m_hChromaShift;

		unsigned int sizeL = size * size;
		unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

		if (!(sizeC & 15) == 0)
			printf("invalid size\n");

		// memory allocation (padded for SIMD reads)
		//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);
		yuv->m_buf[0] = Yuv_create_buf_32;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}
	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_search(Yuv*yuv, uint32_t size, int csp, int num)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

	if (num == 0)
	{
		yuv->m_buf[0] = reconQtYuv_0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	if (num == 1)
	{
		yuv->m_buf[0] = reconQtYuv_1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	if (num == 2)
	{
		yuv->m_buf[0] = reconQtYuv_2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	if (num == 3)
	{
		yuv->m_buf[0] = reconQtYuv_3;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}
	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_search_1(Yuv*yuv, uint32_t size, int csp, int num)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);
	if (num == 0)
	{
		yuv->m_buf[0] = tmpPredYuv_0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 1)
	{
		yuv->m_buf[0] = tmpPredYuv_1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 2)
	{
		yuv->m_buf[0] = tmpPredYuv_2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_search_2(Yuv*yuv, uint32_t size, int csp, int num)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);
	if (num == 0)
	{
		yuv->m_buf[0] = bidirPredYuv_0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 1)
	{
		yuv->m_buf[0] = bidirPredYuv_1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (num == 2)
	{
		yuv->m_buf[0] = bidirPredYuv_2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}


	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_search_3(Yuv*yuv, uint32_t size, int csp, int num)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

	if (num == 0)
	{
		yuv->m_buf[0] = bidirPredYuv0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	if (num == 1)
	{
		yuv->m_buf[0] = bidirPredYuv1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	if (num == 2)
	{
		yuv->m_buf[0] = bidirPredYuv2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_md(Yuv*yuv, uint32_t size, int csp)
{
	if (size == 32)
	{
		yuv->m_csp = csp;
		yuv->m_hChromaShift = 1;
		yuv->m_vChromaShift = 1;

		yuv->m_size = size;
		yuv->m_part = partitionFromSizes(size, size);

		yuv->m_csize = size >> yuv->m_hChromaShift;

		unsigned int sizeL = size * size;
		unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

		if (!(sizeC & 15) == 0)
			printf("invalid size\n");

		// memory allocation (padded for SIMD reads)
		//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

		yuv->m_buf[0] = fencYuv_md_32;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}
	else if (size == 16)
	{
		yuv->m_csp = csp;
		yuv->m_hChromaShift = 1;
		yuv->m_vChromaShift = 1;

		yuv->m_size = size;
		yuv->m_part = partitionFromSizes(size, size);

		yuv->m_csize = size >> yuv->m_hChromaShift;

		unsigned int sizeL = size * size;
		unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

		if (!(sizeC & 15) == 0)
			printf("invalid size\n");

		// memory allocation (padded for SIMD reads)
		//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

		yuv->m_buf[0] = fencYuv_md_16;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}
	else if (size == 8)
	{
		yuv->m_csp = csp;
		yuv->m_hChromaShift = 1;
		yuv->m_vChromaShift = 1;

		yuv->m_size = size;
		yuv->m_part = partitionFromSizes(size, size);

		yuv->m_csize = size >> yuv->m_hChromaShift;

		unsigned int sizeL = size * size;
		unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

		if (!(sizeC & 15) == 0)
			printf("invalid size\n");

		// memory allocation (padded for SIMD reads)
		//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

		yuv->m_buf[0] = fencYuv_md_8;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}
	return TRUE;

fail:
	return FALSE;
}
int Yuv_create_md_pred_0(Yuv*yuv, uint32_t size, int csp)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);
	if (size == 32)
	{
		yuv->m_buf[0] = predYuv_md_0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (size == 16)
	{
		yuv->m_buf[0] = predYuv_md_1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (size == 8)
	{
		yuv->m_buf[0] = predYuv_md_2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_md_pred_1(Yuv*yuv, uint32_t size, int csp)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

	if (size == 32)
	{
		yuv->m_buf[0] = reconYuv_md_0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (size == 16)
	{
		yuv->m_buf[0] = reconYuv_md_1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (size == 8)
	{
		yuv->m_buf[0] = reconYuv_md_2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_md_pred_pred2Nx2N_0(Yuv*yuv, uint32_t size, int csp)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

	if (size == 32)
	{
		yuv->m_buf[0] = predYuv_pred2Nx2N_0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (size == 16)
	{
		yuv->m_buf[0] = predYuv_pred2Nx2N_1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	if (size == 8)
	{
		yuv->m_buf[0] = predYuv_pred2Nx2N_2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;
	}

	return TRUE;

fail:
	return FALSE;
}

int Yuv_create_md_pred_pred2Nx2N_1(Yuv*yuv, uint32_t size, int csp)
{
	yuv->m_csp = csp;
	yuv->m_hChromaShift = 1;
	yuv->m_vChromaShift = 1;

	yuv->m_size = size;
	yuv->m_part = partitionFromSizes(size, size);

	yuv->m_csize = size >> yuv->m_hChromaShift;

	unsigned int sizeL = size * size;
	unsigned int sizeC = sizeL >> (yuv->m_vChromaShift + yuv->m_hChromaShift);

	if (!(sizeC & 15) == 0)
		printf("invalid size\n");

	// memory allocation (padded for SIMD reads)
	//CHECKED_MALLOC(yuv->m_buf[0], pixel, sizeL + sizeC * 2 + 8);

	if (size == 32)
	{
		yuv->m_buf[0] = reconYuv_pred2Nx2N0;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	if (size == 16)
	{
		yuv->m_buf[0] = reconYuv_pred2Nx2N1;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	if (size == 8)
	{
		yuv->m_buf[0] = reconYuv_pred2Nx2N2;
		if (!yuv->m_buf[0])
		{
			printf("malloc of size %d failed\n", sizeof(pixel) * (sizeL + sizeC * 2 + 8));
			goto fail;
		}
		yuv->m_buf[1] = yuv->m_buf[0] + sizeL;
		yuv->m_buf[2] = yuv->m_buf[0] + sizeL + sizeC;

	}

	return TRUE;

fail:
	return FALSE;
}

void Yuv_destroy(Yuv *yuv)
{
	free(yuv->m_buf[0]); yuv->m_buf[0] = NULL;
}

void Yuv_copyToPicYuv(Yuv *yuv, PicYuv *dstPic, uint32_t cuAddr, uint32_t absPartIdx)
{
	pixel* dstY = Picyuv_CUgetLumaAddr(dstPic, cuAddr, absPartIdx);
	primitives.cu[yuv->m_part].copy_pp(dstY, dstPic->m_stride, yuv->m_buf[0], yuv->m_size, pow(2, double(yuv->m_part + 2)), pow(2, double(yuv->m_part + 2)));
	pixel* dstU = Picyuv_CUgetCbAddr(dstPic, cuAddr, absPartIdx);
	pixel* dstV = Picyuv_CUgetCrAddr(dstPic, cuAddr, absPartIdx);

	primitives.chroma[yuv->m_csp].cu[yuv->m_part].copy_pp(dstU, dstPic->m_strideC, yuv->m_buf[1], yuv->m_csize, pow(2, double(yuv->m_part + 1)), pow(2, double(yuv->m_part + 1)));
	primitives.chroma[yuv->m_csp].cu[yuv->m_part].copy_pp(dstV, dstPic->m_strideC, yuv->m_buf[2], yuv->m_csize, pow(2, double(yuv->m_part + 1)), pow(2, double(yuv->m_part + 1)));
}
void Yuv_copyFromPicYuv(Yuv *yuv, PicYuv *srcPic, uint32_t cuAddr, uint32_t absPartIdx)
{
	pixel* srcY = Picyuv_CUgetLumaAddr(srcPic, cuAddr, absPartIdx);
	blockcopy_pp_c(yuv->m_buf[0], yuv->m_size, srcY, srcPic->m_stride, pow(2, double(yuv->m_part + 2)), pow(2, double(yuv->m_part + 2)));
	pixel* srcU = Picyuv_CUgetCbAddr(srcPic, cuAddr, absPartIdx);
	pixel* srcV = Picyuv_CUgetCrAddr(srcPic, cuAddr, absPartIdx);

	blockcopy_pp_c(yuv->m_buf[1], yuv->m_csize, srcU, srcPic->m_strideC, pow(2, double(yuv->m_part + 1)), pow(2, double(yuv->m_part + 1)));
	blockcopy_pp_c(yuv->m_buf[2], yuv->m_csize, srcV, srcPic->m_strideC, pow(2, double(yuv->m_part + 1)), pow(2, double(yuv->m_part + 1)));
}

void Yuv_copyFromYuv(Yuv *yuv, const Yuv srcYuv)
{
	if (!(yuv->m_size >= srcYuv.m_size))
		printf("invalid size\n");

	primitives.cu[yuv->m_part].copy_pp(yuv->m_buf[0], yuv->m_size, srcYuv.m_buf[0], srcYuv.m_size, pow(2, double(yuv->m_part + 2)), pow(2, double(yuv->m_part + 2)));
	primitives.chroma[yuv->m_csp].cu[yuv->m_part].copy_pp(yuv->m_buf[1], yuv->m_csize, srcYuv.m_buf[1], srcYuv.m_csize, pow(2, double(yuv->m_part + 2)), pow(2, double(yuv->m_part + 2)));
	primitives.chroma[yuv->m_csp].cu[yuv->m_part].copy_pp(yuv->m_buf[2], yuv->m_csize, srcYuv.m_buf[2], srcYuv.m_csize, pow(2, double(yuv->m_part + 2)), pow(2, double(yuv->m_part + 2)));
}

/* This version is intended for use by ME, which required FENC_STRIDE for luma fenc pixels */
void Yuv_copyPUFromYuv(struct Yuv *yuv, const struct Yuv* srcYuv, uint32_t absPartIdx, int partEnum, int bChroma, int pwidth, int pheight)
{
	if (!(yuv->m_size == FENC_STRIDE && yuv->m_size >= srcYuv->m_size))
		printf("PU buffer size mismatch\n");

	pixel* srcY = srcYuv->m_buf[0] + Yuv_getAddrOffset(absPartIdx, srcYuv->m_size);
	primitives.pu[partEnum].copy_pp(yuv->m_buf[0], yuv->m_size, srcY, srcYuv->m_size, pwidth, pheight);

	if (bChroma)
	{
		pixel* srcU = srcYuv->m_buf[1] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
		pixel* srcV = srcYuv->m_buf[2] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
		primitives.chroma[yuv->m_csp].pu[partEnum].copy_pp(yuv->m_buf[1], yuv->m_csize, srcU, srcYuv->m_csize, pwidth, pheight);
		primitives.chroma[yuv->m_csp].pu[partEnum].copy_pp(yuv->m_buf[2], yuv->m_csize, srcV, srcYuv->m_csize, pwidth, pheight);
	}
}

// Copy Small YUV buffer to the part of other Big YUV buffer
void Yuv_copyToPartYuv(Yuv* srcYuv, Yuv* dstYuv, uint32_t absPartIdx)
{
	pixel* dstY = Yuv_getLumaAddr(dstYuv, absPartIdx);
	primitives.cu[srcYuv->m_part].copy_pp(dstY, dstYuv->m_size, srcYuv->m_buf[0], srcYuv->m_size, pow(2, double(srcYuv->m_part + 2)), pow(2, double(srcYuv->m_part + 2)));

	pixel* dstU = Yuv_getCbAddr(dstYuv, absPartIdx);
	pixel* dstV = Yuv_getCrAddr(dstYuv, absPartIdx);
	primitives.chroma[srcYuv->m_csp].cu[srcYuv->m_part].copy_pp(dstU, dstYuv->m_csize, srcYuv->m_buf[1], srcYuv->m_csize, pow(2, double(srcYuv->m_part + 2)), pow(2, double(srcYuv->m_part + 2)));
	primitives.chroma[srcYuv->m_csp].cu[srcYuv->m_part].copy_pp(dstV, dstYuv->m_csize, srcYuv->m_buf[2], srcYuv->m_csize, pow(2, double(srcYuv->m_part + 2)), pow(2, double(srcYuv->m_part + 2)));
}

void Yuv_copyPartToYuv(Yuv *yuv, Yuv* dstYuv, uint32_t absPartIdx)
{
	pixel* srcY = yuv->m_buf[0] + Yuv_getAddrOffset(absPartIdx, yuv->m_size);
	pixel* dstY = dstYuv->m_buf[0];
	primitives.cu[dstYuv->m_part].copy_pp(dstY, dstYuv->m_size, srcY, yuv->m_size, pow(2, double(dstYuv->m_part + 2)), pow(2, double(dstYuv->m_part + 2)));

	pixel* srcU = yuv->m_buf[1] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
	pixel* srcV = yuv->m_buf[2] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
	pixel* dstU = dstYuv->m_buf[1];
	pixel* dstV = dstYuv->m_buf[2];
	primitives.chroma[yuv->m_csp].cu[dstYuv->m_part].copy_pp(dstU, dstYuv->m_csize, srcU, yuv->m_csize, pow(2, double(dstYuv->m_part + 2)), pow(2, double(dstYuv->m_part + 2)));
	primitives.chroma[yuv->m_csp].cu[dstYuv->m_part].copy_pp(dstV, dstYuv->m_csize, srcV, yuv->m_csize, pow(2, double(dstYuv->m_part + 2)), pow(2, double(dstYuv->m_part + 2)));
}



void Yuv_copyPartToPartLuma(Yuv *yuv, struct Yuv *dstYuv, uint32_t absPartIdx, uint32_t log2Size)
{
	pixel* src = Yuv_getLumaAddr(yuv, absPartIdx);
	pixel* dst = Yuv_getLumaAddr(dstYuv, absPartIdx);

	primitives.cu[log2Size - 2].copy_pp(dst, dstYuv->m_size, src, yuv->m_size, pow(2, double(log2Size)), pow(2, double(log2Size)));
}

void Yuv_copyPartToPartChroma(Yuv *yuv, Yuv *dstYuv, uint32_t absPartIdx, uint32_t log2SizeL)
{
	pixel* srcU = Yuv_getCbAddr(yuv, absPartIdx);
	pixel* srcV = Yuv_getCrAddr(yuv, absPartIdx);
	pixel* dstU = Yuv_getCbAddr(dstYuv, absPartIdx);
	pixel* dstV = Yuv_getCrAddr(dstYuv, absPartIdx);

	primitives.chroma[yuv->m_csp].cu[log2SizeL - 2].copy_pp(dstU, dstYuv->m_csize, srcU, yuv->m_csize, pow(2, double(log2SizeL)), pow(2, double(log2SizeL)));
	primitives.chroma[yuv->m_csp].cu[log2SizeL - 2].copy_pp(dstV, dstYuv->m_csize, srcV, yuv->m_csize, pow(2, double(log2SizeL)), pow(2, double(log2SizeL)));
}

int Yuv_getAddrOffset(uint32_t absPartIdx, uint32_t width)
{
	int blkX = g_zscanToPelX[absPartIdx];
	int blkY = g_zscanToPelY[absPartIdx];

	return blkX + blkY * width;
}

int Yuv_getChromaAddrOffset(const  Yuv *yuv, uint32_t absPartIdx)
{
	int blkX = g_zscanToPelX[absPartIdx] >> yuv->m_hChromaShift;
	int blkY = g_zscanToPelY[absPartIdx] >> yuv->m_vChromaShift;

	return blkX + blkY * yuv->m_csize;
}

pixel* Yuv_getLumaAddr(Yuv* yuv, uint32_t absPartIdx)
{
	return yuv->m_buf[0] + Yuv_getAddrOffset(absPartIdx, yuv->m_size);
}
pixel* Yuv_getCbAddr(Yuv* yuv, uint32_t absPartIdx)
{
	return yuv->m_buf[1] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
}
pixel* Yuv_getCrAddr(Yuv* yuv, uint32_t absPartIdx)
{
	return yuv->m_buf[2] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
}
pixel* Yuv_getChromaAddr(Yuv* yuv, uint32_t chromaId, uint32_t absPartIdx)
{
	return yuv->m_buf[chromaId] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
}

pixel* Yuv_getLumaAddr_const(Yuv* yuv, uint32_t absPartIdx)
{
	return yuv->m_buf[0] + Yuv_getAddrOffset(absPartIdx, yuv->m_size);
}
const pixel* Yuv_getCbAddr_const(const  Yuv* yuv, uint32_t absPartIdx)
{
	return yuv->m_buf[1] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
}
const pixel* Yuv_getCrAddr_const(const  Yuv* yuv, uint32_t absPartIdx)
{
	return yuv->m_buf[2] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
}
const pixel* Yuv_getChromaAddr_const(const Yuv* yuv, uint32_t chromaId, uint32_t absPartIdx)
{
	return yuv->m_buf[chromaId] + Yuv_getChromaAddrOffset(yuv, absPartIdx);
}

void Yuv_addClip(Yuv *yuv, const Yuv* srcYuv0, const struct ShortYuv* srcYuv1, uint32_t log2SizeL)
{
	primitives.cu[log2SizeL - 2].add_ps(yuv->m_buf[0], yuv->m_size, srcYuv0->m_buf[0], srcYuv1->m_buf[0], srcYuv0->m_size, srcYuv1->m_size, pow(2, double(log2SizeL)), pow(2, double(log2SizeL)));
	primitives.chroma[yuv->m_csp].cu[log2SizeL - 2].add_ps(yuv->m_buf[1], yuv->m_csize, srcYuv0->m_buf[1], srcYuv1->m_buf[1], srcYuv0->m_csize, srcYuv1->m_csize, pow(2, double(log2SizeL)), pow(2, double(log2SizeL)));
	primitives.chroma[yuv->m_csp].cu[log2SizeL - 2].add_ps(yuv->m_buf[2], yuv->m_csize, srcYuv0->m_buf[2], srcYuv1->m_buf[2], srcYuv0->m_csize, srcYuv1->m_csize, pow(2, double(log2SizeL)), pow(2, double(log2SizeL)));
}

