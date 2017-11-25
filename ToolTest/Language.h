#pragma once

#include "ClassInstance.h"
#include <map>
#include <windows.h>
#include <vector>
#include <string>
class CLanguage : public CClassInstance<CLanguage>
{
public:
	CLanguage();
	~CLanguage() = default;
public:
	CONST std::wstring GetOtherText_By_LocalText(_In_ CONST std::wstring& wsText);
private:
	struct stringView
	{
		UINT uIndex;
		UINT uLength;
		CONST std::wstring& wsText;
		stringView(CONST std::wstring& refwsText) : wsText(refwsText)
		{

		}
		std::wstring GetText() CONST
		{
			return wsText.substr(uIndex, uLength);
		}

		stringView& operator = (CONST stringView&) = delete;
	};
	static size_t max_key_size;
private:
	// 添加当前服的Key--Value
	VOID InsertKeyValueMap(_In_ std::map<std::wstring, std::vector<stringView>>& Mp, _In_ CONST std::wstring& wsText);

	// 添加其他服的Value--Key
	VOID InsertValueKeyMap(_In_ std::map<std::wstring, std::vector<stringView>>& Mp, _In_ CONST std::wstring& wsText);
private:
	std::map<std::wstring, std::vector<stringView>> Map_Language_Local;
	std::map<std::wstring, std::vector<stringView>> Map_Language_Other;
	
};


