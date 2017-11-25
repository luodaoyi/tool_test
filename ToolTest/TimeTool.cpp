#include "stdafx.h"
#include "TimeTool.h"


#include <chrono>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
namespace time_tool
{
	std::time_t GetTimeFromString(const std::string & s, const char * format_str)
	{
		std::tm tm = {};
		std::istringstream str_stream(s);
		str_stream >> std::get_time(&tm, format_str);
		return std::mktime(&tm);
	}

}