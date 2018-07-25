//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDThread
//* 内容摘要：跨平台Thread
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：
//***************************************************************************/

#if !defined(SDTHREAD_H)
#define SDTHREAD_H

#include "SDCommon.h"
#include "SdEvent.h"

#define  THRD_STATUS_RUN     1   // 运行状态
#define  THRD_STATUS_CLOSE   2   // 关闭状态

class CSDThread
{
public:
	CSDThread(const char* strName = NULL);
	virtual ~CSDThread();

public:
	void SetName(const char* strName);
	char* GetName() {return m_strName;};
	BOOL CreateThread(CallBack1 sdCallBack, CallBack1 pThreadExit, void* pParam);
	BOOL CreateThread(CallBack2 sdCallBack, CallBack2 pThreadExit, void* pParam1, void* pParam2);
	BOOL CloseThread();
	BOOL isRun()
	{
		return (m_nStatus != THRD_STATUS_CLOSE || m_pth != NULL) ? TRUE : FALSE;
	};

#ifdef WIN32
	static DWORD WINAPI ThreadProc(LPVOID pParam);
#else
	static void* ThreadProc(void* pParam);
#endif

private:
	CallBack1   m_sdCallBack;
	CallBack2   m_sdCallBack2;
	CallBack1   m_ThreadExit;
	CallBack2   m_ThreadExit2;
	void*       m_pParam1;
	void*       m_pParam2;
	void*       m_pth;
	char        m_strName[64];
	int         m_nStatus;
	CSDEvent    m_exitevent;
	static char m_strPrtTag[];
};

#endif // !defined(SDTHREAD_H)
