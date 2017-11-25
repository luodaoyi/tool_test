#pragma once
#include <vector>
#include <functional>

namespace public_tool
{

	template<typename PtrT>
	inline static void		SetPtr(PtrT* p, const PtrT& t)
	{
		if (p != nullptr)
			*p = t;
	}

	template<class T, class Fn>
	inline bool Vec_find_if(const std::vector<T>& vlst, T* p, Fn _Pred)
	{
		auto itr = std::find_if(vlst.begin(), vlst.end(), _Pred);
		if (itr != vlst.end())
			SetPtr(p, *itr);
		return itr != vlst.end();
	}

	template<class T, class Finder>
	static const T* Vec_find_if_Const(const std::vector<T>& vlst, Finder _Pred)
	{
		auto itr = std::find_if(vlst.begin(), vlst.end(), _Pred);
		return itr == vlst.end() ? nullptr : &*itr;
	}

	template<class T, class Finder>
	static T* Vec_find_if(_In_ std::vector<T>& vlst, _In_ Finder _Pred)
	{
		auto itr = std::find_if(vlst.begin(), vlst.end(), _Pred);
		return itr == vlst.end() ? nullptr : &*itr;
	}



	
}