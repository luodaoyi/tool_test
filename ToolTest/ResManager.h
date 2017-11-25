#pragma once
#include <functional>
#define SetResDeleter(VarName, Deleter) CLResManager<decltype(VarName),std::function<VOID(decltype(VarName)&)>> VarName##Manager(VarName,Deleter)

template<typename ResHandle, typename Deleter>
class CLResManager
{
public:
	CLResManager(_In_ ResHandle& ResHandle_, _In_ Deleter fnDeletePtr) : m_ResHandle(ResHandle_), m_fnDeletePtr(fnDeletePtr)
	{

	}
	~CLResManager()
	{
		m_fnDeletePtr(m_ResHandle);
	}

	CLResManager& operator=(CONST CLResManager&) = delete;
private:
	ResHandle& m_ResHandle;
	Deleter m_fnDeletePtr;
};
