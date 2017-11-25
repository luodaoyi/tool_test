#include "stdafx.h"
#include "BoostLog.h"


#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <boost/move/utility.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/locale/generator.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/shared_ptr.hpp>


#include "StringTool.h"
#include "DebugOutput.h"

namespace boost_log
{

	namespace logging = boost::log;
	namespace src = boost::log::sources;
	namespace expr = boost::log::expressions;
	namespace sinks = boost::log::sinks;
	namespace attrs = boost::log::attributes;
	namespace keywords = boost::log::keywords;



	//宽字符要加模版
	template< typename CharT, typename TraitsT >
	inline std::basic_ostream< CharT, TraitsT >& operator<< (
		std::basic_ostream< CharT, TraitsT >& strm, severity_level lvl)
	{
		static const char* const str[] =
		{
			"normal",
			"notification",
			"warning",
			"error",
			"critical"
		};
		if (static_cast<std::size_t>(lvl) < (sizeof(str) / sizeof(*str)))
			strm << str[lvl];
		else
			strm << static_cast<int>(lvl);
		return strm;
	}


	BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
	BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)


	bool is_auto_flush = false;//此变量内部使用
	void InitBoostLog(const wchar_t * szFileName,bool auto_flush)
	{
		static bool is_init = false;
		if (is_init)
			return;
		is_init = true;
		is_auto_flush = auto_flush;
		boost::shared_ptr< sinks::synchronous_sink< sinks::text_file_backend > > sink = logging::add_file_log
			(
			szFileName,
			keywords::open_mode = std::ios_base::app,//追加方式
			keywords::auto_flush = is_auto_flush,
			boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(boost::gregorian::greg_day(1)),//每月1号换日志文件
			keywords::format = expr::stream
			<< expr::format_date_time(timestamp, "%Y-%m-%d, %H:%M:%S.%f")
			<< " <" << severity.or_default(normal)
			<< "> " << expr::message
			);
		
		std::locale loc = boost::locale::generator()("en_US.UTF-8");
		sink->imbue(loc);
		logging::add_common_attributes();
		OutputDebugStr(L"InitBoostLog Init Success!");
	}



	src::wseverity_logger< severity_level > & GetLogSrcW()
	{
		static src::wseverity_logger< severity_level > lgW;
		return lgW;
	}
	

	void Flush()
	{
		if (!is_auto_flush)
			logging::core::get()->flush();
	}

	void LogW(severity_level level, const wchar_t * wszBuff)
	{
		OutputDebugStr(wszBuff);
		BOOST_LOG_SEV(GetLogSrcW(), level) << wszBuff;
	}
	void LogA(severity_level mode, const char * szBuff)
	{
		OutputDebugStrA(szBuff);
		wstring temp = string_tool::CharToWide(szBuff);
		LogW(mode, temp.c_str());
	}
#define MAX_LOG_FMT_LEN 1024
	void LogFmtW(severity_level mode, const wchar_t * wszBuff, ...)
	{
		va_list pArgList;
		va_start(pArgList, wszBuff);
		WCHAR temp[MAX_LOG_FMT_LEN] = { 0 };
		vswprintf_s(temp, wszBuff, pArgList);
		va_end(pArgList);
		LogW(mode, temp);
	}

	void LogFmtA(severity_level mode, const char * szBuff, ...)
	{
		va_list pArgList;
		va_start(pArgList, szBuff);
		char temp[MAX_LOG_FMT_LEN] = { 0 };
		vsprintf_s(temp, szBuff, pArgList);
		va_end(pArgList);
		LogA(mode, temp);
	}
}

