//**************************************************************************
//* 版权所有
//*
//* 文件名称：X264EncoderSdk
//* 内容摘要：X264编码DLL封装，输入YUV420P
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-11-20
//***************************************************************************/
#ifndef _X264ENC_SDK_H_
#define _X264ENC_SDK_H_

#ifdef WIN32
#ifndef BUILDING_DLL
#define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
#define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */
#else
#define DLLIMPORT
#endif

#include "SDLog.h"
#include "SDX264Encoder.h"

extern "C"
{
//////////////////////////////////////////////////////////////////////////
// HWCodec接口

/***
* 【可选接口】环境初始化，系统只需调用一次，若不调用则无日志输出
* @param logPath:日志文件存放路径，内部自动创建文件夹
* @param logLevel:日志文件输出级别，只有高于或等于该级别的日志才会输出，可用级别如下：
*					LOG_LEVEL_DEBUG = 1;
*					LOG_LEVEL_INFO = 2;
*					LOG_LEVEL_WARN = 3;
*					LOG_LEVEL_ERROR = 4;
*					LOG_LEVEL_ALARM = 5;
*					LOG_LEVEL_FATAL = 6;
*					LOG_LEVEL_NONE = 7;
* 当设置为LOG_LEVEL_NONE时将关闭日志功能
* @return: 
*/
DLLIMPORT void  X264Enc_Enviroment_Init(const char* logPath, int logLevel);

DLLIMPORT void  X264Enc_Enviroment_Free();


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/***
* X264编码器对象创建接口
* @return: 返回模块指针，为NULL则失败
*/
DLLIMPORT void* X264Enc_Create();


/***
* X264编码器Open接口
* @param pX264Enc: 模块指针
* @param nWidth: 输入YUV420P图像宽度
* @param nHeight: 输入YUV420P图像高度
* @param nFrameRate: 编码帧率 fps
* @param nBitrate: 编码码率 kbps， 比如设置1000 表示1Mbps
* @param nKeyFrameInterval: 编码IDR间隔秒数  秒
* @param nNaluSize: 编码器生成单个NALU的限制字节数
* @param ptExternParams: 编码器可供设置的额外参数，比如编码的Level、Profile、Preset、是否使用VBV
* @return: true or false
*/
DLLIMPORT bool X264Enc_Open(void* pX264Enc, int nWidth, int nHeight, int nFrameRate, int nBitrate, 
							int nKeyFrameInterval, int nNaluSize, X264ExternParams *ptExternParams);


/***
* X264编码Reset接口
* @param pX264Enc: 模块指针
* @param nWidth: 输入YUV420P图像宽度
* @param nHeight: 输入YUV420P图像高度
* @param nFrameRate: 编码帧率 fps
* @param nBitrate: 编码码率 kbps
* @param nKeyFrameInterval: 编码IDR间隔秒数  秒
* @param nNaluSize: 编码器生成单个NALU的限制字节数
* @return: true or false
*/
DLLIMPORT bool X264Enc_Reset(void* pX264Enc, int nWidth, int nHeight, int nFrameRate, int nBitrate, 
							int nKeyFrameInterval, int nNaluSize);


/***
* X264编码器Close接口
* @param pX264Enc: 模块指针
* @return: 
*/
DLLIMPORT void X264Enc_Close(void* pX264Enc);


/***
* X264编码器对象销毁接口
* @param pX264Enc: 模块指针
* @return: 
*/
DLLIMPORT void X264Enc_Delete(void* pX264Enc);


/***
* X264编码接口
* @param pX264Enc: 模块指针
* @param pucInputYuv[IN]: 输入YUV420P图像数据
* @param pucOutputStream[OUT]: 待存放码流的区域，编码器生成带起始码的一帧码流存入其中
* @param pnNaluLen[OUT]: 待存放编码器生成的NALU信息，编码器内部将生成的各个NALU长度信息存放其中，便于外层以NALU为网络发送单元，无需自行解析
* @param unMaxNaluCnt[OUT]: 一帧画面最大支持的NALU数目（即pnNaluLen 数组的大小）
* @param bForceIdr[IN]: 是否强制当前帧使用IDR编码，true则强制编码为IDR
* @return: 成功返回>0，编码错误返回<0，当前帧暂无码流输出返回0。返回值大于0时，即表示pucOutputStream中存放的一帧实际码流长度
*/
DLLIMPORT int X264Enc_Encode(void* pX264Enc, BYTE *pucInputYuv, BYTE *pucOutputStream, int *pnNaluLen, UINT unMaxNaluCnt, bool bForceIdr);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


};

#endif // _X264ENC_SDK_H_
