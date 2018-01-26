#pragma once
#include <windows.h>
#include <GdiPlus.h>
using namespace Gdiplus;
#pragma comment(lib,"GdiPlus.lib") 

class CScreeShot
{
public:
	CScreeShot(void);
	~CScreeShot(void);

	BOOL MakePNG(HDC hDC, RECT rect, LPCWSTR  FileName);
	BOOL BMptoPNG(LPCWSTR StrBMp, LPCWSTR StrPNG);
	BOOL PNGtoBMp(LPCWSTR StrPNG, LPCWSTR StrBMp);
	BOOL GetEncoderClsid(WCHAR* pFormat, CLSID* pClsid);
private:
	GdiplusStartupInput m_gdiplusStartupInput;
	ULONG_PTR m_pGdiToken;
};
