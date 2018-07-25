//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDBmpZoom
//* 内容摘要：FFMPEG进行缩放和色度空间转换
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：
//***************************************************************************/
#include "SDCommon.h"
#include "SDLog.h"

#include "SDBmpZoom.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSDBmpZoom::CSDBmpZoom()
{
	m_ptImgConvertCtx = NULL;
	m_ptAVFrameSrc = NULL;
	m_ptAVFrameDes = NULL;
	m_bCreated = false;
}

CSDBmpZoom::~CSDBmpZoom()
{

}


bool CSDBmpZoom::CreateBmpZoom(int nSrcWidth, int nSrcHeight, int nDesWidth, int nDesHeight,
							AVPixelFormat eSrcPixFmt, AVPixelFormat eDstPixFmt, bool bHighQuality)
{
	m_ptAVFrameSrc = av_frame_alloc();
	m_eSrcPixFmt = eSrcPixFmt;
	if (m_ptAVFrameSrc == NULL)
	{
		SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "m_ptAVFrameSrc av_frame_alloc failed!");
		return false;
	}

	m_ptAVFrameDes = av_frame_alloc();
	m_eDstPixFmt = eDstPixFmt;
	if (m_ptAVFrameDes == NULL)
	{
		SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "m_ptAVFrameDes av_frame_alloc failed!");
		av_frame_free(&m_ptAVFrameSrc);
		m_ptAVFrameSrc = NULL;
		return false;
	}

	if (bHighQuality == false)
	{
		m_ptImgConvertCtx = sws_getContext(nSrcWidth, nSrcHeight, eSrcPixFmt, nDesWidth,
			nDesHeight, eDstPixFmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	}
	else
	{
		m_ptImgConvertCtx = sws_getContext(nSrcWidth, nSrcHeight, eSrcPixFmt, nDesWidth,
			nDesHeight, eDstPixFmt, SWS_LANCZOS, NULL, NULL, NULL);
	}

	if(m_ptImgConvertCtx == NULL)
	{
		SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "img_convert_ctx create failed!");
		av_frame_free(&m_ptAVFrameDes);
		m_ptAVFrameDes = NULL;
		av_frame_free(&m_ptAVFrameSrc);
		m_ptAVFrameSrc = NULL;
		return false;
	}
	m_bCreated = true;
	return true;
}


void CSDBmpZoom::CloseBmpZoom()
{
	m_bCreated = false;

	if (m_ptImgConvertCtx)
	{
		sws_freeContext(m_ptImgConvertCtx);
		m_ptImgConvertCtx = NULL;
	}

	if (m_ptAVFrameSrc)
	{
		av_frame_free(&m_ptAVFrameSrc);
		m_ptAVFrameSrc = NULL;
	}

	if (m_ptAVFrameDes)
	{
		av_frame_free(&m_ptAVFrameDes);
		m_ptAVFrameDes = NULL;
	}
}


bool CSDBmpZoom::BmpZoom(int nSrcWidth, int nSrcHeight, BYTE *pSrcImgbuf, int nDesWidth, int nDesHeight, BYTE *pDesImgbuf)
{
	if (m_bCreated == false)
	{
		return false;
	}

	if(nSrcWidth == nDesWidth && nSrcHeight == nDesHeight && m_eSrcPixFmt == AV_PIX_FMT_RGB24 && m_eDstPixFmt == AV_PIX_FMT_RGB24)
	{
		memcpy(pDesImgbuf, pSrcImgbuf, nSrcWidth*nSrcHeight*3);
		return true;
	}

	if(nSrcWidth == nDesWidth && nSrcHeight == nDesHeight && m_eSrcPixFmt == AV_PIX_FMT_BGR24 && m_eDstPixFmt == AV_PIX_FMT_BGR24)
	{
		memcpy(pDesImgbuf, pSrcImgbuf, nSrcWidth*nSrcHeight*3);
		return true;
	}
 
	if(nSrcWidth == nDesWidth && nSrcHeight == nDesHeight && m_eSrcPixFmt == AV_PIX_FMT_YUV420P && m_eDstPixFmt == AV_PIX_FMT_YUV420P)
	{
		memcpy(pDesImgbuf, pSrcImgbuf, nSrcWidth*nSrcHeight*3/2);
		return true;
	}

	avpicture_fill((AVPicture*)m_ptAVFrameSrc, pSrcImgbuf, m_eSrcPixFmt, nSrcWidth, nSrcHeight);
	avpicture_fill((AVPicture*)m_ptAVFrameDes, pDesImgbuf, m_eDstPixFmt, nDesWidth, nDesHeight);

	int rDesHeight = sws_scale(m_ptImgConvertCtx, (const uint8_t * const*)m_ptAVFrameSrc->data, m_ptAVFrameSrc->linesize, 
								0, nSrcHeight, m_ptAVFrameDes->data, m_ptAVFrameDes->linesize);
	if(rDesHeight <= 0 )
	{
		SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "sws_scale failed!");
		return false;
	}
	return true;
}



bool CSDBmpZoom::BmpZoom(AVFrame *pAVFrameSrc, int nDesWidth, int nDesHeight, BYTE* pDesImgbuf)
{
	if (pAVFrameSrc == NULL)
	{
		SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "pAVFrameSrc is null!");
		return false;
	}

	if (m_ptAVFrameDes == NULL || m_ptImgConvertCtx == NULL || m_bCreated == false)
	{
		SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "m_ptAVFrameDes:%d m_ptImgConvertCtx:%d m_bCreated:%d is invalid!", 
					m_ptAVFrameDes, m_ptImgConvertCtx, m_bCreated);
		return false;
	}

	//avpicture_fill((AVPicture*)m_ptAVFrameDes, pDesImgbuf, m_eDstPixFmt, nDesWidth, nDesHeight);

	int nRet = av_image_fill_arrays(m_ptAVFrameDes->data, m_ptAVFrameDes->linesize, (BYTE*)pDesImgbuf, 
									m_eDstPixFmt, nDesWidth, nDesHeight, 1);
	if (nRet > 0)
	{
		m_ptAVFrameDes->width = nDesWidth;
		m_ptAVFrameDes->height = nDesHeight;
		m_ptAVFrameDes->format = m_eDstPixFmt;
	
		int rDesHeight = sws_scale(m_ptImgConvertCtx, (const uint8_t * const*)pAVFrameSrc->data, pAVFrameSrc->linesize, 0, 
									pAVFrameSrc->height, m_ptAVFrameDes->data, m_ptAVFrameDes->linesize);
		if(rDesHeight <= 0 )
		{
			SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "sws_scale failed:%d!", rDesHeight);
			return false;
		}
	}
	else
	{
		SDLOG_PRINTF("CSDImgProcess", SD_LOG_LEVEL_ERROR, "sws_scale av_image_fill_arrays failed:%d!", nRet);
		return false;
	}
	return true;
}