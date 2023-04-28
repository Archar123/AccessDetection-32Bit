#include "ModuleJudge.h"
#include "Universal.h"


KSPIN_LOCK					g_Judge_Lock;
KSPIN_LOCK					g_Print_Lock;
KTIMER							TimerLog;
KDPC								DPCLog;
LARGE_INTEGER			IntervalLog;

PModuleJudgeInfo		g_pMyModuleJudgeInfo = NULL;


VOID
ModuleJudge_Init()
{
	NTSTATUS	Status = STATUS_SUCCESS;

	g_pMyModuleJudgeInfo = ExAllocatePoolWithTag(NonPagedPool,sizeof(TModuleJudgeInfo)*16,'MODL');
	memset(g_pMyModuleJudgeInfo,0,sizeof(TModuleJudgeInfo)*16);
	
//	给结构体赋值
	MyCopyMemory((char*)(g_pMyModuleJudgeInfo->ProcessModules[0].ImageName), "VCop2.exe",256);
	g_pMyModuleJudgeInfo->ProcessModules[0].IsDoubleProc = 0;	//未开启双进程保护
	g_pMyModuleJudgeInfo->ulCount = 1;//现在初始化还输手动进行修改   这表示只有一个进程

	KeInitializeSpinLock(&g_Judge_Lock);
	KeInitializeSpinLock(&g_Print_Lock);

	//设置DPC
	KeInitializeDpc(&DPCLog,(PKDEFERRED_ROUTINE)DpcCallBack_ModuleJudge,NULL);
	KeInitializeTimer(&TimerLog);
	IntervalLog.QuadPart=-(10000*1000*30);
	KeSetTimer(&TimerLog,IntervalLog,&DPCLog);

	//设置模块回调
	//模块回调函数
	Status = PsSetLoadImageNotifyRoutine((PLOAD_IMAGE_NOTIFY_ROUTINE)MyLoadImageNotify);
	DbgPrint("[DataAccessDetection]PsSetLoadImageNotifyRoutine return: %x;0 Is SUCCESS\r\n",Status);
}

VOID
ModuleJudge_UnInit()
{
	//卸载模块回调
	PsRemoveLoadImageNotifyRoutine((PLOAD_IMAGE_NOTIFY_ROUTINE)MyLoadImageNotify);
	DbgPrint("[DataAccessDetection]PsRemoveLoadImageNotifyRoutine return SUCCESS!\r\n");

	KeCancelTimer(&TimerLog);

	if(g_pMyModuleJudgeInfo!=NULL)
		ExFreePoolWithTag(g_pMyModuleJudgeInfo,'MODL');
}


//普通判定接口
VOID  	
__stdcall	
DataAccessDetection_NormalJudge(ULONG EIp)
{
	KIRQL	Irql;
	LONG	i = 0;
	KeAcquireSpinLock(&g_Judge_Lock,&Irql);

	for(i=0;i<(LONG)g_pMyModuleJudgeInfo->ProcessModules[0].ulCount;i++)	//已知模块
	{		
		if( ( EIp>=(g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].StartAddress) )	&&	(EIp<=(g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].EndAddress) ) )
		{
			g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].UserCount++;
			break;
		}
	}

	if(i>=(LONG)g_pMyModuleJudgeInfo->ProcessModules[0].ulCount)	//未知模块
		DbgPrint("[DataAccessDetection] Module Judge Is Illegal ! EIpIs:%x \r\n",EIp);

	KeReleaseSpinLock(&g_Judge_Lock,Irql);
}



VOID 		
__stdcall	
PrintModuleAcess()
{
	KIRQL		Irql;
	LONG 		i;
	KeAcquireSpinLock(&g_Print_Lock,&Irql);

	for(i=0;i<(LONG)g_pMyModuleJudgeInfo->ProcessModules[0].ulCount;i++)
	{
		if( (g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].UserCount) >0 )
		{
			DbgPrint("[DataAccessDetection]序号=%d;Name=%s;访问次数%d.\r\n",i,g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].DllName,g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].UserCount);
		}
	}

	KeReleaseSpinLock(&g_Print_Lock,Irql);
}



VOID
DpcCallBack_ModuleJudge(
	IN	PKDPC	Dpc,
	IN	PVOID	DeferredContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2)
{
	PrintModuleAcess();
	KeSetTimer(&TimerLog,IntervalLog,&DPCLog);
}




//回调函数
//_________________________________________________________________________________________________________________________________________

//模块加载回调
VOID
MyLoadImageNotify(
	PUNICODE_STRING FullImageName,
	HANDLE	ProcessId,
	PIMAGE_INFO	ImageInfo)
{
	NTSTATUS	Status = STATUS_SUCCESS;

	if(	(FullImageName!=NULL) &&  MmIsAddressValid(FullImageName)	)
	{
		if(ProcessId==0)	//驱动
		{}
		else	//if(ProcessId == g_GamePID)	//DLL
		{
			LONG				uRetIndex = 0;
			uRetIndex = GetProcessModulesIndexFromPId(ProcessId);
			if(uRetIndex!=-1)
			{
				ULONG				ulDllSize = 0;
				ULONG				ulCount = 0;
				PVOID				pDLLBase;
				PEPROCESS		TargetEProcess = NULL;
				char					szDllInforName[256]={0};
				ulDllSize			=	ImageInfo->ImageSize;
				pDLLBase 			= ImageInfo->ImageBase;
			
				ulCount = g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ulCount;
				if (ulCount<4096)
				{
					//给节点赋值
					UnicodeToChar(FullImageName,szDllInforName);
					MyCopyMemory((char*)(g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ModuleModes[ulCount].DllName), szDllInforName,256);

					//给节点赋值
					g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ModuleModes[ulCount].StartAddress	= (ULONG)pDLLBase;
					g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ModuleModes[ulCount].EndAddress	 = (ULONG)pDLLBase + ulDllSize;

					//多了一个模块
					g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ulCount++;
				}
				else
					DbgPrint("[DataAccessDetection][Error] Module Num Is Too Large!\r\n");
			}
		}
	}
}



LONG
GetProcessModulesIndexFromPId(HANDLE PId)
{
	LONG	i = 0;

	for(i=0;i<(LONG)(g_pMyModuleJudgeInfo->ulCount);i++)
	{
		if(PId = g_pMyModuleJudgeInfo->ProcessModules[i].GamePID)
			return i;
	}

	//说明找的不是我们的游戏PID
	if(i==(LONG)(g_pMyModuleJudgeInfo->ulCount))
		return -1;

	return -1;
}
