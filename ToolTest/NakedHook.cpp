#include "NakedHook.h"
#include <stdexcept>
#include <system_error>  


#pragma pack(push, 1)
typedef struct _JMPCODE_
{
	BYTE jmp;
	DWORD addr;
}JMPCODE_, * PJMPCODE_;
#pragma pack(pop)

#define THROW_SYSTEM_ERROR(X) { DWORD dwErrVal = GetLastError();\
 std::error_code ec(dwErrVal, std::system_category()); \
 throw std::system_error(ec, X); }


void NakedHook::CreateHook(PVOID hook_addr, PVOID nake_func_addr, DWORD nop_count, bool jump_back ) {
	FixRealAddr((DWORD&)nake_func_addr);
	FixRealAddr((DWORD&)hook_addr);
	//得到NakeCode Size
	DWORD nop_offset = 0;
	DWORD nake_code_size = GetSrcNakeCodeSizeAndNopOffset(nake_func_addr, nop_offset);

	//分配ShellCode空间
	const LPVOID nake_shell_code = ::VirtualAlloc(NULL, nake_code_size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!nake_shell_code)
		THROW_SYSTEM_ERROR("VirtualAlloc NakeCode Failed")
		//写入 ShellCode
		SIZE_T writes = 0;
	if (!::WriteProcessMemory(GetCurrentProcess(), nake_shell_code, nake_func_addr, nake_code_size, &writes))
		THROW_SYSTEM_ERROR("WriteProcessMemory NakeCode Failed")
		//Fix ShellCode CALL
		auto fix_items = GetSrcNakeCodeFixItems(nake_func_addr, nake_code_size);
	for (const auto& fix_item : fix_items) {
		if (*((BYTE*)nake_shell_code + fix_item.code_offset) == 0xE8) {
			//JMP的地址(88881234) – 代码地址(010073bb) – 5（字节） = 机器码跳转地址(E9 87879e74)
			DWORD machine_code = fix_item.real_dest_addr - (DWORD)((BYTE*)nake_shell_code + fix_item.code_offset) - 5;
			if (!::WriteProcessMemory(GetCurrentProcess(), (BYTE*)nake_shell_code + fix_item.code_offset + 1, &machine_code, 4, &writes))
				THROW_SYSTEM_ERROR("WriteProcessMemory FixItem Failed")
		}
	}
	std::vector<BYTE> hook_addr_save_data(5 + nop_count);
	if (jump_back) {
		//原目标函数头
		BYTE* nake_shellcode_ptr = (BYTE*)nake_shell_code + nop_offset;
		memcpy(hook_addr_save_data.data(), (LPVOID)hook_addr, 5 + nop_count);
		if (!::WriteProcessMemory(::GetCurrentProcess(), nake_shellcode_ptr, hook_addr_save_data.data(), hook_addr_save_data.size(), &writes))
			THROW_SYSTEM_ERROR("WriteProcessMemory HeadData Failed")
			nake_shellcode_ptr += hook_addr_save_data.size();
		//写JUMP回
		////JMP的地址(88881234) – 代码地址(010073bb) – 5（字节） = 机器码跳转地址(E9 87879e74)
		JMPCODE_ back_jump;
		back_jump.jmp = 0xE9;
		back_jump.addr = (DWORD)hook_addr + 5 + nop_count - (DWORD)nake_shellcode_ptr - 5;
		if (!::WriteProcessMemory(::GetCurrentProcess(), nake_shellcode_ptr, &back_jump, sizeof(JMPCODE_), &writes))
			THROW_SYSTEM_ERROR("WriteProcessMemory JumpBack Failed")
			nake_shellcode_ptr += 5;
	}

	shell_code_ = nake_shell_code;
	hook_addr_ = hook_addr;
	hook_headr_code_data_ = hook_addr_save_data;
}
void NakedHook::Hook() {
	//写目标函数
	JMPCODE_ hookJmp;
	hookJmp.jmp = 0xe9;
	hookJmp.addr = (DWORD)shell_code_ - (DWORD)hook_addr_ - 5;
	DWORD old_protect = 0;
	SIZE_T writes = 0;
	if (!VirtualProtect(hook_addr_, 5, PAGE_EXECUTE_READWRITE, &old_protect))
		THROW_SYSTEM_ERROR("HOOK VirtualProtect Failed")
		if (!WriteProcessMemory(GetCurrentProcess(), hook_addr_, &hookJmp, sizeof(JMPCODE_), &writes))
			THROW_SYSTEM_ERROR("HOOK WriteProcessMemory Failed")
			VirtualProtect(hook_addr_, 5, old_protect, &old_protect);
}
void NakedHook::UnHook() {
	DWORD old_protect = 0;
	SIZE_T writes = 0;
	if (!VirtualProtect(hook_addr_, 5, PAGE_EXECUTE_READWRITE, &old_protect))
		THROW_SYSTEM_ERROR("UnHook VirtualProtect Failed")
		if (!WriteProcessMemory(GetCurrentProcess(), hook_addr_, hook_headr_code_data_.data(), hook_headr_code_data_.size(), &writes))
			THROW_SYSTEM_ERROR("UnHook WriteProcessMemory Failed")
			VirtualProtect(hook_addr_, 5, old_protect, &old_protect);
}

DWORD NakedHook::GetSrcNakeCodeSizeAndNopOffset(PVOID nake_func_addr, DWORD& nop_offset)
{
	nop_offset = 0;
	DWORD nake_code_size = 0;
	BYTE feature_code[] = { 0x90,0x90, 0x90, 0x90 };
	for (BYTE* start = (BYTE*)nake_func_addr; start < (BYTE*)nake_func_addr + 200; start++) {
		if (*start == 0x90 && nop_offset == 0) {
			if (memcmp(start, feature_code, sizeof(feature_code)) == 0) {
				nop_offset = start - (BYTE*)nake_func_addr;
			}
		}
		else if (*start != 0x90 && nop_offset > 0) {
			nake_code_size = start - (BYTE*)nake_func_addr;
			break;
		}
	}
	return nake_code_size;
}


std::vector<NakedHook::FixItem> NakedHook::GetSrcNakeCodeFixItems(PVOID src_nake_code, DWORD len)
{
	std::vector<FixItem> fix_items;
	BYTE* start = (BYTE*)src_nake_code;
	BYTE feature_code[] = { 0x90,0x90, 0x90, 0x90 };
	do {
		if (*start == 0xE8) {
			//CALL xxxxx
			//JMP的地址(88881234) – 代码地址(010073bb) – 5（字节） = 机器码跳转地址(E9 87879e74)
			DWORD call_addr = *(DWORD*)(start + 1) + 5 + (DWORD)start;
			FixItem item = { (DWORD)(start - (BYTE*)src_nake_code),call_addr };
			fix_items.push_back(item);
			start += 5;
		}
		else if (*start == 0x90) {
			if (memcmp(start, feature_code, sizeof(feature_code)) == 0)
				break;
			else
				start++;
		}
		else
			start++;
	} while ((start - (BYTE*)src_nake_code) < len);
	return fix_items;
}

void NakedHook::FixRealAddr(DWORD& naked_addr) {
	if (*(BYTE*)naked_addr == 0xe9)
		naked_addr = naked_addr + 5 + *(DWORD*)(naked_addr + 1);
}
