#pragma once

#include <Windows.h>

void SetDbgOutput(bool b);
VOID OutputDebugStr(const WCHAR * buffer, ...);
VOID OutputDebugStrA(const char * buffer, ...);

VOID DebugShowMsg(const WCHAR * buffer, ...);

VOID StdCout(const WCHAR * buffer);