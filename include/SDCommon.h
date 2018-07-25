////////////////////////////////////////////////////////////////////////////////
//  File Name: SDCommon.h
//
//  Functions:
//     公共头文件.
//
//  History:
//  Date        Modified_By  Reason  Description
//
////////////////////////////////////////////////////////////////////////////////

#if !defined(COMMON_H)
#define COMMON_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#ifdef WIN32
#include "WINSOCK2.H"
#include "WTYPES.H"
#include "WINBASE.H"
#include <io.h>
#pragma  warning(disable:4081);
#pragma  warning(disable:4996);
#pragma comment(lib, "ws2_32.lib")

#else

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#ifndef __APPLE__
#include <net/if_arp.h>
typedef int BOOL;
#else
#include <objc/objc.h>	
#endif
#include <net/if.h>
#include <errno.h>
#include <fcntl.h>

typedef int INT;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef long  LONG;
typedef unsigned short USHORT;
typedef USHORT WORD;
typedef unsigned long ULONG;
typedef short SHORT;
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif	
#endif

// 功能返回成功码
#define   RET_SUCCESS     0          
#define   RET_FAILED	 -1	

#ifdef ANDROID
#include <android/log.h>
#include <jni.h>
//#define max(a,b)    (((a) > (b)) ? (a) : (b))
//#define min(a,b)    (((a) < (b)) ? (a) : (b))
#else
#include <map>
#include <vector>
#include <list>
#endif

class CSDLog;
extern CSDLog g_mglog;

typedef int (*CallBack)(void);
typedef int (*CallBack1)(void*pParam);
typedef int (*CallBack2)(void*pParam, void *pObj);


/*
 *  获取系统当前时间，精确到毫秒
 */
long SD_GetTickCount(void);

/*
 *  阻塞线程执行多长时间，时间单位为毫秒（跟系统的sleep类似）
 */
void SD_Sleep(DWORD dwMilliseconds);

#endif // COMMON_H
