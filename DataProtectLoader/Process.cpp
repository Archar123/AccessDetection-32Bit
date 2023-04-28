#include "stdafx.h"
#include "Process.H"
#include <Strsafe.h>
#pragma comment(lib,"Version.lib")


#include "DataProtectLoader.h"
#include "DataProtectLoaderDlg.h"
extern	HANDLE g_hDevice;

#define	WM_MyUpdateBar	WM_USER+101	//更新StatusBar的消息
PPROCESSINFO	NormalProcessInfo = NULL;
ULONG					ulPID;



//获得进程列表
void	
QuerySystemProcess(HWND hWnd,CMyList *m_ListCtrl,CImageList *m_ProImageList,HICON hKenny)
{
	ULONG	Title = 2014;
	WCHAR lpwzTextOut[100];
	ULONG_PTR  Index = 0;

	memset(lpwzTextOut,0,sizeof(lpwzTextOut));
	wsprintfW(lpwzTextOut,L"正在扫描 请稍等...版本号 %d",Title);

	SendMessage(hWnd,WM_MyUpdateBar,(WPARAM)(0),(LPARAM)lpwzTextOut);

	if (NormalProcessInfo)
	{
		VirtualFree(NormalProcessInfo,sizeof(PROCESSINFO)*256,MEM_RESERVE | MEM_COMMIT);
		NormalProcessInfo = NULL;
	}

	NormalProcessInfo = (PPROCESSINFO)VirtualAlloc(0, sizeof(PROCESSINFO)*256,MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);


	if (NormalProcessInfo)
	{
		memset(NormalProcessInfo,0,sizeof(PROCESSINFO)*256);   //初始化内存

		//向驱动层发送LIST_PROCESS消息
		DWORD  dwReturn = 0;

		DWORD  dwRet = DeviceIoControl(g_hDevice,DP_IOCTL(DP_IOCTL_GET_PROCESS),NormalProcessInfo,sizeof(PROCESSINFO)*256,NormalProcessInfo,sizeof(PROCESSINFO)*256,&dwReturn,NULL);

		WCHAR lpwzPid[50];
		WCHAR lpwzInheritedPid[50];	
		WCHAR lpwzEProcess[100];
		WCHAR lpwzStatus[50];
		WCHAR	lpwzIsHide[50];
		CString  strBuffer;

		//写入每条和进程相关的数据
		for (int i=0;i<(int)(NormalProcessInfo->ulCount);i++)
		{
			memset(lpwzPid,0,sizeof(lpwzPid));
			memset(lpwzInheritedPid,0,sizeof(lpwzInheritedPid));
			memset(lpwzStatus,0,sizeof(lpwzStatus));
			memset(lpwzIsHide,0,sizeof(lpwzIsHide));
			memset(lpwzEProcess,0,sizeof(lpwzEProcess));

			//获取PID,PPID和EPROCESS
			wsprintfW(lpwzPid,L"%d",NormalProcessInfo->ProcessInfo[i].ulPid);
			wsprintfW(lpwzInheritedPid,L"%d",NormalProcessInfo->ProcessInfo[i].ulInheritedFromProcessId);

			if (sizeof(ULONG_PTR)==8)
			{
				wsprintfW(lpwzEProcess,L"0x%016I64x",NormalProcessInfo->ProcessInfo[i].EProcess);
			}
			else
			{
				wsprintfW(lpwzEProcess,L"0x%08x",NormalProcessInfo->ProcessInfo[i].EProcess);
			}

			//在应用层尝试打开进程
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_OPERATION, TRUE,NormalProcessInfo->ProcessInfo[i].ulPid);
			if (hProcess)
			{
				wcscat_s(lpwzStatus,L"Y /");
				CloseHandle(hProcess);
			}else
			{
				wcscat_s(lpwzStatus,L"N /");
			}

			if (NormalProcessInfo->ProcessInfo[i].ulKernelOpen == 1)
			{
				wcscat_s(lpwzStatus,L" Y");
			}else
			{
				wcscat_s(lpwzStatus,L" N");
			}
			
			//获得厂商信息

			strBuffer = NormalProcessInfo->ProcessInfo[i].lpwzFullProcessPath;

			m_ListCtrl->SetProcessIcon(strBuffer);
			//
			//插入列表
			if (NormalProcessInfo->ProcessInfo[i].IntHideType == 1)
			{
				//被隐藏的进程
				Index = m_ListCtrl->InsertItem(i,NormalProcessInfo->ProcessInfo[i].ImageName,(COLORREF)RGB(255,20,147));  //隐藏
				
				m_ListCtrl->SetItem(Index, 0, LVIF_TEXT | LVIF_IMAGE,NormalProcessInfo->ProcessInfo[i].ImageName, i-2, 0, 0, 0);
				m_ListCtrl->SetItemColor(i,(COLORREF)RGB(255,20,147));
				wcscat_s(lpwzIsHide,L"隐藏进程");
			}
			else
			{
				//正常的进程
				Index = m_ListCtrl->InsertItem(i,NormalProcessInfo->ProcessInfo[i].ImageName,(COLORREF)RGB(192,192,192));
				
				m_ListCtrl->SetItem(Index, 0, LVIF_TEXT | LVIF_IMAGE,NormalProcessInfo->ProcessInfo[i].ImageName, i-2, 0, 0, 0);
				m_ListCtrl->SetItemColor(i,(COLORREF)RGB(192,192,192));
				wcscat_s(lpwzIsHide,L"正常进程");
			}

			

			m_ListCtrl->SetItemText(i,1,lpwzPid);
			m_ListCtrl->SetItemText(i,2,lpwzInheritedPid);
			m_ListCtrl->SetItemText(i,3,NormalProcessInfo->ProcessInfo[i].lpwzFullProcessPath);
			m_ListCtrl->SetItemText(i,4,lpwzEProcess);
			m_ListCtrl->SetItemText(i,5,lpwzStatus);
			m_ListCtrl->SetItemText(i,6,lpwzIsHide);

			strBuffer = NormalProcessInfo->ProcessInfo[i].lpwzFullProcessPath;
			strBuffer = GetFileCompanyName(strBuffer);

			m_ListCtrl->SetItemText(i,7,(LPCTSTR)strBuffer);
		}

	}

	else
	{
		memset(lpwzTextOut,0,sizeof(lpwzTextOut));
		wsprintfW(lpwzTextOut,L"申请内存错误:%d 稍后运行\n",GetLastError());
		MessageBox(0,lpwzTextOut,0,0);
	}


	memset(lpwzTextOut,0,sizeof(lpwzTextOut));
	wsprintfW(lpwzTextOut,L"系统进程扫描完毕，共有 %d 个进程",(int)NormalProcessInfo->ulCount);

	SendMessage(hWnd,WM_MyUpdateBar,(WPARAM)(0),(LPARAM)lpwzTextOut);
}


//获得文件的厂商
CString
GetFileCompanyName(CString strPath)
{
	CString strCompanyName = 0;;

	if (strPath.IsEmpty())
	{
		return NULL;
	}

	if (!strPath.CompareNoCase(L"Idle") || !strPath.CompareNoCase(L"System"))
	{
		return NULL;
	}

	if (!PathFileExists(strPath))   //有些进程在这里可能会出错 要编译成64位的
	{	
		return strCompanyName;
	}

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	LPWSTR lpstrFilename = (LPWSTR)(LPCWSTR)strPath;
	DWORD dwHandle = 0;
	DWORD dwVerInfoSize = GetFileVersionInfoSize(lpstrFilename, &dwHandle);

	if (dwVerInfoSize)
	{
		LPVOID Buffer = malloc(sizeof(char)*dwVerInfoSize);

		if (Buffer)
		{
			if (GetFileVersionInfo(lpstrFilename, dwHandle, dwVerInfoSize, Buffer))
			{
				UINT cbTranslate = 0;

				if ( VerQueryValue(Buffer, L"\\VarFileInfo\\Translation", (LPVOID*) &lpTranslate, &cbTranslate))
				{                
					LPCWSTR lpwszBlock = 0;          
					UINT    cbSizeBuf  = 0;
					WCHAR   wzSubBlock[MAX_PATH] = {0};

					if ((cbTranslate/sizeof(struct LANGANDCODEPAGE)) > 0)   
					{
						StringCchPrintf(wzSubBlock, sizeof(wzSubBlock)/sizeof(WCHAR), 
							L"\\StringFileInfo\\%04x%04x\\CompanyName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage); 
					}

					if ( VerQueryValue(Buffer, wzSubBlock, (LPVOID*)&lpwszBlock, &cbSizeBuf))
					{
						WCHAR wzCompanyName[MAX_PATH] = {0};

						StringCchCopy(wzCompanyName, MAX_PATH/sizeof(WCHAR), (LPCWSTR)lpwszBlock);   //将系统中内存的数据拷贝到我们自己内存当中
						strCompanyName = wzCompanyName;
					}
				}
			}

			free(Buffer);
		}
	}

	return strCompanyName;
}

