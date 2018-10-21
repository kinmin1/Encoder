#include "api.h"
#include "param.h"


Encoder *x265_encoder_open(x265_param *p)
{
	
	if (!p)
		return NULL;

	Encoder *encoder = NULL;
	
	if (x265_set_globals(p))
		goto fail;
	/*
	x265_setup_primitives();

	encoder = (Encoder *)malloc(sizeof(Encoder));//完成所有帧编码后释放&Encoder_1;//
	//printf("sizeof(Encoder)=%d\n",sizeof(Encoder));
	if (!encoder)
		printf("malloc Encoder fail!\n");

	HRDInfo_Info(&encoder->m_vps.hrdParameters);
	Window_dow(&encoder->m_sps.conformanceWindow);
	HRDInfo_Info(&encoder->m_sps.vuiParameters.hrdParameters);
	Window_dow(&encoder->m_sps.vuiParameters.defaultDisplayWindow);
	NAL_nal(&encoder->m_nalList);
	Window_dow(&encoder->m_conformanceWindow);

	Encoder_init(encoder);
	Encoder_configure(encoder, p);

	// may change rate control and CPB params
	if (!enforceLevel(p, &encoder->m_vps))
		goto fail;

	// will detect and set profile/tier/level in VPS
	determineLevel(p, &encoder->m_vps);

	Encoder_create(encoder);

	if (encoder->m_aborted)
		goto fail;

	return (Encoder *)encoder;
	*/
fail:
	free(encoder);
	encoder = NULL;
	return NULL;
}