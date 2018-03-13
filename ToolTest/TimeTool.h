#pragma once
#include <ctime>
#include <string>
#include <windows.h>
namespace time_tool
{
	std::time_t GetTimeFromString(const std::wstring & s);
	std::wstring TimeToString(const time_t time);
	SYSTEMTIME GetCurTime();
	std::wstring SecondToString(const time_t time);
}