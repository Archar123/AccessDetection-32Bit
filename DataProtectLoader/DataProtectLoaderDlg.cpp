
// DataProtectLoaderDlg.cpp : 实现文件
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
	UCHAR		szProcName[256];		//游戏名称
	ULONG		EProcess;					//游戏EProcess
	ULONG		GameID;					//CF ID为１
	ULONG		PointList[16];				//16个基地址  需要保护的点数组
	ULONG		PointNum;				//保护的点的个数			
	HANDLE		GamePID;					
	ULONG		IsDoubleProcess;		//0 表示未开启 双进程    1表示开启双进程		
	ULONG		IsOpenOrClose;			//1表示开启保护		0表示关闭保护
}TGameInfor,*PGameInfor;




typedef struct _COLUMNSTRUCT_
{
	WCHAR	*	title;				//列表的名称
	int		nWidth;			//列表的宽度
}COLUMNSTRUCT;


#define	 PROCESS_MAX_COLUMN	8

COLUMNSTRUCT g_Column_Process_Data[] = 
{
	{L"进程名",				130	},
	{L"进程PID",			90	},
	{L"父进程PID",		90	},
	{L"进程路径",			360	},
	{L"EPROCESS",		100	},
	{L"R3/R0",				80	},
	{L"是否隐藏",  		90	},
	{L"制造厂商",			100	}
};

int g_Column_Process_Count=8;  //列表的个数
int g_Column_Process_Width=0;  //列总宽度

//定义全局变量
WCHAR lpwzNum[50];  //全局变量  保存进程ID
HWND	MainhWnd;

BOOL		bIsChecking = FALSE; //当前的检查状态

extern PPROCESSINFO NormalProcessInfo;



// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CDataProtectLoaderDlg 对话框




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


// CDataProtectLoaderDlg 消息处理程序

BOOL CDataProtectLoaderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

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

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDataProtectLoaderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDataProtectLoaderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}





///////////////////       开启保护
void CDataProtectLoaderDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	
	PGameInfor	Infor = new	TGameInfor;
	DWORD		dwReturn;
	UpdateData(TRUE);
	//获得EXE的基地址
	HMODULE hModule = DataProtect_GetProcessBase(m_GameId);

	Infor->GamePID = (HANDLE)m_GameId;

	if(g_Set==0)
	{
		SetDlgItemText(IDC_BUTTON1,L"关闭保护");
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
		
		//发送控制码
		BOOL dwRet = DeviceIoControl(g_hDevice,DP_IOCTL(DP_IOCTL_DATAACCESSDETECTION_OPEN),Infor,sizeof(TGameInfor),NULL,0,&dwReturn,NULL);
	}
	else 
	{
		SetDlgItemText(IDC_BUTTON1,L"开启保护");
		g_Set = 0;
		Infor->IsOpenOrClose = 0;
		//发送控制码
		BOOL dwRet = DeviceIoControl(g_hDevice,DP_IOCTL(DP_IOCTL_DATAACCESSDETECTION_CLOSE),Infor,sizeof(TGameInfor),NULL,0,&dwReturn,NULL);
	}
}


//点击枚举进程
void CDataProtectLoaderDlg::OnBnClickedProcessbutton()
{
	if (bIsChecking == TRUE)
	{
		MessageBoxW(L"当前检查正在进行,请稍后操作...",L"提示!",MB_ICONWARNING);
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



//状态栏
static UINT indicators[] =
{
	IDR_STATUSBAR_STRING
};

//状态栏
void CDataProtectLoaderDlg::CreatStatusBar(void)
{
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		sizeof(indicators)/sizeof(UINT)))                    //创建状态条并设置字符资源的ID
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
//接口函数

//进程相关

DWORD WINAPI QuerySystemProcessFunction(CMyList *m_ListCtrl,	CImageList *m_ProImageList,HICON hKenny)
{
	bIsChecking = TRUE;
	QuerySystemProcess(MainhWnd,m_ListCtrl,m_ProImageList,hKenny);
	bIsChecking = FALSE;
	return 0;
}

//消息响应函数  进行状态栏的数据显示
LRESULT CDataProtectLoaderDlg::OnBar(WPARAM wParam, LPARAM lParam)
{
	m_wndStatusBar.SetPaneText(0,(LPCTSTR)lParam);

	return 0;
}

//初始化列表
int CDataProtectLoaderDlg::InitList(void)
{

	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	for (int i = 0; i < g_Column_Process_Count; i++)
	{
		m_ListCtrl.InsertColumn(i, LPCTSTR(g_Column_Process_Data[i].title),LVCFMT_LEFT,g_Column_Process_Data[i].nWidth);
		g_Column_Process_Width+=g_Column_Process_Data[i].nWidth;       //得到总宽度

	}

	m_case = 1;

	return 0;
}

//改变大小时进行刷新
void CDataProtectLoaderDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码

	double dcx=cx;     //对话框的总宽度
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
