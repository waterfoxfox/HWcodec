//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDEvent
//* 内容摘要：跨平台Event
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：
//***************************************************************************/
#include "SDCommon.h"
#include "SDEvent.h"
#include "SDLog.h" 

#ifndef WIN32
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSDEvent::CSDEvent(const char *strName)
{
	m_pEvent = NULL;
	memset(m_strEventName, 0, sizeof(m_strEventName));

	if (strName)
	{
		if (strlen(strName) >= 64)
		{
			memcpy(m_strEventName, strName, 64);
			m_strEventName[63] = '\0';
		}
		else
		{
			strcpy(m_strEventName, strName);
		}
	}
	else
	{
		strcpy(m_strEventName, "DefaultName");
	}
}

CSDEvent::~CSDEvent()
{

}

BOOL CSDEvent::CreateSdEvent()
{
#ifdef WIN32
	m_pEvent = CreateEvent(NULL,true,false,NULL);
	if ( m_pEvent == NULL )
	{
		SDLOG_PRINTF("SDEvent", SD_LOG_LEVEL_ERROR, "SDEvent:%s create failed!", m_strEventName);
		return FALSE;
	}
#else
	int nRet = 0;
	m_event_status = false;
	nRet = pthread_mutex_init(&m_event_mutex, NULL);
	if (nRet != 0)
	{
		SDLOG_PRINTF("SDEvent", SD_LOG_LEVEL_ERROR, "SDEvent:%s create failed! pthread_mutex_init failed return:%d!", 
					m_strEventName, nRet);
		return FALSE;
	}

	nRet = pthread_cond_init(&m_event_cond, NULL);
	if (nRet != 0)
	{
		SDLOG_PRINTF("SDEvent", SD_LOG_LEVEL_ERROR, "SDEvent:%s create failed! pthread_cond_init failed return:%d!", 
					m_strEventName, nRet);
		pthread_mutex_destroy(&m_event_mutex);
		return FALSE;
	}
#endif

	return TRUE;
}

void CSDEvent::ReleaseSdEvent()
{
#ifdef WIN32
	if ( m_pEvent != NULL )
		CloseHandle(m_pEvent);
	m_pEvent = NULL;
#else
	pthread_mutex_destroy(&m_event_mutex);
	pthread_cond_destroy(&m_event_cond);
#endif
}

void CSDEvent::Reset()
{
#ifdef WIN32
	ResetEvent(m_pEvent);
#else
	pthread_mutex_lock(&m_event_mutex);
	m_event_status = false;
	pthread_mutex_unlock(&m_event_mutex);
#endif
}

BOOL CSDEvent::wait()
{
#ifdef WIN32
	if ( m_pEvent == NULL )
		return FALSE;

	if (WaitForSingleObject( m_pEvent, INFINITE) == WAIT_OBJECT_0)
	{
		ResetEvent(m_pEvent);
		return TRUE;
	}
	else
	{
		ResetEvent(m_pEvent);
		return FALSE;
	}

#else
	int error = 0;

	pthread_mutex_lock(&m_event_mutex);
 
	while (!m_event_status && error == 0)
		error = pthread_cond_wait(&m_event_cond, &m_event_mutex);

	// NOTE(liulk): Exactly one thread will auto-reset this event. All
	// the other threads will think it's unsignaled.  This seems to be
	// consistent with auto-reset events in WEBRTC_WIN
	if (error == 0)
		m_event_status = false;

	pthread_mutex_unlock(&m_event_mutex);

	return (error == 0) ? TRUE:FALSE;
#endif
}

BOOL CSDEvent::waittime(int milliseconds)
{
#ifdef WIN32
	if ( m_pEvent == NULL )
		return FALSE;

	DWORD ulRet = WaitForSingleObject( m_pEvent, milliseconds);
	if (ulRet == STATUS_TIMEOUT)
	{
		ResetEvent(m_pEvent);
		return FALSE;
	}
	else if (ulRet == STATUS_WAIT_0)
	{
		ResetEvent(m_pEvent);
		return TRUE;
	}
	else
	{
		SDLOG_PRINTF("SDEvent", SD_LOG_LEVEL_ERROR, "SDEvent:%s waittime failed ret:%d!", m_strEventName, ulRet);
		ResetEvent(m_pEvent);
		return FALSE;
	}
#else

	int error = 0;
	struct timespec ts;

	// Converting from seconds and microseconds (1e-6) plus
	// milliseconds (1e-3) to seconds and nanoseconds (1e-9).
	struct timeval tv;
	gettimeofday(&tv, NULL);

	ts.tv_sec = tv.tv_sec + (milliseconds / 1000);
	ts.tv_nsec = tv.tv_usec * 1000 + (milliseconds % 1000) * 1000000;

	// Handle overflow.
	if (ts.tv_nsec >= 1000000000) 
	{
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}

	pthread_mutex_lock(&m_event_mutex);

	while (!m_event_status && error == 0) 
	{
		error = pthread_cond_timedwait(&m_event_cond, &m_event_mutex, &ts);
	}

	// NOTE(liulk): Exactly one thread will auto-reset this event. All
	// the other threads will think it's unsignaled.  This seems to be
	// consistent with auto-reset events in WEBRTC_WIN
	if (error == 0)
		m_event_status = false;

	pthread_mutex_unlock(&m_event_mutex);

	return (error == 0) ? TRUE:FALSE;
#endif
}

BOOL CSDEvent::post()
{
#ifdef WIN32
	if (m_pEvent == NULL)
	{
		SDLOG_PRINTF("SDEvent", SD_LOG_LEVEL_ERROR, "SDEvent:%s post failed point is null!!", m_strEventName);
		return FALSE;
	}
	
	BOOL nRet = SetEvent(m_pEvent);
	if (nRet == FALSE)
	{
		SDLOG_PRINTF("SDEvent", SD_LOG_LEVEL_ERROR, "SDEvent:%s SetEvent failed!!", m_strEventName);
	}
	return nRet;
#else
	int nRet = 0;
	pthread_mutex_lock(&m_event_mutex);
	m_event_status = true;
	nRet = pthread_cond_broadcast(&m_event_cond);
	pthread_mutex_unlock(&m_event_mutex);
	if (nRet == 0)
	{
		return TRUE;
	}
	else
	{
		SDLOG_PRINTF("SDEvent", SD_LOG_LEVEL_ERROR, "SDEvent:%s SetEvent failed return:%d!!", m_strEventName, nRet);
		return FALSE;
	}
#endif
}
