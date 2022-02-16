//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDX264Decoder
//* 内容摘要：使用ffmpeg接口的软H264解码封装，由外层负责接口锁保护
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：
//***************************************************************************/
#include "SDCommon.h"
#include "SDLog.h"
#include "SDMutex.h"
#include "SDX264Decoder.h"


CX264_Decoder::CX264_Decoder()
{
    m_codec = NULL;
    m_ctx = NULL;
    m_picture = NULL;
	m_h264Parser = NULL; 
	m_eOutputType = DEC_OUTPUT_YUV420;
	//用于使用对齐方式存放输入的NALU码流数据
	m_piInputBuff = (int*)malloc(1024*sizeof(int)*512); 
}

CX264_Decoder::~CX264_Decoder()
{
    m_codec = NULL;
    m_ctx = NULL;
    m_picture = NULL;
	m_h264Parser = NULL; 
	if (m_piInputBuff != NULL)
	{
		free(m_piInputBuff);
	}
}


bool CX264_Decoder::Open(int nWidth, int nHeight, DecodeOutputImageType eOutputType)
{
    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "avcodec_find_decoder find h264 failed!!");
        return false;
    }

    m_ctx = avcodec_alloc_context3(m_codec);
    m_picture=  av_frame_alloc();

	m_ctx->width = nWidth;
	m_ctx->height = nHeight;
	m_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
 
	m_eOutputType = eOutputType;
    if(m_codec->capabilities&CODEC_CAP_TRUNCATED)
        m_ctx->flags|= CODEC_FLAG_TRUNCATED;

    if (avcodec_open2(m_ctx, m_codec, NULL) < 0)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "avcodec_open2 failed!!");
		m_codec = NULL;
		avcodec_close(m_ctx);
		av_free(m_ctx);
		m_ctx = NULL;
		av_frame_free(&m_picture);
		m_picture = NULL;
        return false;
    }

	m_h264Parser = av_parser_init(AV_CODEC_ID_H264);
	if(m_h264Parser == NULL)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "av_parser_init failed!!");
		m_codec = NULL;
		avcodec_close(m_ctx);
		av_free(m_ctx);
		m_ctx = NULL;
		av_frame_free(&m_picture);
		m_picture = NULL;
        return false;
	}
	return true;
}


void CX264_Decoder::Close()
{
	if(m_ctx != NULL)
	{
		avcodec_close(m_ctx);
		av_free(m_ctx);
		m_ctx = NULL;
	}
	if(m_picture != NULL)
	{
		av_frame_free(&m_picture);
		m_picture = NULL;
	}
	m_codec = NULL;
	if (m_h264Parser != NULL)
	{
		av_parser_close(m_h264Parser);
		m_h264Parser = NULL;
	}
}


int CX264_Decoder::Decode(void *data, UINT size, void *pRetBuf, int* pnWidth, int* pnHeight)
{
#ifndef ANDROID
	try
	{
#endif
		// CSDMutex cs(m_cs);
		return x264_dec((BYTE*)data, (int)size, (BYTE *)pRetBuf, pnWidth, pnHeight);
#ifndef ANDROID
	}
	catch (...)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_FATAL, "decode Exception");
		return -1;
	}
#endif
}


BOOL YUVtoBGR24(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf)
{
	struct SwsContext *img_convert_ctx = NULL;
	AVFrame *pAVFrameRGB = av_frame_alloc();
	if (pAVFrameRGB == NULL)
	{
		return FALSE;
	}
	
	avpicture_fill((AVPicture*)pAVFrameRGB, byOutbuf, AV_PIX_FMT_BGR24, width, height);

	img_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_BGR24,
		SWS_BICUBIC, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, pAVFrame->data, pAVFrame->linesize,0, height, pAVFrameRGB->data, pAVFrameRGB->linesize);

	av_frame_free(&pAVFrameRGB);
	sws_freeContext(img_convert_ctx);
	
	return TRUE;
	
}

BOOL YUVtoRGB24(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf)
{
	struct SwsContext *img_convert_ctx = NULL;
	AVFrame *pAVFrameRGB = av_frame_alloc();
	if (pAVFrameRGB == NULL)
	{
		return FALSE;
	}
	
	avpicture_fill((AVPicture*)pAVFrameRGB, byOutbuf, AV_PIX_FMT_RGB24, width, height);
	
	img_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB24,
		SWS_BICUBIC, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, pAVFrame->data, pAVFrame->linesize,0, height, pAVFrameRGB->data, pAVFrameRGB->linesize);
	
	av_frame_free(&pAVFrameRGB);
	sws_freeContext(img_convert_ctx);
	
	return TRUE;
	
}
BOOL YUVtoRGBA32(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf,bool bIsRGBOut)
{
	struct SwsContext *img_convert_ctx = NULL;
	AVFrame *pAVFrameRGBA = av_frame_alloc();
	if (pAVFrameRGBA == NULL)
	{
		return FALSE;
	}
	if (bIsRGBOut)
	{
		avpicture_fill((AVPicture*)pAVFrameRGBA, byOutbuf, AV_PIX_FMT_RGBA, width, height);
		
		img_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGBA,
										SWS_BICUBIC, NULL, NULL, NULL);
	}
	else
	{
		avpicture_fill((AVPicture*)pAVFrameRGBA, byOutbuf, AV_PIX_FMT_BGRA, width, height);
		
		img_convert_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_BGRA,
										SWS_BICUBIC, NULL, NULL, NULL);
	}
	
	sws_scale(img_convert_ctx, pAVFrame->data, pAVFrame->linesize,0, height, pAVFrameRGBA->data, pAVFrameRGBA->linesize);
	av_frame_free(&pAVFrameRGBA);
	sws_freeContext(img_convert_ctx);
	return TRUE;
	
}

static BOOL YUVToYUV420(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf)
{
	struct SwsContext *img_convert_ctx = NULL;
	AVFrame *pAVFrameYUV = av_frame_alloc();
	if (pAVFrameYUV == NULL)
	{
		return FALSE;
	}

	avpicture_fill((AVPicture*)pAVFrameYUV, byOutbuf, AV_PIX_FMT_YUV420P, width, height);

	img_convert_ctx = sws_getContext(width, height, (AVPixelFormat)pAVFrame->format, width, height, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, pAVFrame->data, pAVFrame->linesize,0, height, pAVFrameYUV->data, pAVFrameYUV->linesize);

	av_frame_free(&pAVFrameYUV);
	sws_freeContext(img_convert_ctx);

	return TRUE;

}

int CX264_Decoder::x264_dec(const BYTE* in_buf_IN, int in_buf_size, BYTE *out_buf, int* pnWidth, int* pnHeight)
{
	static BYTE byHeadLong[] = {0x00, 0x00, 0x00, 0x01};
	static BYTE byHeadShort[] = {0x00, 0x00, 0x01};
	
	int got_picture = 0;
	AVPacket avpkt;
	BYTE* in_buf = NULL;
	
	//拷贝到字节对齐的内存区，尾段初始化0
	memset(m_piInputBuff, 0x0, sizeof(int)*1024*512);
	if (in_buf_size > (int)(sizeof(int)*1024*512))
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Invalid NALU with too large size:%d!", in_buf_size);
		return(0);		
	}
	memcpy((BYTE *)m_piInputBuff, in_buf_IN, in_buf_size);
	in_buf = (BYTE *)m_piInputBuff;


    memset(&avpkt, 0, sizeof(AVPacket));
	av_init_packet(&avpkt);
	
	//返回值， 0无数据输出 正数表示解码图像帧大小
	int iRet = 0;
	//bitstream_save(in_buf, in_buf_size, "D:\\recv.264");
	
	//修改为NALU格式后，新的码流头校验
	if (in_buf_size <= 5)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Invalid NALU with too short Head!!!");
		return(0);		
	}

	if( (memcmp(byHeadLong, in_buf, sizeof(byHeadLong)) != 0) && (memcmp(byHeadShort, in_buf, sizeof(byHeadShort)) != 0))
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Invalid NALU Head!!!");
		return(0);
	}
	
	while(in_buf_size > 0)
	{

		int len = av_parser_parse2(m_h264Parser, m_ctx, &avpkt.data, &avpkt.size, in_buf, in_buf_size, 0, 0, 0);

		in_buf += len;
		in_buf_size -= len;
		if(avpkt.size > 0)
		{
			avcodec_decode_video2(m_ctx, m_picture, &got_picture, &avpkt);
			if(got_picture > 0)
			{

 				if (m_picture->width*m_picture->height != m_ctx->width*m_ctx->height)
 				{
 					continue;
 				}

				if (m_eOutputType == DEC_OUTPUT_BGRA)
				{
					YUVtoRGBA32(m_picture, m_ctx->width, m_ctx->height, out_buf, false);
					iRet = (m_ctx->width * m_ctx->height * 4);
				}
				else if (m_eOutputType == DEC_OUTPUT_RGBA)
				{
					YUVtoRGBA32(m_picture, m_ctx->width, m_ctx->height, out_buf, true);
					iRet = (m_ctx->width * m_ctx->height * 4);
				}
				else if (m_eOutputType == DEC_OUTPUT_BGR)
				{
					YUVtoBGR24(m_picture, m_ctx->width, m_ctx->height, out_buf);	
					iRet = (m_ctx->width * m_ctx->height * 3);
				}
				else if (m_eOutputType == DEC_OUTPUT_RGB)
				{
					YUVtoRGB24(m_picture, m_ctx->width, m_ctx->height, out_buf);
					iRet = (m_ctx->width * m_ctx->height * 3);
				}
				else if (m_eOutputType == DEC_OUTPUT_YUV420)
				{
					YUVToYUV420(m_picture, m_ctx->width, m_ctx->height, out_buf);
					iRet = (m_ctx->width * m_ctx->height * 3 / 2);

				}
				
			}
		}
	}

	//在解码出图像时，告之外界图像的宽高
	if ((pnWidth != NULL) && (pnHeight != NULL))
	{
		*pnHeight = m_ctx->height;
		*pnWidth = m_ctx->width;
	}

	return(iRet);
}



