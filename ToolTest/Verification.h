#pragma once
#include <string>

namespace verification
{
	
	std::string GetHearwareString();
	std::string GetMacInfo();
	std::string GetVolumnInfo();
	bool GetHdSerialNum(std::string& mode, std::string& serial);
	std::string GetCpuStr();
}