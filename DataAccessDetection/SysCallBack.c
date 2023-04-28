#include "SysCallBack.h"
#include "Universal.h"
#include "ModuleJudge.h"


extern				PModuleJudgeInfo		g_pMyModuleJudgeInfo;



//�ⲿ���ýӿ�  ���ý��̺��̻߳ص�
VOID
DataAccessDetection_SysCallBack_Init()
{
	NTSTATUS Status;
	//���̻ص�����
	Status = PsSetCreateProcessNotifyRoutine((PCREATE_PROCESS_NOTIFY_ROUTINE)MyCreateProcessNotify,FALSE);
	DbgPrint("[DataAccessDetection]PsSetCreateProcessNotifyRoutine return: %x;0 Is SUCCESS\r\n",Status);
	//�̻߳ص�����
	Status = PsSetCreateThreadNotifyRoutine(MyCreateThreadNotify);
	DbgPrint("[DataAccessDetection]PsSetCreateThreadNotifyRoutine return: %x;0 Is SUCCESS\r\n",Status);

}

VOID
DataAccessDetection_SysCallBack_UnInit()
{
	//ж�ؽ��̻ص�
	PsSetCreateProcessNotifyRoutine((PCREATE_PROCESS_NOTIFY_ROUTINE)MyCreateProcessNotify,TRUE);
	DbgPrint("[DataAccessDetection]PsSetCreateProcessNotifyRoutine return SUCCESS!\r\n");
	//ж���̻߳ص�
	PsRemoveCreateThreadNotifyRoutine(MyCreateThreadNotify);
	DbgPrint("[DataAccessDetection]PsRemoveCreateThreadNotifyRoutine return SUCCESS!\r\n");

}


//_________________________________________________________________________________________________________________________________________
//�ص�����		
//���̻ص�
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

			ulRet = IsOurDetectionGames(lpszProcName,ProcessId,1);//��1��ʾ�Ǵ�������
			if(ulRet==1)
				DbgPrint("[DataAccessDetection]��⵽ Ŀ����̣����ԣ� ����PID!  ������Ϊ:%s;\r\n",lpszProcName);
			
			ObfDereferenceObject(TargetEProcess);
		}
		
	}
	else
	{
		Status = PsLookupProcessByProcessId(ProcessId, &TargetEProcess);
		if(NT_SUCCESS(Status))
		{
			lpszProcName = PsGetProcessImageFileName(TargetEProcess);

			ulRet = IsOurDetectionGames(lpszProcName,ProcessId,0);//��0��ʾ�ǽ�������
			if(ulRet==1)
				DbgPrint("[DataAccessDetection]��⵽ Ŀ����̣����ԣ� �˳�PID!  ������Ϊ:%s;\r\n",lpszProcName);

			ObfDereferenceObject(TargetEProcess);
		}
	}

}




//ע��һ��ϵͳ�̻߳ص�
VOID 
MyCreateThreadNotify(
	IN HANDLE  	ProcessId,
	IN HANDLE  	ThreadId,
	IN BOOLEAN  Create)
{
	NTSTATUS		ns = STATUS_SUCCESS;
	PEPROCESS		TargetEProcess = NULL;
	PEPROCESS		CurrentEProcess = NULL;
	UCHAR*			lpszImageName = NULL;//ĸ����
	UCHAR*			lpszProcName = NULL;

	if(Create)
	{
		LOG_INFO("[DataAccessDetection]���̱߳�����! PID=%ld\r\n",ProcessId);
		//��������
		CurrentEProcess = PsGetCurrentProcess();
		lpszImageName = (UCHAR*)PsGetProcessImageFileName(CurrentEProcess);
		LOG_INFO("[DataAccessDetection]ĸ������:%s\r\n",lpszImageName);
				
		ns = PsLookupProcessByProcessId(ProcessId, &TargetEProcess);
		if(NT_SUCCESS(ns))
		{
			lpszProcName = PsGetProcessImageFileName(TargetEProcess);
			LOG_INFO("[DataAccessDetection]���߳������Ľ�����Ϊ:%s\r\n",lpszProcName);
			ObfDereferenceObject(TargetEProcess);
		}
	}
}




//�����ж��ǲ������Ǽ�ص���Ϸ
ULONG
IsOurDetectionGames(char* szProcName,HANDLE ProcessId,BOOLEAN IsCreate)
{
	LONG		i = 0;
	ULONG		uLength = 0;
	ULONG		ulCount = 0;

	for (i=0;i<(LONG)(g_pMyModuleJudgeInfo->ulCount);i++ )
	{
		uLength = strlen(g_pMyModuleJudgeInfo->ProcessModules[i].ImageName);

		if(szProcName && (strlen(szProcName)==uLength))		//�ַ���������ڣ�Ȼ�󳤶�Ҫһ������Ҫ����
		{
			if((_strnicmp(g_pMyModuleJudgeInfo->ProcessModules[i].ImageName,szProcName,uLength)==0)  )//Ȼ���ٱȽ��ַ����Ƿ�һ��
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

				//�ж��ǲ���˫����
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