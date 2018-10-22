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
	/* 显示时间戳（pts），由用户定义，在输出端返回 */
	int64_t pts;

	/* 解码时间戳（pts），在输入端忽略，从已记录的PTS中复制，在输出端返回 */
	int64_t dts;

	/*强制量化步长（用在非X265_QP_AUTO模式下）。该值由输入端提供，并在输出端由相同帧（POC）返回。*/
	void*   userData;

	/*必须在输入帧上指定，平面数由彩色空间值确定，即表示是否为彩色视频*/
	void*   planes[3];

	/* 相邻两行间的字节跨度 */
	int     stride[3];

	/* 必须在输入帧上指定。x265_picture_init()将它设置为编码器的内部比特深度，
	* 但这个字段必须描述输入帧的深度。必须在8与16之间。值大于8意味着每输入样本16位。
	* 如果输入比特深度大于内部比特深度，则编码器将向下调整（降档）像素。大于8位的输入样本将被掩膜到内部比特深度。
	* 在输出端bitDepth将是内部编码器比特深度。 */
	int     bitDepth;

	/* 必须在输入帧上指定：X265_TYPE_AUTO或其它。x265_picture_init()将它设置为auto，在输出端返回 */
	int     sliceType;

	/* 在输入端忽略它，设置帧计数器，在输出端返回 */
	int     poc;

	/*必须在输入帧上指定：X265_CSP_I420或其它。它必须与编码器内部颜色空间匹配。x265_picture_init()将这个值初始化为内部颜色空间。 */
	int     colorSpace;

	/* 为该帧在编码器内指定条带基QP。设置为0，允许编码器确定基QP。 */
	int     forceqp;

	/*如果param.analysisMode为x265_analysis_off，该字段在输入和输出端被忽略。
	* 否则用户必须为每一传递到编码器的帧调用x265_alloc_analysis_data()分配分析缓冲区。
	* 在输入端，当param.analysisMode为X265_ANALYSIS_LOAD且analysisData成员指针有效时，
	* 编码器将使用这里存储的数据来减少编码器工作。在输出端，当param.analysisMode为X265_ANALYSIS_SAVE且analysisData成员指针有效时，
	* 编码器将输出分析写入这个数据结构。*/
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

	/*== 内部帧规范 ==*/

	/* 内部编码器位深. 如果 x265 被用于8位深度的图像 (HIGH_BIT_DEPTH=0), 这帧必须是8, 否者是10. */
	int       internalBitDepth;

	/* 帧率的分子和分母 */
	uint32_t  fpsNum;

	uint32_t  fpsDenom;

	/*源图像的宽度（像素）。如果这个宽度不是4的偶数倍，编码器将在内部填充图像，以满足这一最低要求。HEVC支持所有有效宽度*/
	int       sourceWidth;

	/*源图像的高度（像素）。如果这个宽度不是4的偶数倍，编码器将在内部填充图像，以满足这一最低要求。HEVC支持所有有效高度*/
	int       sourceHeight;

	/*源图像的隔行类型。0-逐行图像（默认）。1-第一帧为顶场，2-第一帧为底场 */
	int       interlaceMode;

	/*要被编码的帧总数，可从用户输入的（--frames）和(--seek)中计算得到。假如输入是从某个管道读入，
	* 这可以保持为0.它会用于后面的2 pass RateControl，因此将此值存储在参数中。*/
	int       totalFrames;

	/*== 档次/层/级 ==*/
	/*Note:档次由x265_param_apply_profile()定义最小解码器需求水平（级）。
	*默认为零，它暗含了经由编码器的自动检测。如果被指定，编码器将尝试产生该指定级内的编码规范。如果编码器达不到这个级，
	*它就发出警告并退出编码。如果所请求的需求级高于实际级，则实际需求级被标识。该值被定义为整数，即level（级）乘以10，
	*例如5.1级指定为”5.1”或”51”,5.0级（别）指定为”5.0”或”50”.*/

	int       levelIdc;

	/*如果levelIdc已被指定（非零），该标志将区分Main（0）和High（1）层。默认是Main tier (0) */
	int       bHighTier;

	/*P或B条带可以使用的L0参考的最大数量。这会影响解码帧缓冲区的大小。这个数字越高，将有更多的参考帧用于运动搜索，以性能（速度）为代价提高了压缩效率。取值必须在1和16之间。默认为3*/
	int       maxNumReferences;

	/*允许libx265发出不符合严格等级要求的HEVC码流。默认为假*/
	int       bAllowNonConformance;

	/*==  比特流选项 ==*/

	int       internalCsp;
	/*该标志表明是否应该与每个关键帧一起输出VPS, SPS 和PPS头。默认为假*/
	int       bRepeatHeaders;

	/*该标志指示编码器是否应该在NAL单元之前生成起始码（Annex B 格式）或长度（文件格式）。默认为真，Annex B。Muxers应该将其设置为正确的值
	*注释：视音频复用器（Muxers）可将视频压缩数据（列如H.264）和音频压缩数据（例如AAC）合并到一个封装格式数据（例如MKV）中去。*/
	int       bAnnexB;

	/*该标志指示编码器是否应在每一访问单元开始处发出一个访问单元分隔NAL。默认为假*/
	int       bEnableAccessUnitDelimiters;

	/*启用缓冲周期SEI和帧计时SEI来标识HRD（Hypothetical Reference Decoder）参数，默认禁用*/
	int       bEmitHRDSEI;

	/*启用带有流头的用户数据SEI的发送，他描述了编码器版本、构建信息和编码参数。针对调试目的，这是非常有用的，但编码版本号和构建信息可能会让你的码流偏离和干扰回归测试。默认启用*/
	int       bEmitInfoSEI;

	/*针对每一包含三个重建图像平面哈希的编码帧，启用SEI消息的生成。大多数解码器将验证它所产生的重建图像，并报告任何不匹配。
	* 这是一种调试特征。哈希类型为MD5（1），CRC(2), Checksum(3). 默认是0，none*/
	int       decodedPictureHashSEI;

	/*编码时启用时域子层，使用temporalId来标识已编码条带的NAL单元。输出码流可以在时域基层（层0）以大约一半帧率提取，或在时域高层（层1）解码序列中的所有帧。*/
	int       bEnableTemporalSubLayers;

	/*==  GOP结构和条带类型决策（lookahead:未来帧） ==*/

	/*启用open GOP-意味着I条带不一定是IDR，因此在I条带之后编码的帧可能会参考该I帧之前的编码帧，这些之前的编码帧仍保留在解码帧缓冲区中。
	* open GOP通常具有更高的压缩效率，对编码性能的影响可以忽略不计，但用例可以排除它。默认为真
	* 注释：
	1.I帧与IDR帧的区别：
	I帧和IDR帧（也称为关键帧）都使用帧内预测。在编码和解码中为了方便，要将首个I帧和其他I帧区别开，所以才把首个I帧叫IDR帧，
	这样就方便控制编码和解码流程。IDR帧的作用是立即刷新，使错误不致传播，从IDR帧开始，重新算一个新的开始编码。而I帧不具有随机访问的能力，
	这个功能由IDR承担。IDR会导致DPB（Decoded Picture Buffer即解码帧缓冲区或翻译为参考帧列表--这是关键所在）清空，
	而I帧不会。IDR帧一定是I帧。一个序列中可以有很多I帧，I帧之后的帧可以引用该I帧之前的帧做运动参考。对于IDR帧来说，
	在IDR帧之后的所有帧都不能引用任何IDR帧之前的帧内容，与此相反，对于普通的I帧来说，位于其之后的B-和P-帧可以引用位于普通I-帧之前的I-帧。
	从随机存取的视频流中，播放器永远可以从一个IDR帧播放，因为在他之后没有任何帧引用之前的帧。但是，不能在一个没有IDR帧的视频中从任意点开始播放，
	因为后面的帧总是会引用前面的帧。IDR帧属于I帧。解码器收到IDR帧时，将所有的参考帧队列丢弃（用x265_reference_reset函数实现--在encoder.c文件中）。
	解码器另外需要做的工作就是：把所有的PPS和SPS参数进行更新。由此可见，在编码器端，每发一个IDR帧，就相应地发一个PPS&PPS_nal_uint.
	2.关于keyint:设置x265输出中最大的IDR帧（亦称关键帧）间距。IDR帧是视频流的“分隔符”，所有帧都不可以使用越过关键帧的帧作为参考帧。IDR帧是I帧的一种，
	所以他们也不参考其他帧。这意味着他们可以作为视频的搜索（seek）点。通过这个设置可以设置IDR帧的最大间隔帧数（亦称最大GOP组长度）。
	较大的值将导致IDR帧减少（会用占用空间更少的P帧和和B帧取代），也就同时减弱了参考帧选择的限制。较小的值导致减少搜索一个随机帧所需的平均时间。
	建议：默认值（fps的10倍，如250）对大多数视频都很好。如果在为蓝光、广播、直播流或者其它什么专业流编码，也许会需要更小的图像组（GOP）长度（一般等于fps）。
	参见min-keyint,scenecut,intra-refresh
	3.关于min-keyint默认：auto(keyint/10)说明：参见keyint的说明。过小的keyint范围会导致产生“错误的”IDR帧（比如说，一个闪屏场景）。
	此选项限制了IDR帧之间的最小距离。建议：默认，或者与fps相等 参见：keyint,scenecut
	关于scenecut设置决策使用I帧、IDR帧的阈值（场景变换检测）。X265会计算每一帧与前一帧的相似度，如果这个值低于scenecut，那么就认为检测到一个“场景变换”。
	如果此时距离上一帧的距离小于min-keyint则插入一个I帧，反之则插入一个IDR帧。较高的值会增加侦测到“场景变换”的几率。设置scenecut=0与no-scenecut等效。
	建议使用默认值40.参考：keyint,min-keyint,no-scenecut*/
	int       bOpenGOP;

	/*这个变量即注释3中提到的min-keyint,它限制了IDR帧之间的最小距离，如果发现当前帧是scenecut，且他与前一IDR帧的距离小于min-keyint(即keyframeMin),
	* 则在此处插入一个I帧，反之则插入一个IDR帧。*/
	int       keyframeMin;

	/*keyframeMax定义了最大关键帧间距或一个周期内的帧数（即注释2中提到的keyint）。如果为0或1，则所有帧都是I帧（所谓的全帧内模式）。
	* 负值在内部被映射为MAX_INT(2147483647,max value of signed 32-bit integer),它使帧0成为唯一的I帧。缺省值为250*/
	int       keyframeMax;

	/*可由lookahead给出的最大连续B帧。当b-adapt为0且keyframeMax大于bframes时，lookahead在每二个P之间给出固定模式的‘bframes’个B帧。
	* b-adapt为1时，大多数情况下lookahead忽略bframes的值。b-adapt为2时，bframes的值决定了二个方向上完成的搜索（POC,picture order count）距离，
	* 增加了lookahead的计算负担。这个值越高。Lookahead能够连续使用的B帧越多，通常可以改进压缩。默认值为3，最大值16*/
	int       bframes;

	/*设置lookahead的运行模式。b-adapt为0时，基于keyframeMax和bframes的值，GOP结构是固定的，b-adapt为1时，使用轻量级的lookahead选择B帧位置。
	* b-adapt为2时，完成viterbi B路径选择。*/
	int       bFrameAdaptive;

	/*启用时，编码器将使用大于2个B帧的每个mini-GOP中间的B帧作为邻近B帧的运动参考。这提高了压缩效率，而性能（指编码速度）损失却很小。
	* 经由速率控制在某个B帧和P帧间的某处处理被参考的B帧。默认启用。*/
	int       bBPyramid;

	/*加入到lookahead中B帧成本估计的某个值。他可以是一个正值（使B帧似乎更昂贵，会使得lookahead选择更多的P帧），或是一个负值，
	* 这使得lookahead选择更多的B帧。默认为0，没有任何限制。*/
	int       bFrameBias;

	/*在做出条带决策前，必须在lookahead中排队的帧数量。加大这个值直接增加了编码延迟。队列越长，lookahead的决策结果就越优，特别是对b-adapt 2情形。
	* 启用cu-tree时，cu-tree分析的有效性与队列的长度成线性关系。缺省为40帧，最大为250.*/
	int       lookaheadDepth;

	/*在lookahead中，使用多个线程来测量每帧的估计成本。
	* 当bFrameAdaptive 等于 2时,大多数帧成本估计将以批处理形式完成，与此同时针对批处理估计，许多成本估计和lookaheadSlices被忽略。
	* 对性能的影响可以相当的小。这个参数越高，帧成本估计越不精确（因为条带边界处的上下文丢失了），这导致B-帧和场景切换决策的不精确。默认为0-禁用。1与0相同。最大16.*/
	int       lookaheadSlices;

	/*确定lookahead如何积极检测场景切换的任意阈值。缺省值推荐为40.该变量即注释中提到的scenecut.*/
	int       scenecutThreshold;

	/*==   编码单元CU定义==*/

	/*采用像素表示的最大CU宽度和高度。大小必须是64,32和16.该尺寸越大，x265对低复杂度区域的编码更有效，大大提高了高分辨率图像的压缩效率。尺寸越小，波前和帧并行将变得更有效，
	* 因为行数增加了。默认为64.同一个进程内所有的编码器必须使用相同的maxCUSize，直到所有编码器都关闭，并调用x265_cleanup()来重置这个值。*/
	uint32_t  maxCUSize;

	/*采用像素表示的最小CU宽度和高度。大小必须是64、32、16或8.同一个进程内的所有编码器必须使用相同的minCUSize。*/
	uint32_t  minCUSize;

	/*启用矩形运动预测划分（垂直和水平），在所有CU深度从64x64到8x8可用。默认禁用*/
	int       bEnableRectInter;

	/*启用非对称运动预测。在CU深度64、32和16处，沿上、下、左、右方向可以使用25%/75%的分割划分。对于某些情形，这将以额外的分析成本换来压缩效率的提高。默认禁用*/
	int       bEnableAMP;

	/*==  残差四叉树变换单元定义 ==*/
	/*采用像素表示的最大TU宽度和高度。大小必须是32、16、8或4.尺寸越大，经由DCT变换对残差的压缩更有效，当然计算代价也更大。*/
	uint32_t  maxTUSize;

	/*针对帧间编码块，超出编码四叉树之外所允许的残差四叉树递归附加深度。该值必须在1和4之间。该值越大，经由DCT变换对残差的压缩更有效，自然计算代价也更大。*/
	uint32_t  tuQTMaxInterDepth;

	/*针对帧内编码块，超出编码四叉树之外所允许的残差四叉树递归附加深度。该值必须在1和4之间。该值越大，经由DCT变换对残差的压缩更有效，自然计算代价也更大。*/
	uint32_t  tuQTMaxIntraDepth;

	/*设置量化中要用的率失真分析的量。Level 0意味着量化中不考虑率失真优化。在Level 1，率失真成本被用来为每一量化级（level）找出最优的四舍五入值（且允许psy-rdoq被启用）。
	* 在level 2，率失真代价被用来对每个4x4编码组做出decimate决策（包括在这个组位图内标识组的成本）。当RDOQ位于level 2时，Psy-rdoq在保护能量方面效率不高。*/
	int       rdoqLevel;

	/*启用隐式标识每个变换单元的最后一个系数的符号位。这样可以每TU节省1比特，付出的代价是要搞清楚哪一个系数可以被修改，且带有最小的失真。默认启用*/
	int       bEnableSignHiding;

	/*允许帧内编码块作为残差直接编码，无需DCT变换，这能提高效率。对块从此选项中是否受益进行检查会导致（速度）性能损失。默认禁用*/
	int       bEnableTransformSkip;

	/*该整数值的范围为0到2000，它代表了帧内CUs中噪声衰减的程度。0意味着禁用。*/
	int       noiseReductionIntra;

	/*该整数值的范围为0到2000，它代表了帧间CUs中噪声衰减的程度。0意味着禁用*/
	int       noiseReductionInter;

	/*量化缩放列表。HEVC支持定义6个量化缩放列表；其中一组用于帧内预测的Y, Cb, Cr，另一组用于帧间预测
	*-NULL和”off”将禁用量化缩放（默认）
	*-”default”将启用HEVC缺省缩放列表，无需标识，因为是被指定的
	*-所有其他字段都代表了一个文件名，该文件包含了采用HM格式的自定义缩放列表。如果文件不能被正确解析，则编码将失败。自定义列表必须在SPS中标识。*/
	char *scalingLists;

	/*==   帧内编码工具 ==*/
	/*启用受约束的帧内预测。这会导致对帧间预测（模式下）的输入样本进行帧内预测。针对某些情形，能够增强对码流错误的鲁棒性，但P和B条带的压缩性能会受影响。默认禁用*/
	int       bEnableConstrainedIntra;

	/*为32x32的块启用强帧内平滑，注意这些块中的参考样本（像素）必须是平坦的。能不能提高压缩效率，取决于你的源视频。默认禁用*/
	int       bEnableStrongIntraSmoothing;

	/*== 帧间编码工具 ==*/
	/*在帧间分析期间所考虑的最大合并候选数目。这个数字（在5和1之间）标识在码流头部，
	* 它决定了标识某个合并所需的比特数，因此应对该数字折中。这个数字越小，性能越高（指计算速度），
	* 但压缩效率越低。默认为3*/
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

	/*== 速率控制 ==*/

	/*速率控制的主要工作是建立编码速率与量化参数的关系模型，根据目标码率确定视频编码参数中的量化参数。
	*无损标志启用真正的无损编码、旁路缩放、变换、量化和环路滤波过程。这用于无损超高比特率。意味着没有速度控制。*/
	int       bLossless;

	/*通常是一个小的符号整数，它使用于量化Cb色度残差的QP产生偏移（亮度QP的delta由速率控制指定）。默认为0，推荐使用 */
	int       cbQpOffset;

	/*通常是一个小的符号整数，它使用于量化Cr色度残差的QP产生偏移（亮度QP的delta由速率控制指定）。默认为0，推荐使用*/
	int       crQpOffset;

	struct
	{
		/*显式模式的速率控制，对API用户而言是必不可少的。它一定是X265_RC_METHODS枚举值中的一个。*/
		int       rateControlMode;

		/*用于恒定量化参数（CQP）码率控制的基本QP。针对每个块，自适应QP方法（参见红皮6.3.2节）可以改变其QP值。如果在命令行中指定了QP，就意味着使用CQP速率控制。 */
		int       qp;

		/*用于AverageBitRate（ABR）码率控制的目标比特率。如果在命令行中指定了非零bitrate，则意味着使用了ABR。默认为0。*/
		int       bitrate;

		/*qComp设置量化曲线压缩因子。它基于残差复杂度（由lookahead度量）对帧量化器加权（参见红皮6.3.2节）。默认值为0.6。将其增加到1将产生有效的CQP。 */
		double    qCompress;

		/*I/P和P/B帧间的QP偏移。默认ipFactor：1.4，默认pbFactor：1.3*/
		double    ipFactor;
		double    pbFactor;

		/*Ratefactor常数：以某个恒定“质量”为目标。可接受值在0和51之间。默认值：28 */
		double    rfConstant;

		/*帧间最大QP差异。默认：4*/
		int       qpStep;

		/*启用自适应量化。这种模式将在一帧的所有CTUs间提供可用的比特，复杂度低的区域分配的比特更多。打开它通常会对PSNR有负面影响，但是SSIM和视觉质量普遍提高。 */
		int       aqMode;

		/*设置AQ偏向于低细节CTUs的强度。只有AQ启用才有效。默认值：1.0。可接受值在0和3之间。 */
		double    aqStrength;

		/*设置VBV可用的最大速率（码率），在默认情况下，VBV（VideoBufferingVerifier视频缓冲检验器）缓冲区应被假定为零填充。*/
		int       vbvMaxBitrate;

		/*以kilobits为单位设置VBV缓冲区的大小。默认是0*/
		int       vbvBufferSize;

		/*设置在开始播放前必须将VBV缓冲区填充到多满。如果它低于1，那么初始填充是vbv-initvbvBufferSize。否则，它被解释为以kbits为单位的初始填充。默认为0.9 */
		double    vbvBufferInit;

		/*启用CUTree速率控制。它对跨帧的时域传播CUs进行跟踪，并为这些CUs分配更多的比特。从而改进了编码效率。默认：启用*/
		int       cuTree;

		/*在CRF模式中，由VBV决定的最大CRF。0意味着没有限制。*/
		double    rfConstantMax;

		/*在CRF模式中，由VBV决定的最小CRF*/
		double    rfConstantMin;

		/*多通编码
		*注释：pass默认值:无
		*此为两通编码的一个重要设置。它控制x265如何处理--stats档案。有三种设定:
		*1：建立一个新的统计资料档案。在第一阶段使用此选项.
		*2：读取统计资料档案。在最终阶段使用此选项.
		*3：读取统计资料档案并更新。
		*统计资料档案包含每个输入帧的信息，可以输入到x265以改进输出。构想是执行第一阶段来产生统计资料档案，然后第二阶段将建立一个最佳化的视频编码。目的是要从更好的速率控制中获益。
		*在某个多通编码中，启用写状态信息到状态输出文件。*/
		int       bStatWrite;
		int       bStatRead;
		/*2通输出/输入状态文件的文件名，如果没有指定，编码器将默认采用x265_2pass.log。*/
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