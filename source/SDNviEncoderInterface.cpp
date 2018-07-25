//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDNviEncoderInterface
//* 内容摘要：Nvidia nvenc硬编码封装
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-4-12
//***************************************************************************/
#include "SDNviEncoderInterface.h"

//Nvi编码封装类
CSDNviEnc::CSDNviEnc()
{
	m_pvEnc = CSDMutex::CreateObject();
	m_bEncClose = TRUE;
	m_pNvi264Enc = new CNvi264_Encoder;
	m_ptAVFrameEncode = NULL;
	m_tEncFormat = AV_PIX_FMT_NONE;
}

CSDNviEnc::~CSDNviEnc()
{
	if (m_bEncClose == FALSE)
	{
		mfCloseEncoder();
	}

	CSDMutex::RealseObject(m_pvEnc);
	delete m_pNvi264Enc;
}

BOOL CSDNviEnc::CreateEncoder(h264_input_params* pInputParams, h264_enc_params* pEncParams)
{
	CSDMutex cs(m_pvEnc);
	if (m_bEncClose == FALSE)
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_WARNING, "Encode Open failed! Please close first!");
		mfCloseEncoder();
	}
	
	if ((pInputParams == NULL) || (pEncParams == NULL))
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "Encode Open failed! Point is NULL %p %p!", pInputParams, pEncParams);
		return FALSE;
	}

	AVPixelFormat eInputFormat = GetInputFormat(pInputParams->eInputFormat);
	if (eInputFormat == AV_PIX_FMT_NONE)
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "Encode Open failed! Input format:%d invalid!", pInputParams->eInputFormat);
		return FALSE;
	}

	m_unInputWidth = pInputParams->unInputWidth;
	m_unInputHeight = pInputParams->unInputHeight;
	m_unEncWidth = pEncParams->unEncWidth;
	m_unEncHeight = pEncParams->unEncHeight;

	//打开编码器
	Nvi264ExternParams tExternParamsHW;
	tExternParamsHW.nLevel = pEncParams->unLevel;
	tExternParamsHW.nProfile = pEncParams->unProfile; 
	tExternParamsHW.bEnableVbv = pEncParams->bEnableVbv;
	mfGetPresetString(pEncParams->ePreset, tExternParamsHW.acPreset);

	bool bRet = m_pNvi264Enc->Open(pEncParams->unEncWidth, pEncParams->unEncHeight, pEncParams->unFramerate, 
									pEncParams->unBitrateKbps, pEncParams->unGopSeconds, &tExternParamsHW);
	if (bRet == false)
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "Encode Open failed with params:%dx%d  %dfps  %dkbps  gop:%d!",
					pEncParams->unEncWidth, pEncParams->unEncHeight, pEncParams->unFramerate, 
					pEncParams->unBitrateKbps, pEncParams->unGopSeconds);
		return FALSE;
	}

	//由硬件编码器获知其要求的输入格式
	int nFormat = AV_PIX_FMT_NONE; 
	int nWidth = 0; 
	int nHeight = 0;

	m_pNvi264Enc->GetInputFormat(&nFormat, &nWidth, &nHeight);
	int nInputSize = av_image_get_buffer_size((AVPixelFormat)nFormat, nWidth, nHeight, 16);
	if ((nInputSize < 0) || (nFormat == AV_PIX_FMT_NONE))
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "av_image_get_buffer_size failed:%d! format:%d", nInputSize, nFormat);
		m_pNvi264Enc->Close();
		return FALSE;
	}
	m_tEncFormat = (AVPixelFormat)nFormat;

	//分配存放待编码图像的区域
	m_ptAVFrameEncode = av_frame_alloc();
	if (m_ptAVFrameEncode == NULL)
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR ,"QsvH264 av_frame_alloc failed!");
		m_pNvi264Enc->Close();
		return FALSE;
	}
	
	m_pucBuffEncode = (BYTE*)malloc(nInputSize + 1024);
	if (m_pucBuffEncode == NULL)
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR ,"QsvH264 malloc failed with size:%d!", nInputSize);
		av_frame_free(&m_ptAVFrameEncode);
		m_ptAVFrameEncode = NULL;
		m_pNvi264Enc->Close();
		return FALSE;
	}

	//创建SWS进行色度空间转换（若有需要附带完成缩放处理）
	bRet = m_BmpZoom.CreateBmpZoom(pInputParams->unInputWidth, pInputParams->unInputHeight, pEncParams->unEncWidth, pEncParams->unEncHeight, 
								  eInputFormat, m_tEncFormat, true);
	if (bRet == false)
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "BmpZoom create failed with input:%dx%d format:%d output:%dx%d format:%d!", 
					pInputParams->unInputWidth, pInputParams->unInputHeight, eInputFormat, pEncParams->unEncWidth, pEncParams->unEncHeight, m_tEncFormat);
		av_frame_free(&m_ptAVFrameEncode);
		m_ptAVFrameEncode = NULL;
		free(m_pucBuffEncode);
		m_pucBuffEncode = NULL;
		m_pNvi264Enc->Close();
		return FALSE;
	}

	m_bEncClose = FALSE;
	return TRUE;
}

void CSDNviEnc::CloseEncoder()
{
	CSDMutex cs(m_pvEnc);
	mfCloseEncoder();
}

void CSDNviEnc::mfCloseEncoder()
{
	m_bEncClose = TRUE;
	m_pNvi264Enc->Close();
	m_BmpZoom.CloseBmpZoom();
	if (m_ptAVFrameEncode)
	{
		av_frame_free(&m_ptAVFrameEncode);
		m_ptAVFrameEncode = NULL;
	}

	if (m_pucBuffEncode)
	{
		free(m_pucBuffEncode);
		m_pucBuffEncode = NULL;
	}

}

int CSDNviEnc::Encode(const char* pImageData, char* pBitstreamData, int nMaxSupportLen)
{
	int nRet = 0;
	CSDMutex cs(m_pvEnc);
	if ((m_bEncClose == TRUE))
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "Encode fail, encoder is closed!");
		return nRet;
	}

	//色度空间转换
	if (!m_BmpZoom.BmpZoom(m_unInputWidth, m_unInputHeight, (BYTE*)pImageData, 
						  m_unEncWidth, m_unEncHeight, (BYTE*)m_pucBuffEncode))
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "BmpZoom process failed!");
		return nRet;
	}

	nRet = av_image_fill_arrays(m_ptAVFrameEncode->data, m_ptAVFrameEncode->linesize, (BYTE*)m_pucBuffEncode, 
								m_tEncFormat, m_unEncWidth, m_unEncHeight, 1);
	if (nRet > 0)
	{
		m_ptAVFrameEncode->width = m_unEncWidth;
		m_ptAVFrameEncode->height = m_unEncHeight;
		m_ptAVFrameEncode->format = m_tEncFormat;
		int64_t nPts, nDts;

		nRet = m_pNvi264Enc->Encode(m_ptAVFrameEncode, (BYTE*)pBitstreamData, nMaxSupportLen, &nPts, &nDts);

	}
	else
	{
		SDLOG_PRINTF("CSDNviEnc", SD_LOG_LEVEL_ERROR, "av_image_fill_arrays failed:%d!!!!", nRet);
	}

	return nRet;
}

void CSDNviEnc::mfGetPresetString(video_enc_preset ePreset, char* strPreset)
{
	switch(ePreset)
	{
	case VIDEO_ENC_VERY_FAST:
		sprintf(strPreset, "fast");
		break;
	case VIDEO_ENC_FASTER:
		sprintf(strPreset, "llhp");
		break;
	case VIDEO_ENC_FAST:
		sprintf(strPreset, "llhq");
		break;
	case VIDEO_ENC_MEDIUM:
		sprintf(strPreset, "ll");
		break;
	case VIDEO_ENC_SLOW:
		sprintf(strPreset, "hq");
		break;
	case VIDEO_ENC_SLOWER:
		sprintf(strPreset, "hq");
		break;
	case VIDEO_ENC_VERY_SLOW:
		sprintf(strPreset, "hq");
		break;
	default:
		SDLOG_PRINTF("Enc", SD_LOG_LEVEL_WARNING, "Input enc preset:%d is not support!", ePreset);
		sprintf(strPreset, "ll");
		break;
	}
}