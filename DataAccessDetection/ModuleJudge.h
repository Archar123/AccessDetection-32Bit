#ifndef _MODULEJUDGE_H_
#define _MODULEJUDGE_H_

#include <NTIFS.h>


typedef struct _MODULENODE_
{
	char			DllName[256];	//模块名
	ULONG	KernelCount;		//这个模块被多少应用层访问过				
	ULONG	UserCount;		//这个模块被多少应用层访问过
	ULONG	StartAddress;	//模块首地址
	ULONG	EndAddress;		//模块结束地址
}TModuleNode,*PModuleNode;

typedef struct _PROCESSMODULE_
{
	char				ImageName[256];					//进程名
	PVOID			lpvEProcess;								//进程的EProcess
	HANDLE		GamePID;									//进程ID
	ULONG		IsDoubleProc;							//1是  0不是
	ULONG		IsDoubleProc_Createcount;	//创建计数器	
	ULONG		IsDoubleProc_Exitcount;		//启关闭计数器	
	ULONG		ulCount;										//判断加载了多少个模块
	TModuleNode ModuleModes[4096];		//访问Dll信息
}TProcessModule,*PProcessModule;

typedef struct _MODULEJUDGEINFO_ 
{
	ULONG					ulCount;
	TProcessModule	ProcessModules[1];
}TModuleJudgeInfo,*PModuleJudgeInfo;



//初始化
VOID
ModuleJudge_Init();

//反初始化
VOID
ModuleJudge_UnInit();

/////////////////////////////////////////////////////////////////////////////

//普通判定接口
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