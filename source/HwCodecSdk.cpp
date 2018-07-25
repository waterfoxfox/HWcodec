//**************************************************************************
//* 版权所有
//*
//* 文件名称：HwCodecSdk
//* 内容摘要：Windows平台Intel QSV\英伟达NVI 硬编解码DLL封装
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-6-20
//***************************************************************************/
#include "HwCodecSdk.h"

#define HW_CODEC_SDK_VERSION	2018071901


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 环境初始化，系统只需调用一次
void  HwCodec_Enviroment_Init(const char* logPath, int logLevel)
{
	SDLOG_INIT(logPath, logLevel);
	ffmpeg_global_init();
}

//环境反初始化
void  HwCodec_Enviroment_Free()
{
	ffmpeg_global_uninit();
	SDLOG_CLOSE();
}

//判断当前平台是否支持QSV H264硬编码
bool HwCodec_Is_QsvH264Enc_Support()
{
	return CQsv264_Encoder::IsHwAcclSupported();
}

//判断当前平台是否支持NVI H264硬编码
bool HwCodec_Is_NviH264Enc_Support()
{
	return CNvi264_Encoder::IsHwAcclSupported();
}

//判断当前平台是否支持H264硬解码
bool HwCodec_Is_HwH264Dec_Support()
{
	return CH264_HW_Decoder::IsHwAcclSupported();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Qsv硬编码接口
void* QsvH264Enc_Create(h264_input_params* pInputParams, h264_enc_params* pEncParams)
{
	if ((pInputParams == NULL) || (pEncParams == NULL))
	{
		SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "QsvEnc_Create Failed, params null:%p %p!", pInputParams, pEncParams);
		return NULL;
	}

    CSDQsvEnc* qsv_encoder = new CSDQsvEnc();
    if (qsv_encoder)
    {
		BOOL bRet = qsv_encoder->CreateEncoder(pInputParams, pEncParams);
		if (bRet == FALSE)
		{
			SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "QsvEnc_Create Create Failed!!");
			delete qsv_encoder;
			qsv_encoder = NULL;
		}
		else
		{	
			SDLOG_PRINTF("HW", SD_LOG_LEVEL_INFO, "QsvEnc_Create(Ver:%d) Input:%dx%d Enc:%dx%d success!!", 
						HW_CODEC_SDK_VERSION, pInputParams->unInputWidth, pInputParams->unInputHeight,
						pEncParams->unEncWidth, pEncParams->unEncHeight);
		}
    }

    return qsv_encoder;
}

//使用者应该做好互斥保护
void QsvH264Enc_Delete(void* pQsvEnc)
{
    CSDQsvEnc* qsv_encoder = (CSDQsvEnc*)pQsvEnc;
     if (qsv_encoder)
     {
		 qsv_encoder->CloseEncoder();
         delete qsv_encoder;
     }
}

int  QsvH264Enc_Encode(void* pQsvEnc, const char* pImageData, char* pBitstreamData, int nMaxSupportLen)
{
	CSDQsvEnc* qsv_encoder = (CSDQsvEnc*)pQsvEnc;
	if (qsv_encoder && pImageData && pBitstreamData)
	{
		return qsv_encoder->Encode(pImageData, pBitstreamData, nMaxSupportLen);
	}
	else
	{
		SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "QsvEnc_Encode Failed!. pQsvEnc:%p  pImageData:%p pBitstreamData:%p", 
					pQsvEnc, pImageData, pBitstreamData);
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Nvi硬编码接口
void* NviH264Enc_Create(h264_input_params* pInputParams, h264_enc_params* pEncParams)
{
	if ((pInputParams == NULL) || (pEncParams == NULL))
	{
		SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "NviEnc_Create Failed, params null:%p %p!", pInputParams, pEncParams);
		return NULL;
	}

	CSDNviEnc* nvi_encoder = new CSDNviEnc();
	if (nvi_encoder)
	{
		BOOL bRet = nvi_encoder->CreateEncoder(pInputParams, pEncParams);
		if (bRet == FALSE)
		{
			SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "NviEnc_Create Create Failed!!");
			delete nvi_encoder;
			nvi_encoder = NULL;
		}
		else
		{	
			SDLOG_PRINTF("HW", SD_LOG_LEVEL_INFO, "NviEnc_Create(Ver:%d) Input:%dx%d Enc:%dx%d success!!", 
				HW_CODEC_SDK_VERSION, pInputParams->unInputWidth, pInputParams->unInputHeight,
				pEncParams->unEncWidth, pEncParams->unEncHeight);
		}
	}

	return nvi_encoder;
}

//使用者应该做好互斥保护
void NviH264Enc_Delete(void* pNviEnc)
{
	CSDNviEnc* nvi_encoder = (CSDNviEnc*)pNviEnc;
	if (nvi_encoder)
	{
		nvi_encoder->CloseEncoder();
		delete nvi_encoder;
	}
}

int  NviH264Enc_Encode(void* pNviEnc, const char* pImageData, char* pBitstreamData, int nMaxSupportLen)
{
	CSDNviEnc* nvi_encoder = (CSDNviEnc*)pNviEnc;
	if (nvi_encoder && pImageData && pBitstreamData)
	{
		return nvi_encoder->Encode(pImageData, pBitstreamData, nMaxSupportLen);
	}
	else
	{
		SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "NviEnc_Encode Failed!. pNviEnc:%p  pImageData:%p  pBitstreamData:%p", 
			pNviEnc, pImageData, pBitstreamData);
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// H264硬解码接口
void* HwH264Dec_Create(int nMaxWidth, int nMaxHeight, DecodeOutputImageType eOutputType)
{
	CSDH264Dec* decoder = new CSDH264Dec();
	if (decoder)
	{
		BOOL bRet = decoder->CreateDecoder(nMaxWidth, nMaxHeight, eOutputType);
		if (bRet == FALSE)
		{
			SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "CreateDecoder Create Failed!!");
			delete decoder;
			decoder = NULL;
		}
		else
		{	
			SDLOG_PRINTF("HW", SD_LOG_LEVEL_INFO, "CreateDecoder(Ver:%d) Input:%dx%d d success!!", 
						HW_CODEC_SDK_VERSION, nMaxWidth, nMaxHeight, eOutputType);
		}
	}

	return decoder;
}

void  HwH264Dec_Delete(void* pHwDec)
{
	CSDH264Dec* decoder = (CSDH264Dec*)pHwDec;
	if (decoder)
	{
		decoder->CloseDecoder();
		delete decoder;
	}
}

int  HwH264Dec_Decode(void* pHwDec, const void* pBitstreamData, int nBitstreamSize, void* pImageData, int* pnWidth, int* pnHeight)
{
	CSDH264Dec* decoder = (CSDH264Dec*)pHwDec;
	if (decoder && pImageData && pBitstreamData)
	{
		return decoder->Decode(pBitstreamData, nBitstreamSize, pImageData, pnWidth, pnHeight);
	}
	else
	{
		SDLOG_PRINTF("HW", SD_LOG_LEVEL_ERROR, "HwH264Dec_Decode Failed!. decoder:%p  pImageData:%p  pBitstreamData:%p", 
					decoder, pImageData, pBitstreamData);
		return 0;
	}
}