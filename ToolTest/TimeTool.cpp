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
	std::time_t GetTimeFromString(const std::wstring & s)
	{

		struct std::tm tm = { 0 };
		if (6 != swscanf_s(s.c_str(), L"%d-%d-%d %d:%d:%d",
			&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec))
			throw std::runtime_error("error GetTimeFromString");
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
		auto ret = mktime(&tm);
		if (-1 == ret)
			throw std::bad_cast((std::string("GetTimeFromString ") + string_tool::WideToChar(s.c_str())).c_str());
		return ret;
	}

	std::wstring TimeToString(const time_t time)
	{
		tm tm = { 0 };
		localtime_s(&tm,&time);
		wchar_t time_buffer[60] = { 0 };
		//_wasctime_s(time_buffer, &tm);
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
		if (sec >= 360)
		{
			//大于一小时
			int minutes = sec % 60;
			std::wstring minutes_str;
			if (minutes > 0)
				minutes_str = std::to_wstring(minutes) + L"分";
			return std::to_wstring(sec / 60) + L"时" + minutes_str;
		}
		else if (sec >= 60)
		{
			int seccond = sec % 60;
			std::wstring seccond_str;
			if (seccond > 0)
				seccond_str = std::to_wstring(seccond) + L"秒";
			return std::to_wstring(sec / 60) + L"分" + seccond_str;
		}
		else if (sec >= 0)
			return std::to_wstring(sec) + L"秒";
		else
			return L"";
	}

}