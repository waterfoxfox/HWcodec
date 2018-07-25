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
#if !defined(H264_DEC_PACKAGE_H)
#define H264_DEC_PACKAGE_H

#include "SDCommon.h"
#include "SDLog.h"
#include "SDMutex.h"
#include "SDCodecCommon.h"
#include "SDHW264Decoder.h"

class CSDH264Dec
{
public:
	CSDH264Dec();
	virtual ~CSDH264Dec();

private:
	//对外接口锁
	void* m_pvDec;
	BOOL m_bDecClose;
	CH264_HW_Decoder* m_pH264Dec;
	
private:
	void mfCloseDecoder();
	
public:
	//创建解码器	
	BOOL CreateDecoder(int nMaxWidth, int nMaxHeight, DecodeOutputImageType eOutputType);
	//关闭解码器
	void CloseDecoder();

	//解码一帧
	int Decode(const void* pBitstreamData, int nBitstreamSize, void* pImageData, int* pnWidth, int* pnHeight);

};

#endif
