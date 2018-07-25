//**************************************************************************
//* 版权所有
//*
//* 文件名称：CNvi264_Encoder
//* 内容摘要：Nvidia nvenc硬编码封装
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-4-12
//***************************************************************************/
#if !defined(NVI264_ENCODER_H)
#define NVI264_ENCODER_H

#include "SDCommon.h"
#include "SDCodecCommon.h"



//请勿修改，需要保证和X264 preset对应关系
//static const char * const x264_preset_names[] = { "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo", 0 };
static const char * const nvi264_preset_names[] = { "veryfast", "veryfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "veryslow", 0 };

#define NVI264_BASELINE_PROFILE					   0
#define NVI264_HIGH_PROFILE						   1  	


//用户指定Profile 和 Level等参数
typedef struct Nvi264ExternParams
{
	int nLevel;
	int nProfile;
	int bEnableVbv;
	char acPreset[64];
}Nvi264ExternParams;


class CNvi264_Encoder 
{
public:
	CNvi264_Encoder();
	virtual ~CNvi264_Encoder();

	bool	Open(int nWidth, int nHeight, int nFrameRate, int nBitrate, int nKeyFrameInterval, Nvi264ExternParams *pExternParams);
	void	Close();
	int		Encode(AVFrame* ptEncFrame, BYTE* pucBitstreamData, int nMaxSupportLen, int64_t* pPts, int64_t* pDts);
	bool	Control(int nFrameRate, int bitrate, int nIdrInterval, int nWidth, int nHeight);

	//获取硬编码所需输入的格式（一般为AV_PIX_FMT_NV12），外层在Open后调用本API获得当前编码器支持的输入格式
	//外层一般在缩放处理后得到本格式数据送编码
	bool	GetInputFormat(int *pnFormat, int *pnWidth, int *pnHeight);	
	//预判当前平台是否支持该硬编码，真实支持情况以Open函数返回为准
	static bool IsHwAcclSupported(void);

	//请求即刻编码IDR帧
	bool	IdrRequest();
	//请求即刻编码IDR帧并且重新初始化时间戳
	void	ReInit();

private:
	bool				m_bClosed;
	void*				m_pClosedCs;

	AVCodecContext*		m_ptEncContext;

	//在HP时防止DTS出现负数而加入的PTS基值
	int64_t				m_nPtsBaseNum;
	int64_t				m_nPtsStartTime;

	int					m_nKeyFrameIntervalTime;
	int					m_nReqFormat;
	int					m_nReqWidth;
	int					m_nReqHeight;

	//上一次编码IDR帧的时间，用于避免由于实际输入帧频不足配置的帧率，而导致长时间（大于指定的最大I帧间隔）无IDR帧的情况
	long				m_nPrevIdrTime;

	Nvi264ExternParams	m_tExternParams;
	//输出的NALU是否包含起始码(0x000001 0x00000001)
	bool				m_bUseNaluStartCode;
	//是否使用90KHZ时间戳（否则使用实际机器时间）
	bool				m_bUse90KTimeStamp;
	//使用的码率控制算法
	int					m_nRateControlMethod;
	//请求编码IDR标志
	bool				m_bRequestIdr;

private:
	bool mfOpen(int nWidth, int nHeight, int nFrameRate, int bitrate, int nKeyFrameInterval, Nvi264ExternParams *pExternParams);
	void mfClose();
	int64_t mfGeneratePts();	
};


#endif // !defined(NVI264_ENCODER_H)
