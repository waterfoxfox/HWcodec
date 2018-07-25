// SDLog.h: interface for the CSDLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SDLOG_H)
#define SDLOG_H

#include "SDCommon.h"
#include "log4z.h"

using namespace zsummer::log4z;
#define SD_LOG_LEVEL_DEBUG      LOG_LEVEL_DEBUG
#define SD_LOG_LEVEL_INFO		LOG_LEVEL_INFO
#define SD_LOG_LEVEL_WARNING    LOG_LEVEL_WARN
#define SD_LOG_LEVEL_ERROR      LOG_LEVEL_ERROR
#define SD_LOG_LEVEL_FATAL      LOG_LEVEL_FATAL
#define SD_LOG_LEVEL_ALARM		LOG_LEVEL_ALARM
#define SD_LOG_LEVEL_NONE		(LOG_LEVEL_FATAL + 1)

#define SD_LOG_LEVEL_FORCE_FILE SD_LOG_LEVEL_INFO

// 参数描述(CallBack2 FucCustom, void *pObj)
#define SDLOG_INIT             g_mglog.Init

// 参数无
#define SDLOG_CLOSE            g_mglog.Close

#define SDLOG_PRINTF(module, level, fmt, ...)  LOG_FORMAT(LOG4Z_MAIN_LOGGER_ID, level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)


extern CSDLog g_mglog;


/*
 *   系统日志管理主类
 */
class CSDLog 
{
public:
	CSDLog();
	virtual ~CSDLog();

public:

	BOOL  Init(const char * outputPath, int outputLevel, const char* configFile = NULL);
	void  Close();

};

#endif // !defined(SDLOG_H)
