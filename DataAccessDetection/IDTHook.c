#include "IDTHook.h"
#include "Universal.h"
#include "DataAccessDetection_Struct.h"
#include "ModuleJudge.h"
#include "MemoryPage.h"



//HOOK	IDT
KEVENT										g_Event;
ULONG										g_CurrentCpuAffinity = 0;		//��ǰ��CPU
ULONG										g_uOldTrap01;								
ULONG										g_uOldTrap0E;		



#pragma	optimize("",off)
//Int 1 Hook����

//EFLAG
//CS
//IP
void	__declspec(naked)	NewTrap01(void)
{

	__asm
	{
			push edx
			push eax
			pushfd
			pushad
			
			
			mov eax,Cr3
			push eax
			call FilterProcess
			cmp eax,0
			je PassDown		
			
			mov eax,Dr6
			push eax
			mov eax,Cr2
			push eax
			
			push fs

			mov eax,0x30
			mov fs,ax	

			mov eax, esp
			add eax,0x4
			
			push eax
			call MyOneStep
			

			pop fs
			
			add esp,0x8
			cmp eax,0
			je	PassDown
			
			xor eax,eax
			mov dr6,eax
			popad
			popfd
			pop eax
			pop edx
			iretd
			
PassDown:		
			popad
			popfd
			pop eax
			pop edx
			jmp g_uOldTrap01
	}

}

//EFLAG
//CS
//IP
//Error Code
void 	__declspec(naked) NewTrap0E(void)
{
		__asm
		{
				push eax
				pushfd
				pushad
				
				mov eax,dword ptr [esp+0x28]
				test	eax,0x01
				jne	PassDown

				
				mov eax,Cr3
				push eax
				call FilterProcess
				cmp eax,0
				je PassDown			
								
				mov eax,Cr3
				push eax
				mov eax,Cr2
				push eax
				
				push fs

				mov eax,0x30
				mov fs,ax
			
				
				mov eax, esp
				add eax,0x4
				
				push eax
				call MyPageFault
				
				pop fs
				
				add esp,0x8			
				cmp	eax,0
				je	PassDown

				popad	
				popfd
				pop	eax
				add esp,0x4
				iretd
							
				
PassDown:
				popad	
				popfd
				pop	eax
				jmp	g_uOldTrap0E
						
		}
	
}

#pragma optimize("",on)

////////////////////////////////////////////////////////////////////////////////
/*
1��ʾ���������õ�int1
0��ʾ����
*/
ULONG
__stdcall
MyOneStep(ULONG* TrapFrame)
{
	ULONG		RetValue;
	ULONG		CodeSize;
	ULONG		SetDBEIp;
	ULONG		ErrorAddress;
	ULONG 		ErrorPteAddress;
	ULONG		DB_Dr6;
    ULONG		DB_Cr2;
	ULONG		DB_EIp;
	PVOID 		EProcess;
	PVOID			EThread;

	DB_Dr6 = TrapFrame[TRAPFRAME_DR6];
	DB_Cr2 = TrapFrame[TRAPFRAME_CR2];
	DB_EIp = TrapFrame[TRAPFRAME_DEIP];
  
	EProcess = PsGetCurrentProcess();
	EThread  = PsGetCurrentThread();

	//��������ָ��������TF��־�����
	if( (DB_Dr6&0x00004000) == 0x00004000 )//BS λΪ1
	{
		RetValue = FindCellList_GetEIp(EProcess,&SetDBEIp,(ULONG)EThread,&ErrorAddress);
		LOG_INFO("[DataAccessDetection]#DB_Current EIP Is:%x\r\n",DB_EIp);
		if(RetValue!=0)
		{

			//if(RetValue==2)				//����ģ���ж�
			//{
			//	DbgPrint("[DataAccessDetection] Judge The Point\r\n");
				DataAccessDetection_NormalJudge(SetDBEIp);
			//}

			//�����ֶ�����һ�� EFlags��TFλ
			TrapFrame[TRAPFRAME_DFLG] &=  0xFFFFFEFF;
			LOG_INFO("[DataAccessDetection]#DB_�޸�TFλ֮��ջ�ϵ�EFlagsIs:%x\r\n",TrapFrame[TRAPFRAME_DFLG]);

			//if (TheDataProtectIsClose(EProcess)==0)
			//{
				//����PλΪ0
				ErrorPteAddress = ErrorAddress&0xFFFFF000;
				DataProtect_SetPTEBySelf((PVOID)ErrorPteAddress,EProcess,0,0);
			//}

			LOG_INFO("[DataAccessDetection]DB__________________________________________________________________DB\r\n");
			return 1;
		  }
	}
  
  return 0;
}


/*
1��ʾ�������ǵ��ж�
0��ʾ����ϵͳ�Լ����ж�
*/
ULONG
__stdcall
MyPageFault(ULONG* TrapFrame)
{
	
	ULONG		PF_Cr2;
	ULONG		PF_Cr3;
	ULONG		PF_EIp;
	ULONG		RetValue;
	ULONG 	ErrorPteAddress;
	PVOID 		EProcess; 
	PVOID		EThread;

	PF_Cr2			= TrapFrame[TRAPFRAME_CR2];
	PF_Cr3			= TrapFrame[TRAPFRAME_CR3];
	PF_EIp			= TrapFrame[TRAPFRAME_TEIP];
	EProcess		= PsGetCurrentProcess();
	EThread		= PsGetCurrentThread();

	//���ҵ�
	RetValue = FindCellList_FindPoint(EProcess,PF_Cr2,PF_EIp,(ULONG)EThread);
	if(RetValue==0)	//��ʾ����㣨ҳ������û�н��б���
	{
		return 0;
	}
	else
	{
		ErrorPteAddress = PF_Cr2&0xFFFFF000;			
		RetValue = DataProtect_SetPTEBySelf((PVOID)ErrorPteAddress,EProcess,1,0);

		if(RetValue!=2)
		{
			TrapFrame[TRAPFRAME_TFLG] |= 0x00000100;	//TF			//�޸�TF��־λ
			LOG_INFO("[DataAccessDetection]#PF_�޸�TFλ֮��ջ�ϵ�EFlagsIs:%x\r\n",TrapFrame[TRAPFRAME_TFLG]);
		}

		LOG_INFO("[DataAccessDetection]PF__________________________________________________________________PF\r\n");
		return 1;
	}

	return 0;
}




/////////////////////////////////
//////////
//
VOID
DataAccessDetection_IDT_Init()
{
	 DataAccessDetection_HOOKIDT(&g_uOldTrap01,0x01,(ULONG)NewTrap01);
	 DbgPrint("[DataAccessDetection]HOOK Trap01!\r\n");
	 	
	 DataAccessDetection_HOOKIDT(&g_uOldTrap0E,0x0E,(ULONG)NewTrap0E); 
	 DbgPrint("[DataAccessDetection]HOOK Trap0E!\r\n");	
}


VOID
DataAccessDetection_IDT_UnInit()
{
	DataAccessDetection_UNHOOKIDT(0x01, g_uOldTrap01);	
	DbgPrint("[DataAccessDetection]UNHOOK Trap01!\r\n");
			
	DataAccessDetection_UNHOOKIDT(0x0E, g_uOldTrap0E);
	DbgPrint("[DataAccessDetection]UNHOOK Trap0E!\r\n");
}




//�ⲿ���ýӿ�
VOID
DataAccessDetection_HOOKIDT(ULONG *OldTrapAddr,ULONG InterruptIndex,ULONG NewInterruptFunc)
{
		*OldTrapAddr = GetInterruptFuncAddress(InterruptIndex);
		Z_HookIdtByDpc(InterruptIndex, (ULONG)NewInterruptFunc);	
}

VOID
DataAccessDetection_UNHOOKIDT(ULONG InterruptIndex,ULONG OldInterruptFunc)
{
		Z_HookIdtByDpc(InterruptIndex, (ULONG)OldInterruptFunc);		
}








////////////////////////////////////////////////////////////////////////////////
//���ȵõ�IDT�����ĵ�ַ
ULONG
GetInterruptFuncAddress(ULONG InterruptIndex)
{
		IDTR 				idtr;
		PIDTENTRY 	pIdtEntry;
		__asm SIDT 	idtr;
		pIdtEntry = (PIDTENTRY)MAKELONG(idtr.IDT_LOWbase,idtr.IDT_HIGbase);
		return MAKELONG(pIdtEntry[InterruptIndex].LowOffset, pIdtEntry[InterruptIndex].HigOffset);
}

//HOOK �Ĺ���
VOID
HookInterrupt(ULONG uCPU,ULONG InterruptIndex,ULONG NewInterruptFunc)
{
		IDTR				idtr;
		PIDTENTRY		pIdtEntry = NULL;
		__asm SIDT		idtr
		pIdtEntry = (PIDTENTRY)MAKELONG(idtr.IDT_LOWbase, idtr.IDT_HIGbase);

		DbgPrint("[DataAccessDetection]uCPU:%d---pIdtEntry: %X\n", uCPU, pIdtEntry);
		
		PageProtectOff();
		
		pIdtEntry[InterruptIndex].LowOffset = (USHORT)((ULONG)NewInterruptFunc & 0xFFFF);
		pIdtEntry[InterruptIndex].HigOffset = (USHORT)((ULONG)NewInterruptFunc >> 16);
		
		PageProtectOn();
}

//DPC�����н���HOOK
VOID
HookIdtDpc(
					IN struct _KDPC *Dpc,
					IN PVOID 	DeferredContext,
					IN PVOID  SystemArgument1,	//���������
					IN PVOID  SystemArgument2)	//�����ĵ�ַ
{
		HookInterrupt(g_CurrentCpuAffinity,(ULONG)SystemArgument1,(ULONG)SystemArgument2);
		
		KeSetEvent(&g_Event,IO_NO_INCREMENT,FALSE);
}


//HOOK �ӿ�
VOID
Z_HookIdtByDpc(ULONG InterruptIndex,ULONG NewInterruptFunc)
{
		KAFFINITY		CpuAffinity;
		ULONG				uCpuCount = 0;
		ULONG				i = 0;
		KDPC				Dpc;
		
		//�������ڻ��CPU
		CpuAffinity = KeQueryActiveProcessors();
		
		for(i=0;i<sizeof(KAFFINITY);i++)
		{
				if( (CpuAffinity>>i) & 1 )
				{
						uCpuCount++;	
				}
		}
		
		//����ǵ��˵Ļ�
		if(uCpuCount==1)
		{
			 	KIRQL	OldIrql = KeRaiseIrqlToDpcLevel();
			 	HookInterrupt(0,InterruptIndex,NewInterruptFunc);
			 	KeLowerIrql(OldIrql);
		}
		
		//��˵������
		else
		{
				for(i=0;i<sizeof(KAFFINITY);i++)
				{
						if( (CpuAffinity>>i) & 1)
						{
								g_CurrentCpuAffinity = i;
								
								//ÿһ�ι��ض������¼����ж��Ƿ���ɵ�
								KeInitializeEvent(&g_Event,NotificationEvent,FALSE);
								
								KeInitializeDpc(&Dpc,HookIdtDpc,NULL);
								KeSetTargetProcessorDpc(&Dpc,(CCHAR)i);
								KeSetImportanceDpc(&Dpc,HighImportance);
								KeInsertQueueDpc(&Dpc,(PVOID)InterruptIndex, (PVOID)NewInterruptFunc);
								
								
								//ֱ����һ��CPU�Ѿ�HOOK��ɣ��Ž�����һ�ε�HOOK
								if (KeWaitForSingleObject(&g_Event, (KWAIT_REASON)0, 0, 0, 0) == STATUS_SUCCESS)
								{
									continue;
								}
						}	
				}
		}
			
}


//######################################