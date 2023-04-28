#ifndef	_GETWINDOWSVERSION_H_
#define	_GETWINDOWSVERSION_H_

#include <ntifs.h>




typedef 
NTSTATUS 
(*pfnRtlGetVersion)(OUT PRTL_OSVERSIONINFOW lpVersionInformation);


typedef 
enum WIN_VERSION 
{
	WINDOWS_UNKNOW,
	WINDOWS_XP,
	WINDOWS_7,
	WINDOWS_8,
	WINDOWS_8_1
} WIN_VERSION;


WIN_VERSION GetWindowsVersion();






#endif