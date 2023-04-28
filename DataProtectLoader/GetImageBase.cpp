#include "stdafx.h"
#include "GetImageBase.h"





HMODULE 
DataProtect_GetProcessBase(DWORD PID)
{
	//获取进程基址
	HANDLE hSnapShot;
	//通过CreateToolhelp32Snapshot和线程ID，获取进程快照
	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, PID);
	if (hSnapShot == INVALID_HANDLE_VALUE)
		return NULL;

	MODULEENTRY32 ModuleEntry32;
	ModuleEntry32.dwSize = sizeof(ModuleEntry32);
	if (Module32First(hSnapShot, &ModuleEntry32))
	{
		do 
		{
			WCHAR szExt[10];
			wcscpy_s(szExt, ModuleEntry32.szExePath + wcslen(ModuleEntry32.szExePath) - 4);


			if (!wcscmp(_wcsupr(szExt), L".EXE"))
			{
				CloseHandle(hSnapShot);
				return ModuleEntry32.hModule;
			}
		} while (Module32Next(hSnapShot, &ModuleEntry32));
	}
	CloseHandle(hSnapShot);
	return NULL;

}
