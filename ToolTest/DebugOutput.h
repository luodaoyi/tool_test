#pragma once



void SetDbgOutput(bool b);
void OutputDebugStr(const wchar_t * buffer, ...);
void OutputDebugStrA(const char * buffer, ...);

void DebugShowMsg(const wchar_t * buffer, ...);

void StdCout(const wchar_t * buffer);