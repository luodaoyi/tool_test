#pragma once

#include <windows.h>

#include "NonCopyable.h"
#include <functional>

class CInlineHook : public CNonCopyable
{
public:
	CInlineHook(DWORD dwHookAddr,DWORD dwMyAddr,DWORD dwNopCount);
	~CInlineHook();

	bool Hook();
	bool UnHook();

	DWORD GetHookAddr() const;
	typedef BOOL(WINAPI
		* FnVirtualProtect)(
			LPVOID lpAddress,
			SIZE_T dwSize,
			DWORD  flNewProtect,
			PDWORD lpflOldProtect);
	void SetVirtualProtectFunc(FnVirtualProtect func);
private:
	DWORD m_RealHookAddr = 0;
	DWORD m_RealMyFuncAddr = 0;

	const DWORD m_HookAddr = 0;
	const DWORD m_MyFuncAddr = 0;

	DWORD m_dwNopCount = 0;
	enum { max_save_size = 256 };
	BYTE m_SavedSrcCode[max_save_size];
	DWORD m_SaveSrcCodeSize = 0;
	bool m_NakeFuncChangeed = false;

	DWORD GetJumpCostByte();
	DWORD GetMyNakedFunctNopPos();
	void WriteHook();

	bool m_has_hook = false;
	bool m_hook_addr_is_call =false;
	DWORD m_hook_addr_call_addr = 0;


	FnVirtualProtect FnVirtualProtect_ = NULL;
};

