
#ifndef UnitLogH
#define UnitLogH

#include <string>

#include <mutex>
#include <sstream>


#include <WinSock2.h>
#include <windows.h>

struct sockaddr_in;

class CSimpleLog
{
public:
	CSimpleLog();
    ~CSimpleLog();
public:
	enum severity_level
	{
		debug, //调试时启用
		notice,//一般输出
		warn, //
		error, //错误
	};
public:
	static CSimpleLog & GetInstance()
	{
		static CSimpleLog log;
		return log;
	}
public:
	void Log(const  std::wstring & str,severity_level level = notice);
	void LogOnlyFile(const  std::wstring& str, severity_level level = notice);
	void LogFmt(const wchar_t * buffer, severity_level level = notice, ...);
	void SetPipe(int index,const std::wstring & host = L"." );
	void SetFile(const std::wstring & file_name);
	void SetUdp(int index, const std::string & ip);
	void SetCmd(bool b);
	std::wstring GetFileName() const;
	void SetLogMaxSize(LONGLONG max_size);
	void Close();
	void SetSeverity(const severity_level& l);
public:
	class CRecordPump
	{
	public:
		CRecordPump(CSimpleLog & log, severity_level level = notice) : m_log(log), m_level(level){}
		CRecordPump(CRecordPump && other) : m_log(other.m_log){
			ss = std::move(other.ss);
		}
		~CRecordPump()
		{
			if (m_level == severity_level::debug) {
#ifdef _DEBUG
				m_log.Log(ss.str(), m_level);
#endif
			}
			else
				m_log.Log(ss.str(), m_level);
		}
		std::wostringstream & GetStream() 
		{
			return ss;
		}
		std::wostringstream ss;
		CSimpleLog & m_log;
		severity_level m_level;
	};
private:
	void SendPipe(const std::wstring & s);
	void SendUdp(const std::wstring & s);
	bool OpenFile();
	bool ConnectPipe();
	void WriteFile(LPCVOID  pData, size_t size);
	std::wstring GetFileLineHead(severity_level level);
	std::wstring GetPipeLineHead(severity_level level);
private:
	std::mutex m_mutex;
	HANDLE m_file_handle = INVALID_HANDLE_VALUE;
	HANDLE m_pipe = INVALID_HANDLE_VALUE;
	bool m_pipe_switch = false;
	bool m_is_cmd_output = false;
	bool m_udp_switch = false;
	std::wstring m_pipe_name;
	std::wstring m_file_name;
	bool m_is_connected = false;

	std::unique_ptr< sockaddr_in> recv_addr;


	typedef UINT_PTR        SOCKET;
	SOCKET udp_socket;

	CSimpleLog(const CSimpleLog &) = delete;
	void operator ==(const CSimpleLog &) = delete;
	LONGLONG m_max_log_size = 1024 * 1024 * 60;
	severity_level min_severity_ = notice;
};
CSimpleLog::CRecordPump MakeRecordPump(CSimpleLog & log, CSimpleLog::severity_level l);

#define SIMPLE_LOG(log)  MakeRecordPump(log,CSimpleLog::notice).GetStream()
#define SIMPLE_LOG_LEVEL(log,l)  MakeRecordPump(log,CSimpleLog::l).GetStream()
#define SIMPLE_LOG_MEMBER(l) MakeRecordPump(m_log,CSimpleLog::l).GetStream()
#define GLOG(l) MakeRecordPump(CSimpleLog::GetInstance(),CSimpleLog::l).GetStream()
#endif
