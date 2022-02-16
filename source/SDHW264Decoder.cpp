//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDHW264Decoder
//* 内容摘要：使用DXVA2接口的硬解码封装，由外层负责接口锁保护
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-7-19
//***************************************************************************/
#include "SDCommon.h"
#include "SDLog.h"
#include "SDHW264Decoder.h"

static enum AVPixelFormat g_hw_pix_fmt;

static void print_ffmpeg_error(int errNo)
{
	char errbuf[256];
	memset(errbuf, 0x0, sizeof(errbuf));

	if (av_strerror(errNo, errbuf, sizeof(errbuf)) == 0)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "FFmpeg error:%d info:%s", errNo, errbuf);
	}
	else
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "FFmpeg error:%d info unknown", errNo);
	}
}

static enum AVPixelFormat find_fmt_by_hw_type(const enum AVHWDeviceType type)
{
	enum AVPixelFormat fmt;

	switch (type) {
	case AV_HWDEVICE_TYPE_VAAPI:
		fmt = AV_PIX_FMT_VAAPI;
		break;
	case AV_HWDEVICE_TYPE_DXVA2:
		fmt = AV_PIX_FMT_DXVA2_VLD;
		break;
	case AV_HWDEVICE_TYPE_D3D11VA:
		fmt = AV_PIX_FMT_D3D11;
		break;
	case AV_HWDEVICE_TYPE_VDPAU:
		fmt = AV_PIX_FMT_VDPAU;
		break;
	case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
		fmt = AV_PIX_FMT_VIDEOTOOLBOX;
		break;
	default:
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "find_fmt_by_hw_type with invalid type:%d", type);
		fmt = AV_PIX_FMT_NONE;
		break;
	}

	return fmt;
}

int CH264_HW_Decoder::mf_hw_decode_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
	int err = 0;

	if ((err = av_hwdevice_ctx_create(&m_hwDeviceCtx, type, NULL, NULL, 0)) < 0) 
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Failed to create specified HW device(%d)", type);
		print_ffmpeg_error(err);
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(m_hwDeviceCtx);

	return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
	const enum AVPixelFormat *p;

	for (p = pix_fmts; *p != -1; p++) 
	{
		if (*p == g_hw_pix_fmt)
			return *p;
	}

	SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Failed to get HW surface format.");
	return AV_PIX_FMT_NONE;
}

CH264_HW_Decoder::CH264_HW_Decoder()
{
    m_codec = NULL;
    m_ctx = NULL;
    m_picture = NULL;
	m_h264Parser = NULL; 
	m_eOutputType = DEC_OUTPUT_YUV420;
	m_hwDeviceCtx = NULL;
	m_hwPixFmt = AV_PIX_FMT_NONE;

	//用于使用对齐方式存放输入的码流数据
	m_pnInputBuff = (int*)malloc(1024*sizeof(int)*512); 
}

CH264_HW_Decoder::~CH264_HW_Decoder()
{
    m_codec = NULL;
    m_ctx = NULL;
    m_picture = NULL;
	m_h264Parser = NULL; 
	if (m_pnInputBuff != NULL)
	{
		free(m_pnInputBuff);
	}
}

//判断当前环境是否支持硬解码
bool CH264_HW_Decoder::IsHwAcclSupported(void)
{
	enum AVHWDeviceType type = av_hwdevice_find_type_by_name("dxva2");
	enum AVPixelFormat hwPixFmt = find_fmt_by_hw_type(type);
	if (hwPixFmt == AV_PIX_FMT_NONE) 
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_INFO, "hw decode not support, find_fmt_by_hw_type failed!!");
		return false;
	}
	g_hw_pix_fmt = hwPixFmt;

	AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_INFO, "hw decode not support, avcodec_find_decoder find h264 failed!!");
		return false;
	}

	AVBufferRef* hwDeviceCtx = NULL;
	AVCodecContext* ctx = avcodec_alloc_context3(codec);

	ctx->width = 1920;
	ctx->height = 1080;
	ctx->pix_fmt = AV_PIX_FMT_YUV420P;

	if(codec->capabilities&CODEC_CAP_TRUNCATED)
		ctx->flags|= CODEC_FLAG_TRUNCATED;

	ctx->get_format  = get_hw_format;
	av_opt_set_int(ctx, "refcounted_frames", 1, 0);


	int err = av_hwdevice_ctx_create(&hwDeviceCtx, type, NULL, NULL, 0);
	if (err < 0)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_INFO, "hw decode not support, Failed to create specified HW device(%d)", type);
		print_ffmpeg_error(err);

		avcodec_close(ctx);
		av_free(ctx);

		if (hwDeviceCtx)
		{
			av_buffer_unref(&hwDeviceCtx);
			hwDeviceCtx = NULL;
		}
		return false;
	}
	ctx->hw_device_ctx = av_buffer_ref(hwDeviceCtx);


	err = avcodec_open2(ctx, codec, NULL);
	if (err < 0)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_INFO, "hw decode not support, avcodec_open2 failed!!");
		print_ffmpeg_error(err);

		avcodec_close(ctx);
		av_free(ctx);

		if (hwDeviceCtx)
		{
			av_buffer_unref(&hwDeviceCtx);
			hwDeviceCtx = NULL;
		}
		return false;
	}

	avcodec_close(ctx);
	av_free(ctx);

	if (hwDeviceCtx)
	{
		av_buffer_unref(&hwDeviceCtx);
		hwDeviceCtx = NULL;
	}
	return true;
}


bool CH264_HW_Decoder::Open(int nWidth, int nHeight, DecodeOutputImageType eOutputType)
{
	//Windows平台使用DXVA2接口
	enum AVHWDeviceType type = av_hwdevice_find_type_by_name("dxva2");
	m_hwPixFmt = find_fmt_by_hw_type(type);
	if (m_hwPixFmt == AV_PIX_FMT_NONE) 
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "find_fmt_by_hw_type failed!!");
		return false;
	}
	g_hw_pix_fmt = m_hwPixFmt;

    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "avcodec_find_decoder find h264 failed!!");
        return false;
    }

    m_ctx = avcodec_alloc_context3(m_codec);
    m_picture = av_frame_alloc();

	m_ctx->width = nWidth;
	m_ctx->height = nHeight;
	m_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
 
    if(m_codec->capabilities&CODEC_CAP_TRUNCATED)
        m_ctx->flags|= CODEC_FLAG_TRUNCATED;

	m_ctx->get_format  = get_hw_format;
	av_opt_set_int(m_ctx, "refcounted_frames", 1, 0);

	if (mf_hw_decode_init(m_ctx, type) < 0)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "hw_decoder_init failed!!");
		m_codec = NULL;
		avcodec_close(m_ctx);
		av_free(m_ctx);
		m_ctx = NULL;
		av_frame_free(&m_picture);
		m_picture = NULL;
		if (m_hwDeviceCtx)
		{
			av_buffer_unref(&m_hwDeviceCtx);
			m_hwDeviceCtx = NULL;
		}
		return false;
	}

	int nRet = avcodec_open2(m_ctx, m_codec, NULL);
    if (nRet < 0)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "avcodec_open2 failed!!");
		print_ffmpeg_error(nRet);

		m_codec = NULL;
		avcodec_close(m_ctx);
		av_free(m_ctx);
		m_ctx = NULL;
		av_frame_free(&m_picture);
		m_picture = NULL;
		if (m_hwDeviceCtx)
		{
			av_buffer_unref(&m_hwDeviceCtx);
			m_hwDeviceCtx = NULL;
		}
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
		if (m_hwDeviceCtx)
		{
			av_buffer_unref(&m_hwDeviceCtx);
			m_hwDeviceCtx = NULL;
		}
        return false;
	}

	m_eOutputType = eOutputType;
	return true;
}


void CH264_HW_Decoder::Close()
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

	if (m_hwDeviceCtx)
	{
		av_buffer_unref(&m_hwDeviceCtx);
		m_hwDeviceCtx = NULL;
	}

}

//大于0，解码成功
int CH264_HW_Decoder::Decode(const void *data, UINT size, void *pRetBuf, int* pnWidth, int* pnHeight)
{
#ifndef ANDROID
	try
	{
#endif
		return mf_hw_decode_dec((BYTE*)data, (int)size, (BYTE *)pRetBuf, pnWidth, pnHeight);
#ifndef ANDROID
	}
	catch (...)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_FATAL, "decode Exception");
		return -1;
	}
#endif
}


static BOOL OutputToBGR24(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf)
{
	struct SwsContext *img_convert_ctx = NULL;
	AVFrame *pAVFrameRGB = av_frame_alloc();
	if (pAVFrameRGB == NULL)
	{
		return FALSE;
	}

	avpicture_fill((AVPicture*)pAVFrameRGB, byOutbuf, AV_PIX_FMT_BGR24, width, height);

	img_convert_ctx = sws_getContext(width, height, (AVPixelFormat)pAVFrame->format, width, height, AV_PIX_FMT_BGR24,
		SWS_BICUBIC, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, pAVFrame->data, pAVFrame->linesize,0, height, pAVFrameRGB->data, pAVFrameRGB->linesize);

	av_frame_free(&pAVFrameRGB);
	sws_freeContext(img_convert_ctx);

	return TRUE;
	
}

static BOOL OutputToRGB24(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf)
{
	struct SwsContext *img_convert_ctx = NULL;
	AVFrame *pAVFrameRGB = av_frame_alloc();
	if (pAVFrameRGB == NULL)
	{
		return FALSE;
	}
	
	avpicture_fill((AVPicture*)pAVFrameRGB, byOutbuf, AV_PIX_FMT_RGB24, width, height);
	
	img_convert_ctx = sws_getContext(width, height, (AVPixelFormat)pAVFrame->format, width, height, AV_PIX_FMT_RGB24,
		SWS_BICUBIC, NULL, NULL, NULL);
	sws_scale(img_convert_ctx, pAVFrame->data, pAVFrame->linesize,0, height, pAVFrameRGB->data, pAVFrameRGB->linesize);
	
	av_frame_free(&pAVFrameRGB);
	sws_freeContext(img_convert_ctx);
		
	return TRUE;
	
}

static BOOL OutputToRGBA32(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf, bool bIsRGBOut)
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
		
		img_convert_ctx = sws_getContext(width, height, (AVPixelFormat)pAVFrame->format, width, height, AV_PIX_FMT_RGBA,
		SWS_BICUBIC, NULL, NULL, NULL);
	}
	else
	{
		avpicture_fill((AVPicture*)pAVFrameRGBA, byOutbuf, AV_PIX_FMT_BGRA, width, height);
		
		img_convert_ctx = sws_getContext(width, height, (AVPixelFormat)pAVFrame->format, width, height, AV_PIX_FMT_BGRA,
		SWS_BICUBIC, NULL, NULL, NULL);
	}
	
	sws_scale(img_convert_ctx, pAVFrame->data, pAVFrame->linesize,0, height, pAVFrameRGBA->data, pAVFrameRGBA->linesize);
	av_frame_free(&pAVFrameRGBA);
	sws_freeContext(img_convert_ctx);
	return TRUE;
	
}

static BOOL OutputToYUV420(AVFrame * pAVFrame, int width, int height, BYTE* byOutbuf)
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

int CH264_HW_Decoder::avcodec_decode_video_hw(AVCodecContext *avctx, AVFrame *sw_frame, int *got_picture_ptr, const AVPacket *packet)
{
	AVFrame *frame = NULL;
	int ret = 0;

	*got_picture_ptr = 0;

	ret = avcodec_send_packet(avctx, packet);
	if (ret < 0) 
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Error during decoding ret:%d", ret);
		print_ffmpeg_error(ret);
		return ret;
	}

	while (ret >= 0) 
	{
		if (!(frame = av_frame_alloc()))
		{
			SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Can not alloc frame");
			ret = AVERROR(ENOMEM);
			goto fail;
		}

		ret = avcodec_receive_frame(avctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) 
		{
			av_frame_free(&frame);
			return 0;
		} 
		else if (ret < 0) 
		{
			SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Error while decoding ret:%d", ret);
			print_ffmpeg_error(ret);
			goto fail;
		}

		if (frame->format == m_hwPixFmt)
		{
			//从显存拷贝到内存
			if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) 
			{
				SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Error transferring the data to system memory, ret:%d", ret);
				print_ffmpeg_error(ret);
				goto fail;
			}
			*got_picture_ptr = 1;
		}

fail:
		av_frame_free(&frame);
		if (ret < 0)
			return ret;
	}

	return 0;
}

//输入一帧码流解码后输出
int CH264_HW_Decoder::mf_hw_decode_dec(const BYTE* in_buf_IN, int in_buf_size, BYTE *out_buf, int* pnWidth, int* pnHeight)
{
	static BYTE byHeadLong[] = {0x00, 0x00, 0x00, 0x01};
	static BYTE byHeadShort[] = {0x00, 0x00, 0x01};
	
	int got_picture = 0;
	AVPacket avpkt;
	BYTE* in_buf = NULL;
	
	//拷贝到字节对齐的内存区，尾段初始化0
	memset(m_pnInputBuff, 0x0, sizeof(int)*1024*512);
	if (in_buf_size > (int)(sizeof(int)*1024*512))
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Invalid NALU with too large size:%d!", in_buf_size);
		return 0;		
	}
	memcpy((BYTE *)m_pnInputBuff, in_buf_IN, in_buf_size);
	in_buf = (BYTE *)m_pnInputBuff;


    memset(&avpkt, 0, sizeof(AVPacket));
	av_init_packet(&avpkt);
	
	//返回值， 0无数据输出 正数表示解码图像帧大小
	int iRet = 0;
	
	//修改为NALU格式后，新的码流头校验
	if (in_buf_size <= 5)
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Invalid NALU with too short:%d Head!", in_buf_size);
		return 0;		
	}

	//输入码流不完整
	if( (memcmp(byHeadLong, in_buf, sizeof(byHeadLong)) != 0) && (memcmp(byHeadShort, in_buf, sizeof(byHeadShort)) != 0))
	{
		SDLOG_PRINTF("CX264_Decoder", SD_LOG_LEVEL_ERROR, "Invalid NALU Head!!!");
		return 0;
	}
	
	while(in_buf_size > 0)
	{

		int len = av_parser_parse2(m_h264Parser, m_ctx, &avpkt.data, &avpkt.size, in_buf, in_buf_size, 0, 0, 0);
		in_buf += len;
		in_buf_size -= len;

		if(avpkt.size > 0)
		{
			//硬解码
			avcodec_decode_video_hw(m_ctx, m_picture, &got_picture, &avpkt);
			if(got_picture > 0)
			{
 				if (m_picture->width*m_picture->height != m_ctx->width*m_ctx->height)
 				{
 					continue;
 				}

				if (m_eOutputType == DEC_OUTPUT_BGRA)
				{
					OutputToRGBA32(m_picture, m_ctx->width, m_ctx->height, out_buf, false);
					iRet = (m_ctx->width * m_ctx->height * 4);
				}
				else if (m_eOutputType == DEC_OUTPUT_RGBA)
				{
					OutputToRGBA32(m_picture, m_ctx->width, m_ctx->height, out_buf, true);
					iRet = (m_ctx->width * m_ctx->height * 4);
				}
				else if (m_eOutputType == DEC_OUTPUT_BGR)
				{
					OutputToBGR24(m_picture, m_ctx->width, m_ctx->height, out_buf);	
					iRet = (m_ctx->width * m_ctx->height * 3);
				}
				else if (m_eOutputType == DEC_OUTPUT_RGB)
				{
					OutputToRGB24(m_picture, m_ctx->width, m_ctx->height, out_buf);
					iRet = (m_ctx->width * m_ctx->height * 3);
				}
				else if (m_eOutputType == DEC_OUTPUT_YUV420)
				{
					OutputToYUV420(m_picture, m_ctx->width, m_ctx->height, out_buf);
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
   return iRet;
}



