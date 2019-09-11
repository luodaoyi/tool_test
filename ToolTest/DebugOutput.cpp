
#include "DebugOutput.h"

#include <windows.h>
#include <stdio.h>



#include "StringTool.h"
#include <mutex>
#include <iostream>
//#define DEBUG_STR_HEADER L"[ZDSDEBUG]"
//#define DEBUG_STR_HEADERA "[ZDSDEBUG]"

static std::wstring g_header = L"[ZDSDEBUG]";
static std::string g_headerA = "[ZDSDEBUG]";
static bool g_debug_show = true;

void InitDebugHeader(LPCWSTR header)
{
	g_header = header;
	g_headerA = string_tool::WideToChar(header);
}

void SetDbgOutput(bool b)
{
	g_debug_show = b;
}

#define MAX_DEBUG_STRING 5120

void DebugStr(const WCHAR * buffer, va_list pArgList)
{
	WCHAR temp[MAX_DEBUG_STRING] = { 0 };
	vswprintf_s(temp, buffer, pArgList);
	wstring debug_str = g_header + L"[" + std::to_wstring(GetCurrentThreadId()) + L"]" + temp + L"\n";
	OutputDebugStringW(debug_str.c_str());
}
void DebugStrA(const char * buffer, va_list pArgList)
{
	char temp[MAX_DEBUG_STRING] = { 0 };
	vsprintf_s(temp, buffer, pArgList);
	string debug_str = g_headerA + "[" + std::to_string(GetCurrentThreadId()) + "]" + temp + "\n";
	OutputDebugStringA(debug_str.c_str());
}
VOID OutputDebugStr(const WCHAR * buffer, ...)
{
	if (!g_debug_show) return;
	va_list pArgList;
	va_start(pArgList, buffer);
	DebugStr(buffer, pArgList);
	va_end(pArgList);
}
VOID OutputDebugStrA(const char * buffer, ...)
{
	if (!g_debug_show) return;
	va_list pArgList;
	va_start(pArgList, buffer);
	DebugStrA(buffer, pArgList);
	va_end(pArgList);
}
VOID DebugShowMsg(const WCHAR * buffer, ...)
{
#ifdef _DEBUG
	if (!g_debug_show) return;
	va_list pArgList;
	va_start(pArgList, buffer);
	DebugStr(buffer, pArgList);
	va_end(pArgList);
#endif
}

VOID StdCout(const WCHAR * buffer)
{
	static std::mutex m;

	std::lock_guard<std::mutex> l(m);
	std::wcout << buffer << std::endl;
}