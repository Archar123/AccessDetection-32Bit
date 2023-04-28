#include "MemoryPage.h"
#include "Universal.h"



//ͨ��ҳ����� �õ�ҳ��
/*
0��ʾʧ��
1��ʾ�ɹ�
2��ʾ�Ѿ��޸ĳɴ��ڣ�����Ҫ�ظ��޸�
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
				
				LOG_INFO("[DataProtect]�޸�ǰ��Pte Is: 0x%p;\r\n",*pPte);
				
				//�ж�0�������1 ҲҪ�ĳ�1��ô�ͷ���2

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
				
				LOG_INFO("[DataProtect]�޸ĺ��Pte Is: 0x%p;\r\n",*pPte);
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
//ͨ�õĺ���
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
	__asm _emit 0xE0   //����mov eax��cr4
}


//ˢ��TLB
VOID
FlushTLB(PVOID VirAddr)
{
	__asm
	{
		INVLPG	VirAddr
	}	
}
