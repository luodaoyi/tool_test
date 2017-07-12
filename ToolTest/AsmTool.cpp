#include "stdafx.h"
#include "AsmTool.h"

#include "DebugOutput.h"



namespace asm_tool
{
	ReadMemoryType s_ReadMemoryType = Read_Pointer;

	template<typename ReturnValueType>
	ReturnValueType ReadMemoryValue(_In_ UINT_PTR dwAddr)
	{
		ReturnValueType Value = 0;
		if (s_ReadMemoryType == Read_Pointer)
		{
			if ((dwAddr == 0x0 || IsBadCodePtr(FARPROC(dwAddr))))
				return NULL;

			Value = *(ReturnValueType*)dwAddr;
		}
		else if (s_ReadMemoryType == Read_API)
		{
			::ReadProcessMemory(::GetCurrentProcess(), (LPCVOID)dwAddr, (LPVOID)&Value, sizeof(Value), NULL);
		}
		return Value;
	}




	DWORD64 ReadDWORD64(DWORD64 dwAddr)
	{
		return ReadMemoryValue<DWORD64>(static_cast<DWORD>(dwAddr));
	}


	UINT_PTR ReadDWORD(UINT_PTR dwAddr)
	{
		return ReadMemoryValue<UINT_PTR>(dwAddr);
	}


	WORD ReadWORD(UINT_PTR dwAddr)
	{
		return ReadMemoryValue<WORD>(dwAddr);
	}

	BYTE ReadBYTE(UINT_PTR dwAddr)
	{
		return ReadMemoryValue<BYTE>(dwAddr);
	}

	float ReadFloat(UINT_PTR dwAddr)
	{
		return ReadMemoryValue<float>(dwAddr);
	}

	double ReadDouble(UINT_PTR dwAddr)
	{
		return ReadMemoryValue<double>(dwAddr);
	}

	BOOL WriteDWORD(UINT_PTR dwAddr, DWORD dwValue)
	{
		__try
		{
			if (s_ReadMemoryType == Read_Pointer)
			{
				if ((dwAddr == 0x0 || IsBadCodePtr(FARPROC(dwAddr))))
					return NULL;

				DWORD dwOldProtect = NULL;
				::VirtualProtect((LPVOID)dwAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
				*(UINT_PTR*)dwAddr = dwValue;

				if (dwOldProtect != NULL)
					::VirtualProtect((LPVOID)dwAddr, 4, dwOldProtect, &dwOldProtect);
			}
			else if (s_ReadMemoryType == Read_API)
			{
				DWORD dwOldProtect = NULL;
				::VirtualProtect((LPVOID)dwAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
				::WriteProcessMemory(::GetCurrentProcess(), (LPVOID)dwAddr, (LPCVOID)&dwValue, sizeof(DWORD), NULL);

				if (dwOldProtect != NULL)
					::VirtualProtect((LPVOID)dwAddr, 4, dwOldProtect, &dwOldProtect);
			}

			return TRUE;
		}
		_except(EXCEPTION_EXECUTE_HANDLER)
		{
			OutputDebugStr(L"WriteDWORD出现异常");
		}
		return FALSE;
	}

	BOOL WriteFloat(UINT_PTR dwAddr, float fValue)
	{
		__try
		{
			if (s_ReadMemoryType == Read_Pointer)
			{
				if ((dwAddr == 0x0 || IsBadCodePtr(FARPROC(dwAddr))))
					return NULL;

				DWORD dwOldProtect = NULL;
				::VirtualProtect((LPVOID)dwAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
				*(float*)dwAddr = fValue;

				if (dwOldProtect != NULL)
					::VirtualProtect((LPVOID)dwAddr, 4, dwOldProtect, &dwOldProtect);
			}
			else if (s_ReadMemoryType == Read_API)
			{
				::WriteProcessMemory(::GetCurrentProcess(), (LPVOID)dwAddr, (LPCVOID)&fValue, sizeof(fValue), NULL);
			}

			return TRUE;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			OutputDebugStr(L"WriteFloat出现异常");
		}
		return FALSE;
	}

	BOOL WriteBYTE(UINT_PTR dwAddr, BYTE bValue)
	{
		__try
		{
			if (s_ReadMemoryType == Read_Pointer)
			{
				if ((dwAddr == 0x0 || IsBadCodePtr(FARPROC(dwAddr))))
					return NULL;

				DWORD dwOldProtect = NULL;
				::VirtualProtect((LPVOID)dwAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
				*(float*)dwAddr = bValue;

				if (dwOldProtect != NULL)
					::VirtualProtect((LPVOID)dwAddr, 4, dwOldProtect, &dwOldProtect);
			}
			else if (s_ReadMemoryType == Read_API)
			{
				::WriteProcessMemory(::GetCurrentProcess(), (LPVOID)dwAddr, (LPCVOID)&bValue, sizeof(bValue), NULL);
			}

			return TRUE;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			OutputDebugStr(L"WriteFloat出现异常");
		}
		return FALSE;
	}
};