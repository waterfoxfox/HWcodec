//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDQsvEncoderInterface
//* 内容摘要：Intel Qsv硬编码封装
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-4-12
//***************************************************************************/
#if !defined(QSV_ENC_PACKAGE_H)
#define QSV_ENC_PACKAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDCommon.h"
#include "SDLog.h"
#include "SDMutex.h"
#include "SDQsv264Encoder.h"
#include "SDCodecCommon.h"
#include "SDBmpZoom.h"


class CSDQsvEnc
{
public:
	CSDQsvEnc();
	virtual ~CSDQsvEnc();

private:
	//对外接口锁
	void* m_pvEnc;
	BOOL m_bEncClose;

	CQsv264_Encoder* m_pQsv264Enc;
	CSDBmpZoom m_BmpZoom;
	AVFrame* m_ptAVFrameEncode;
	AVPixelFormat m_tEncFormat;
	BYTE* m_pucBuffEncode;

	UINT m_unInputWidth;
	UINT m_unInputHeight;
	UINT m_unEncWidth;
	UINT m_unEncHeight;

private:
	void mfGetPresetString(video_enc_preset ePreset, char* strPreset);
	void mfCloseEncoder();

public:
	//创建编码器	
	BOOL CreateEncoder(h264_input_params* pInputParams, h264_enc_params* pEncParams);
	//关闭编码器
	void CloseEncoder();

	//编码一帧
	int Encode(const char* pImageData, char* pBitstreamData, int nMaxSupportLen);

};

#endif
