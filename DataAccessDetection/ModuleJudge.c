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
	
//	���ṹ�帳ֵ
	MyCopyMemory((char*)(g_pMyModuleJudgeInfo->ProcessModules[0].ImageName), "VCop2.exe",256);
	g_pMyModuleJudgeInfo->ProcessModules[0].IsDoubleProc = 0;	//δ����˫���̱���
	g_pMyModuleJudgeInfo->ulCount = 1;//���ڳ�ʼ�������ֶ������޸�   ���ʾֻ��һ������

	KeInitializeSpinLock(&g_Judge_Lock);
	KeInitializeSpinLock(&g_Print_Lock);

	//����DPC
	KeInitializeDpc(&DPCLog,(PKDEFERRED_ROUTINE)DpcCallBack_ModuleJudge,NULL);
	KeInitializeTimer(&TimerLog);
	IntervalLog.QuadPart=-(10000*1000*30);
	KeSetTimer(&TimerLog,IntervalLog,&DPCLog);

	//����ģ��ص�
	//ģ��ص�����
	Status = PsSetLoadImageNotifyRoutine((PLOAD_IMAGE_NOTIFY_ROUTINE)MyLoadImageNotify);
	DbgPrint("[DataAccessDetection]PsSetLoadImageNotifyRoutine return: %x;0 Is SUCCESS\r\n",Status);
}

VOID
ModuleJudge_UnInit()
{
	//ж��ģ��ص�
	PsRemoveLoadImageNotifyRoutine((PLOAD_IMAGE_NOTIFY_ROUTINE)MyLoadImageNotify);
	DbgPrint("[DataAccessDetection]PsRemoveLoadImageNotifyRoutine return SUCCESS!\r\n");

	KeCancelTimer(&TimerLog);

	if(g_pMyModuleJudgeInfo!=NULL)
		ExFreePoolWithTag(g_pMyModuleJudgeInfo,'MODL');
}


//��ͨ�ж��ӿ�
VOID  	
__stdcall	
DataAccessDetection_NormalJudge(ULONG EIp)
{
	KIRQL	Irql;
	LONG	i = 0;
	KeAcquireSpinLock(&g_Judge_Lock,&Irql);

	for(i=0;i<(LONG)g_pMyModuleJudgeInfo->ProcessModules[0].ulCount;i++)	//��֪ģ��
	{		
		if( ( EIp>=(g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].StartAddress) )	&&	(EIp<=(g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].EndAddress) ) )
		{
			g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].UserCount++;
			break;
		}
	}

	if(i>=(LONG)g_pMyModuleJudgeInfo->ProcessModules[0].ulCount)	//δ֪ģ��
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
			DbgPrint("[DataAccessDetection]���=%d;Name=%s;���ʴ���%d.\r\n",i,g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].DllName,g_pMyModuleJudgeInfo->ProcessModules[0].ModuleModes[i].UserCount);
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




//�ص�����
//_________________________________________________________________________________________________________________________________________

//ģ����ػص�
VOID
MyLoadImageNotify(
	PUNICODE_STRING FullImageName,
	HANDLE	ProcessId,
	PIMAGE_INFO	ImageInfo)
{
	NTSTATUS	Status = STATUS_SUCCESS;

	if(	(FullImageName!=NULL) &&  MmIsAddressValid(FullImageName)	)
	{
		if(ProcessId==0)	//����
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
					//���ڵ㸳ֵ
					UnicodeToChar(FullImageName,szDllInforName);
					MyCopyMemory((char*)(g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ModuleModes[ulCount].DllName), szDllInforName,256);

					//���ڵ㸳ֵ
					g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ModuleModes[ulCount].StartAddress	= (ULONG)pDLLBase;
					g_pMyModuleJudgeInfo->ProcessModules[uRetIndex].ModuleModes[ulCount].EndAddress	 = (ULONG)pDLLBase + ulDllSize;

					//����һ��ģ��
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

	//˵���ҵĲ������ǵ���ϷPID
	if(i==(LONG)(g_pMyModuleJudgeInfo->ulCount))
		return -1;

	return -1;
}
