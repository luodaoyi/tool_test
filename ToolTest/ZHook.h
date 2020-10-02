#pragma once
#include <Windows.h>
#include <vector>
class ZHook
{
public:
	void Hook(PVOID hook_addr, PVOID detours_addr, DWORD nop_count, DWORD& fp_srouce_addr);
	void Unhook();
	LPVOID GetHookedAddrss() const;
private:
	LPVOID prv_code_ = 0;
	LPVOID hook_addr_ = 0;
	std::vector<unsigned char> saved_head_bytes_;
};