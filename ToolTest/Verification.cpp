#include "stdafx.h"
#include "Verification.h"

#include "DebugOutput.h"


#include <Windows.h>
#include <Ntddscsi.h>
#include <winioctl.h>
#include <WinDef.h>
#include <IPHlpApi.h>
#pragma comment(lib, "IPHLPAPI.lib")

namespace verification
{

	typedef struct _IDSECTOR {
		USHORT  wGenConfig;
		USHORT  wNumCyls;
		USHORT  wReserved;
		USHORT  wNumHeads;
		USHORT  wBytesPerTrack;
		USHORT  wBytesPerSector;
		USHORT  wSectorsPerTrack;
		USHORT  wVendorUnique[3];
		CHAR  sSerialNumber[20];
		USHORT  wBufferType;
		USHORT  wBufferSize;
		USHORT  wECCSize;
		CHAR  sFirmwareRev[8];
		CHAR  sModelNumber[40];
		USHORT  wMoreVendorUnique;
		USHORT  wDoubleWordIO;
		USHORT  wCapabilities;
		USHORT  wReserved1;
		USHORT  wPIOTiming;
		USHORT  wDMATiming;
		USHORT  wBS;
		USHORT  wNumCurrentCyls;
		USHORT  wNumCurrentHeads;
		USHORT  wNumCurrentSectorsPerTrack;
		ULONG  ulCurrentSectorCapacity;
		USHORT  wMultSectorStuff;
		ULONG  ulTotalAddressableSectors;
		USHORT  wSingleWordDMA;
		USHORT  wMultiWordDMA;
		BYTE  bReserved[128];
	} IDSECTOR, *PIDSECTOR;

#define IDE_ATAPI_IDENTIFY 0xA1
#define IDE_ATA_IDENTIFY 0xEC
#define IDENTIFY_BUFFER_SIZE 512




	BOOL __fastcall DoIdentify(HANDLE hPhysicalDriveIOCTL,
		PSENDCMDINPARAMS pSCIP,
		PSENDCMDOUTPARAMS pSCOP,
		BYTE btIDCmd,
		BYTE btDriveNum,
		PDWORD pdwBytesReturned);

	char *__fastcall ConvertToString(DWORD dwDiskData[256], int nFirstIndex, int nLastIndex);


#include <string>

#ifdef _UNICODE
#define stl_string wstring
#else
#define stl_string string
#endif





	bool GetHdSerialNum(std::string  &mode, std::string & serial)

	{
		TCHAR szFileName[] = { _T("\\\\.\\PHYSICALDRIVE0") };
		HANDLE hFile = ::CreateFile(szFileName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			return false;
		}


		DWORD dwByteReturned;
		GETVERSIONINPARAMS gvopVersionParams;
		if (!DeviceIoControl(hFile,
			SMART_GET_VERSION,
			NULL,
			0,
			&gvopVersionParams,
			sizeof(gvopVersionParams),
			&dwByteReturned,
			NULL))
		{
			::CloseHandle(hFile);
			return false;
		}

		if (gvopVersionParams.bIDEDeviceMap <= 0)
		{
			::CloseHandle(hFile);
			return false;
		}

		int btIDCmd = 0;
		SENDCMDINPARAMS InParams;
		int nDrive = 0;
		btIDCmd = (gvopVersionParams.bIDEDeviceMap >> nDrive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

		//输出参数
		BYTE btIDOutCmd[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
		if (DoIdentify(hFile,
			&InParams,
			(PSENDCMDOUTPARAMS)btIDOutCmd,
			btIDCmd,
			nDrive,
			&dwByteReturned) == FALSE)
		{
			::CloseHandle(hFile);
			return false;
		}

		::CloseHandle(hFile);

		DWORD dwDiskData[256] = { 0 };
		USHORT * pIDSector; //对应结构IDSECTOR
		pIDSector = (USHORT *)((SENDCMDOUTPARAMS*)btIDOutCmd)->bBuffer;

		for (int i = 0; i < 256; i++)
		{
			dwDiskData[i] = pIDSector[i];
		}


		//用IDSECTOR结构获取
		/*
		IDSECTOR* pisd = ( IDSECTOR* )&btIDOutCmd[ sizeof( SENDCMDOUTPARAMS ) - 1 ];
		cout<<"硬盘序列号:"<<pisd->sSerialNumber<<endl;
		cout<<"硬盘模型:"<<pisd->sModelNumber<<endl;
		*/

		//IOCTL_ATA_PASS_THROUGH = 4D02C



		//取系列号
		mode =  ConvertToString(dwDiskData, 10, 19);

		//取型号
		serial =  ConvertToString(dwDiskData, 27, 46);
		return true;
	}
	BOOL __fastcall DoIdentify(HANDLE hPhysicalDriveIOCTL,
		PSENDCMDINPARAMS pSCIP,
		PSENDCMDOUTPARAMS pSCOP,
		BYTE btIDCmd,
		BYTE btDriveNum,
		PDWORD pdwBytesReturned)
	{
		pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;
		pSCIP->irDriveRegs.bFeaturesReg = 0;
		pSCIP->irDriveRegs.bSectorCountReg = 1;
		pSCIP->irDriveRegs.bSectorNumberReg = 1;
		pSCIP->irDriveRegs.bCylLowReg = 0;
		pSCIP->irDriveRegs.bCylHighReg = 0;

		pSCIP->irDriveRegs.bDriveHeadReg = (btDriveNum & 1) ? 0xB0 : 0xA0;
		pSCIP->irDriveRegs.bCommandReg = btIDCmd;
		pSCIP->bDriveNumber = btDriveNum;
		pSCIP->cBufferSize = IDENTIFY_BUFFER_SIZE;

		return DeviceIoControl(hPhysicalDriveIOCTL,
			SMART_RCV_DRIVE_DATA,
			(LPVOID)pSCIP,
			sizeof(SENDCMDINPARAMS),
			(LPVOID)pSCOP,
			sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
			pdwBytesReturned,
			NULL);
	}


	char *__fastcall ConvertToString(DWORD dwDiskData[256], int nFirstIndex, int nLastIndex)
	{
		static char szResBuf[1024];
		char ss[256];
		int nIndex = 0;
		int nPosition = 0;

		for (nIndex = nFirstIndex; nIndex <= nLastIndex; nIndex++)
		{
			ss[nPosition] = (char)(dwDiskData[nIndex] / 256);
			nPosition++;

			// Get low BYTE for 2nd character
			ss[nPosition] = (char)(dwDiskData[nIndex] % 256);
			nPosition++;
		}

		// End the string
		ss[nPosition] = '\0';

		int i, index = 0;
		for (i = 0; i<nPosition; i++)
		{
			if (ss[i] == 0 || ss[i] == 32)    continue;
			szResBuf[index] = ss[i];
			index++;
		}
		szResBuf[index] = 0;

		return szResBuf;
	}


	std::string GetVolumnInfo()
	{
		char szWindowDirectory[MAX_PATH] = { 0 };
		GetWindowsDirectoryA(szWindowDirectory, MAX_PATH);
		int nWinDirLen = strlen(szWindowDirectory);
		if (nWinDirLen < 3)
			return "";
		for (int i = 0; i < nWinDirLen; i++)
		{
			if (szWindowDirectory[i] == '\\')
			{
				szWindowDirectory[i + 1] = 0;
			}
		}

		DWORD serial_no = 0;
		if (::GetVolumeInformationA(szWindowDirectory, 0, 0, &serial_no, 0, 0, 0, 0))
		{
			char buff[256] = { 0 };
			sprintf_s(buff, "%X", serial_no);
			return buff;
		}
		else
		{
			return "";
		}
	}



	std::string GetMacInfo()
	{
		std::string retstr;
		ULONG BufferLength = 0;
		BYTE* pBuffer = 0;
		if (ERROR_BUFFER_OVERFLOW == GetAdaptersInfo(0, &BufferLength))
		{
			// Now the BufferLength contain the required buffer length.
			// Allocate necessary buffer.
			pBuffer = new BYTE[BufferLength];
		}
		else
		{
			// Error occurred. handle it accordingly.
			return retstr;
		}

		// Get the Adapter Information.
		PIP_ADAPTER_INFO pAdapterInfo =
			reinterpret_cast<PIP_ADAPTER_INFO>(pBuffer);
		if (GetAdaptersInfo(pAdapterInfo, &BufferLength) != ERROR_SUCCESS)
		{
			delete[] pBuffer;
			return retstr;
		}

		// Iterate the network adapters and print their MAC address.
		while (pAdapterInfo)
		{
			char szMac[200] = { 0 };
			sprintf_s(szMac,"%02x%02x%02x%02x%02x%02x", pAdapterInfo->Address[0],
				pAdapterInfo->Address[1],
				pAdapterInfo->Address[2],
				pAdapterInfo->Address[3],
				pAdapterInfo->Address[4],
				pAdapterInfo->Address[5]);
			szMac[12] = 0;
			retstr = szMac;
			break;
			pAdapterInfo = pAdapterInfo->Next;
		}

		// deallocate the buffer.
		delete[] pBuffer;
		return retstr;
	}

	std::string GetHearwareString()
	{
		std::string ret_mc_str;
		std::string hd_mode, hd_serial;
		if (GetHdSerialNum(hd_mode, hd_serial))
		{
			ret_mc_str += hd_mode;
			ret_mc_str += hd_serial;
		}

		ret_mc_str += GetVolumnInfo();
		ret_mc_str += GetMacInfo();
		return ret_mc_str;
	}
}