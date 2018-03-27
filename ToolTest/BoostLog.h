#pragma once

#include <boost/log/common.hpp>

namespace boost_log
{
	//这里千万不能改，包括增加元素，修改顺序等
	enum severity_level
	{
		notice, //一般输出
		warning, //警告
		error, //错误
		critical //极其重要的
	};

	void InitBoostLog(const wchar_t * szFileName, bool auto_flush = false);
	void InitDebugShow(unsigned index = 0);
	void SetGlobalFilter(const severity_level min_level);
	void ResetFilter();
	boost::log::sources::wseverity_logger_mt< severity_level > & GetLogSrcW();

	void LogW(severity_level mode, const wchar_t * wszBuff);
	void LogA(severity_level mode, const char * szBuff);

	void LogFmtW(severity_level mode, const wchar_t * wszBuff, ...);
	void LogFmtA(severity_level mode, const char * szBuff, ...);

	void Flush();

}


#define LOGW(n) BOOST_LOG_SEV(boost_log::GetLogSrcW(), boost_log::n)
#define LOGW_FMT(LEVEL,BUFFER,...) boost_log::LogFmtW(boost_log::LEVEL,BUFFER,## __VA_ARGS__)
#define LOGW_ERROR(BUFF) boost_log::LogW(boost_log::error,BUFF)
#define LOGW_NOTICE(BUFF) boost_log::LogW(boost_log::notice,BUFF)