#pragma once
#include <ctime>
#include <string>
#include <windows.h>
namespace time_tool
{
	bool GetTimeFromString(const std::wstring & s, time_t & ret_time);
	std::wstring TimeToString(const time_t time);
	SYSTEMTIME GetCurTime();
	std::wstring SecondToString(const time_t time);
	SYSTEMTIME GetTimeStruct(time_t time);
}