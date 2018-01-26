#include "stdafx.h"
#include "PipeServer.h"
#include <thread>
#include <memory>
#include "DebugOutput.h"
#include <process.h>
#define BUFF_SIZE 2048

CPipeServer::CPipeServer(LPCTSTR pipe_name, std::function<bool(unsigned char *, size_t, std::vector<unsigned char>  &)> recv_proc) : m_process_recv(recv_proc)
{
	wcscpy_s(m_pipe_name, pipe_name);
}


CPipeServer::~CPipeServer()
{
}



bool CPipeServer::Run()
{
	m_run = TRUE;
	for (;;)
	{
		HANDLE hPipe = CreateNamedPipe(m_pipe_name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES , BUFF_SIZE, BUFF_SIZE, 0, NULL);
		if (hPipe == INVALID_HANDLE_VALUE)
			return false;
		BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (fConnected)
		{
			std::thread msg_proc_thread([this](HANDLE hPipe){
				while (1)
				{
					unsigned char szBufRecv[BUFF_SIZE] = { 0 };
					DWORD dwReadSize = 0;
					BOOL bRet = ::ReadFile(hPipe, szBufRecv, BUFF_SIZE, &dwReadSize, NULL);
					if (!bRet || dwReadSize == 0)
					{
						DWORD dwLastError = ::GetLastError();
						if (dwLastError == ERROR_BROKEN_PIPE)
							;//cout << "断开连接！" << endl;
						else
							;//cout << "ReadFile Error:" << dwLastError << endl;
						break;
					}
					else
					{
						//cout << "服务器收到" << dwReadSize << "字节:" << szBufRecv << endl;
						std::vector<unsigned char> reply;
						if (m_process_recv(szBufRecv, dwReadSize, reply))
						{
							DWORD dwWritten = 0;
							bRet = WriteFile(hPipe, reply.data(), reply.size(), &dwWritten, NULL);
							if (!bRet || dwWritten == 0)
							{
								int nError = ::GetLastError();
								OutputDebugStr(L"服务器WriteFile失败，errorid:%d", nError);
								break;
							}
						}
						else
						{
							//不作写入动作
						}
					}
				}
				CloseHandle(hPipe);
			},hPipe);
			msg_proc_thread.detach();
		}
		else
			CloseHandle(hPipe);
	}
}

