#include "stdafx.h"
#include "TimeTool.h"


#include <chrono>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <locale>

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
		return mktime(&tm);
	}

}