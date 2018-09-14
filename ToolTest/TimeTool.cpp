#include "stdafx.h"
#include "TimeTool.h"


#include <chrono>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <locale>


#include <time.h>
#include "StringTool.h"

namespace time_tool
{
	bool GetTimeFromString(const std::wstring & s ,time_t & ret_time)
	{

		struct std::tm tm = { 0 };
		if (6 != swscanf_s(s.c_str(), L"%d-%d-%d %d:%d:%d",
			&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec))
			return false;
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
		ret_time = mktime(&tm);
		if (-1 == ret_time)
			return false;
		else
			return true;
	}

	SYSTEMTIME GetTimeStruct(time_t time)
	{
		tm tm = { 0 };
		localtime_s(&tm, &time);
		SYSTEMTIME ret;
		ret.wYear = tm.tm_year + 1900;
		ret.wMonth = tm.tm_mon + 1;
		ret.wDay = tm.tm_mday;
		ret.wHour = tm.tm_hour;
		ret.wMinute = tm.tm_min;
		ret.wSecond = tm.tm_sec;
		return ret;
	}

	std::wstring TimeToString(const time_t time)
	{
		tm tm = { 0 };
		localtime_s(&tm,&time);
		wchar_t time_buffer[60] = { 0 };
		swprintf_s(time_buffer, L"%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		return time_buffer;
	}

	SYSTEMTIME  GetCurTime()
	{
		SYSTEMTIME cur_local_time;
		GetLocalTime(&cur_local_time);
		return cur_local_time;
	}

	std::wstring SecondToString(const time_t sec)
	{
		int hour_count = (int)sec / 3600;
		int min_count = (int)sec % 3600 / 60;
		int second_count = sec % 60;
		return (hour_count > 0 ? (std::to_wstring(hour_count) + L" ±") : L"")
			+ ((min_count > 0 || hour_count )? (std::to_wstring(min_count) + L"∑÷") : L"")
			+ (std::to_wstring(second_count) + L"√Î");
	}
};