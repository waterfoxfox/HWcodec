//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDCommon
//* 内容摘要：基础函数
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：
//***************************************************************************/
#include "SDCommon.h"
#include "SDLog.h"

#ifdef WIN32
#define OS_WINDOWS WIN32
#include <windows.h>
#include <time.h>
#include <Tlhelp32.h>
#include <mmsystem.h>
#pragma comment( lib,"winmm.lib")
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#endif

void SD_Sleep(DWORD dwMilliseconds)
{
#ifdef WIN32
	timeBeginPeriod(1);
	Sleep(dwMilliseconds);
	timeEndPeriod(1);
#else
	// usleep(dwMilliseconds*1000);
	// POSIX has both a usleep() and a nanosleep(), but the former is deprecated,
	// so we use nanosleep() even though it has greater precision than necessary.
	struct timespec ts;
	ts.tv_sec = dwMilliseconds / 1000;
	ts.tv_nsec = (dwMilliseconds % 1000) * 1000000;
	int ret = nanosleep(&ts, NULL);
	if (ret != 0) 
	{
		static int nCount = 0;
		if ((nCount % 5000) == 0)
		{
			SDLOG_PRINTF("common", SD_LOG_LEVEL_ERROR, "nanosleep() returning early!!!");
		}
		nCount++;
		
		usleep(dwMilliseconds*1000);
	}
#endif
}


//获取当前时间毫秒
long SD_GetTickCount(void)
{
	long currentTime = 0;
#ifdef WIN32
	timeBeginPeriod(1);
	currentTime = timeGetTime();
	timeEndPeriod(1);
#else
#ifdef __APPLE__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	currentTime = (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
#endif
	return currentTime;
}
