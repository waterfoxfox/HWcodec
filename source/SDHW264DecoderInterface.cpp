//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDHW264DecoderInterface
//* 内容摘要：H264硬解码封装
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-7-19
//***************************************************************************/
#include "SDHW264DecoderInterface.h"

//Nvi编码封装类
CSDH264Dec::CSDH264Dec()
{
	m_pvDec = CSDMutex::CreateObject();
	m_bDecClose = TRUE;
	m_pH264Dec = new CH264_HW_Decoder;
}

CSDH264Dec::~CSDH264Dec()
{
	if (m_bDecClose == FALSE)
	{
		mfCloseDecoder();
	}

	CSDMutex::RealseObject(m_pvDec);
	delete m_pH264Dec;
}

BOOL CSDH264Dec::CreateDecoder(int nMaxWidth, int nMaxHeight, DecodeOutputImageType eOutputType)
{
	CSDMutex cs(m_pvDec);
	if (m_bDecClose == FALSE)
	{
		SDLOG_PRINTF("CSDH264Dec", SD_LOG_LEVEL_WARNING, "Decode still open, close first!");
		mfCloseDecoder();
	}
	
	bool bRet = m_pH264Dec->Open(nMaxWidth, nMaxHeight, eOutputType);
	if (bRet == false)
	{
		SDLOG_PRINTF("CSDH264Dec", SD_LOG_LEVEL_ERROR, "Decode Open failed with params:%dx%d %d!", 
					nMaxWidth, nMaxHeight, eOutputType);
		return FALSE;
	}

	m_bDecClose = FALSE;
	return TRUE;
}

void CSDH264Dec::CloseDecoder()
{
	CSDMutex cs(m_pvDec);
	mfCloseDecoder();
}

void CSDH264Dec::mfCloseDecoder()
{
	m_bDecClose = TRUE;
	m_pH264Dec->Close();
}

int CSDH264Dec::Decode(const void* pBitstreamData, int nBitstreamSize, void* pImageData, int* pnWidth, int* pnHeight)
{
	int nRet = 0;
	CSDMutex cs(m_pvDec);
	if (m_bDecClose == TRUE)
	{
		SDLOG_PRINTF("CSDH264Dec", SD_LOG_LEVEL_ERROR, "Decode fail, decoder is closed!");
		return nRet;
	}
	
	if ((pBitstreamData == NULL) || (pImageData == NULL))
	{
		SDLOG_PRINTF("CSDH264Dec", SD_LOG_LEVEL_ERROR, "Decode fail, Point is NULL %p %p!", pBitstreamData, pImageData);
		return nRet;
	}

	nRet = m_pH264Dec->Decode(pBitstreamData, nBitstreamSize, pImageData, pnWidth, pnHeight);
	return nRet;
}

