#include "GetWindowsVersion.h"
#include "Universal.h"


ULONG_PTR  g_BuildNumber = 0;


WIN_VERSION 
GetWindowsVersion()
{
	RTL_OSVERSIONINFOEXW osverInfo = {sizeof(osverInfo)}; 

	pfnRtlGetVersion RtlGetVersion = NULL;

	WIN_VERSION WinVersion;

	WCHAR szRtlGetVersion[] = L"RtlGetVersion";

	RtlGetVersion = GetFunctionAddressByName(szRtlGetVersion); 

	if (RtlGetVersion)
	{
		RtlGetVersion((PRTL_OSVERSIONINFOW)&osverInfo); 
	} 
	else 
	{
		PsGetVersion(&osverInfo.dwMajorVersion, &osverInfo.dwMinorVersion, &osverInfo.dwBuildNumber, NULL);
	}

	DbgPrint("Build Number: %d\r\n", osverInfo.dwBuildNumber);

	g_BuildNumber = osverInfo.dwBuildNumber;

	if (osverInfo.dwMajorVersion == 5 && osverInfo.dwMinorVersion == 1) 
	{
		DbgPrint("WINDOWS_XP\r\n");
		WinVersion = WINDOWS_XP;
	}
	else if (osverInfo.dwMajorVersion == 6 && osverInfo.dwMinorVersion == 1)
	{
		DbgPrint("WINDOWS 7\r\n");
		WinVersion = WINDOWS_7;
	}
	else if (osverInfo.dwMajorVersion == 6 && osverInfo.dwMinorVersion == 2 && osverInfo.dwBuildNumber == 9200)
	{
		DbgPrint("WINDOWS 8\r\n");
		WinVersion = WINDOWS_8;
	}
	else if (osverInfo.dwMajorVersion == 6 && osverInfo.dwMinorVersion == 3 && osverInfo.dwBuildNumber == 9600)
	{
		DbgPrint("WINDOWS 8.1\r\n");
		WinVersion = WINDOWS_8_1;
	}
	else
	{
		DbgPrint("WINDOWS_UNKNOW\r\n");
		WinVersion = WINDOWS_UNKNOW;
	}

	return WinVersion;
}


