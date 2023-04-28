//���̹���

#pragma once
#include <WINDOWS.H>
#include "MyList.h"

/*
{L"������",					},
{L"����PID",				},
{L"������PID",			},
{L"����·��",				},
{L"EPROCESS",			},
{L"����Ȩ��",				},
{L"�Ƿ�����",  			},
{L"���쳧��",				}
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





//����ļ��ĳ���
CString
GetFileCompanyName(CString strPath);





//Ӧ�ó���ӿ�
//��������
void	
QuerySystemProcess(HWND hWnd,CMyList *m_ListCtrl,CImageList *m_ProImageList,HICON hKenny);
