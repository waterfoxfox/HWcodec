//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDBmpZoom
//* 内容摘要：FFMPEG进行缩放和色度空间转换
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：
//***************************************************************************/

#if !defined(SDBMPZOOM_H)
#define SDBMPZOOM_H

#include <stdio.h>
#include <stdlib.h>

extern "C"{
#ifdef WIN32
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libswscale/swscale.h"
#include "ffmpeg/libavutil/frame.h"
#include "ffmpeg/libavutil/imgutils.h"
#else
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/frame.h"
#endif
}
 

//使用swscale进行图像缩放
//注意：本模块接口的互斥由外层负责
class CSDBmpZoom  
{
public:	 
	CSDBmpZoom();
	virtual ~CSDBmpZoom();

	bool CreateBmpZoom(int nSrcWidth, int nSrcHeight, int nDesWidth, int nDesHeight,
		AVPixelFormat eSrcPixFmt, AVPixelFormat eDstPixFmt, bool bHighQuality);

	bool BmpZoom(int nSrcWidth, int nSrcHeight, BYTE *pSrcImgbuf, 
		int nDesWidth, int nDesHeight, BYTE *pDesImgbuf);

	bool BmpZoom(AVFrame *pAVFrameSrc, int nDesWidth, int nDesHeight, BYTE* pDesImgbuf);

	void CloseBmpZoom();

private:
	struct SwsContext *m_ptImgConvertCtx;

	AVFrame *m_ptAVFrameSrc;
	AVPixelFormat m_eSrcPixFmt;

	AVFrame *m_ptAVFrameDes;
	AVPixelFormat m_eDstPixFmt;

	bool m_bCreated;
};

#endif // !defined(SDBMPZOOM_H)
