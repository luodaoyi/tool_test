#include "stdafx.h"
#include "DebugOutput.h"

#include <windows.h>
#include <stdio.h>

#include "commom_include.h"

#include "StringTool.h"

#define DEBUG_STR_HEADER L"[ZDSDEBUG]"
#define DEBUG_STR_HEADERA "[ZDSDEBUG]"

#define MAX_DEBUG_STRING 5120

void DebugStr(const WCHAR * buffer, va_list pArgList)
{
	WCHAR temp[MAX_DEBUG_STRING] = { 0 };
	vswprintf_s(temp, buffer, pArgList);
	wstring debug_str = std::wstring(DEBUG_STR_HEADER) + L"[" + std::to_wstring(GetCurrentThreadId()) + L"]" + temp + L"\n";
	OutputDebugStringW(debug_str.c_str());
}
void DebugStrA(const char * buffer, va_list pArgList)
{
	char temp[MAX_DEBUG_STRING] = { 0 };
	vsprintf_s(temp, buffer, pArgList);
	string debug_str = string(DEBUG_STR_HEADERA) + "[" + std::to_string(GetCurrentThreadId()) + "]" + temp + "\n";
	OutputDebugStringA(debug_str.c_str());
}
VOID OutputDebugStr(const WCHAR * buffer, ...)
{
	va_list pArgList;
	va_start(pArgList, buffer);
	DebugStr(buffer, pArgList);
	va_end(pArgList);
}
VOID OutputDebugStrA(const char * buffer, ...)
{
	va_list pArgList;
	va_start(pArgList, buffer);
	DebugStrA(buffer, pArgList);
	va_end(pArgList);
}
