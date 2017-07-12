#pragma once

#include <boost/log/common.hpp>

namespace boost_log
{
	enum severity_level
	{
		normal,
		notification,
		warning,
		error,
		critical
	};

	void InitBoostLog(const wchar_t * szFileName, bool auto_flush = false);
	boost::log::sources::wseverity_logger< severity_level > & GetLogSrcW();

	void LogW(severity_level mode, const wchar_t * wszBuff);
	void LogA(severity_level mode, const char * szBuff);

	void LogFmtW(severity_level mode, const wchar_t * wszBuff, ...);
	void LogFmtA(severity_level mode, const char * szBuff, ...);

	void Flush();

}


#define LOGW(n) BOOST_LOG_SEV(boost_log::GetLogSrcW(), boost_log::n)
#define LOGW_FMT(LEVEL,BUFFER,...) boost_log::LogFmtW(boost_log::LEVEL,BUFFER,## __VA_ARGS__)
#define LOGW_ERROR(BUFF) boost_log::LogW(boost_log::error,BUFF)
#define LOGW_NOTICE(BUFF) boost_log::LogW(boost_log::notification,BUFF)