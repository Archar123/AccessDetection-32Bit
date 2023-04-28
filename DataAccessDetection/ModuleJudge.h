#ifndef _MODULEJUDGE_H_
#define _MODULEJUDGE_H_

#include <NTIFS.h>


typedef struct _MODULENODE_
{
	char			DllName[256];	//ģ����
	ULONG	KernelCount;		//���ģ�鱻����Ӧ�ò���ʹ�				
	ULONG	UserCount;		//���ģ�鱻����Ӧ�ò���ʹ�
	ULONG	StartAddress;	//ģ���׵�ַ
	ULONG	EndAddress;		//ģ�������ַ
}TModuleNode,*PModuleNode;

typedef struct _PROCESSMODULE_
{
	char				ImageName[256];					//������
	PVOID			lpvEProcess;								//���̵�EProcess
	HANDLE		GamePID;									//����ID
	ULONG		IsDoubleProc;							//1��  0����
	ULONG		IsDoubleProc_Createcount;	//����������	
	ULONG		IsDoubleProc_Exitcount;		//���رռ�����	
	ULONG		ulCount;										//�жϼ����˶��ٸ�ģ��
	TModuleNode ModuleModes[4096];		//����Dll��Ϣ
}TProcessModule,*PProcessModule;

typedef struct _MODULEJUDGEINFO_ 
{
	ULONG					ulCount;
	TProcessModule	ProcessModules[1];
}TModuleJudgeInfo,*PModuleJudgeInfo;



//��ʼ��
VOID
ModuleJudge_Init();

//����ʼ��
VOID
ModuleJudge_UnInit();

/////////////////////////////////////////////////////////////////////////////

//��ͨ�ж��ӿ�
VOID  	
__stdcall	
DataAccessDetection_NormalJudge(ULONG EIp);


VOID 		
__stdcall	
PrintModuleAcess();


VOID
DpcCallBack_ModuleJudge(
	IN	PKDPC	Dpc,
	IN	PVOID	DeferredContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2);


VOID 
MyLoadImageNotify(
	PUNICODE_STRING FullImageName,
	HANDLE	ProcessId,
	PIMAGE_INFO ImageInfo);


LONG
GetProcessModulesIndexFromPId(HANDLE PId);

#endif