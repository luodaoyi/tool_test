#pragma once
#include <Windows.h>
#include <vector>

class NakedHook
{
public:
	void CreateHook(PVOID hook_addr, PVOID nake_func_addr, DWORD nop_count, bool jump_back = true);
	void Hook();
	void UnHook();
private:
	DWORD GetSrcNakeCodeSizeAndNopOffset(PVOID nake_func_addr, DWORD& nop_offset);
	struct FixItem {
		DWORD code_offset;
		DWORD real_dest_addr;
	};
	std::vector<FixItem> GetSrcNakeCodeFixItems(PVOID src_nake_code, DWORD len);
	void FixRealAddr(DWORD& naked_addr);
private:
	PVOID shell_code_ = nullptr;
	PVOID hook_addr_ = nullptr;
	DWORD nop_count_ = 0;
	std::vector<BYTE> hook_headr_code_data_;
};