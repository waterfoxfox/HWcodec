
#include "SDX264Encoder.h"
#include "SDLog.h"
#include "SDMutex.h"


//HP时为了防止DTS出现负数的情况，为PTS加上基值
#define PTS_BASE_NUM	1000	

static void bitstream_save(const unsigned char *buf,int size, const char *bitfilename)
{
	FILE *f;

	f=fopen(bitfilename,"ab+");   

	fwrite(buf, 1, size, f );

	fclose(f);
}


CX264_Encoder::CX264_Encoder()
{
	m_ptCtx = new Ctx;;
	m_bClosed = true;
	m_pClosedCs = CSDMutex::CreateObject();
	m_nKeyFrameIntervalTime = 10;

	m_tExternParams.nLevel = 40;
	m_tExternParams.nProfile = X264_HIGH_PROFILE;
	m_tExternParams.bEnableVbv = 1;
	strcpy(m_tExternParams.acPreset, "slow");

	m_nPtsBaseNum = 0;
	m_nPtsStartTime = 0;
	m_nForceKeyframe = 0;
}

CX264_Encoder::~CX264_Encoder()
{
	delete m_ptCtx;
	m_ptCtx = NULL;
	CSDMutex::RealseObject(m_pClosedCs);
	m_pClosedCs = NULL;

}

bool CX264_Encoder::Open(int nWidth, int nHeight, int nFrameRate, int nBitrate, int nKeyFrameInterval, int nNaluSize, X264ExternParams *pExternParams)
{
	CSDMutex cs(m_pClosedCs);
	if(m_bClosed == false)
	{
		SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "Already opened then call open, should closed first!");
		return false;
	}

	if (pExternParams == NULL)
	{
		SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "pExternParams is invalid NULL!");
		return false;
	}
	
	m_nPtsStartTime = 0;
	
	return mfOpen(nWidth, nHeight, nFrameRate, nBitrate, nKeyFrameInterval, nNaluSize, pExternParams);

}

bool CX264_Encoder::mfOpen(int nWidth, int nHeight, int nFrameRate, int nBitrate, int nKeyFrameInterval, int nNaluSize, X264ExternParams *pExternParams)
{	

	int nKeyInt = nKeyFrameInterval*nFrameRate;
	nKeyInt = nKeyInt > 200 ? 200:nKeyInt;
	nKeyInt = nKeyInt < 4 ? 4:nKeyInt;
	m_nKeyFrameIntervalTime = nKeyFrameInterval;

	///////////////////////////////////////////////start//////////////////////////////////////////	
	x264_param_default(&m_ptCtx->param);

	//外层指定Level
	m_ptCtx->param.i_level_idc = pExternParams->nLevel;

	//外层指定预设
	if (pExternParams->acPreset[0])
	{
		x264_param_default_preset(&m_ptCtx->param, pExternParams->acPreset, "zerolatency");
	}
	else
	{
		//默认预设
		x264_param_default_preset(&m_ptCtx->param, "slow", "zerolatency");
	}

	//外层指定Profile
	if (pExternParams->nProfile == X264_BASELINE_PROFILE)
	{
		//Baseline
		x264_param_apply_profile(&m_ptCtx->param, "baseline");
		m_ptCtx->param.i_frame_reference = 1;
		m_nPtsBaseNum = 0;
	}
	else
	{
		if (pExternParams->nProfile == X264_HIGH_PROFILE)
		{
			//HighProfile
			x264_param_apply_profile(&m_ptCtx->param, "high");
			//开启8X8
			m_ptCtx->param.analyse.b_transform_8x8 = 1;
		}
		else
		{
			//MainProfile
			x264_param_apply_profile(&m_ptCtx->param, "main");
		}

		//开启CABAC
		m_ptCtx->param.b_cabac = 1;
		//实时系统关闭B帧
		m_ptCtx->param.i_bframe = 0;
		//HP使用2个参考帧
		m_ptCtx->param.i_frame_reference = 2;
		if (m_ptCtx->param.i_bframe)
		{
			//仅在存在B帧时PTS!=DTS，PTS需要一定的基值防止出现DTS负值的情况
			m_nPtsBaseNum = PTS_BASE_NUM;
		}
		else
		{
			m_nPtsBaseNum = 0;
		}
	}

	//外层指定是否采用VBV控制码率波动
	if (m_tExternParams.bEnableVbv)
	{
		m_ptCtx->param.rc.f_rate_tolerance   = 0.1f;
		m_ptCtx->param.rc.i_vbv_max_bitrate  = (int)(nBitrate*0.95f);
		m_ptCtx->param.rc.i_vbv_buffer_size  = (int)(nBitrate*0.95f);
		m_ptCtx->param.rc.f_vbv_buffer_init  = 0.5f;
		m_ptCtx->param.rc.f_ip_factor		 = 1.0f;	
	}
	
	//通用设置
	m_ptCtx->param.i_threads				= 1; 
	m_ptCtx->param.b_sliced_threads			= 0;
	m_ptCtx->param.i_width					= nWidth;
	m_ptCtx->param.i_height					= nHeight;
	m_ptCtx->param.i_csp					= X264_CSP_I420;
	m_ptCtx->param.i_fps_num				= nFrameRate;
	m_ptCtx->param.i_fps_den				= 1;
	m_ptCtx->param.i_keyint_min				= 4;
	m_ptCtx->param.i_keyint_max				= nKeyInt;
	m_ptCtx->param.analyse.intra			= X264_ANALYSE_I4x4 | X264_ANALYSE_I8x8 | X264_ANALYSE_PSUB16x16 
											| X264_ANALYSE_BSUB16x16 | X264_ANALYSE_PSUB8x8;
	//多Slice提高网络友好性
	m_ptCtx->param.i_slice_max_size			= nNaluSize;
	m_ptCtx->param.b_repeat_headers			= 1;

	//关闭兼容性不佳的权重预测
	m_ptCtx->param.analyse.i_weighted_pred	= 0;
	//实时性场合关闭lookahead
	m_ptCtx->param.rc.i_lookahead			= 0;
	//关闭一些2pass相关
	m_ptCtx->param.rc.psz_stat_out			= 0;
	m_ptCtx->param.rc.psz_stat_in			= 0;
	m_ptCtx->param.rc.i_bitrate				= nBitrate;
	m_ptCtx->param.rc.i_rc_method           = X264_RC_ABR;
	m_ptCtx->param.rc.i_qp_min              = 15;
	m_ptCtx->param.rc.i_qp_max              = 45;
	m_ptCtx->param.i_scenecut_threshold     = 0;

	//存储外层传入参数
	m_tExternParams.nLevel = pExternParams->nLevel;
	m_tExternParams.nProfile = pExternParams->nProfile;
	m_tExternParams.bEnableVbv = pExternParams->bEnableVbv;
	strcpy(m_tExternParams.acPreset, pExternParams->acPreset);
	///////////////////////////////////////////////end//////////////////////////////////////////

	m_ptCtx->x264 = x264_encoder_open(&m_ptCtx->param);
	if (!m_ptCtx->x264) 
	{
		SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "x264_encoder_open failed!");
		return(false);
	}

	x264_picture_init(&m_ptCtx->picture);
	m_ptCtx->picture.img.i_csp = X264_CSP_I420;
	m_ptCtx->picture.img.i_plane = 3;
	
	m_bClosed = false;
	m_nPrevIdrTime = 0;

	return(true);
}

bool CX264_Encoder::Control(int nWidth, int nHeight, int nFrameRate, int nBitrate, int nKeyFrameInterval,  int nNaluSize)
{
	CSDMutex cs(m_pClosedCs);
	if( m_bClosed )
	{
		SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "Already closed then call Control!");
		return false;
	}

	if (m_ptCtx != NULL)
	{

		if (m_ptCtx->x264 != NULL)
		{
			mfClose();
			return mfOpen(nWidth, nHeight, nFrameRate, nBitrate, nKeyFrameInterval, nNaluSize, &m_tExternParams);	
		}
		else
		{
			SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "m_ptCtx->x264 == NULL!");
			return false;
		}
		
	}
	else
	{
		SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "m_ptCtx == NULL!");
		return false;
	}

}

void CX264_Encoder::Close()
{
	CSDMutex cs(m_pClosedCs);
	mfClose();
}

void CX264_Encoder::mfClose()
{
	m_bClosed = true;

	if(m_ptCtx != NULL)
	{
		if(m_ptCtx->x264 != NULL)
		{
			x264_encoder_close(m_ptCtx->x264);
			m_ptCtx->x264 = NULL;
		}
	}
}

//输入一帧YUV420图像，编码为H264码流，并将NALU信息告知外层
//返回>0 当前帧码流大小; < 0 编码错误; =0 当前无码流输出
int CX264_Encoder::Encode(BYTE *pucInputYuv , BYTE *pucOutputStream, int *pnNaluLen, UINT unMaxNaluCnt, bool bForceIdr, int64_t *pPts, int64_t *pDts)
{
	int ret = -1;
	CSDMutex cs(m_pClosedCs);
	if( m_bClosed )
	{
		return(0);
	}
	
	if((pucInputYuv == NULL) || (pucOutputStream == NULL))
	{
		SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, " (data == %x) || (pRetData == %x)", pucInputYuv, pucOutputStream);
		return(-1);
	}


	unsigned char *pucImg[4];
	pucImg[0] = pucInputYuv;
	pucImg[1] = pucInputYuv + m_ptCtx->param.i_width * m_ptCtx->param.i_height;
	pucImg[2] = pucInputYuv + (m_ptCtx->param.i_width * m_ptCtx->param.i_height * 5) / 4;
	pucImg[3] = NULL;

	int stride[4];	
	stride[0] = m_ptCtx->param.i_width;
	stride[1] = m_ptCtx->param.i_width / 2;
	stride[2] = m_ptCtx->param.i_width / 2;
	stride[3] = 0;

#if 0
	pgm_save(pucImg[0], stride[0],
		m_ptCtx->param.i_width, m_ptCtx->param.i_height,  "D:\\send.yuv");

	pgm_save(pucImg[1], stride[1],
		m_ptCtx->param.i_width/2, m_ptCtx->param.i_height/2,  "D:\\send.yuv");

	pgm_save(pucImg[2], stride[2],
		m_ptCtx->param.i_width/2, m_ptCtx->param.i_height/2,  "D:\\send.yuv");
#endif	
	
	//初始化NALU长度数组
	memset(pnNaluLen, 0x0, sizeof(int)*unMaxNaluCnt);
	
	ret = mfCompress(pucImg, stride, pucOutputStream, pnNaluLen, unMaxNaluCnt, bForceIdr, pPts, pDts);

	return ret;
}


int CX264_Encoder::mfDumpnals (x264_nal_t *nal, int nals, unsigned char *pucOutputStream, int *pnNaluLen)
{
	int i_total_size = 0;
#if 0	
	for (int i = 0; i < nals; i++) 
	{
		bitstream_save(nal[i].p_payload, nal[i].i_payload, "D://stst.h264");
	}
#endif

	//输出带NALU起始码
	for (int i = 0; i < nals; i++) 
	{
		memcpy(pucOutputStream, nal[i].p_payload, nal[i].i_payload);
		pucOutputStream += nal[i].i_payload;
		i_total_size += nal[i].i_payload; 
		pnNaluLen[i] = nal[i].i_payload;
	}

	return(i_total_size);
}

int CX264_Encoder::mfCompress (unsigned char *data[4], int stride[4], unsigned char *pRetData, int *pnRetLen, UINT unMaxBlkCnt, bool bForceIdr, int64_t *pPts, int64_t *pDts)
{
	Ctx *c = m_ptCtx;

	// 设置 picture 数据
	for (int i = 0; i < 4; i++) 
	{
		c->picture.img.plane[i] = data[i];
		c->picture.img.i_stride[i] = stride[i];
	}

	// encode
	x264_nal_t *nals;
	int nal_cnt;
	x264_picture_t pic_out;
	bool bForceIdrFrame = false;

	c->picture.i_pts = mfGeneratePts();

	x264_picture_t *pic = &c->picture;
	
	//上一次编码IDR帧的时间，用于避免由于实际输入帧频不足配置的帧率，
	//而导致长时间（大于指定的最大I帧间隔）无IDR帧的情况
	long nCurrTime = SD_GetTickCount();
	if ((nCurrTime - m_nPrevIdrTime >= m_nKeyFrameIntervalTime*1000) || (bForceIdr == true))
	{
		bForceIdr = false;
		bForceIdrFrame = true;	
		m_nPrevIdrTime = SD_GetTickCount();
	}


	if (bForceIdrFrame) 
	{
		c->picture.i_type = X264_TYPE_IDR;
	}
	else 
	{
		c->picture.i_type = X264_TYPE_AUTO;
	}


	int rc = x264_encoder_encode(c->x264, &nals, &nal_cnt, pic, &pic_out);
	if (rc < 0) 
	{
		SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "x264_encoder_encode failed return:%d", rc);
		return -1;
	}
	
	//single slice mode
	if (nal_cnt > 0) 
	{
		int bitstream_size = 0;

		if (nal_cnt > (int)unMaxBlkCnt)
		{
			SDLOG_PRINTF("CX264_Encoder", SD_LOG_LEVEL_ERROR, "More than unMaxBlkCnt:%d num of NALUs:%d \
				generated by encoder!", unMaxBlkCnt, nal_cnt);
			nal_cnt = (int)unMaxBlkCnt;
		}
		
		bitstream_size = mfDumpnals(nals, nal_cnt, pRetData, pnRetLen);

		if (pPts && pDts)
		{
			*pPts = pic_out.i_pts;
			*pDts = pic_out.i_dts;
		}

		return(bitstream_size);
	}
	else 
	{
		return 0; 
	}
}

int64_t CX264_Encoder::mfGeneratePts()
{
	int64_t nNow = (int64_t)SD_GetTickCount();

	if (m_nPtsStartTime == 0)
	{
		m_nPtsStartTime = nNow;
	}

	//1KHZ时基
	return (nNow - m_nPtsStartTime)*1 + m_nPtsBaseNum;
}



