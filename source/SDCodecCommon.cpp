//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDCodecCommon
//* 内容摘要：H264软硬编解码所需的一些公共函数
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：2018-4-12
//***************************************************************************/

#include "SDLog.h"
#include "SDMutex.h"
#include "SDCodecCommon.h"
extern "C"{
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavdevice/avdevice.h"
#include "ffmpeg/libavcodec/qsv.h"
#include "ffmpeg/libavutil/imgutils.h"
#include "ffmpeg/libavutil/opt.h"
}

static BOOL g_ffmpeg_global_init = FALSE;

//ffmpeg多路所需的锁保护回调接口
static int lockmgr(void **mtx, enum AVLockOp op)
{
	switch(op) {
	case AV_LOCK_CREATE:
		*mtx = new CSDMutexX;
		if(!*mtx)
			return 1;
		return 0;
	case AV_LOCK_OBTAIN:
		{
			if (*mtx != NULL)
			{
				((CSDMutexX *)(*mtx))->lock();
			}
		}
		return 0;
	case AV_LOCK_RELEASE:
		{
			if (*mtx != NULL)
			{
				((CSDMutexX *)(*mtx))->Unlock();
			}		
		}
		return 0;
	case AV_LOCK_DESTROY:
		if (*mtx != NULL)
		{
			delete (CSDMutexX *)(*mtx);
			*mtx = NULL;
		}
		return 0;
	}
	return 1;
}

//ffmpeg的初始化，系统中调用一次即可
void ffmpeg_global_init()
{
	if (g_ffmpeg_global_init) 
	{
		return;
	}
	av_register_all(); 
	avdevice_register_all();
	av_lockmgr_register(lockmgr);
	g_ffmpeg_global_init = TRUE;
}

//ffmpeg的反初始化
void ffmpeg_global_uninit()
{
	if (g_ffmpeg_global_init)
	{
		av_lockmgr_register(NULL);
	}
	g_ffmpeg_global_init = FALSE;
}


/* NAL unit types */
enum {
    NAL_SLICE           = 1,
    NAL_DPA             = 2,
    NAL_DPB             = 3,
    NAL_DPC             = 4,
    NAL_IDR_SLICE       = 5,
    NAL_SEI             = 6,
    NAL_SPS             = 7,
    NAL_PPS             = 8,
    NAL_AUD             = 9,
    NAL_END_SEQUENCE    = 10,
    NAL_END_STREAM      = 11,
    NAL_FILLER_DATA     = 12,
    NAL_SPS_EXT         = 13,
    NAL_AUXILIARY_SLICE = 19,
    NAL_FF_IGNORE       = 0xff0f001,
};

static const char* AVCFindStartCodeInternal(const char *p, const char *end)
{
    const char *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        unsigned int x = *(const unsigned int*)p;
        //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
        //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

//找到目标码流中的起始码（包括长短头）位置
static const char* AVCFindStartCode(const char *p, const char *end)
{
    const char *out= AVCFindStartCodeInternal(p, end);
    if(p<out && out<end && !out[-1]) out--;
    return out;
}

//获取当前NALU的类型
static bool AVCGetNaluType(const unsigned char *pBuff, unsigned int nLen, unsigned int* nNaluType)
{
	static BYTE byHeadLong[] = {0x00, 0x00, 0x00, 0x01};
	static BYTE byHeadShort[] = {0x00, 0x00, 0x01};

	if (nLen > 5)
	{
		if(memcmp(byHeadLong, pBuff, sizeof(byHeadLong)) == 0)
		{
			*nNaluType = (pBuff[4] & 0x1f);
			return true;
		}
		else if (memcmp(byHeadShort, pBuff, sizeof(byHeadShort)) == 0)
		{
			*nNaluType = (pBuff[3] & 0x1f);
			return true;
		}
		else
		{
			SDLOG_PRINTF("H264Codec", SD_LOG_LEVEL_ERROR, "Invalid NALU found with invalid start code!");	
		}
	}
	else
	{
		SDLOG_PRINTF("H264Codec", SD_LOG_LEVEL_ERROR, "Invalid NALU found with len:%d!", nLen);	
	}

	return false;
}

//指数哥伦布解码相关函数
static unsigned int Ue(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{
    //計算0bit的個數
    unsigned int nZeroNum = 0;
    while (nStartBit < nLen * 8)
    {
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            break;
        }
        nZeroNum++;
        nStartBit++;
    }
    nStartBit ++;

    unsigned long dwRet = 0;
    for (unsigned int i=0; i<nZeroNum; i++)
    {
        dwRet <<= 1;
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            dwRet += 1;
        }
        nStartBit++;
    }
    return (1 << nZeroNum) - 1 + dwRet;
}


static int Se(unsigned char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{
    int UeVal=Ue(pBuff,nLen,nStartBit);
    double k=UeVal;
    int nValue=(int)ceil(k/2);
    if (UeVal % 2==0)
        nValue=-nValue;
    return nValue;
}


static unsigned long u(unsigned int BitCount, unsigned char * buf, unsigned int &nStartBit)
{
    unsigned long dwRet = 0;
    for (unsigned int i=0; i<BitCount; i++)
    {
        dwRet <<= 1;
        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            dwRet += 1;
        }
        nStartBit++;
    }
    return dwRet;
}

//传入SPS NALU并进行解析
bool AVCParseSpsNal(unsigned char* buf, unsigned int nLen, int &Width, int &Height)
{
    unsigned int StartBit=0; 
    int forbidden_zero_bit=u(1,buf,StartBit);
    int nal_ref_idc=u(2,buf,StartBit);
    int nal_unit_type=u(5,buf,StartBit);
    if(nal_unit_type==7)
    {
        int profile_idc=u(8,buf,StartBit);
        int constraint_set0_flag=u(1,buf,StartBit);//(buf[1] & 0x80)>>7;
        int constraint_set1_flag=u(1,buf,StartBit);//(buf[1] & 0x40)>>6;
        int constraint_set2_flag=u(1,buf,StartBit);//(buf[1] & 0x20)>>5;
        int constraint_set3_flag=u(1,buf,StartBit);//(buf[1] & 0x10)>>4;
        int reserved_zero_4bits=u(4,buf,StartBit);
        int level_idc=u(8,buf,StartBit);

        int seq_parameter_set_id=Ue(buf,nLen,StartBit);

        if( profile_idc == 100 || profile_idc == 110 ||
            profile_idc == 122 || profile_idc == 144 )
        {
            int chroma_format_idc=Ue(buf,nLen,StartBit);
            if( chroma_format_idc == 3 )
                int residual_colour_transform_flag=u(1,buf,StartBit);
            int bit_depth_luma_minus8=Ue(buf,nLen,StartBit);
            int bit_depth_chroma_minus8=Ue(buf,nLen,StartBit);
            int qpprime_y_zero_transform_bypass_flag=u(1,buf,StartBit);
            int seq_scaling_matrix_present_flag=u(1,buf,StartBit);

            int seq_scaling_list_present_flag[8];
            if( seq_scaling_matrix_present_flag )
            {
                for( int i = 0; i < 8; i++ ) {
                    seq_scaling_list_present_flag[i]=u(1,buf,StartBit);
                }
            }
        }
        int log2_max_frame_num_minus4=Ue(buf,nLen,StartBit);
        int pic_order_cnt_type=Ue(buf,nLen,StartBit);
        if( pic_order_cnt_type == 0 )
            int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,StartBit);
        else if( pic_order_cnt_type == 1 )
        {
            int delta_pic_order_always_zero_flag=u(1,buf,StartBit);
            int offset_for_non_ref_pic=Se(buf,nLen,StartBit);
            int offset_for_top_to_bottom_field=Se(buf,nLen,StartBit);
            int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);

            int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];
            for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
                offset_for_ref_frame[i]=Se(buf,nLen,StartBit);
            delete [] offset_for_ref_frame;
        }
        int num_ref_frames=Ue(buf,nLen,StartBit);
        int gaps_in_frame_num_value_allowed_flag=u(1,buf,StartBit);
        int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);
        int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);

        Width=(pic_width_in_mbs_minus1+1)*16;
        Height=(pic_height_in_map_units_minus1+1)*16;

        return true;
    }
    else
        return false;
}

//传入一帧码流若其中包含SPS则从中解析出图像宽高，若不包含SPS则解析失败
bool AVCGetWidthHeightFromFrame(const char* frameBuf, int bufLen, int& width, int& height)
{
    const char *p = frameBuf;
    const char *end = p + bufLen;
    const char *nal_start, *nal_end;

    nal_start = AVCFindStartCode(p, end);
    while (nal_start < end)
    {
        while(!*(nal_start++));

        nal_end = AVCFindStartCode(nal_start, end);

        unsigned int nal_size = (unsigned int)(nal_end - nal_start);
        unsigned int nal_type = nal_start[0] & 0x1f;

        if (nal_type == NAL_SPS)  // sps buf
        {
            return AVCParseSpsNal((unsigned char*)nal_start, nal_size, width, height);
        }
        nal_start = nal_end;
    }
    return false;
}

//将一帧码流中不需要的NALU过滤，只保留基本的NALU，用于网络传输时节省带宽
//过滤的NALU包括：NAL_SEI、NAL_AUD、NAL_FILLER_DATA
bool AVCFilterNalUnits(const char *bufIn, int inSize, char* bufOut, int* outSize)
{
    const char *p = bufIn;
    const char *end = p + inSize;
    const char *nal_start, *nal_end;

    char* pbuf = bufOut;
	bool bFoundSps = false;
	unsigned int start_code_len = 0;

	static BYTE byHeadLong[] = {0x00, 0x00, 0x00, 0x01};
	static BYTE byHeadShort[] = {0x00, 0x00, 0x01};

    *outSize = 0;
    nal_start = AVCFindStartCode(p, end);
    while (nal_start < end)
    {
		start_code_len = 0;
        while(!*(nal_start++))
		{
			start_code_len++;
		}

        nal_end = AVCFindStartCode(nal_start, end);

        unsigned int nal_size = (unsigned int)(nal_end - nal_start);
		unsigned int nal_type = nal_start[0] & 0x1f;

		if (nal_type == NAL_SPS)
		{
			bFoundSps = true;
		}

		//SEI AUD 以及部分编码器为了实现CBR而填充的NALU均可以过滤
		//部分编码器每帧都附带了PPS，可以过滤，仅保留IDR帧SPS一起的PPS
		if ((nal_type != NAL_SEI) && (nal_type != NAL_AUD) && (nal_type != NAL_FILLER_DATA))
		{
			if ((nal_type != NAL_PPS) || (bFoundSps == true))
			{
				//先复原起始码
				if (start_code_len == 3)
				{
					//长头
					memcpy(pbuf, byHeadLong, sizeof(byHeadLong));
					pbuf += sizeof(byHeadLong);
				}
				else
				{
					//短头
					memcpy(pbuf, byHeadShort, sizeof(byHeadShort));
					pbuf += sizeof(byHeadShort);
				}

				memcpy(pbuf, nal_start, nal_size);
				pbuf += nal_size;
			}

		}

        nal_start = nal_end;
    }

    *outSize = (int)(pbuf - bufOut);
	if (*outSize == 0)
	{
		SDLOG_PRINTF("H264Codec", SD_LOG_LEVEL_ERROR, "Invalid frame stream found with len:%d!", inSize);
		return false;
	}
	else
	{
		return true;
	}
}

AVPixelFormat GetInputFormat(video_input_format eVideoInputFormat)
{
	switch(eVideoInputFormat)
	{
	case VIDEO_INPUT_YUV420P:
		return AV_PIX_FMT_YUV420P;
	case VIDEO_INPUT_YUYV422:
		return AV_PIX_FMT_YUYV422;
	case VIDEO_INPUT_RGB24:
		return AV_PIX_FMT_RGB24;
	case VIDEO_INPUT_BGR24:
		return AV_PIX_FMT_BGR24;
	case VIDEO_INPUT_YUV422P:
		return AV_PIX_FMT_YUV422P;
	case VIDEO_INPUT_YUV444P:
		return AV_PIX_FMT_YUV444P;
	case VIDEO_INPUT_NV12:
		return AV_PIX_FMT_NV12;
	case VIDEO_INPUT_NV21:
		return AV_PIX_FMT_NV21;
	case VIDEO_INPUT_ARGB:
		return AV_PIX_FMT_ARGB;
	case VIDEO_INPUT_RGBA:
		return AV_PIX_FMT_RGBA;
	case VIDEO_INPUT_ABGR:
		return AV_PIX_FMT_ABGR;
	case VIDEO_INPUT_BGRA:
		return AV_PIX_FMT_BGRA;
	default:
		SDLOG_PRINTF("Enc", SD_LOG_LEVEL_ERROR, "Input video format:%d is not support!", eVideoInputFormat);
		return AV_PIX_FMT_NONE;
	}

}
