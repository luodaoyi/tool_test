
#ifndef UnitLogH
#define UnitLogH

#include <string>
<<<<<<< HEAD
#include <windows.h>
=======

>>>>>>> b6511b88ba75d001a9609e276e567adc4089cb00
#include <mutex>
#include <sstream>


<<<<<<< HEAD

=======
#include <WinSock2.h>
#include <windows.h>
>>>>>>> b6511b88ba75d001a9609e276e567adc4089cb00

class CSimpleLog
{
public:
	CSimpleLog();
    ~CSimpleLog();
public:
	enum severity_level
	{
		notice, //Ò»°ãÊä³ö
		error, //´íÎó
	};
public:
	static CSimpleLog & GetInstance()
	{
		static CSimpleLog log;
		return log;
	}
public:
	void Log(const  std::wstring & str);
	void LogFmt(const wchar_t * buffer, ...);
	void SetPipe(int index,const std::wstring & host = L"." );
	void SetFile(const std::wstring & file_name);
<<<<<<< HEAD
=======
	void SetUdp(int index, const std::string & ip);
>>>>>>> b6511b88ba75d001a9609e276e567adc4089cb00
	std::wstring GetFileName() const;
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
			m_log.Log(ss.str());
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
<<<<<<< HEAD
=======
	void SendUdp(const std::wstring & s);
>>>>>>> b6511b88ba75d001a9609e276e567adc4089cb00
	bool OpenFile();
	bool ConnectPipe();
	void WriteFile(LPCVOID  pData, size_t size);
	std::wstring GetFileLineHead();
	std::wstring GetPipeLineHead();
private:
	std::mutex m_mutex;
<<<<<<< HEAD
	HANDLE m_file_handle;
	HANDLE m_pipe;
=======
	HANDLE m_file_handle = INVALID_HANDLE_VALUE;
	HANDLE m_pipe = INVALID_HANDLE_VALUE;
>>>>>>> b6511b88ba75d001a9609e276e567adc4089cb00
	bool m_pipe_switch = false;
	std::wstring m_pipe_name;
	std::wstring m_file_name;
	bool m_is_connected = false;
<<<<<<< HEAD
=======

	sockaddr_in recv_addr = { 0 };

	bool m_udp_switch = false;
	typedef UINT_PTR        SOCKET;
	SOCKET udp_socket;

>>>>>>> b6511b88ba75d001a9609e276e567adc4089cb00
	CSimpleLog(const CSimpleLog &) = delete;
	void operator ==(const CSimpleLog &) = delete;
};
CSimpleLog::CRecordPump MakeRecordPump(CSimpleLog & log, CSimpleLog::severity_level l);

#define SIMPLE_LOG(log)  MakeRecordPump(log).GetStream()
#define SIMPLE_LOG_LEVEL(log,l)  MakeRecordPump(log,CSimpleLog::l).GetStream()
#define SIMPLE_LOG_MEMBER(l) MakeRecordPump(m_log,CSimpleLog::l).GetStream()
#define GLOG(l) MakeRecordPump(CSimpleLog::GetInstance(),CSimpleLog::l).GetStream()
#endif
