//进程管理

#pragma once
#include <WINDOWS.H>
#include "MyList.h"

/*
{L"进程名",					},
{L"进程PID",				},
{L"父进程PID",			},
{L"进程路径",				},
{L"EPROCESS",			},
{L"访问权限",				},
{L"是否隐藏",  			},
{L"制造厂商",				}
*/
typedef struct _SAFESYSTEM_PROCESS_INFORMATION {        
	WCHAR				ImageName[64];
	ULONG_PTR		ulPid;
	ULONG_PTR		ulInheritedFromProcessId;
	ULONG_PTR		EProcess;
	WCHAR				lpwzFullProcessPath[256];
	ULONG_PTR		ulKernelOpen;
	ULONG_PTR		IntHideType;
	WCHAR				Firm[256];
}SAFESYSTEM_PROCESS_INFORMATION, *PSAFESYSTEM_PROCESS_INFORMATION;

typedef struct _PROCESSINFO {          
	ULONG_PTR		ulCount;
	SAFESYSTEM_PROCESS_INFORMATION ProcessInfo[1];
} PROCESSINFO, *PPROCESSINFO;





//获得文件的厂商
CString
GetFileCompanyName(CString strPath);





//应用程序接口
//遍历进程
void	
QuerySystemProcess(HWND hWnd,CMyList *m_ListCtrl,CImageList *m_ProImageList,HICON hKenny);
