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
	void InitStdout();
	void SetGlobalFilter(const severity_level min_level);
	void ResetFilter();


	void LogW(severity_level mode, const wchar_t * wszBuff);
	void LogA(severity_level mode, const char * szBuff);

	void LogFmtW(severity_level mode, const wchar_t * wszBuff, ...);
	void LogFmtA(severity_level mode, const char * szBuff, ...);
	void LogFmtWD(severity_level mode, const wchar_t * wszBuff, ...);
	void Flush();

}


BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, boost::log::sources::wseverity_logger_mt< boost_log::severity_level >)
/*
以上宏展开为
struct my_logger
{
	typedef boost::log::sources::wseverity_logger_mt< boost_log::severity_level > logger_type;
	enum registration_line_t { registration_line = __LINE__ };
	static const char* registration_file() { return __FILE__; }
	static logger_type construct_logger();
	static inline logger_type& get()
	{
		return ::boost::log::sources::aux::logger_singleton< my_logger >::get();
	}
};
inline  my_logger::logger_type my_logger::construct_logger()
{
	return logger_type();
}
*/


#define LOGW(n) BOOST_LOG_SEV(my_logger::get(), boost_log::n)
#define LOGW_FMT(LEVEL,BUFFER,...) boost_log::LogFmtW(boost_log::LEVEL,BUFFER,## __VA_ARGS__)
#define LOGW_FMT_DEBUG(BUFFER,...) boost_log::LogFmtWD(boost_log::notice,BUFFER,## __VA_ARGS__)
#define LOGW_ERROR(BUFF) boost_log::LogW(boost_log::error,BUFF)
#define LOGW_NOTICE(BUFF) boost_log::LogW(boost_log::notice,BUFF)

#define LOG_ASSERT(X) if( !(X) ) {LOGW(error)<<L"ASSERT FAILED  "<< __FUNCTIONW__ << L"  ("<< L#X << L")";  } 

