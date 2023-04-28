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



//ʹ�ñ���ö�ٽ��̵ķ���
BOOLEAN
	EnumProcessByForce();



//��ȡ���̵�����·��

BOOLEAN
	GetProcessPathBySectionObject(ULONG_PTR ulProcessID,WCHAR* wzProcessPath);

BOOLEAN 
	GetPathByFileObject(PFILE_OBJECT FileObject, WCHAR* wzPath);
//��ȡ���̵�����·�� ��ʵ��



//���Idle���̵�EPROCESS
PEPROCESS 
	GetIdleProcess();


//
//��������
//�жϱ���ö�ٳ����Ľ����ǲ��������Ľ��̣���ֹ��������Ҳ������ö���ˣ�
extern
	POBJECT_TYPE* PsProcessType;

typedef 
	ULONG_PTR (*pfnObGetObjectType)(PVOID pObject);


BOOLEAN
	IsRealProcess(PEPROCESS EProcess);

//��ȡ���������
ULONG_PTR 
	KeGetObjectType(PVOID Object);


BOOLEAN
	IsProcessDie(PEPROCESS EProcess);


//�ں��Ƿ��ܹ�����
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
//��ø����̵�PID
ULONG_PTR 
	GetInheritedProcessPid(PEPROCESS Eprocess);


////////////////////////////////////////          �ӿ�
//

/*
{L"������",				148	},
{L"����PID",			150	},
{L"������PID",		160	},
{L"����·��",			128	},
{L"EPROCESS",		80	},
{L"����Ȩ��",			81	},
{L"�Ƿ�����",  		81	},
{L"���쳧��",			81	}
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


//�оٽ��̽ӿ�
BOOLEAN 
	GetNormalProcessList(PPROCESSINFO Info,PPROCESSINFO HideInfo);



#endif