#include "ZHook.h"
#pragma pack(push, 1)
typedef struct _JMPCODE_
{
	BYTE jmp;
	DWORD addr;
}JMPCODE_, * PJMPCODE_;
#pragma pack(pop)

void ZHook::Hook(PVOID hook_addr, PVOID detours_addr, DWORD nop_count, DWORD& fp_srouce_addr) {

	//做ShellCode
	std::vector<unsigned char> saved_bytes(nop_count + 5);
	memcpy(saved_bytes.data(), hook_addr, saved_bytes.size());
	LPVOID my_code = ::VirtualAlloc(NULL, saved_bytes.size() + 5, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	SIZE_T writes = 0;
	::WriteProcessMemory(GetCurrentProcess(), my_code, saved_bytes.data(), saved_bytes.size(), &writes);

	JMPCODE_ jmpCode2;
	jmpCode2.jmp = 0xe9;
	//JMP的地址(88881234) – 代码地址(010073bb) – 5（字节） = 机器码跳转地址(E9 87879e74)
	jmpCode2.addr = (DWORD)hook_addr + saved_bytes.size() - ((DWORD)my_code + saved_bytes.size()) - 5;
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)((DWORD)my_code + saved_bytes.size()), &jmpCode2, sizeof(JMPCODE_), &writes);
	fp_srouce_addr = (DWORD)my_code;
	prv_code_ = my_code;

	JMPCODE_ hookJmp;
	hookJmp.jmp = 0xe9;
	hookJmp.addr = (DWORD)detours_addr - (DWORD)hook_addr - 5;
	DWORD old_protect = 0;
	VirtualProtect(hook_addr, 5, PAGE_EXECUTE_READWRITE, &old_protect);
	WriteProcessMemory(GetCurrentProcess(), hook_addr, &hookJmp, sizeof(JMPCODE_), &writes);
	VirtualProtect(hook_addr, 5, old_protect, &old_protect);

	saved_head_bytes_ = saved_bytes;
	hook_addr_ = hook_addr;
}
void ZHook::Unhook()
{
	SIZE_T writes = 0;
	if (saved_head_bytes_.size()) {
		DWORD old_protect = 0;
		VirtualProtect(hook_addr_, 5, PAGE_EXECUTE_READWRITE, &old_protect);
		::WriteProcessMemory(::GetCurrentProcess(), hook_addr_, saved_head_bytes_.data(), saved_head_bytes_.size(), &writes);
		VirtualProtect(hook_addr_, 5, old_protect, &old_protect);
	}
	if (prv_code_) {
		::VirtualFree(prv_code_, NULL, MEM_RELEASE);
		prv_code_ = NULL;
	}
}
LPVOID ZHook::GetHookedAddrss() const
{
	return hook_addr_;
}
