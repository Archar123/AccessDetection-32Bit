#include "SysCallBack.h"
#include "Universal.h"
#include "ModuleJudge.h"


extern				PModuleJudgeInfo		g_pMyModuleJudgeInfo;



//外部调用接口  设置进程和线程回调
VOID
DataAccessDetection_SysCallBack_Init()
{
	NTSTATUS Status;
	//进程回调函数
	Status = PsSetCreateProcessNotifyRoutine((PCREATE_PROCESS_NOTIFY_ROUTINE)MyCreateProcessNotify,FALSE);
	DbgPrint("[DataAccessDetection]PsSetCreateProcessNotifyRoutine return: %x;0 Is SUCCESS\r\n",Status);
	//线程回调函数
	Status = PsSetCreateThreadNotifyRoutine(MyCreateThreadNotify);
	DbgPrint("[DataAccessDetection]PsSetCreateThreadNotifyRoutine return: %x;0 Is SUCCESS\r\n",Status);

}

VOID
DataAccessDetection_SysCallBack_UnInit()
{
	//卸载进程回调
	PsSetCreateProcessNotifyRoutine((PCREATE_PROCESS_NOTIFY_ROUTINE)MyCreateProcessNotify,TRUE);
	DbgPrint("[DataAccessDetection]PsSetCreateProcessNotifyRoutine return SUCCESS!\r\n");
	//卸载线程回调
	PsRemoveCreateThreadNotifyRoutine(MyCreateThreadNotify);
	DbgPrint("[DataAccessDetection]PsRemoveCreateThreadNotifyRoutine return SUCCESS!\r\n");

}


//_________________________________________________________________________________________________________________________________________
//回调函数		
//进程回调
VOID 
MyCreateProcessNotify(
	IN HANDLE  	ParentId,
	IN HANDLE  	ProcessId,
	IN BOOLEAN  Create)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PEPROCESS			TargetEProcess = NULL;
	UCHAR*				lpszProcName = NULL;
	ULONG					ulRet = 0;

	if(Create)
	{
		Status = PsLookupProcessByProcessId(ProcessId, &TargetEProcess);
		if(NT_SUCCESS(Status))
		{
			lpszProcName = PsGetProcessImageFileName(TargetEProcess);

			ulRet = IsOurDetectionGames(lpszProcName,ProcessId,1);//传1表示是创建进程
			if(ulRet==1)
				DbgPrint("[DataAccessDetection]检测到 目标进程（测试） 启动PID!  进程名为:%s;\r\n",lpszProcName);
			
			ObfDereferenceObject(TargetEProcess);
		}
		
	}
	else
	{
		Status = PsLookupProcessByProcessId(ProcessId, &TargetEProcess);
		if(NT_SUCCESS(Status))
		{
			lpszProcName = PsGetProcessImageFileName(TargetEProcess);

			ulRet = IsOurDetectionGames(lpszProcName,ProcessId,0);//传0表示是结束进程
			if(ulRet==1)
				DbgPrint("[DataAccessDetection]检测到 目标进程（测试） 退出PID!  进程名为:%s;\r\n",lpszProcName);

			ObfDereferenceObject(TargetEProcess);
		}
	}

}




//注册一个系统线程回调
VOID 
MyCreateThreadNotify(
	IN HANDLE  	ProcessId,
	IN HANDLE  	ThreadId,
	IN BOOLEAN  Create)
{
	NTSTATUS		ns = STATUS_SUCCESS;
	PEPROCESS		TargetEProcess = NULL;
	PEPROCESS		CurrentEProcess = NULL;
	UCHAR*			lpszImageName = NULL;//母进程
	UCHAR*			lpszProcName = NULL;

	if(Create)
	{
		LOG_INFO("[DataAccessDetection]新线程被创建! PID=%ld\r\n",ProcessId);
		//主调进程
		CurrentEProcess = PsGetCurrentProcess();
		lpszImageName = (UCHAR*)PsGetProcessImageFileName(CurrentEProcess);
		LOG_INFO("[DataAccessDetection]母进程名:%s\r\n",lpszImageName);
				
		ns = PsLookupProcessByProcessId(ProcessId, &TargetEProcess);
		if(NT_SUCCESS(ns))
		{
			lpszProcName = PsGetProcessImageFileName(TargetEProcess);
			LOG_INFO("[DataAccessDetection]该线程所属的进程名为:%s\r\n",lpszProcName);
			ObfDereferenceObject(TargetEProcess);
		}
	}
}




//用于判定是不是我们监控的游戏
ULONG
IsOurDetectionGames(char* szProcName,HANDLE ProcessId,BOOLEAN IsCreate)
{
	LONG		i = 0;
	ULONG		uLength = 0;
	ULONG		ulCount = 0;

	for (i=0;i<(LONG)(g_pMyModuleJudgeInfo->ulCount);i++ )
	{
		uLength = strlen(g_pMyModuleJudgeInfo->ProcessModules[i].ImageName);

		if(szProcName && (strlen(szProcName)==uLength))		//字符串必须存在，然后长度要一样。首要条件
		{
			if((_strnicmp(g_pMyModuleJudgeInfo->ProcessModules[i].ImageName,szProcName,uLength)==0)  )//然后再比较字符串是否一样
			{
				if(IsCreate)
				{
					g_pMyModuleJudgeInfo->ProcessModules[i].IsDoubleProc_Createcount++;
					ulCount = g_pMyModuleJudgeInfo->ProcessModules[i].IsDoubleProc_Createcount;
				}
				else
				{
					g_pMyModuleJudgeInfo->ProcessModules[i].IsDoubleProc_Exitcount++;
					ulCount = g_pMyModuleJudgeInfo->ProcessModules[i].IsDoubleProc_Createcount;
				}

				//判断是不是双进程
				if( ((ulCount==1) && (g_pMyModuleJudgeInfo->ProcessModules[i].IsDoubleProc==0) )||
					( (ulCount==2) && (g_pMyModuleJudgeInfo->ProcessModules[i].IsDoubleProc==1) ) )
				{
					g_pMyModuleJudgeInfo->ProcessModules[i].GamePID = ProcessId;
					return 1;
				}
				
			}
		}
	}

	return 0;
}