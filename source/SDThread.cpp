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

#include "SDCommon.h"
#include "SDThread.h"
#include "SDLog.h"

char CSDThread::m_strPrtTag[] = "CSDThread";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSDThread::CSDThread(const char *strName)
{
	m_sdCallBack2 = NULL;
	m_pParam2 = NULL;
	m_sdCallBack = NULL;
	m_pth = NULL;
	m_pParam1 = NULL;
	m_nStatus = THRD_STATUS_CLOSE;
	m_ThreadExit = NULL;
	m_ThreadExit2 = NULL;

	memset(m_strName, 0, sizeof(m_strName));
	if (strName)
	{
		if (strlen(strName) >= 64)
		{
			memcpy(m_strName, strName, 64);
			m_strName[63] = '\0';
		}
		else
		{
			strcpy(m_strName, strName);
		}
	}
	else
	{
		strcpy(m_strName, "DefaultName");
	}

	m_exitevent.CreateSdEvent();
}

CSDThread::~CSDThread()
{
//	CloseThread();
	m_exitevent.ReleaseSdEvent();
}

BOOL CSDThread::CreateThread(CallBack2 sdCallBack, CallBack2 pThreadExit, void* pParam1, void* pParam2)
{
	DWORD ThreadID;
	if ( sdCallBack == NULL ||
		pThreadExit == NULL ||
		m_nStatus == THRD_STATUS_RUN)
	{
		SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_ERROR, "CreateThread:%s failed, status:%d!", m_strName, m_nStatus);
		return FALSE;
	}
	m_sdCallBack2 = sdCallBack;
	m_pParam2 = pParam2;
	m_ThreadExit2 = pThreadExit;
	m_pParam1 = pParam1;
	m_exitevent.Reset();
#ifdef WIN32
	m_pth = ::CreateThread(NULL, 0, ThreadProc , this, 0, &ThreadID);
	if ( m_pth == NULL )
	{
		SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_FATAL, "CreateThread:%s failed!", m_strName);
		m_sdCallBack2 = NULL;
		m_pParam2 = NULL;
		m_pParam1 = NULL;
		return FALSE;
	}
#else
	ThreadID = pthread_create((pthread_t*)&m_pth, NULL, ThreadProc, this);
	if( ThreadID != 0 )
	{
		SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_FATAL, "Create pthread:%s error!", m_strName);
		m_sdCallBack2 = NULL;
		m_pParam2 = NULL;
		m_pParam1 = NULL;
		return FALSE;
	}
	
#endif
	return TRUE;
}

BOOL CSDThread::CreateThread(CallBack1 sdCallBack, CallBack1 pThreadExit, void *pParam)
{
	DWORD ThreadID;
	if ( sdCallBack == NULL ||
		 pThreadExit == NULL ||
		 m_nStatus == THRD_STATUS_RUN)
	{
		SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_FATAL, "CreateThread:%s failed, status:%d!", m_strName, m_nStatus);
		return FALSE;
	}	
	m_sdCallBack = sdCallBack;
	m_ThreadExit = pThreadExit;
	m_pParam1 = pParam;
	m_exitevent.Reset();
#ifdef WIN32
	m_pth = ::CreateThread(NULL, 0, ThreadProc , this, 0, &ThreadID);
	if ( m_pth == NULL )
	{
		m_sdCallBack = NULL;
		m_pParam1 = NULL;
		SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_FATAL, "CreateThread:%s failed!", m_strName);
		return FALSE;
	}
#else
	ThreadID = pthread_create((pthread_t*)&m_pth, NULL, ThreadProc, this);
	if( ThreadID != 0 )
	{
		SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_FATAL, "Create pthread:%s error!", m_strName);
		m_sdCallBack = NULL;
		m_pParam1 = NULL;
		return FALSE;
	}

#endif
	return TRUE;
}

BOOL CSDThread::CloseThread()
{

	int  nRepeatCnt = 0;
	BOOL bRet = TRUE;

	if ( !isRun() ) return TRUE;
	
	if ((m_pth != NULL))
	{
		if ((m_sdCallBack != NULL) && (m_ThreadExit != NULL))
		{
			//关闭线程前执行的回调接口
			(*m_ThreadExit)(m_pParam1);
		}
		else if ((m_sdCallBack2 != NULL) && (m_ThreadExit2 != NULL))
		{
			(*m_ThreadExit2)(m_pParam1, m_pParam2);
		}
	}
	
	//线程主体已经结束
	if (m_nStatus == THRD_STATUS_CLOSE)
	{
		//让系统回收线程资源
#ifdef WIN32
		CloseHandle(m_pth);	
#else
		pthread_join((pthread_t)m_pth, NULL);  
#endif
	}

	//线程主体还未结束
	if ( m_nStatus == THRD_STATUS_RUN)
	{
		if ( m_pth != NULL )
		{
			//稍作等待
			while (m_nStatus != THRD_STATUS_CLOSE && nRepeatCnt < 8)
			{
				
				bRet = m_exitevent.waittime(200);
				if (bRet)
				{
					break;
				}

				nRepeatCnt++;
			}
			
			//若仍未退出，则强行退出(应用程序员应该避免出现本分支的情况，应该在调用本Close之前让线程退出)
			//TerminateThread的使用是非常危险的，Android平台暂不调用
			if(m_nStatus != THRD_STATUS_CLOSE)
			{
#ifdef WIN32
				TerminateThread(m_pth, 0);
				WaitForSingleObject(m_pth, 2000);
#else

#endif

				SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_FATAL, "Thread:%s force close is call,force the running thread out!", m_strName);
				bRet = FALSE;
			}
			else
			{
#ifdef WIN32
				CloseHandle(m_pth);	
#else
				pthread_join((pthread_t)m_pth, NULL);  
#endif
			}
		}
		else
		{
			SDLOG_PRINTF(m_strPrtTag, SD_LOG_LEVEL_ERROR, "Thread:%s is not closed,but m_pth is NULL!", m_strName);
		}
	}

	m_sdCallBack = NULL;
	m_sdCallBack2 = NULL;
	m_pParam1 = NULL;
	m_pParam2 = NULL;
	m_pth = NULL;
	m_ThreadExit = NULL;
	m_ThreadExit2 = NULL;
	m_nStatus = THRD_STATUS_CLOSE;
	return bRet;
}

#ifdef WIN32
DWORD CSDThread::ThreadProc(LPVOID pParam)
#else
void *CSDThread::ThreadProc(void *pParam)
#endif
{
	CSDThread *pthd = (CSDThread*)pParam;
	pthd->m_nStatus = THRD_STATUS_RUN;
#ifdef WIN32
	DWORD nRet = 0;
	if (pthd->m_sdCallBack2)
	{
		nRet = (*pthd->m_sdCallBack2)(pthd->m_pParam1, pthd->m_pParam2);
		pthd->m_nStatus = THRD_STATUS_CLOSE;
		pthd->m_exitevent.post();
	}
	else if (pthd->m_sdCallBack != NULL)
	{
		nRet = (*pthd->m_sdCallBack)(pthd->m_pParam1);
		pthd->m_nStatus = THRD_STATUS_CLOSE;
		pthd->m_exitevent.post();
	}
	return nRet;
#else
	void *pvRet = NULL;
	if (pthd->m_sdCallBack2)
	{
		pvRet = (void*)(*pthd->m_sdCallBack2)(pthd->m_pParam1, pthd->m_pParam2);
		pthd->m_nStatus = THRD_STATUS_CLOSE;
		pthd->m_exitevent.post();
	}
	else if (pthd->m_sdCallBack != NULL)
	{
		pvRet = (void*)(*pthd->m_sdCallBack)(pthd->m_pParam1);
		pthd->m_nStatus = THRD_STATUS_CLOSE;
		pthd->m_exitevent.post();
	}
	pthread_exit(0);
	return pvRet;
#endif

}

void CSDThread::SetName(const char *strName)
{
	if (strName)
	{
		if (strlen(strName) >= 64)
		{
			memcpy(m_strName, strName, 64);
			m_strName[63] = '\0';
		}
		else
		{
			strcpy(m_strName, strName);
		}
	}

}
