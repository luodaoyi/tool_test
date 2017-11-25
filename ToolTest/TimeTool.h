#pragma once
#include <ctime>
#include <string>

namespace time_tool
{

	std::time_t GetTimeFromString(const std::string & s, const char * format_str);
}