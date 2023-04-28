
// DataProtectLoaderDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "DataProtectLoader.h"
#include "DataProtectLoaderDlg.h"
#include "afxdialogex.h"
#include "GetImageBase.h"
#include "MyList.h"
#include "Process.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern	HANDLE g_hDevice;
int        g_Set = 0;

typedef struct _GAMEINFOR_	
{
	UCHAR		szProcName[256];		//��Ϸ����
	ULONG		EProcess;					//��ϷEProcess
	ULONG		GameID;					//CF IDΪ��
	ULONG		PointList[16];				//16������ַ  ��Ҫ�����ĵ�����
	ULONG		PointNum;				//�����ĵ�ĸ���			
	HANDLE		GamePID;					
	ULONG		IsDoubleProcess;		//0 ��ʾδ���� ˫����    1��ʾ����˫����		
	ULONG		IsOpenOrClose;			//1��ʾ��������		0��ʾ�رձ���
}TGameInfor,*PGameInfor;




typedef struct _COLUMNSTRUCT_
{
	WCHAR	*	title;				//�б������
	int		nWidth;			//�б�Ŀ��
}COLUMNSTRUCT;


#define	 PROCESS_MAX_COLUMN	8

COLUMNSTRUCT g_Column_Process_Data[] = 
{
	{L"������",				130	},
	{L"����PID",			90	},
	{L"������PID",		90	},
	{L"����·��",			360	},
	{L"EPROCESS",		100	},
	{L"R3/R0",				80	},
	{L"�Ƿ�����",  		90	},
	{L"���쳧��",			100	}
};

int g_Column_Process_Count=8;  //�б�ĸ���
int g_Column_Process_Width=0;  //���ܿ��

//����ȫ�ֱ���
WCHAR lpwzNum[50];  //ȫ�ֱ���  �������ID
HWND	MainhWnd;

BOOL		bIsChecking = FALSE; //��ǰ�ļ��״̬

extern PPROCESSINFO NormalProcessInfo;



// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDataProtectLoaderDlg �Ի���




CDataProtectLoaderDlg::CDataProtectLoaderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDataProtectLoaderDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_GameId = 0;
}

void CDataProtectLoaderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_GameId);
	DDX_Control(pDX, IDC_PROCESSLIST, m_ListCtrl);
}

BEGIN_MESSAGE_MAP(CDataProtectLoaderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()

	ON_BN_CLICKED(IDC_BUTTON1, &CDataProtectLoaderDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_ProcessBUTTON, &CDataProtectLoaderDlg::OnBnClickedProcessbutton)
	ON_MESSAGE(WM_MyUpdateBar, OnBar)
END_MESSAGE_MAP()


// CDataProtectLoaderDlg ��Ϣ�������

BOOL CDataProtectLoaderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	m_ListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EDITLABELS|LVS_EX_SUBITEMIMAGES);
	m_ListCtrl.g_SmallIcon.Create(16,16,ILC_COLOR32|ILC_MASK,2,2);
	m_ListCtrl.SetImageList(&(m_ListCtrl.g_SmallIcon),LVSIL_SMALL);

	InitList();  //init all  list
	CreatStatusBar(); 

	CRect rect;
	GetWindowRect(&rect);
	rect.bottom+=20;
	rect.left+=200;
	MoveWindow(rect);
	
	MainhWnd = m_hWnd;
	CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)QuerySystemProcessFunction,&m_ListCtrl, 0,NULL);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CDataProtectLoaderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CDataProtectLoaderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CDataProtectLoaderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}





///////////////////       ��������
void CDataProtectLoaderDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	
	PGameInfor	Infor = new	TGameInfor;
	DWORD		dwReturn;
	UpdateData(TRUE);
	//���EXE�Ļ���ַ
	HMODULE hModule = DataProtect_GetProcessBase(m_GameId);

	Infor->GamePID = (HANDLE)m_GameId;

	if(g_Set==0)
	{
		SetDlgItemText(IDC_BUTTON1,L"�رձ���");
		g_Set = 1;
		Infor->IsOpenOrClose = 1;
		
		Infor->PointList[0] = 0x4F000 + (ULONG)hModule;
		Infor->PointList[1] = 0x4F100+ (ULONG)hModule;
		Infor->PointList[2] = 0x50000+ (ULONG)hModule;
		Infor->PointList[3] = 0x50150 + (ULONG)hModule;
		Infor->PointList[4] = 0x60150 + (ULONG)hModule;
		Infor->PointList[5] = 0x63100 + (ULONG)hModule;
		Infor->PointList[6] = 0x64280 + (ULONG)hModule;
		Infor->PointList[7] = 0x70000 + (ULONG)hModule;
		Infor->PointList[8] = 0x70100 + (ULONG)hModule;
		Infor->PointList[9] = 0x72050 + (ULONG)hModule;

		
		Infor->PointNum = 10;
		
		//���Ϳ�����
		BOOL dwRet = DeviceIoControl(g_hDevice,DP_IOCTL(DP_IOCTL_DATAACCESSDETECTION_OPEN),Infor,sizeof(TGameInfor),NULL,0,&dwReturn,NULL);
	}
	else 
	{
		SetDlgItemText(IDC_BUTTON1,L"��������");
		g_Set = 0;
		Infor->IsOpenOrClose = 0;
		//���Ϳ�����
		BOOL dwRet = DeviceIoControl(g_hDevice,DP_IOCTL(DP_IOCTL_DATAACCESSDETECTION_CLOSE),Infor,sizeof(TGameInfor),NULL,0,&dwReturn,NULL);
	}
}


//���ö�ٽ���
void CDataProtectLoaderDlg::OnBnClickedProcessbutton()
{
	if (bIsChecking == TRUE)
	{
		MessageBoxW(L"��ǰ������ڽ���,���Ժ����...",L"��ʾ!",MB_ICONWARNING);
		return;
	}

	while (m_ListCtrl.g_SmallIcon.Remove(0));

	m_ListCtrl.DeleteAllItems();

	for (int Index=88;Index> -1;Index--)
	{
		m_ListCtrl.DeleteColumn(Index);
	}

	m_ListCtrl.SetSelectedColumn(-1);

	CRect rect;
	GetWindowRect(&rect);
	MoveWindow(rect);

	double dcx = rect.Width();

	for(int Index = 0;Index< PROCESS_MAX_COLUMN; Index++)
	{
		double dd=g_Column_Process_Data[Index].nWidth;    
		dd/=g_Column_Process_Width;                    
		dd*=dcx;                                      
		int lenth=(int)dd;

		m_ListCtrl.InsertColumn(Index, g_Column_Process_Data[Index].title ,LVCFMT_LEFT, lenth);
	}
	m_case = 1;

	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)QuerySystemProcessFunction,&m_ListCtrl, 0,NULL);
}



//״̬��
static UINT indicators[] =
{
	IDR_STATUSBAR_STRING
};

//״̬��
void CDataProtectLoaderDlg::CreatStatusBar(void)
{
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		sizeof(indicators)/sizeof(UINT)))                    //����״̬���������ַ���Դ��ID
	{
		TRACE0("Failed to create status bar\n");
		return ;      // fail to create
	}
	CRect rc;
	::GetWindowRect(m_wndStatusBar.m_hWnd,rc);             
	m_wndStatusBar.MoveWindow(rc);                   
}





















//////////////////////////////////////
//_________________________________________  
//�ӿں���

//�������

DWORD WINAPI QuerySystemProcessFunction(CMyList *m_ListCtrl,	CImageList *m_ProImageList,HICON hKenny)
{
	bIsChecking = TRUE;
	QuerySystemProcess(MainhWnd,m_ListCtrl,m_ProImageList,hKenny);
	bIsChecking = FALSE;
	return 0;
}

//��Ϣ��Ӧ����  ����״̬����������ʾ
LRESULT CDataProtectLoaderDlg::OnBar(WPARAM wParam, LPARAM lParam)
{
	m_wndStatusBar.SetPaneText(0,(LPCTSTR)lParam);

	return 0;
}

//��ʼ���б�
int CDataProtectLoaderDlg::InitList(void)
{

	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	for (int i = 0; i < g_Column_Process_Count; i++)
	{
		m_ListCtrl.InsertColumn(i, LPCTSTR(g_Column_Process_Data[i].title),LVCFMT_LEFT,g_Column_Process_Data[i].nWidth);
		g_Column_Process_Width+=g_Column_Process_Data[i].nWidth;       //�õ��ܿ��

	}

	m_case = 1;

	return 0;
}

//�ı��Сʱ����ˢ��
void CDataProtectLoaderDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������

	double dcx=cx;     //�Ի�����ܿ��
	if (m_ListCtrl .m_hWnd!=NULL)
	{
		CRect rc;
		rc.right=cx-1; 
		rc.top=21;         
		rc.left=200;    
		rc.bottom=cy-20;  
		m_ListCtrl.MoveWindow(rc);

		for(int i=0;i<g_Column_Process_Count;i++)
		{
			double dd=g_Column_Process_Data[i].nWidth;    
			dd/=g_Column_Process_Width;                    
			dd*=dcx;                                      
			int lenth=(int)dd;                                  
			m_ListCtrl.SetColumnWidth(i,(lenth)); 
		}

	}

	if(m_wndStatusBar.m_hWnd!=NULL)
	{
		CRect rc;
		rc.top=cy-20;
		rc.right=cx-1;
		rc.left=200;
		rc.bottom=cy;
		m_wndStatusBar.MoveWindow(rc);
		m_wndStatusBar.SetPaneInfo(0, m_wndStatusBar.GetItemID(0),SBPS_POPOUT, cx-10);
	}
}
