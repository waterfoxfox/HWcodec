//**************************************************************************
//* 版权所有
//*
//* 文件名称：HwCodecSdk
//* 内容摘要：Windows平台Intel QSV\英伟达NVI H264 H265硬编解码DLL封装
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-6-20
//***************************************************************************/
#ifndef _HW_CODEC_SDK_H_
#define _HW_CODEC_SDK_H_

#ifndef BUILDING_DLL
#define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
#define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */

#include "SDLog.h"
#include "SDCodecCommon.h"
#include "SDNviEncoderInterface.h"
#include "SDQsvEncoderInterface.h"
#include "SDHW264DecoderInterface.h"

extern "C"
{
//////////////////////////////////////////////////////////////////////////
// HWCodec接口

/***
* 环境初始化，系统只需调用一次
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
DLLIMPORT void  HwCodec_Enviroment_Init(const char* logPath, int logLevel);

DLLIMPORT void  HwCodec_Enviroment_Free();

/***
* 判断当前平台是否支持Intel QSV H264硬编码
* @param：
* @return: 支持true, 不支持false 一般用于预先判断以决定Create的类型
*/
DLLIMPORT bool  HwCodec_Is_QsvH264Enc_Support();


/***
* 判断当前平台是否支持英伟达 NVI H264硬编码
* @param：
* @return: 支持true, 不支持false 一般用于预先判断以决定Create的类型
*/
DLLIMPORT bool  HwCodec_Is_NviH264Enc_Support();

/***
* 判断当前平台是否支持H264硬解码
* @param：
* @return: 支持true, 不支持false 一般用于预先判断以决定Create的类型
*/
DLLIMPORT bool  HwCodec_Is_HwH264Dec_Support();

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/***
* 创建QSV H264硬编码
* @param pInputParams: 输入图像格式的描述，比如输入图像的宽高、色度空间
* @param pEncParams: 编码器可供设置的参数，比如编码的宽高、码率、Level、Profile等
*					本模块内部自带色度空间转换和缩放功能，但建议外层保证输入宽高即编码宽高
* @return: 返回模块指针，为NULL则失败
*/
DLLIMPORT void* QsvH264Enc_Create(h264_input_params* pInputParams, h264_enc_params* pEncParams);


/***
* 销毁QSV硬编码
* @param pQsvEnc: 模块指针
* @return: 
*/
DLLIMPORT void  QsvH264Enc_Delete(void* pQsvEnc);


/***
* QSV编码一帧
* @param pQsvEnc: 模块指针
* @param pImageData: 输入图像数据
* @param pBitstreamData: 待存放码流的区域
* @param nMaxSupportLen: 待存放码流的区域的大小
* @return: 成功返回>0，否则<=0。返回值大于0时，即表示pBitstreamData中存放的一帧实际码流长度
*/
DLLIMPORT int  QsvH264Enc_Encode(void* pQsvEnc, const char* pImageData, char* pBitstreamData, int nMaxSupportLen);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/***
* 创建NVI H264硬编码
* @param pInputParams: 输入图像格式的描述，比如输入图像的宽高、色度空间
* @param pEncParams: 编码器可供设置的参数，比如编码的宽高、码率、Level、Profile等
*					本模块内部自带色度空间转换和缩放功能，但建议外层保证输入宽高即编码宽高
* @return: 返回模块指针，为NULL则失败
*/
DLLIMPORT void* NviH264Enc_Create(h264_input_params* pInputParams, h264_enc_params* pEncParams);


/***
* 销毁NVI硬编码
* @param pNviEnc: 模块指针
* @return: 
*/
DLLIMPORT void  NviH264Enc_Delete(void* pNviEnc);


/***
* NVI编码一帧
* @param pNviEnc: 模块指针
* @param pImageData: 输入图像数据
* @param pBitstreamData: 待存放码流的区域
* @param nMaxSupportLen: 待存放码流的区域的大小
* @return: 成功返回>0，否则<=0。返回值大于0时，即表示pBitstreamData中存放的一帧实际码流长度
*/
DLLIMPORT int  NviH264Enc_Encode(void* pNviEnc, const char* pImageData, char* pBitstreamData, int nMaxSupportLen);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/***
* 创建H264硬解码
* @param nMaxWidth: 解码器支持的最大宽度
* @param nMaxHeight: 解码器支持的最大高度
* @param eOutputType: 解码器输出的视频格式
* @return: 返回模块指针，为NULL则失败
*/
DLLIMPORT void* HwH264Dec_Create(int nMaxWidth, int nMaxHeight, DecodeOutputImageType eOutputType);


/***
* 销毁H264硬解码
* @param pHwDec: 模块指针
* @return: 
*/
DLLIMPORT void  HwH264Dec_Delete(void* pHwDec);


/***
* 解码一帧
* @param pHwDec: 模块指针
* @param pBitstreamData: 输入一帧码流数据，需要包含起始码
* @param nBitstreamSize: 输入码流的长度
* @param pImageData: 待存放图像的区域
* @param pnWidth: 输出图像的宽度
* @param pnHeight: 输出图像的高度
* @return: 成功返回>0，否则<=0。返回值大于0时，即表示pImageData中存放的一帧实际图像字节数
*/
DLLIMPORT int  HwH264Dec_Decode(void* pHwDec, const void* pBitstreamData, int nBitstreamSize, void* pImageData, int* pnWidth, int* pnHeight);


};

#endif // _HW_CODEC_SDK_H_
