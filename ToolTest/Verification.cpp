#include "stdafx.h"
#include "Verification.h"

#include "DebugOutput.h"


#include <Windows.h>
#include <Ntddscsi.h>
#include <winioctl.h>
#include <WinDef.h>
#include <IPHlpApi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>

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

	namespace cpu_info_space {
		class InstructionSet
		{
			// forward declarations
			class InstructionSet_Internal;

		public:
			// getters
			static std::string Vendor(void) { return CPU_Rep.vendor_; }
			static std::string Brand(void) { return CPU_Rep.brand_; }

			static bool SSE3(void) { return CPU_Rep.f_1_ECX_[0]; }
			static bool PCLMULQDQ(void) { return CPU_Rep.f_1_ECX_[1]; }
			static bool MONITOR(void) { return CPU_Rep.f_1_ECX_[3]; }
			static bool SSSE3(void) { return CPU_Rep.f_1_ECX_[9]; }
			static bool FMA(void) { return CPU_Rep.f_1_ECX_[12]; }
			static bool CMPXCHG16B(void) { return CPU_Rep.f_1_ECX_[13]; }
			static bool SSE41(void) { return CPU_Rep.f_1_ECX_[19]; }
			static bool SSE42(void) { return CPU_Rep.f_1_ECX_[20]; }
			static bool MOVBE(void) { return CPU_Rep.f_1_ECX_[22]; }
			static bool POPCNT(void) { return CPU_Rep.f_1_ECX_[23]; }
			static bool AES(void) { return CPU_Rep.f_1_ECX_[25]; }
			static bool XSAVE(void) { return CPU_Rep.f_1_ECX_[26]; }
			static bool OSXSAVE(void) { return CPU_Rep.f_1_ECX_[27]; }
			static bool AVX(void) { return CPU_Rep.f_1_ECX_[28]; }
			static bool F16C(void) { return CPU_Rep.f_1_ECX_[29]; }
			static bool RDRAND(void) { return CPU_Rep.f_1_ECX_[30]; }

			static bool MSR(void) { return CPU_Rep.f_1_EDX_[5]; }
			static bool CX8(void) { return CPU_Rep.f_1_EDX_[8]; }
			static bool SEP(void) { return CPU_Rep.f_1_EDX_[11]; }
			static bool CMOV(void) { return CPU_Rep.f_1_EDX_[15]; }
			static bool CLFSH(void) { return CPU_Rep.f_1_EDX_[19]; }
			static bool MMX(void) { return CPU_Rep.f_1_EDX_[23]; }
			static bool FXSR(void) { return CPU_Rep.f_1_EDX_[24]; }
			static bool SSE(void) { return CPU_Rep.f_1_EDX_[25]; }
			static bool SSE2(void) { return CPU_Rep.f_1_EDX_[26]; }

			static bool FSGSBASE(void) { return CPU_Rep.f_7_EBX_[0]; }
			static bool BMI1(void) { return CPU_Rep.f_7_EBX_[3]; }
			static bool HLE(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[4]; }
			static bool AVX2(void) { return CPU_Rep.f_7_EBX_[5]; }
			static bool BMI2(void) { return CPU_Rep.f_7_EBX_[8]; }
			static bool ERMS(void) { return CPU_Rep.f_7_EBX_[9]; }
			static bool INVPCID(void) { return CPU_Rep.f_7_EBX_[10]; }
			static bool RTM(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[11]; }
			static bool AVX512F(void) { return CPU_Rep.f_7_EBX_[16]; }
			static bool RDSEED(void) { return CPU_Rep.f_7_EBX_[18]; }
			static bool ADX(void) { return CPU_Rep.f_7_EBX_[19]; }
			static bool AVX512PF(void) { return CPU_Rep.f_7_EBX_[26]; }
			static bool AVX512ER(void) { return CPU_Rep.f_7_EBX_[27]; }
			static bool AVX512CD(void) { return CPU_Rep.f_7_EBX_[28]; }
			static bool SHA(void) { return CPU_Rep.f_7_EBX_[29]; }

			static bool PREFETCHWT1(void) { return CPU_Rep.f_7_ECX_[0]; }

			static bool LAHF(void) { return CPU_Rep.f_81_ECX_[0]; }
			static bool LZCNT(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_ECX_[5]; }
			static bool ABM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[5]; }
			static bool SSE4a(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[6]; }
			static bool XOP(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[11]; }
			static bool TBM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[21]; }

			static bool SYSCALL(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[11]; }
			static bool MMXEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[22]; }
			static bool RDTSCP(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[27]; }
			static bool _3DNOWEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[30]; }
			static bool _3DNOW(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[31]; }

		private:
			static const InstructionSet_Internal CPU_Rep;

			class InstructionSet_Internal
			{
			public:
				InstructionSet_Internal()
					: nIds_{ 0 },
					nExIds_{ 0 },
					isIntel_{ false },
					isAMD_{ false },
					f_1_ECX_{ 0 },
					f_1_EDX_{ 0 },
					f_7_EBX_{ 0 },
					f_7_ECX_{ 0 },
					f_81_ECX_{ 0 },
					f_81_EDX_{ 0 },
					data_{},
					extdata_{}
				{
					//int cpuInfo[4] = {-1};
					std::array<int, 4> cpui;

					// Calling __cpuid with 0x0 as the function_id argument
					// gets the number of the highest valid function ID.
					__cpuid(cpui.data(), 0);
					nIds_ = cpui[0];

					for (int i = 0; i <= nIds_; ++i)
					{
						__cpuidex(cpui.data(), i, 0);
						data_.push_back(cpui);
					}

					// Capture vendor string
					char vendor[0x20];
					memset(vendor, 0, sizeof(vendor));
					*reinterpret_cast<int*>(vendor) = data_[0][1];
					*reinterpret_cast<int*>(vendor + 4) = data_[0][3];
					*reinterpret_cast<int*>(vendor + 8) = data_[0][2];
					vendor_ = vendor;
					if (vendor_ == "GenuineIntel")
					{
						isIntel_ = true;
					}
					else if (vendor_ == "AuthenticAMD")
					{
						isAMD_ = true;
					}

					// load bitset with flags for function 0x00000001
					if (nIds_ >= 1)
					{
						f_1_ECX_ = data_[1][2];
						f_1_EDX_ = data_[1][3];
					}

					// load bitset with flags for function 0x00000007
					if (nIds_ >= 7)
					{
						f_7_EBX_ = data_[7][1];
						f_7_ECX_ = data_[7][2];
					}

					// Calling __cpuid with 0x80000000 as the function_id argument
					// gets the number of the highest valid extended ID.
					__cpuid(cpui.data(), 0x80000000);
					nExIds_ = cpui[0];

					char brand[0x40];
					memset(brand, 0, sizeof(brand));

					for (int i = 0x80000000; i <= nExIds_; ++i)
					{
						__cpuidex(cpui.data(), i, 0);
						extdata_.push_back(cpui);
					}

					// load bitset with flags for function 0x80000001
					if (nExIds_ >= 0x80000001)
					{
						f_81_ECX_ = extdata_[1][2];
						f_81_EDX_ = extdata_[1][3];
					}

					// Interpret CPU brand string if reported
					if (nExIds_ >= 0x80000004)
					{
						memcpy(brand, extdata_[2].data(), sizeof(cpui));
						memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
						memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
						brand_ = brand;
					}
				};

				int nIds_;
				int nExIds_;
				std::string vendor_;
				std::string brand_;
				bool isIntel_;
				bool isAMD_;
				std::bitset<32> f_1_ECX_;
				std::bitset<32> f_1_EDX_;
				std::bitset<32> f_7_EBX_;
				std::bitset<32> f_7_ECX_;
				std::bitset<32> f_81_ECX_;
				std::bitset<32> f_81_EDX_;
				std::vector<std::array<int, 4>> data_;
				std::vector<std::array<int, 4>> extdata_;
			};
		};

		const InstructionSet::InstructionSet_Internal InstructionSet::CPU_Rep;
	}

	std::string GetCpuStr()
	{
		return cpu_info_space::InstructionSet::Vendor() + cpu_info_space::InstructionSet::Brand();
	}
}