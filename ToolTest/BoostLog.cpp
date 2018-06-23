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

#include <boost/log/sinks//debug_output_backend.hpp>
#include <boost/core/null_deleter.hpp>
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


	template< typename CharT >
	class basic_indexed_debug_output_backend :
		public sinks::basic_formatted_sink_backend< CharT, sinks::concurrent_feeding >
	{
		//! Base type
		typedef sinks::basic_formatted_sink_backend< CharT, sinks::concurrent_feeding > base_type;
	private:
		const std::wstring m_pipe_name;
		HANDLE m_pipe = INVALID_HANDLE_VALUE;
		bool m_is_connected = false;
		bool Connnect()
		{
			//先关闭之前的pipe
			if (m_pipe && m_pipe != INVALID_HANDLE_VALUE)
				::CloseHandle(m_pipe);

			m_pipe = CreateFile(m_pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (m_pipe != INVALID_HANDLE_VALUE)
			{
				DWORD dwMode = PIPE_READMODE_MESSAGE;
				if (!SetNamedPipeHandleState(m_pipe, &dwMode, NULL, NULL))
				{
					OutputDebugStr(L"SetNamedPipeHandleState failed!");
					::CloseHandle(m_pipe);
					m_pipe = INVALID_HANDLE_VALUE;
					m_is_connected = false;
				}
				else
					m_is_connected = true;
			}
			else
			{
				if (::GetLastError() == ERROR_PIPE_BUSY)
					OutputDebugStr(L"ERROR_PIPE_BUSY  busy!");
				m_is_connected = false;
			}
				
			return m_pipe != INVALID_HANDLE_VALUE;
		}
	public:
		//! Character type
		typedef typename base_type::char_type char_type;
		//! String type to be used as a message text holder
		typedef typename base_type::string_type string_type;

	public:
		/*!
		* Constructor. Initializes the sink backend.
		*/
		basic_indexed_debug_output_backend(unsigned index) : m_pipe_name(L"\\\\.\\pipe\\zds_debug_" + std::to_wstring(index))
		{
			OutputDebugStr(L"basic_indexed_debug_output_backend 构造:%s", m_pipe_name.c_str());
			//Connnect();
		}
		/*!
		* Destructor
		*/
		~basic_indexed_debug_output_backend()
		{
			OutputDebugStr(L"basic_indexed_debug_output_backend 析构");
			if (m_pipe && m_pipe != INVALID_HANDLE_VALUE)
				::CloseHandle(m_pipe);
		}

		/*!
		* The method passes the formatted message to debugger
		*/
		BOOST_LOG_API void consume(boost::log::record_view const& rec, string_type const& formatted_message)
		{
			bool can_write = false;
			if (!m_is_connected)
			{
				if (Connnect())
					can_write = true;
				else
					can_write = false;
			}
			else
				can_write = true;

			if (!can_write)
				return;

			DWORD temp = 0;
			if (!WriteFile(m_pipe, formatted_message.c_str(), static_cast<DWORD>( formatted_message.size() * sizeof(char_type)), &temp, NULL))
				m_is_connected = false;
		}
	};



	//宽字符要加模版
	template< typename CharT, typename TraitsT >
	inline std::basic_ostream< CharT, TraitsT >& operator<< (
		std::basic_ostream< CharT, TraitsT >& strm, severity_level lvl)
	{
		static const char* const str[] =
		{
			"N",
			"W",
			"E",
			"C"
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
		logging::add_common_attributes();

		logging::core::get()->add_global_attribute("ThreadID", attrs::current_thread_id());
		boost::shared_ptr< sinks::synchronous_sink< sinks::text_file_backend > > sink = logging::add_file_log
			(
			szFileName,
			keywords::open_mode = std::ios_base::app,//追加方式
			keywords::auto_flush = is_auto_flush,
			keywords::rotation_size = 10 * 1024 * 1024,
			//boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(boost::gregorian::greg_day(1)),//每月1号换日志文件
			keywords::format = expr::stream
			<< expr::format_date_time(timestamp, "%Y-%m-%d, %H:%M:%S")
			<< "[" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID") << "]"
			<< " <" << severity.or_default(notice)<< "> " 
			<< expr::message
			);
		std::locale loc = boost::locale::generator()("en_US.UTF-8");
		sink->imbue(loc);
		//sink->set_filter(severity >= warning);

		logging::core::get()->set_filter(severity >= warning);
	}


	void InitDebugShow(unsigned index ) 
	{
		static bool is_init = false;
		if (is_init)
			return;
		is_init = true;
		typedef sinks::synchronous_sink< basic_indexed_debug_output_backend<wchar_t>> debug_output_sync_sink_t;
		boost::shared_ptr<basic_indexed_debug_output_backend<wchar_t>> backend_ptr = boost::make_shared<basic_indexed_debug_output_backend<wchar_t>>(index);
		boost::shared_ptr<debug_output_sync_sink_t> debutg_output_sink(new debug_output_sync_sink_t(backend_ptr));
		// 		std::locale loc2 = std::locale("");
		//  		debutg_output_sink->imbue(loc2);
		debutg_output_sink->set_formatter(expr::stream
			<< "[" << expr::format_date_time(timestamp, L"%H:%M:%S") << "]"
			//<< "[" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID") << "]"
			<< "[" << severity.or_default(notice) << "]"
			<< expr::message
			);
		
		logging::core::get()->add_sink(debutg_output_sink);
		//debutg_output_sink->set_filter(severity >= warning);
		OutputDebugStr(L"InitBoostLog InitInitDebugShow Success!");
	}


	void InitStdout()
	{
		static bool is_init = false;
		if (is_init)
			return;
		is_init = true;
		typedef sinks::synchronous_sink< sinks::wtext_ostream_backend > stdout_sync_sink_t;
		boost::shared_ptr<sinks::wtext_ostream_backend> backend_ptr = boost::make_shared<sinks::wtext_ostream_backend>();
		backend_ptr->add_stream(boost::shared_ptr< std::wostream >(&std::wcout, boost::null_deleter()));
		boost::shared_ptr<stdout_sync_sink_t> debutg_output_sink(new stdout_sync_sink_t(backend_ptr));
		// 		std::locale loc2 = std::locale("");
		//  		debutg_output_sink->imbue(loc2);
		debutg_output_sink->set_formatter(expr::stream
			<< "[" << expr::format_date_time(timestamp, L"%H:%M:%S") << "]"
			//<< "[" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID") << "]"
			<< "[" << severity.or_default(notice) << "]"
			<< expr::message
			);

		logging::core::get()->add_sink(debutg_output_sink);
		//debutg_output_sink->set_filter(severity >= warning);
		OutputDebugStr(L"InitBoostLog InitInitDebugShow Success!");
	}

	void SetGlobalFilter(const severity_level min_level)
	{
		logging::core::get()->set_filter(severity >= min_level);
	}

	void ResetFilter()
	{
		logging::core::get()->reset_filter();
	}



	void Flush()
	{
		if (!is_auto_flush)
			logging::core::get()->flush();
	}

	void LogW(severity_level level, const wchar_t * wszBuff)
	{
		BOOST_LOG_SEV(my_logger::get(), level) << wszBuff;
	}
	void LogA(severity_level mode, const char * szBuff)
	{
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

