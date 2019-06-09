#include "stdafx.h"
#include "InlineHook.h"
#include "DebugOutput.h"

CInlineHook::CInlineHook(DWORD dwHookAddr, DWORD dwMyAddr, DWORD nopCount) : m_HookAddr(dwHookAddr), m_MyFuncAddr(dwMyAddr), m_dwNopCount(nopCount)
{

}


CInlineHook::~CInlineHook()
{
}
#pragma pack(push, 1)
typedef struct _JMPCODE_
{
	BYTE jmp;
	DWORD addr;
}JMPCODE_, *PJMPCODE_;
#pragma pack(pop)

bool CInlineHook::Hook()
{
	if (m_has_hook)
	{
		OutputDebugStr(L"m_has_hook  = true");
		return true;
	}
		
	if (!m_HookAddr || !m_MyFuncAddr)
	{
		OutputDebugStr(L"!m_HookAddr || !m_MyFuncAddr");
		return false;
	}
		

	if (*(BYTE*)m_HookAddr == 0xe9)
	{
		//特殊情况 说明这个要hook的地方就是一个 jump，例如：JMP 5A517301
		DWORD SrcJumpAddr = m_HookAddr + 5 + *(DWORD*)(m_HookAddr + 1);
		if (SrcJumpAddr == SrcJumpAddr)
			return true;//由可以是已经被HOOK过了
		else
			m_RealHookAddr = SrcJumpAddr;
	}
	else if (*(BYTE*)m_HookAddr == 0xe8)
	{
		//说明HOOK的那个地址正常是call xxxxxxx，这样自动填充的就会有问题call后面的xxxxxx得重新计算
		m_hook_addr_is_call = true;
		m_hook_addr_call_addr = *(DWORD*)(m_HookAddr + 1) + 0x5 + m_HookAddr;
		m_RealHookAddr = m_HookAddr;
	}
	else
		m_RealHookAddr = m_HookAddr;

	if (*(BYTE*)m_MyFuncAddr == 0xe9)
	{
		//保存以前的样子
		DWORD dwE9_Hook_WillGo = m_MyFuncAddr + 5 + *(DWORD*)(m_MyFuncAddr + 1);
		m_RealMyFuncAddr = dwE9_Hook_WillGo;
	}
	else
		m_RealMyFuncAddr = m_MyFuncAddr;


	m_SaveSrcCodeSize = GetJumpCostByte();//做一个JUMP实际费了多少字节(>=5)
	OutputDebugStr(L"Cost byte:%d", m_SaveSrcCodeSize);
	const DWORD dwJumpBackAddr = m_RealHookAddr + m_SaveSrcCodeSize;//自己函数跳回的地址
	if (m_SaveSrcCodeSize == 0)
		return false;

	memcpy_s(m_SavedSrcCode, max_save_size, (const void*)m_RealHookAddr, m_SaveSrcCodeSize);

	//修改自己给定的那个函数
	if (!m_NakeFuncChangeed)
	{
		const DWORD dwNopPos = GetMyNakedFunctNopPos();
		if (dwNopPos == 0)
			return false;
		if (!m_hook_addr_is_call)
		{
			if (!WriteProcessMemory(::GetCurrentProcess(), (LPVOID)dwNopPos, (LPCVOID)m_SavedSrcCode, m_SaveSrcCodeSize, NULL))
			{
				OutputDebugStr(L"WriteProcessMemory11 failed:%d", ::GetLastError());
			}
		}
			
		else
		{
			//这个机器码得计算 JMP的地址(88881234) – 代码地址(010073bb) – 5（字节） = 机器码跳转地址(E9 87879e74);
			JMPCODE_ jmpCode;
			jmpCode.jmp = 0xe8;
			jmpCode.addr = m_hook_addr_call_addr - dwNopPos - 5;
			if(!::WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwNopPos, &jmpCode, sizeof(JMPCODE_), NULL))
				OutputDebugStr(L"WriteProcessMemory22 failed:%d", ::GetLastError());
		}

		//下面开始跳转到原函数的代码
		const DWORD dwNextCodeAddr = dwNopPos + m_SaveSrcCodeSize;
		JMPCODE_ jmpCode;
		jmpCode.jmp = 0xe9;
		jmpCode.addr = dwJumpBackAddr - dwNextCodeAddr - 5;
		if(!::WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwNextCodeAddr, &jmpCode, sizeof(JMPCODE_), NULL))
			OutputDebugStr(L"WriteProcessMemory33 failed:%d", ::GetLastError());

		m_NakeFuncChangeed = true;
	}
	//下面开始正式HOOK
	WriteHook();

	m_has_hook = true;
	return true;
}


void CInlineHook::UnHook()
{
	if (m_SavedSrcCode == 0)
		return;
	DWORD old_protected = NULL;
	::VirtualProtect((LPVOID)m_RealHookAddr, 5 + m_dwNopCount, PAGE_EXECUTE_READWRITE, &old_protected);
	WriteProcessMemory(GetCurrentProcess(),
		(LPVOID)m_RealHookAddr,
		m_SavedSrcCode,
		m_SaveSrcCodeSize,
		NULL);
	m_has_hook = false;
	DWORD temp = 0;
	::VirtualProtect((LPVOID)m_RealHookAddr, 5 + m_dwNopCount, old_protected, &temp);
}

void CInlineHook::WriteHook()
{
	///////////////下面正式进行HOOK

	DWORD old_protected = NULL;
	::VirtualProtect((LPVOID)m_RealHookAddr, 5 + m_dwNopCount, PAGE_EXECUTE_READWRITE, &old_protected);


	JMPCODE_ jmpCode2;
	jmpCode2.jmp = 0xe9;
	//JMP的地址(88881234) – 代码地址(010073bb) – 5（字节） = 机器码跳转地址(E9 87879e74)
	jmpCode2.addr = (DWORD)m_RealMyFuncAddr - (DWORD)m_RealHookAddr - 5;
	BOOL bRet = ::WriteProcessMemory(GetCurrentProcess(), (LPVOID)m_RealHookAddr, &jmpCode2, sizeof(JMPCODE_), NULL);
	if (!bRet)
	{
		DWORD last_error = ::GetLastError();
		OutputDebugStr(L"写入游戏 JUMP失败:%d  地址：%x JMP目标:%x", last_error, m_RealHookAddr, m_RealMyFuncAddr);
		wchar_t msg_buffer[256];
		swprintf_s(msg_buffer, 256, L"写入游戏 JUMP失败:%d  地址：%x JMP目标:%x", last_error, m_RealHookAddr, m_RealMyFuncAddr);
		::MessageBoxW(NULL, msg_buffer, NULL, MB_OK);
	}

	BYTE nopFlag = 0x90;
	for (DWORD i = 0; i < m_dwNopCount; i++)
	{
		if(!::WriteProcessMemory(GetCurrentProcess(), (LPVOID)(m_RealHookAddr + 5 + i), &nopFlag, 1, NULL))
			OutputDebugStr(L"WriteProcessMemory444 failed:%d", ::GetLastError());
	}

	DWORD temp = 0;
	::VirtualProtect((LPVOID)m_RealHookAddr, 5 + m_dwNopCount, old_protected, &temp);

}

DWORD CInlineHook::GetMyNakedFunctNopPos()
{
	//这里要注意一点。这个0x90不一定是我们要找的地方例如下面
	//所以要比较多个0x90
	/*
	CE401380      60            PUSHAD
	CE401381      8BEC          MOV EBP,ESP
	CE401383      83C5 20       ADD EBP,20
	CE401386      8B46 10       MOV EAX,DWORD PTR DS:[ESI+10]
	CE401389      50            PUSH EAX
	CE40138A      53            PUSH EBX
	CE40138B      E8 6890E2FF   CALL Smart_Bn.CE22A3F8   //!!!!注意这里也有一个90
	CE401390      61            POPAD
	*/
	BYTE * pCode = NULL;
	int i = 0;
	const unsigned char buff[] = { 0x90,0x90,0x90,0x90,0x90 };
	for (i = 0, pCode = (BYTE*)m_RealMyFuncAddr; i < 100; i++,pCode++)
	{
		if(memcmp(pCode, buff,sizeof(buff)) == 0)
			return (DWORD)pCode;
	}
	return 0;
}

DWORD CInlineHook::GetJumpCostByte()
{
	//前面没有进行修正E9？
	if (*(BYTE*)m_RealMyFuncAddr == 0xe9)
		return 0;

	DWORD dwCostLen = 5 + m_dwNopCount;
	return dwCostLen;//一条Jump语句所耗费的字节数
}



DWORD CInlineHook::GetHookAddr() const
{
	return m_HookAddr;
}