#pragma once

#include <boost/log/common.hpp>

namespace boost_log
{
	//����ǧ���ܸģ���������Ԫ�أ��޸�˳���
	enum severity_level
	{
		notice, //һ�����
		warning, //����
		error, //����
		critical //������Ҫ��
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

	void Flush();

}


BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, boost::log::sources::wseverity_logger_mt< boost_log::severity_level >)


#define LOGW(n) BOOST_LOG_SEV(my_logger::get(), boost_log::n)
#define LOGW_FMT(LEVEL,BUFFER,...) boost_log::LogFmtW(boost_log::LEVEL,BUFFER,## __VA_ARGS__)
#define LOGW_ERROR(BUFF) boost_log::LogW(boost_log::error,BUFF)
#define LOGW_NOTICE(BUFF) boost_log::LogW(boost_log::notice,BUFF)

#define LOG_ASSERT(X) if( !(X) ) {LOGW(error)<<L"ASSERT FAILED  "<< __FUNCTIONW__ << L"  ("<< L#X << L")";  } 