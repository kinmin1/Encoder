#include<stdio.h>
#include"x265.h"
#include"param.h"
#include"api.h"

int ECS_encode(const char* infile, int width, int height, int type, const char* outfile);

int main()
{
	printf("HELLO WORLD!\n");
	ECS_encode("akiyo_cif_352_288.yuv", 352, 288, 1, "str.bin");
	while(1);
	return 0;
}

int ECS_encode(const char* infile, int width, int height, int type, const char* outfile)
{
	int frame_cnt = 40;
	FILE *fp_src = NULL;
	FILE *fp_dst = NULL;
	
	uint32_t luma_size = 0;
	uint32_t chroma_size = 0;
	int i_frame = 0;
	int ret = 0;

	x265_param param;//=param_1;//x265_param��x265.h�ж���Ľṹ�壬������x265���������
	
	x265_param_default(&param);

	x265_picture *pic_in = NULL;

	int csp = X265_CSP_I420;
	
	fp_src = fopen(infile, "r");
	fp_dst = fopen(outfile, "wb");
	if (fp_src == NULL || fp_dst == NULL)
	{
		printf("Error open yuv files:");
		return -1;
	}
	
	param.internalCsp = csp;
	param.sourceWidth = width;
	param.sourceHeight = height;
	param.fpsNum = 50000;//25; // ֡��
	param.fpsDenom = 1000;//1; // ֡��
	
	Encoder *encoder = x265_encoder_open(&param);
	/*
	pic_in = x265_picture_alloc();
	if (pic_in == NULL)
	{
		goto out;
	}

	x265_picture_init(&param, pic_in);

	// Y������С
	luma_size = width * height;

	pic_in->stride[0] = width;
	pic_in->stride[1] = width / 2;
	pic_in->stride[2] = width / 2;
	*/
	/*
	// ������֡��
	fseek(fp_src, 0, SEEK_END);
	switch(csp)
	{
	case X265_CSP_I444:
	i_frame = ftell(fp_src) / (luma_size*3);
	chroma_size = luma_size;
	break;
	case X265_CSP_I420:
	i_frame = ftell(fp_src) / (luma_size*3/2*10);
	chroma_size = luma_size / 4;
	break;
	default:
	printf("Colorspace Not Support.\n");
	return -1;
	}
	fseek(fp_src,0,SEEK_SET);
	printf("framecnt: %d, y: %d u: %d\n", i_frame, luma_size, chroma_size);
	*/

/*	x265_encoder_parameters(encoder, &param);

	x265_nal *p_nal;
	uint32_t nal;
	if (!param.bRepeatHeaders)
	{
		if (x265_encoder_headers(encoder, &p_nal, &nal) < 0)
		{
			printf("Failure generating stream headers\n");
		}

		fwrite(p_nal->payload, sizeof(uint8_t), encoder->m_nalList.m_occupancy, fp_dst);
		free(encoder->m_nalList.m_buffer); encoder->m_nalList.m_buffer = NULL;
	}
	fclose(fp_dst);

	//unsigned char *buffer_temp;//=(unsigned char *)0x58000000;
	pic_in->planes[0] = buffer;
	pic_in->planes[1] = buffer + luma_size;
	pic_in->planes[2] = buffer + luma_size * 5 / 4;

	ReadPic_1(buffer);
	for (int i = 0; i < frame_cnt; i++)
	{
		int cc0_1 = __sysreg_read(CC0);//��CC0

		ret = x265_encoder_encode(encoder, &p_nal, &nal, pic_in, NULL);
		printf("encode frame: %5d\n", i + 1);

		int cc0_2 = __sysreg_read(CC0);//��CC0
		printf("finish one frame Time=%d\n", cc0_2 - cc0_1);

		ReadPic(buffer, (i + 1) * 88 * 72 * 3 / 2);
		if (ret < 0)
		{
			printf("Error encode frame: %d.\n", i + 1);
			goto out;
		}
	}
out:
	x265_encoder_close(encoder);
	x265_picture_free(pic_in);
	fclose(fp_src);
	fclose(fp_dst);*/
	return 0;
}
