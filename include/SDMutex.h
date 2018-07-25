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

#if !defined(SDMUTEX_H)
#define SDMUTEX_H

class CSDMutex  
{
public:
	CSDMutex(void *cs);
	virtual ~CSDMutex();

public:
	static void *CreateObject();
	static void RealseObject(void*m);
#ifdef ANDROID
	bool lock();
	bool Unlock();
#endif // ANDROID
private:
	void *m_cs;
};

class CSDMutexX
{
public:
	CSDMutexX();
	virtual ~CSDMutexX();
	
public:
	bool lock();
	bool Unlock();
	
private:
	void *m_cs;
};




#endif // !defined(SDMUTEX_H)
