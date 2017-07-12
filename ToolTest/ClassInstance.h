
#pragma once

template<class T = DWORD>
class CClassInstance
{
public:
	CClassInstance() = default;
	~CClassInstance() = default;

	CClassInstance(CONST CClassInstance&) = delete;

	static T& GetInstance()
	{
		return GetStaticVariable<T>();
	}

	void operator = (CONST CClassInstance&) = delete;

	template<typename Var>
	static Var& GetStaticVariable()
	{
		static Var Var_;
		return Var_;
	}
private:

};