#pragma once
#include <windows.h>
#include <functional>
#include <vector>

class CPipeServer
{
public:
	CPipeServer(LPCTSTR pipe_name, std::function<bool (unsigned char *, size_t,std::vector<unsigned char> & reply)>);
	~CPipeServer();
	bool Run();
private:
	wchar_t m_pipe_name[MAX_PATH];
	std::function<bool(unsigned char *, size_t, std::vector<unsigned char> & reply)> m_process_recv;
	BOOL m_run = TRUE;
};

