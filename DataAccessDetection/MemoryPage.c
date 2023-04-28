#include "MemoryPage.h"
#include "Universal.h"



//通过页表机制 得到页表。
/*
0表示失败
1表示成功
2表示已经修改成存在，不需要重复修改
*/
ULONG
DataProtect_SetPTEBySelf(PVOID VirtualAddress,PEPROCESS TargetEProcess,ULONG IsValidOrNot,ULONG SetIndex)
{
		ULONG		ulRet = 0;
		KAPC_STATE	ApcState;
		KeStackAttachProcess(TargetEProcess, &ApcState);
	
		if(JudgeIsPAE())
			ulRet = GetPTEHasPAEBySelf(VirtualAddress,IsValidOrNot,SetIndex);
		else
			ulRet = GetPTENoPAEBySelf(VirtualAddress,IsValidOrNot,SetIndex);


		FlushTLB(VirtualAddress);
		KeUnstackDetachProcess(&ApcState);
		
		return ulRet;	
}


ULONG
GetPTENoPAEBySelf(PVOID VirtualAddress,ULONG IsValidOrNot,ULONG SetIndex)
{
		MMPTE*	pPde;
		MMPTE*	pPte;
		
		pPde = MiGetPdeAddress(VirtualAddress);
		
		if(pPde->u.Hard.Valid)
		{
				pPte = MiGetPteAddress(VirtualAddress);	
				
				if(SetIndex==0) //Valid
				{
						LOG_INFO("[DataProtect]Before Valid Is: %d",pPte->u.Hard.Valid);
						if ( (pPte->u.Hard.Valid==1) && (IsValidOrNot==1) )
							return 2;
						pPte->u.Hard.Valid = IsValidOrNot;
						LOG_INFO("[DataProtect]After Valid Is: %d",pPte->u.Hard.Valid);	
				}
				else if(SetIndex==1)//Write
				{
						LOG_INFO("[DataProtect]Before Valid Is: %d",pPte->u.Hard.Write);
						pPte->u.Hard.Write = IsValidOrNot;
						LOG_INFO("[DataProtect]After Valid Is: %d",pPte->u.Hard.Write);
				}
				else
				{
				}	
		}
		else
		{
			return 0;
		}
		
		return 1;
}



ULONG
GetPTEHasPAEBySelf(PVOID VirtualAddress,ULONG IsValidOrNot,ULONG SetIndex)
{
		MMPTE_PAE* pPde;
		MMPTE_PAE* pPte;
	
		pPde = MiGetPdeAddressPae(VirtualAddress);
	
		if( pPde->u.Hard.Valid )
		{
				if( pPde->u.Hard.LargePage != 0 )			//2M
					pPte = pPde;
				else														//4K
					pPte  = MiGetPteAddressPae(VirtualAddress);
				
				LOG_INFO("[DataProtect]修改前的Pte Is: 0x%p;\r\n",*pPte);
				
				//判断0好如果是1 也要改成1那么就返回2

				if(SetIndex==0) //Valid
				{
						LOG_INFO("[DataProtect]Before Valid Is: %d",pPte->u.Hard.Valid);
						if ( (pPte->u.Hard.Valid==1) && (IsValidOrNot==1) )
							return 2;
						pPte->u.Hard.Valid = IsValidOrNot;
						LOG_INFO("[DataProtect]After Valid Is: %d",pPte->u.Hard.Valid);	
				}
				else if(SetIndex==1)//Write
				{
						LOG_INFO("[DataProtect]Before Valid Is: %d",pPte->u.Hard.Write);	
						pPte->u.Hard.Write = IsValidOrNot;
						LOG_INFO("[DataProtect]After Valid Is: %d",pPte->u.Hard.Write);
				}
				else if(SetIndex==63)
				{
						LOG_INFO("[DataProtect]Before Valid Is: %d",pPte->u.Hard.dep);	
						pPte->u.Hard.dep = IsValidOrNot;
						LOG_INFO("[DataProtect]After Valid Is: %d",pPte->u.Hard.dep);	
				}
				else
				{
				}
				
				LOG_INFO("[DataProtect]修改后的Pte Is: 0x%p;\r\n",*pPte);
		}
		else
		{
			return 0;
		}

		return 1;
}


//////////////////////////////////////
///////
//
//通用的函数
ULONG
JudgeIsPAE()
{
		ULONG		IsPAE = 0;		
	
		if( (CR4() & 0x00000020) == 0x00000020 )
				IsPAE = 1;	
		return IsPAE;
}


ULONG 
CR4()
{
	__asm _emit 0x0F 
	__asm _emit 0x20 
	__asm _emit 0xE0   //就是mov eax，cr4
}


//刷新TLB
VOID
FlushTLB(PVOID VirAddr)
{
	__asm
	{
		INVLPG	VirAddr
	}	
}
