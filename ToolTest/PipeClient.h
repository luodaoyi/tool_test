#pragma once
#include <windows.h>
#include <vector>
class CPipeClient
{
public:
	CPipeClient(LPCTSTR pipe_name);
	~CPipeClient();
	void Close();
	BOOL SendBuff(unsigned char* buff, size_t size);
	BOOL RecvBuff(unsigned char * buff, size_t size,size_t * out_size);
	BOOL SendAndRecv(unsigned char * buff, size_t send_size, unsigned char * buff_recv, size_t recv_size,size_t * pout_size);
protected:
	BOOL Connect();
	BOOL RawSendBuffer(unsigned char* buffer, size_t size,BOOL with_connect = TRUE);
private:
	TCHAR m_name[MAX_PATH];
	HANDLE m_handle = INVALID_HANDLE_VALUE;
	unsigned m_conn_time_out = 0;
};

