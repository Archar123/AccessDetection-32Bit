#ifndef		_PROCESS_H_
#define		_PROCESS_H_

#include <ntifs.h>   




typedef struct _CONTROL_AREA64
{
	PVOID64 Segment;
	PVOID64 p1;
	PVOID64 p2;
	ULONG64 NumberOfSectionReferences;
	ULONG64 NumberOfPfnReferences;
	ULONG64 NumberOfMappedViews;
	ULONG64 NumberOfUserReferences;
	union
	{
		ULONG LongFlags;
		ULONG Flags;
	} u;
	PVOID64 FilePointer;
} CONTROL_AREA64, *PCONTROL_AREA64;



typedef struct _CONTROL_AREA
{
	PVOID Segment;
	LIST_ENTRY DereferenceList;
	ULONG NumberOfSectionReferences;
	ULONG NumberOfPfnReferences;
	ULONG NumberOfMappedViews;
	ULONG NumberOfSystemCacheViews;
	ULONG NumberOfUserReferences;
	union
	{
		ULONG LongFlags;
		ULONG Flags;
	} u;
	PFILE_OBJECT FilePointer;
} CONTROL_AREA, *PCONTROL_AREA;




typedef struct _SEGMENT64
{
	PVOID64 ControlArea;
	ULONG TotalNumberOfPtes;
	ULONG NonExtendedPtes;
	ULONG Spare0;
}SEGMENT64,*PSEGMENT64;


typedef struct _SEGMENT
{
	struct _CONTROL_AREA *ControlArea;
	ULONG TotalNumberOfPtes;
	ULONG NonExtendedPtes;
	ULONG Spare0;
} SEGMENT, *PSEGMENT;




typedef struct _SECTION_OBJECT
{
	PVOID StartingVa;
	PVOID EndingVa;
	PVOID Parent;
	PVOID LeftChild;
	PVOID RightChild;
	PSEGMENT Segment;
} SECTION_OBJECT, *PSECTION_OBJECT;


typedef struct _SECTION_OBJECT64
{
	PVOID64 StartingVa;
	PVOID64 EndingVa;
	PVOID64 Parent;
	PVOID64 LeftChild;
	PVOID64 RightChild;
	PVOID64 Segment;
} SECTION_OBJECT64, *PSECTION_OBJECT64;


typedef struct ProcessIdAndEProcess_
{
	ULONG_PTR		ulPid;
	ULONG_PTR		EProcess;
}ProcessIdAndEProcess_;


typedef struct _PROCESSINFO_SYS {          
	ULONG_PTR ulCount;
	ProcessIdAndEProcess_ ProcessIdAndEProcess[1];
}PROCESSINFO_SYS, *PPROCESSINFO_SYS;



//使用暴力枚举进程的方法
BOOLEAN
	EnumProcessByForce();



//获取进程的完整路径

BOOLEAN
	GetProcessPathBySectionObject(ULONG_PTR ulProcessID,WCHAR* wzProcessPath);

BOOLEAN 
	GetPathByFileObject(PFILE_OBJECT FileObject, WCHAR* wzPath);
//获取进程的完整路径 的实现



//获得Idle进程的EPROCESS
PEPROCESS 
	GetIdleProcess();


//
//辅助函数
//判断暴力枚举出来的进程是不是正常的进程（防止僵死进程也被我们枚举了）
extern
	POBJECT_TYPE* PsProcessType;

typedef 
	ULONG_PTR (*pfnObGetObjectType)(PVOID pObject);


BOOLEAN
	IsRealProcess(PEPROCESS EProcess);

//获取对象的类型
ULONG_PTR 
	KeGetObjectType(PVOID Object);


BOOLEAN
	IsProcessDie(PEPROCESS EProcess);


//内核是否能够访问
BOOLEAN 
	KernelStatus(HANDLE hPid);


extern
	NTSTATUS  ZwQueryInformationProcess(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength
	);
//获得父进程的PID
ULONG_PTR 
	GetInheritedProcessPid(PEPROCESS Eprocess);


////////////////////////////////////////          接口
//

/*
{L"进程名",				148	},
{L"进程PID",			150	},
{L"父进程PID",		160	},
{L"进程路径",			128	},
{L"EPROCESS",		80	},
{L"访问权限",			81	},
{L"是否隐藏",  		81	},
{L"制造厂商",			81	}
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


//列举进程接口
BOOLEAN 
	GetNormalProcessList(PPROCESSINFO Info,PPROCESSINFO HideInfo);



#endif