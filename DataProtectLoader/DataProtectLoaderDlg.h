
// DataProtectLoaderDlg.h : ͷ�ļ�
//

#pragma once
#include <WinIoCtl.h>
#include <list>
#include "MyList.h"
using namespace std;


#define _WM_NOTIFYICON WM_USER+1000
#define	WM_MyUpdateBar	WM_USER+101	//����StatusBar����Ϣ

#define CTL_CODE( DeviceType, Function, Method, Access ) (  ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )

#define DP_IOCTL(i)			CTL_CODE(FILE_DEVICE_UNKNOWN,i,METHOD_NEITHER,FILE_ANY_ACCESS)


enum DP_ENUM_IOCTL
{
	DP_IOCTL_DATAPROTECT = 0x100,					
	DP_IOCTL_DATAACCESSDETECTION_OPEN,			
	DP_IOCTL_DATAACCESSDETECTION_CLOSE,	
	DP_IOCTL_GET_PROCESS,	
};





#define DP_DEVICE_NAME					L"\\Device\\DataAccessDetection_Device"             // Driver Name
#define DP_LINK_NAME						L"\\\\.\\DataAccessDetection_Link"							// Win32 Link Name


typedef struct _ProcessInfo
{
	ULONG uEprocess;
	ULONG uProcessId;
	ULONG uCR3;
	char pszImageFileName[16];
}ProcessInfo;





// CDataProtectLoaderDlg �Ի���
class CDataProtectLoaderDlg : public CDialogEx
{
// ����
public:
	CDataProtectLoaderDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DATAPROTECTLOADER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;
	CStatusBar  m_wndStatusBar;                //״̬��

	//��ǰѡ�е���
	int			m_case;
	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//������Ϣ
	afx_msg LRESULT OnBar(WPARAM wParam, LPARAM lParam);
	CMyList			m_ListCtrl;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	int		InitList(void);
	afx_msg void OnBnClickedButton1();
	DWORD			m_GameId;
	afx_msg void OnBnClickedProcessbutton();
public:
	//״̬��
	void CreatStatusBar(void);
};


//////////////////////////////////////
//_________________________________________  
//�ӿں���

//������صĽӿں���
DWORD WINAPI QuerySystemProcessFunction(CMyList *m_ListCtrl,	CImageList *m_ProImageList,HICON hKenny);