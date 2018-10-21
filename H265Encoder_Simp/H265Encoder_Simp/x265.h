#ifndef _X265_H_
#define _X265_H_

#include "common.h"


#define X265_B_ADAPT_NONE       0
#define X265_B_ADAPT_FAST       1
#define X265_B_ADAPT_TRELLIS    2

/* NOTE! For this release only X265_CSP_I420 and X265_CSP_I444 are supported */

/* Supported internal color space types (according to semantics of chroma_format_idc) */
#define X265_CSP_I400           0  /* yuv 4:0:0 planar */
#define X265_CSP_I420           1  /* yuv 4:2:0 planar */
#define X265_CSP_I422           2  /* yuv 4:2:2 planar */
#define X265_CSP_I444           3  /* yuv 4:4:4 planar */
#define X265_CSP_COUNT          4  /* Number of supported internal color spaces */


#define X265_TYPE_AUTO          0x0000  /* Let x265 choose the right type */
#define X265_TYPE_IDR           0x0001
#define X265_TYPE_I             0x0002
#define X265_TYPE_P             0x0003
#define X265_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define X265_TYPE_B             0x0005

#define IS_X265_TYPE_I(x) ((x) == X265_TYPE_I || (x) == X265_TYPE_IDR)
#define IS_X265_TYPE_B(x) ((x) == X265_TYPE_B || (x) == X265_TYPE_BREF)

#define X265_REF_LIMIT_DEPTH    1
#define X265_REF_LIMIT_CU       2

#define X265_BFRAME_MAX         16
#define X265_MAX_FRAME_THREADS  16

#define X265_QP_AUTO                 0

/* Analysis options */
#define X265_ANALYSIS_OFF  0
#define X265_ANALYSIS_SAVE 1
#define X265_ANALYSIS_LOAD 2


#define X265_AQ_NONE                 0
#define X265_AQ_VARIANCE             1
#define X265_AQ_AUTO_VARIANCE        2

#define X265_MAX_SUBPEL_LEVEL   7

#define X265_BUILD 59

typedef enum
{
	NAL_UNIT_CODED_SLICE_TRAIL_N = 0,
	NAL_UNIT_CODED_SLICE_TRAIL_R,
	NAL_UNIT_CODED_SLICE_TSA_N,
	NAL_UNIT_CODED_SLICE_TLA_R,
	NAL_UNIT_CODED_SLICE_STSA_N,
	NAL_UNIT_CODED_SLICE_STSA_R,
	NAL_UNIT_CODED_SLICE_RADL_N,
	NAL_UNIT_CODED_SLICE_RADL_R,
	NAL_UNIT_CODED_SLICE_RASL_N,
	NAL_UNIT_CODED_SLICE_RASL_R,
	NAL_UNIT_CODED_SLICE_BLA_W_LP = 16,
	NAL_UNIT_CODED_SLICE_BLA_W_RADL,
	NAL_UNIT_CODED_SLICE_BLA_N_LP,
	NAL_UNIT_CODED_SLICE_IDR_W_RADL,
	NAL_UNIT_CODED_SLICE_IDR_N_LP,
	NAL_UNIT_CODED_SLICE_CRA,
	NAL_UNIT_VPS = 32,
	NAL_UNIT_SPS,
	NAL_UNIT_PPS,
	NAL_UNIT_ACCESS_UNIT_DELIMITER,
	NAL_UNIT_EOS,
	NAL_UNIT_EOB,
	NAL_UNIT_FILLER_DATA,
	NAL_UNIT_PREFIX_SEI,
	NAL_UNIT_SUFFIX_SEI,
	NAL_UNIT_INVALID = 64,
} NalUnitType;


/* The data within the payload is already NAL-encapsulated; the type is merely
* in the struct for easy access by the calling application.  All data returned
* in an x265_nal, including the data in payload, is no longer valid after the
* next call to x265_encoder_encode.  Thus it must be used or copied before
* calling x265_encoder_encode again. */
typedef struct x265_nal
{
	uint32_t type;        /* NalUnitType */
	uint32_t sizeBytes;   /* size in bytes */
	uint8_t* payload;
} x265_nal;

typedef struct x265_analysis_data
{
	void*            interData;
	void*            intraData;
	uint32_t         frameRecordSize;
	uint32_t         poc;
	uint32_t         sliceType;
	uint32_t         numCUsInFrame;
	uint32_t         numPartitions;
} x265_analysis_data;


/* Zones: override ratecontrol for specific sections of the video.
* If zones overlap, whichever comes later in the list takes precedence. */
typedef struct x265_zone
{
	int   startFrame, endFrame; /* range of frame numbers */
	int   bForceQp;             /* whether to use qp vs bitrate factor */
	int   qp;
	float bitrateFactor;
} x265_zone;

/* Used to pass pictures into the encoder, and to get picture data back out of
* the encoder.  The input and output semantics are different */
typedef struct x265_picture
{
	/* ��ʾʱ�����pts�������û����壬������˷��� */
	int64_t pts;

	/* ����ʱ�����pts����������˺��ԣ����Ѽ�¼��PTS�и��ƣ�������˷��� */
	int64_t dts;

	/*ǿ���������������ڷ�X265_QP_AUTOģʽ�£�����ֵ��������ṩ���������������ͬ֡��POC�����ء�*/
	void*   userData;

	/*����������֡��ָ����ƽ�����ɲ�ɫ�ռ�ֵȷ��������ʾ�Ƿ�Ϊ��ɫ��Ƶ*/
	void*   planes[3];

	/* �������м���ֽڿ�� */
	int     stride[3];

	/* ����������֡��ָ����x265_picture_init()��������Ϊ���������ڲ�������ȣ�
	* ������ֶα�����������֡����ȡ�������8��16֮�䡣ֵ����8��ζ��ÿ��������16λ��
	* ������������ȴ����ڲ�������ȣ�������������µ��������������ء�����8λ����������������Ĥ���ڲ�������ȡ�
	* �������bitDepth�����ڲ�������������ȡ� */
	int     bitDepth;

	/* ����������֡��ָ����X265_TYPE_AUTO��������x265_picture_init()��������Ϊauto��������˷��� */
	int     sliceType;

	/* ������˺�����������֡��������������˷��� */
	int     poc;

	/*����������֡��ָ����X265_CSP_I420����������������������ڲ���ɫ�ռ�ƥ�䡣x265_picture_init()�����ֵ��ʼ��Ϊ�ڲ���ɫ�ռ䡣 */
	int     colorSpace;

	/* Ϊ��֡�ڱ�������ָ��������QP������Ϊ0�����������ȷ����QP�� */
	int     forceqp;

	/*���param.analysisModeΪx265_analysis_off�����ֶ������������˱����ԡ�
	* �����û�����Ϊÿһ���ݵ���������֡����x265_alloc_analysis_data()���������������
	* ������ˣ���param.analysisModeΪX265_ANALYSIS_LOAD��analysisData��Աָ����Чʱ��
	* ��������ʹ������洢�����������ٱ�����������������ˣ���param.analysisModeΪX265_ANALYSIS_SAVE��analysisData��Աָ����Чʱ��
	* ���������������д��������ݽṹ��*/
	x265_analysis_data analysisData;

} x265_picture;

typedef enum
{
	X265_DIA_SEARCH,
	X265_HEX_SEARCH,
	X265_UMH_SEARCH,
	X265_STAR_SEARCH,
	X265_FULL_SEARCH
} X265_ME_METHODS;

/* rate tolerance method */
typedef enum
{
	X265_RC_ABR,
	X265_RC_CQP,
	X265_RC_CRF
} X265_RC_METHODS;


/* Stores all analysis data for a single frame */

typedef struct x265_param
{

	int frameNumThreads;

	/* Enable analysis and logging distribution of CUs encoded across various
	* modes during mode decision. Default disabled */
	int       bLogCuStats;

	/* Enable the measurement and reporting of PSNR. Default is enabled */
	int       bEnablePsnr;

	/* Enable the measurement and reporting of SSIM. Default is disabled */
	int       bEnableSsim;

	/*== �ڲ�֡�淶 ==*/

	/* �ڲ�������λ��. ��� x265 ������8λ��ȵ�ͼ�� (HIGH_BIT_DEPTH=0), ��֡������8, ������10. */
	int       internalBitDepth;

	/* ֡�ʵķ��Ӻͷ�ĸ */
	uint32_t  fpsNum;

	uint32_t  fpsDenom;

	/*Դͼ��Ŀ�ȣ����أ�����������Ȳ���4��ż�����������������ڲ����ͼ����������һ���Ҫ��HEVC֧��������Ч���*/
	int       sourceWidth;

	/*Դͼ��ĸ߶ȣ����أ�����������Ȳ���4��ż�����������������ڲ����ͼ����������һ���Ҫ��HEVC֧��������Ч�߶�*/
	int       sourceHeight;

	/*Դͼ��ĸ������͡�0-����ͼ��Ĭ�ϣ���1-��һ֡Ϊ������2-��һ֡Ϊ�׳� */
	int       interlaceMode;

	/*Ҫ�������֡�������ɴ��û�����ģ�--frames����(--seek)�м���õ������������Ǵ�ĳ���ܵ����룬
	* ����Ա���Ϊ0.�������ں����2 pass RateControl����˽���ֵ�洢�ڲ����С�*/
	int       totalFrames;

	/*== ����/��/�� ==*/
	/*Note:������x265_param_apply_profile()������С����������ˮƽ��������
	*Ĭ��Ϊ�㣬�������˾��ɱ��������Զ���⡣�����ָ���������������Բ�����ָ�����ڵı���淶������������ﲻ���������
	*���ͷ������沢�˳����롣�������������󼶸���ʵ�ʼ�����ʵ�����󼶱���ʶ����ֵ������Ϊ��������level����������10��
	*����5.1��ָ��Ϊ��5.1����51��,5.0������ָ��Ϊ��5.0����50��.*/

	int       levelIdc;

	/*���levelIdc�ѱ�ָ�������㣩���ñ�־������Main��0����High��1���㡣Ĭ����Main tier (0) */
	int       bHighTier;

	/*P��B��������ʹ�õ�L0�ο���������������Ӱ�����֡�������Ĵ�С���������Խ�ߣ����и���Ĳο�֡�����˶������������ܣ��ٶȣ�Ϊ���������ѹ��Ч�ʡ�ȡֵ������1��16֮�䡣Ĭ��Ϊ3*/
	int       maxNumReferences;

	/*����libx265�����������ϸ�ȼ�Ҫ���HEVC������Ĭ��Ϊ��*/
	int       bAllowNonConformance;

	/*==  ������ѡ�� ==*/

	int       internalCsp;
	/*�ñ�־�����Ƿ�Ӧ����ÿ���ؼ�֡һ�����VPS, SPS ��PPSͷ��Ĭ��Ϊ��*/
	int       bRepeatHeaders;

	/*�ñ�־ָʾ�������Ƿ�Ӧ����NAL��Ԫ֮ǰ������ʼ�루Annex B ��ʽ���򳤶ȣ��ļ���ʽ����Ĭ��Ϊ�棬Annex B��MuxersӦ�ý�������Ϊ��ȷ��ֵ
	*ע�ͣ�����Ƶ��������Muxers���ɽ���Ƶѹ�����ݣ�����H.264������Ƶѹ�����ݣ�����AAC���ϲ���һ����װ��ʽ���ݣ�����MKV����ȥ��*/
	int       bAnnexB;

	/*�ñ�־ָʾ�������Ƿ�Ӧ��ÿһ���ʵ�Ԫ��ʼ������һ�����ʵ�Ԫ�ָ�NAL��Ĭ��Ϊ��*/
	int       bEnableAccessUnitDelimiters;

	/*���û�������SEI��֡��ʱSEI����ʶHRD��Hypothetical Reference Decoder��������Ĭ�Ͻ���*/
	int       bEmitHRDSEI;

	/*���ô�����ͷ���û�����SEI�ķ��ͣ��������˱������汾��������Ϣ�ͱ����������Ե���Ŀ�ģ����Ƿǳ����õģ�������汾�ź͹�����Ϣ���ܻ����������ƫ��͸��Żع���ԡ�Ĭ������*/
	int       bEmitInfoSEI;

	/*���ÿһ���������ؽ�ͼ��ƽ���ϣ�ı���֡������SEI��Ϣ�����ɡ����������������֤�����������ؽ�ͼ�񣬲������κβ�ƥ�䡣
	* ����һ�ֵ�����������ϣ����ΪMD5��1����CRC(2), Checksum(3). Ĭ����0��none*/
	int       decodedPictureHashSEI;

	/*����ʱ����ʱ���Ӳ㣬ʹ��temporalId����ʶ�ѱ���������NAL��Ԫ���������������ʱ����㣨��0���Դ�Լһ��֡����ȡ������ʱ��߲㣨��1�����������е�����֡��*/
	int       bEnableTemporalSubLayers;

	/*==  GOP�ṹ���������;��ߣ�lookahead:δ��֡�� ==*/

	/*����open GOP-��ζ��I������һ����IDR�������I����֮������֡���ܻ�ο���I֮֡ǰ�ı���֡����Щ֮ǰ�ı���֡�Ա����ڽ���֡�������С�
	* open GOPͨ�����и��ߵ�ѹ��Ч�ʣ��Ա������ܵ�Ӱ����Ժ��Բ��ƣ������������ų�����Ĭ��Ϊ��
	* ע�ͣ�
	1.I֡��IDR֡������
	I֡��IDR֡��Ҳ��Ϊ�ؼ�֡����ʹ��֡��Ԥ�⡣�ڱ���ͽ�����Ϊ�˷��㣬Ҫ���׸�I֡������I֡���𿪣����ԲŰ��׸�I֡��IDR֡��
	�����ͷ�����Ʊ���ͽ������̡�IDR֡������������ˢ�£�ʹ�����´�������IDR֡��ʼ��������һ���µĿ�ʼ���롣��I֡������������ʵ�������
	���������IDR�е���IDR�ᵼ��DPB��Decoded Picture Buffer������֡����������Ϊ�ο�֡�б�--���ǹؼ����ڣ���գ�
	��I֡���ᡣIDR֡һ����I֡��һ�������п����кܶ�I֡��I֮֡���֡�������ø�I֮֡ǰ��֡���˶��ο�������IDR֡��˵��
	��IDR֮֡�������֡�����������κ�IDR֮֡ǰ��֡���ݣ�����෴��������ͨ��I֡��˵��λ����֮���B-��P-֡��������λ����ͨI-֮֡ǰ��I-֡��
	�������ȡ����Ƶ���У���������Զ���Դ�һ��IDR֡���ţ���Ϊ����֮��û���κ�֡����֮ǰ��֡�����ǣ�������һ��û��IDR֡����Ƶ�д�����㿪ʼ���ţ�
	��Ϊ�����֡���ǻ�����ǰ���֡��IDR֡����I֡���������յ�IDR֡ʱ�������еĲο�֡���ж�������x265_reference_reset����ʵ��--��encoder.c�ļ��У���
	������������Ҫ���Ĺ������ǣ������е�PPS��SPS�������и��¡��ɴ˿ɼ����ڱ������ˣ�ÿ��һ��IDR֡������Ӧ�ط�һ��PPS&PPS_nal_uint.
	2.����keyint:����x265���������IDR֡����ƹؼ�֡����ࡣIDR֡����Ƶ���ġ��ָ�����������֡��������ʹ��Խ���ؼ�֡��֡��Ϊ�ο�֡��IDR֡��I֡��һ�֣�
	��������Ҳ���ο�����֡������ζ�����ǿ�����Ϊ��Ƶ��������seek���㡣ͨ��������ÿ�������IDR֡�������֡����������GOP�鳤�ȣ���
	�ϴ��ֵ������IDR֡���٣�����ռ�ÿռ���ٵ�P֡�ͺ�B֡ȡ������Ҳ��ͬʱ�����˲ο�֡ѡ������ơ���С��ֵ���¼�������һ�����֡�����ƽ��ʱ�䡣
	���飺Ĭ��ֵ��fps��10������250���Դ������Ƶ���ܺá������Ϊ���⡢�㲥��ֱ������������ʲôרҵ�����룬Ҳ�����Ҫ��С��ͼ���飨GOP�����ȣ�һ�����fps����
	�μ�min-keyint,scenecut,intra-refresh
	3.����min-keyintĬ�ϣ�auto(keyint/10)˵�����μ�keyint��˵������С��keyint��Χ�ᵼ�²���������ġ�IDR֡������˵��һ��������������
	��ѡ��������IDR֮֡�����С���롣���飺Ĭ�ϣ�������fps��� �μ���keyint,scenecut
	����scenecut���þ���ʹ��I֡��IDR֡����ֵ�������任��⣩��X265�����ÿһ֡��ǰһ֡�����ƶȣ�������ֵ����scenecut����ô����Ϊ��⵽һ���������任����
	�����ʱ������һ֡�ľ���С��min-keyint�����һ��I֡����֮�����һ��IDR֡���ϸߵ�ֵ��������⵽�������任���ļ��ʡ�����scenecut=0��no-scenecut��Ч��
	����ʹ��Ĭ��ֵ40.�ο���keyint,min-keyint,no-scenecut*/
	int       bOpenGOP;

	/*���������ע��3���ᵽ��min-keyint,��������IDR֮֡�����С���룬������ֵ�ǰ֡��scenecut��������ǰһIDR֡�ľ���С��min-keyint(��keyframeMin),
	* ���ڴ˴�����һ��I֡����֮�����һ��IDR֡��*/
	int       keyframeMin;

	/*keyframeMax���������ؼ�֡����һ�������ڵ�֡������ע��2���ᵽ��keyint�������Ϊ0��1��������֡����I֡����ν��ȫ֡��ģʽ����
	* ��ֵ���ڲ���ӳ��ΪMAX_INT(2147483647,max value of signed 32-bit integer),��ʹ֡0��ΪΨһ��I֡��ȱʡֵΪ250*/
	int       keyframeMax;

	/*����lookahead�������������B֡����b-adaptΪ0��keyframeMax����bframesʱ��lookahead��ÿ����P֮������̶�ģʽ�ġ�bframes����B֡��
	* b-adaptΪ1ʱ������������lookahead����bframes��ֵ��b-adaptΪ2ʱ��bframes��ֵ�����˶�����������ɵ�������POC,picture order count�����룬
	* ������lookahead�ļ��㸺�������ֵԽ�ߡ�Lookahead�ܹ�����ʹ�õ�B֡Խ�࣬ͨ�����ԸĽ�ѹ����Ĭ��ֵΪ3�����ֵ16*/
	int       bframes;

	/*����lookahead������ģʽ��b-adaptΪ0ʱ������keyframeMax��bframes��ֵ��GOP�ṹ�ǹ̶��ģ�b-adaptΪ1ʱ��ʹ����������lookaheadѡ��B֡λ�á�
	* b-adaptΪ2ʱ�����viterbi B·��ѡ��*/
	int       bFrameAdaptive;

	/*����ʱ����������ʹ�ô���2��B֡��ÿ��mini-GOP�м��B֡��Ϊ�ڽ�B֡���˶��ο����������ѹ��Ч�ʣ������ܣ�ָ�����ٶȣ���ʧȴ��С��
	* �������ʿ�����ĳ��B֡��P֡���ĳ�������ο���B֡��Ĭ�����á�*/
	int       bBPyramid;

	/*���뵽lookahead��B֡�ɱ����Ƶ�ĳ��ֵ����������һ����ֵ��ʹB֡�ƺ������󣬻�ʹ��lookaheadѡ������P֡��������һ����ֵ��
	* ��ʹ��lookaheadѡ������B֡��Ĭ��Ϊ0��û���κ����ơ�*/
	int       bFrameBias;

	/*��������������ǰ��������lookahead���Ŷӵ�֡�������Ӵ����ֱֵ�������˱����ӳ١�����Խ����lookahead�ľ��߽����Խ�ţ��ر��Ƕ�b-adapt 2���Ρ�
	* ����cu-treeʱ��cu-tree��������Ч������еĳ��ȳ����Թ�ϵ��ȱʡΪ40֡�����Ϊ250.*/
	int       lookaheadDepth;

	/*��lookahead�У�ʹ�ö���߳�������ÿ֡�Ĺ��Ƴɱ���
	* ��bFrameAdaptive ���� 2ʱ,�����֡�ɱ����ƽ�����������ʽ��ɣ����ͬʱ�����������ƣ����ɱ����ƺ�lookaheadSlices�����ԡ�
	* �����ܵ�Ӱ������൱��С���������Խ�ߣ�֡�ɱ�����Խ����ȷ����Ϊ�����߽紦�������Ķ�ʧ�ˣ����⵼��B-֡�ͳ����л����ߵĲ���ȷ��Ĭ��Ϊ0-���á�1��0��ͬ�����16.*/
	int       lookaheadSlices;

	/*ȷ��lookahead��λ�����ⳡ���л���������ֵ��ȱʡֵ�Ƽ�Ϊ40.�ñ�����ע�����ᵽ��scenecut.*/
	int       scenecutThreshold;

	/*==   ���뵥ԪCU����==*/

	/*�������ر�ʾ�����CU��Ⱥ͸߶ȡ���С������64,32��16.�óߴ�Խ��x265�Ե͸��Ӷ�����ı������Ч���������˸߷ֱ���ͼ���ѹ��Ч�ʡ��ߴ�ԽС����ǰ��֡���н���ø���Ч��
	* ��Ϊ���������ˡ�Ĭ��Ϊ64.ͬһ�����������еı���������ʹ����ͬ��maxCUSize��ֱ�����б��������رգ�������x265_cleanup()���������ֵ��*/
	uint32_t  maxCUSize;

	/*�������ر�ʾ����СCU��Ⱥ͸߶ȡ���С������64��32��16��8.ͬһ�������ڵ����б���������ʹ����ͬ��minCUSize��*/
	uint32_t  minCUSize;

	/*���þ����˶�Ԥ�⻮�֣���ֱ��ˮƽ����������CU��ȴ�64x64��8x8���á�Ĭ�Ͻ���*/
	int       bEnableRectInter;

	/*���÷ǶԳ��˶�Ԥ�⡣��CU���64��32��16�������ϡ��¡����ҷ������ʹ��25%/75%�ķָ�֡�����ĳЩ���Σ��⽫�Զ���ķ����ɱ�����ѹ��Ч�ʵ���ߡ�Ĭ�Ͻ���*/
	int       bEnableAMP;

	/*==  �в��Ĳ����任��Ԫ���� ==*/
	/*�������ر�ʾ�����TU��Ⱥ͸߶ȡ���С������32��16��8��4.�ߴ�Խ�󣬾���DCT�任�Բв��ѹ������Ч����Ȼ�������Ҳ����*/
	uint32_t  maxTUSize;

	/*���֡�����飬���������Ĳ���֮��������Ĳв��Ĳ����ݹ鸽����ȡ���ֵ������1��4֮�䡣��ֵԽ�󣬾���DCT�任�Բв��ѹ������Ч����Ȼ�������Ҳ����*/
	uint32_t  tuQTMaxInterDepth;

	/*���֡�ڱ���飬���������Ĳ���֮��������Ĳв��Ĳ����ݹ鸽����ȡ���ֵ������1��4֮�䡣��ֵԽ�󣬾���DCT�任�Բв��ѹ������Ч����Ȼ�������Ҳ����*/
	uint32_t  tuQTMaxIntraDepth;

	/*����������Ҫ�õ���ʧ�����������Level 0��ζ�������в�������ʧ���Ż�����Level 1����ʧ��ɱ�������Ϊÿһ��������level���ҳ����ŵ���������ֵ��������psy-rdoq�����ã���
	* ��level 2����ʧ����۱�������ÿ��4x4����������decimate���ߣ������������λͼ�ڱ�ʶ��ĳɱ�������RDOQλ��level 2ʱ��Psy-rdoq�ڱ�����������Ч�ʲ��ߡ�*/
	int       rdoqLevel;

	/*������ʽ��ʶÿ���任��Ԫ�����һ��ϵ���ķ���λ����������ÿTU��ʡ1���أ������Ĵ�����Ҫ�������һ��ϵ�����Ա��޸ģ��Ҵ�����С��ʧ�档Ĭ������*/
	int       bEnableSignHiding;

	/*����֡�ڱ������Ϊ�в�ֱ�ӱ��룬����DCT�任���������Ч�ʡ��Կ�Ӵ�ѡ�����Ƿ�������м��ᵼ�£��ٶȣ�������ʧ��Ĭ�Ͻ���*/
	int       bEnableTransformSkip;

	/*������ֵ�ķ�ΧΪ0��2000����������֡��CUs������˥���ĳ̶ȡ�0��ζ�Ž��á�*/
	int       noiseReductionIntra;

	/*������ֵ�ķ�ΧΪ0��2000����������֡��CUs������˥���ĳ̶ȡ�0��ζ�Ž���*/
	int       noiseReductionInter;

	/*���������б�HEVC֧�ֶ���6�����������б�����һ������֡��Ԥ���Y, Cb, Cr����һ������֡��Ԥ��
	*-NULL�͡�off���������������ţ�Ĭ�ϣ�
	*-��default��������HEVCȱʡ�����б������ʶ����Ϊ�Ǳ�ָ����
	*-���������ֶζ�������һ���ļ��������ļ������˲���HM��ʽ���Զ��������б�����ļ����ܱ���ȷ����������뽫ʧ�ܡ��Զ����б������SPS�б�ʶ��*/
	char *scalingLists;

	/*==   ֡�ڱ��빤�� ==*/
	/*������Լ����֡��Ԥ�⡣��ᵼ�¶�֡��Ԥ�⣨ģʽ�£���������������֡��Ԥ�⡣���ĳЩ���Σ��ܹ���ǿ�����������³���ԣ���P��B������ѹ�����ܻ���Ӱ�졣Ĭ�Ͻ���*/
	int       bEnableConstrainedIntra;

	/*Ϊ32x32�Ŀ�����ǿ֡��ƽ����ע����Щ���еĲο����������أ�������ƽ̹�ġ��ܲ������ѹ��Ч�ʣ�ȡ�������Դ��Ƶ��Ĭ�Ͻ���*/
	int       bEnableStrongIntraSmoothing;

	/*== ֡����빤�� ==*/
	/*��֡������ڼ������ǵ����ϲ���ѡ��Ŀ��������֣���5��1֮�䣩��ʶ������ͷ����
	* �������˱�ʶĳ���ϲ�����ı����������Ӧ�Ը��������С��������ԽС������Խ�ߣ�ָ�����ٶȣ���
	* ��ѹ��Ч��Խ�͡�Ĭ��Ϊ3*/
	uint32_t  maxNumMergeCand;

	uint32_t  limitReferences;

	int       searchMethod;

	int       subpelRefine;

	int       searchRange;

	int       bEnableTemporalMvp;

	int       bEnableWeightedPred;

	int       bEnableWeightedBiPred;

	int       bEnableLoopFilter;

	int       deblockingFilterTCOffset;

	int       deblockingFilterBetaOffset;

	int       bEnableSAO;

	int       bSaoNonDeblocked;

	/*== Analysis tools ==*/

	int       rdLevel;
	int       bEnableEarlySkip;
	int       bEnableFastIntra;
	int       bEnableTSkipFast;
	int       bCULossless;
	int       bIntraInBFrames;
	int       rdPenalty;

	double    psyRd;
	double    psyRdoq;
	int       analysisMode;

	/* Filename for analysisMode save/load. Default name is "x265_analysis.dat" */
	char* analysisFileName;

	/*== ���ʿ��� ==*/

	/*���ʿ��Ƶ���Ҫ�����ǽ����������������������Ĺ�ϵģ�ͣ�����Ŀ������ȷ����Ƶ��������е�����������
	*�����־����������������롢��·���š��任�������ͻ�·�˲����̡����������𳬸߱����ʡ���ζ��û���ٶȿ��ơ�*/
	int       bLossless;

	/*ͨ����һ��С�ķ�����������ʹ��������Cbɫ�Ȳв��QP����ƫ�ƣ�����QP��delta�����ʿ���ָ������Ĭ��Ϊ0���Ƽ�ʹ�� */
	int       cbQpOffset;

	/*ͨ����һ��С�ķ�����������ʹ��������Crɫ�Ȳв��QP����ƫ�ƣ�����QP��delta�����ʿ���ָ������Ĭ��Ϊ0���Ƽ�ʹ��*/
	int       crQpOffset;

	struct
	{
		/*��ʽģʽ�����ʿ��ƣ���API�û������Ǳز����ٵġ���һ����X265_RC_METHODSö��ֵ�е�һ����*/
		int       rateControlMode;

		/*���ں㶨����������CQP�����ʿ��ƵĻ���QP�����ÿ���飬����ӦQP�������μ���Ƥ6.3.2�ڣ����Ըı���QPֵ���������������ָ����QP������ζ��ʹ��CQP���ʿ��ơ� */
		int       qp;

		/*����AverageBitRate��ABR�����ʿ��Ƶ�Ŀ������ʡ��������������ָ���˷���bitrate������ζ��ʹ����ABR��Ĭ��Ϊ0��*/
		int       bitrate;

		/*qComp������������ѹ�����ӡ������ڲв�Ӷȣ���lookahead��������֡��������Ȩ���μ���Ƥ6.3.2�ڣ���Ĭ��ֵΪ0.6���������ӵ�1��������Ч��CQP�� */
		double    qCompress;

		/*I/P��P/B֡���QPƫ�ơ�Ĭ��ipFactor��1.4��Ĭ��pbFactor��1.3*/
		double    ipFactor;
		double    pbFactor;

		/*Ratefactor��������ĳ���㶨��������ΪĿ�ꡣ�ɽ���ֵ��0��51֮�䡣Ĭ��ֵ��28 */
		double    rfConstant;

		/*֡�����QP���졣Ĭ�ϣ�4*/
		int       qpStep;

		/*��������Ӧ����������ģʽ����һ֡������CTUs���ṩ���õı��أ����Ӷȵ͵��������ı��ظ��ࡣ����ͨ�����PSNR�и���Ӱ�죬����SSIM���Ӿ������ձ���ߡ� */
		int       aqMode;

		/*����AQƫ���ڵ�ϸ��CTUs��ǿ�ȡ�ֻ��AQ���ò���Ч��Ĭ��ֵ��1.0���ɽ���ֵ��0��3֮�䡣 */
		double    aqStrength;

		/*����VBV���õ�������ʣ����ʣ�����Ĭ������£�VBV��VideoBufferingVerifier��Ƶ�����������������Ӧ���ٶ�Ϊ����䡣*/
		int       vbvMaxBitrate;

		/*��kilobitsΪ��λ����VBV�������Ĵ�С��Ĭ����0*/
		int       vbvBufferSize;

		/*�����ڿ�ʼ����ǰ���뽫VBV��������䵽���������������1����ô��ʼ�����vbv-initvbvBufferSize��������������Ϊ��kbitsΪ��λ�ĳ�ʼ��䡣Ĭ��Ϊ0.9 */
		double    vbvBufferInit;

		/*����CUTree���ʿ��ơ����Կ�֡��ʱ�򴫲�CUs���и��٣���Ϊ��ЩCUs�������ı��ء��Ӷ��Ľ��˱���Ч�ʡ�Ĭ�ϣ�����*/
		int       cuTree;

		/*��CRFģʽ�У���VBV���������CRF��0��ζ��û�����ơ�*/
		double    rfConstantMax;

		/*��CRFģʽ�У���VBV��������СCRF*/
		double    rfConstantMin;

		/*��ͨ����
		*ע�ͣ�passĬ��ֵ:��
		*��Ϊ��ͨ�����һ����Ҫ���á�������x265��δ���--stats�������������趨:
		*1������һ���µ�ͳ�����ϵ������ڵ�һ�׶�ʹ�ô�ѡ��.
		*2����ȡͳ�����ϵ����������ս׶�ʹ�ô�ѡ��.
		*3����ȡͳ�����ϵ��������¡�
		*ͳ�����ϵ�������ÿ������֡����Ϣ���������뵽x265�ԸĽ������������ִ�е�һ�׶�������ͳ�����ϵ�����Ȼ��ڶ��׶ν�����һ����ѻ�����Ƶ���롣Ŀ����Ҫ�Ӹ��õ����ʿ����л��档
		*��ĳ����ͨ�����У�����д״̬��Ϣ��״̬����ļ���*/
		int       bStatWrite;
		int       bStatRead;
		/*2ͨ���/����״̬�ļ����ļ��������û��ָ������������Ĭ�ϲ���x265_2pass.log��*/
		char* statFileName;
		double    qblur;
		double    complexityBlur;
		int       bEnableSlowFirstPass;
		int        zoneCount;
		x265_zone* zones;
		int bStrictCbr;
		uint32_t qgSize;
	} rc;

	//== Video Usability Information ==//
	struct
	{
		int aspectRatioIdc;
		int sarWidth;
		int sarHeight;

		int bEnableOverscanInfoPresentFlag;
		int bEnableOverscanAppropriateFlag;
		int bEnableVideoSignalTypePresentFlag;

		int videoFormat;

		int bEnableVideoFullRangeFlag;
		int bEnableColorDescriptionPresentFlag;

		int colorPrimaries;

		int transferCharacteristics;

		int matrixCoeffs;

		int bEnableChromaLocInfoPresentFlag;

		int chromaSampleLocTypeTopField;

		int chromaSampleLocTypeBottomField;

		int bEnableDefaultDisplayWindowFlag;

		int defDispWinLeftOffset;
		int defDispWinRightOffset;
		int defDispWinTopOffset;
		int defDispWinBottomOffset;
	} vui;

	/* Use multiple threads to measure CU mode costs. Recommended for many core
	* CPUs. On RD levels less than 5, it may not offload enough work to warrant
	* the overhead. It is useful with the slow preset since it has the
	* rectangular predictions enabled. At RD level 5 and 6 (preset slower and
	* below), this feature should be an unambiguous win if you have CPU
	* cores available for work. Default disabled */
	int       bDistributeModeAnalysis;

	/* Use multiple threads to perform motion estimation to (ME to one reference
	* per thread). Recommended for many core CPUs. The more references the more
	* motion searches there will be to distribute. This option is often not a
	* win, particularly in video sequences with low motion. Default disabled */
	int       bDistributeMotionEstimation;

	char* masteringDisplayColorVolume;

	char* contentLightLevelInfo;

} x265_param;





#endif