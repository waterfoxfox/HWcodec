
#if !defined(X264_ENCODER_H)
#define X264_ENCODER_H

#include "SDCommon.h"

#include <time.h>
extern "C"
{
#ifdef WIN32
	#include "x264/stdint.h"
#endif
	#include "x264/x264.h"
	#include "x264/x264_config.h"
};

#define X264_BASELINE_PROFILE					   0
#define X264_HIGH_PROFILE						   1  	
#define X264_MAIN_PROFILE						   2  	

struct Ctx
{
	x264_t *x264;
	x264_picture_t picture;
	x264_param_t param;
};

//用户指定Profile 和 Level等参数
typedef struct X264ExternParams
{
	int nLevel;
	int nProfile;
	int bEnableVbv;
	char acPreset[64];
}X264ExternParams;

class CX264_Encoder 
{
public:
	CX264_Encoder();
	virtual ~CX264_Encoder();

	bool            Open(int nWidth, int nHeight, int nFrameRate, int nBitrate, int nKeyFrameInterval,  int nNaluSize, X264ExternParams *ptExternParams);
	void            Close();
	//输入一帧YUV420图像，编码为H264码流，并将NALU信息告知外层
	//返回>0 当前帧码流大小; < 0 编码错误; =0 当前无码流输出
	int             Encode(BYTE *pucInputYuv , BYTE *pucOutputStream, int *pnNaluLen, UINT unMaxNaluCnt, bool bForceIdr, int64_t *pPts = NULL, int64_t *pDts = NULL);
	bool			Control(int nWidth, int nHeight, int nFrameRate, int nBitrate, int nKeyFrameInterval,  int nNaluSize);

private:
	bool				m_bClosed;
	void*				m_pClosedCs;

	Ctx*				m_ptCtx;

	//在HP时防止DTS出现负数而加入的PTS基值
	int64_t				m_nPtsBaseNum;
	int64_t				m_nPtsStartTime;

	int					m_nForceKeyframe;

	int					m_nKeyFrameIntervalTime;

	//上一次编码IDR帧的时间，用于避免由于实际输入帧频不足配置的帧率，而导致长时间（大于指定的最大I帧间隔）无IDR帧的情况
	long				m_nPrevIdrTime;

	X264ExternParams	m_tExternParams;

private:
	bool mfOpen(int nWidth, int nHeight, int nFrameRate, int nBitrate, int nKeyFrameInterval, int nNaluSize, X264ExternParams *ptExternParams);
	void mfClose();
	int mfDumpnals (x264_nal_t *ptNals, int nNalsNum, BYTE *pucOutputStream, int *pnNaluLen);
	int mfCompress (unsigned char *data[4], int stride[4], unsigned char *pRetData, int *pnRetLen, UINT unMaxBlkCnt, bool bForceIdr, int64_t *pPts, int64_t *pDts);
	int64_t mfGeneratePts();	
};


#endif // !defined(X264_ENCODER_H)
