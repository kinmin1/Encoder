#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	FILE *fp_src=NULL;
	fp_src = fopen("BUS_352x288_30_orig_150f.yuv", "r");
	//fp_dst = fopen(outfile, "wb");
	if (fp_src == NULL /*|| fp_dst == NULL*/)
	{
		printf("Error open yuv files:");
		return -1;
	}
	unsigned char *buffer = (unsigned char *)malloc( 352*288 * 3 / 2);

	for (int i = 0; i < 10; i++)
	{
		fseek(fp_src, i * 352 * 288 * 3 / 2, SEEK_SET);
		fread(buffer, sizeof(unsigned char), 352 * 288 * 3 / 2, fp_src);
		
		//int cur_fp = ftell(fp_src);
		//fp_src = fp_src + luma_size * 3 / 2;
		//printf("%d\n", cur_fp);
	}

}