#pragma once
#include <ctime>
#include <string>

namespace time_tool
{
	std::time_t GetTimeFromString(const std::wstring & s);
	std::wstring TimeToString(const time_t time);
}