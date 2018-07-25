//**************************************************************************
//* 版权所有
//*
//* 文件名称：SDMutex
//* 内容摘要：跨平台Mutex
//* 当前版本：V1.0
//*	修	  改：		
//* 作    者：mediapro
//* 完成日期：
//***************************************************************************/

#include "SDMutex.h"
#include "SDCommon.h"
//#include "SdEvent.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSDMutex::CSDMutex(void *cs)
{
	if (cs == NULL)
	{
		return;
	}
	m_cs = cs;
#ifdef WIN32
	EnterCriticalSection((CRITICAL_SECTION*)m_cs);
#else
	pthread_mutex_lock((pthread_mutex_t *)m_cs);
#endif
}

CSDMutex::~CSDMutex()
{
	if (m_cs == NULL)
	{
		return;
	}
#ifdef WIN32
	LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#else
	pthread_mutex_unlock((pthread_mutex_t *)m_cs);
#endif
}

void *CSDMutex::CreateObject()
{
#ifdef WIN32
	CRITICAL_SECTION *lpCriticalSection = new CRITICAL_SECTION;
	InitializeCriticalSection(lpCriticalSection);
	return (void*)lpCriticalSection;
#else
	pthread_mutex_t *pmutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(pmutex, NULL);
	return (void*)pmutex;
#endif
}

#ifdef ANDROID
bool CSDMutex::lock()
{
	if (m_cs == NULL)
	{
		return FALSE;
	}

	pthread_mutex_lock((pthread_mutex_t *)m_cs);
	
	return TRUE;
}

bool CSDMutex::Unlock()
{
	if (m_cs == NULL)
	{
		return FALSE;
	}

	pthread_mutex_unlock((pthread_mutex_t *)m_cs);
	return TRUE;
}
#endif // ANDROID


void CSDMutex::RealseObject(void*m)
{
	if ( m == NULL )
		return;
	{
		CSDMutex cs(m);
	}
#ifdef WIN32
	CRITICAL_SECTION *lpCriticalSection = (CRITICAL_SECTION*)m;
	DeleteCriticalSection(lpCriticalSection);
	delete lpCriticalSection;

#else
	pthread_mutex_t *pmutex = (pthread_mutex_t*)m;
	pthread_mutex_destroy(pmutex);
	free(pmutex);
#endif
}


CSDMutexX::CSDMutexX()
{
	m_cs = CSDMutex::CreateObject();
}

CSDMutexX::~CSDMutexX()
{
	CSDMutex::RealseObject(m_cs);
	m_cs = NULL;
}

bool CSDMutexX::lock()
{
	if (m_cs == NULL)
	{
		return FALSE;
	}
#ifdef WIN32
	EnterCriticalSection((CRITICAL_SECTION*)m_cs);
#else
	pthread_mutex_lock((pthread_mutex_t *)m_cs);
#endif
	return TRUE;
}

bool CSDMutexX::Unlock()
{
	if (m_cs == NULL)
	{
		return FALSE;
	}
#ifdef WIN32
	LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#else
	pthread_mutex_unlock((pthread_mutex_t *)m_cs);
#endif
	return TRUE;
}

