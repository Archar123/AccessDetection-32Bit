#include "DataAccessDetection_Struct.h"
#include "MemoryPage.h"
#include "Universal.h"
#include "ModuleJudge.h"


//ȫ������ ����������Ϣ
PDataProtectInfo			g_pMyDataProtectInfo = NULL;
LONG								g_lMyDataProtectInfoIndex = 0;
KSPIN_LOCK					g_List_Lock;


//�������ʱ����
TCell 			tCellTemp 		= {0};
TPteNode 	tPteNodeTemp[64];


//��ʼ��
VOID
DataAccessDetection_Struct_Init()
{
	//��ʼ����
	KeInitializeSpinLock(&g_List_Lock);
	//����ά�� �������������Ľṹ
	g_pMyDataProtectInfo = ExAllocatePoolWithTag(NonPagedPool,sizeof(TDataProtectInfo)*16,'INIT');
	memset(g_pMyDataProtectInfo,0,sizeof(TDataProtectInfo)*16);
}

//����ʼ��
VOID
DataAccessDetection_Struct_UnInit()
{
	//�ͷ��ڴ�
	if(g_pMyDataProtectInfo)
		ExFreePoolWithTag(g_pMyDataProtectInfo,'INIT');
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														һЩ�ڲ��ӿ�																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//����ĳ�ʼ��
NTSTATUS
InitCellList(ULONG ulIndex)
{	
	NTSTATUS Status = STATUS_SUCCESS;
	//��ʼ�� �� uIndex ����������� 
	InitializeListHead(&(g_pMyDataProtectInfo->DataProtectItems[ulIndex].CellsHead));
	return Status;
}


//ɾ�����еĽڵ㣬����ȥ������
NTSTATUS
DeleteCellList_DelAll(ULONG ulIndex)
{
	NTSTATUS			Status	= STATUS_SUCCESS;
	PLIST_ENTRY			next=NULL;
	PDataProtectCell	pCell=NULL;
	LONG					i=0;
	PVOID					EProcess = NULL;
	KIRQL		Irql;

	KeAcquireSpinLock(&g_List_Lock,&Irql);

	//ͨ�������õ�EProcess
	EProcess = g_pMyDataProtectInfo->DataProtectItems[ulIndex].lpvEProcess;
	while (!IsListEmpty(&(g_pMyDataProtectInfo->DataProtectItems[ulIndex].CellsHead)))
	{
		next = RemoveHeadList(&(g_pMyDataProtectInfo->DataProtectItems[ulIndex].CellsHead));
		pCell = CONTAINING_RECORD(next, TDataProtectCell, CellNext);
		//�����cell�������ҳ�涼���valid
		for(i=0; i<(LONG)(pCell->Cell.PageCount); i++)
		{
			if( (pCell->Cell.PteNodeLists[i].PageStartAddress)  !=0)
			{
				Status = DataProtect_SetPTEBySelf((PVOID)pCell->Cell.PteNodeLists[i].PageStartAddress,EProcess,1,0);
				MyUnLockMemory(EProcess,pCell->Cell.PteNodeLists[i].pMDL);
			}		
		}

		ExFreePoolWithTag(pCell,'CELL');
	}

	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].IsCloseProtect = 1;//��ʾ�ǹرձ�����
	g_pMyDataProtectInfo->DataProtectItems[ulIndex].Cr3 = 0;
	g_pMyDataProtectInfo->DataProtectItems[ulIndex].lpvEProcess = NULL;
	
	KeReleaseSpinLock(&g_List_Lock,Irql);	
	return Status;
}






/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												DataProtect_Mode ���ýӿ�															  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//��ӵ�
/////////////
//������һ��Point�������ʱ����������Ҫ�����������Ȼ��ҳ�����зָ�,�������������
//����ֱ���޸���Pte��ҳ������
NTSTATUS
PointsInit_SortAndCut(PVOID EProcess,PVOID AddressPointerList[],ULONG ulCount)
{
	NTSTATUS	ns		= STATUS_SUCCESS;
	NTSTATUS	Status		= STATUS_SUCCESS;
	LONG 			i 		= 0;
	LONG			j 		= 0;
	ULONG			ulPointerTemp	= 0;
	LONG 			PteIndex 		 	= 0;
	LONG			PointIndex 	 	= 0;
	ULONG			ulPageIndex  	= 0;
	LONG			MyCellIndex 		= 0;
	ULONG			PteCount			=	0;

	//(1)���Ƚ�������
	SortInputArrayQuickly(AddressPointerList,ulCount);

	//(2)����Ҫ����ҳ����зָ�
	for(i=0;i<(LONG)ulCount;i++)
	{
		ulPointerTemp = (ULONG)AddressPointerList[i];

		ulPageIndex = ulPointerTemp&0xFFFFF000;

		//���ǵ�һ����
		if(PointIndex!=0)
		{
			//�����ж��ǲ�����ͬһ��ҳ����
			if( (tPteNodeTemp[PteIndex].PageStartAddress) == ulPageIndex )
			{
				//�����ͬһ��ҳ����
				tPteNodeTemp[PteIndex].ProtectPoints[PointIndex] = ulPointerTemp;
				tPteNodeTemp[PteIndex].PointCount++;
				PointIndex++;
			}
			else//����ͬһ��ҳ����
			{
				//��� 1ҳ�� //����������
				PteIndex++;
				if((PteIndex+1)>63)
				{
					//Error
					break;	
				}
				PointIndex = 0;
				i--;
			}
		}
		else	//��һ�����ʱ��һ���ǵ�һ��ҳ�棨0ҳ�棩
		{
			tPteNodeTemp[PteIndex].PageStartAddress = ulPageIndex;
			tPteNodeTemp[PteIndex].ProtectPoints[PointIndex] = ulPointerTemp;
			tPteNodeTemp[PteIndex].PointCount++;
			PointIndex++;

			//���Ƚ����ҳ����������
			tPteNodeTemp[PteIndex].pMDL=NULL;
			MyLockMemory((PVOID)ulPageIndex,0x1000,EProcess,&tPteNodeTemp[PteIndex].pMDL);
			//����ҳ�������Ϊ 0
			DataProtect_SetPTEBySelf((PVOID)ulPageIndex,EProcess,0,0);

			if (Status!=STATUS_SUCCESS)
			{
				ERR_INFO("[DataProtect][Error] Pde Is Invalid \r\n");
			}
		}

	}

	//(3)����ҳ��ָ���Ϻ��ٽ����ڵ�ҳ�����һ��Cell�Ȼ����뵽������

	InitCellList(g_lMyDataProtectInfoIndex);

	//PteIndex+1 �������ж��ٸ�Pte
	PteCount = PteIndex+1;
	i = 0;
	PteIndex = 0;	//ÿ��Cell��Pte�����

	for(i=0;i<(LONG)PteCount;i++)
	{
		if(PteIndex!=0)
		{
			//˵��ǰ������Pte�ǽ����ŵ�
			if( (tPteNodeTemp[i-1].PageStartAddress + 0x1000) == (tPteNodeTemp[i].PageStartAddress) )
			{
				//�ŵ�ͬһ��Cell��
				tCellTemp.PageCount++;
				tCellTemp.UsedPageCount++;
				tCellTemp.PageSize+=0x1000;
				tCellTemp.PteNodeLists[PteIndex] = tPteNodeTemp[i];
				PteIndex++;

				//����Ѿ������һ�� ��ô˵������ͬһ��
				if(i==PteCount-1)
				{
					PDataProtectCell pMyDataProtectCell = NULL;
					pMyDataProtectCell = ExAllocatePoolWithTag(NonPagedPool,sizeof(TDataProtectCell),'CELL');

					MyCopyMemory(&(pMyDataProtectCell->Cell),&tCellTemp,sizeof(TCell));
					pMyDataProtectCell->CellIndex = MyCellIndex;

					InsertTailList(&(g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].CellsHead), &(pMyDataProtectCell->CellNext));	
				}
			}
			else
			{
				//�������ǰ��ŵ���,����Ӧ������һ��Cell�ˣ������Cell�Ȳ��뵽�����У�Ȼ�����½���Cell�ĸ�ֵ
				//						��������					Cell�ṹ		Cell�����
				PDataProtectCell pMyDataProtectCell = NULL;
				pMyDataProtectCell = ExAllocatePoolWithTag(NonPagedPool,sizeof(TDataProtectCell),'CELL');

				MyCopyMemory(&(pMyDataProtectCell->Cell),&tCellTemp,sizeof(TCell));
				pMyDataProtectCell->CellIndex = MyCellIndex;

				InsertTailList(&(g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].CellsHead), &(pMyDataProtectCell->CellNext));

				memset(&tCellTemp,0,sizeof(tCellTemp));
				MyCellIndex++;
				PteIndex = 0;
				i--;
			}
		}
		else//������Cell�еĵ�һ��Pte	
		{
			//���ҳ���׵�ַ
			(ULONG)tCellTemp.PageAddress = tPteNodeTemp[i].PageStartAddress;
			tCellTemp.PageCount++;
			tCellTemp.UsedPageCount++;
			tCellTemp.PageSize+=0x1000;
			tCellTemp.PteNodeLists[PteIndex] = tPteNodeTemp[i];

			//����Ѿ������һ�� ��ô˵������ͬһ��
			if(i==PteCount-1)
			{
				PDataProtectCell pMyDataProtectCell = NULL;
				pMyDataProtectCell = ExAllocatePoolWithTag(NonPagedPool,sizeof(TDataProtectCell),'CELL');

				MyCopyMemory(&(pMyDataProtectCell->Cell),&tCellTemp,sizeof(TCell));
				pMyDataProtectCell->CellIndex = MyCellIndex;

				InsertTailList(&(g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].CellsHead), &(pMyDataProtectCell->CellNext));	
			}

			PteIndex++;	
		}

	}
	//TIPS:
	//����ṹ��Cr3��#PF�еĲ�����أ������ڳ�ʼ�����֮���ڽ������ã������ڽ��м�ص�ʱ����ӱ�����Ĳ����Ѿ������

	//������ֵ��Ҫ������ �����жϽ��̵�
	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].lpvEProcess = EProcess;
	//ͨ��EProcess�õ�Cr3
	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].Cr3 = GetCr3FromEProcess_XPSP3(EProcess);

	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].IsCloseProtect = 0;//��ʾ�ǿ���������
	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].AccessCount++;
	//�����ʵ�Ǹ������ٲ���ڵ�֮����Ǹ�����
	g_lMyDataProtectInfoIndex++;
	//�ڴ���#PF��ʱ��Index�Ѿ�������

	//End
	return ns;
}











/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														IDT ���ýӿ�																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//��Trap0E ���õĽӿ�
//������һ���㣬�ж���û�������ǵĽṹ��
//return 0 ��ʾû���ҵ����Ӧ�ĵ㣬return 1�����ҵ������Ӧ�ĵ�
ULONG
FindCellList_FindPoint(PVOID EProcess,ULONG HitPoint,ULONG EIp,ULONG EThread)
{
	//���������������EProcess
	LONG		ItemIndex = 0;
	LONG 		i 	 = 0;
	LONG		j 	 = 0;
	LONG		k		 = 0;
	PLIST_ENTRY pFirst = NULL;
	PLIST_ENTRY pNext  = NULL;
	PDataProtectCell pMyDataProtectCell = NULL;
	
	KIRQL		Irql;
	KeAcquireSpinLock(&g_List_Lock,&Irql);
	
	ItemIndex = GetDataProtectItemsIndexFromEProcess(EProcess);

	if(ItemIndex==-1)
	{
		KeReleaseSpinLock(&g_List_Lock,Irql);	
		return 0;//����û���������
	}

	pFirst=&(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].CellsHead);
	//��һ��Cell
	pNext = pFirst->Flink;
	pMyDataProtectCell = CONTAINING_RECORD(pNext, TDataProtectCell, CellNext);

FindNext:	
	//���������� ���Cell �ķ�Χ��
	if( (HitPoint>=pMyDataProtectCell->Cell.PageAddress) && (HitPoint<(pMyDataProtectCell->Cell.PageAddress + pMyDataProtectCell->Cell.PageSize)) )
	{
		//ѭ������
		//pMyDataProtectCell->Cell.PageCount  �����Cell��Pte���ܸ���
		for(j=0;j<(LONG)(pMyDataProtectCell->Cell.PageCount);j++)
		{
			//Ȼ���ж��ǲ��������Pte��
			if( (HitPoint>=pMyDataProtectCell->Cell.PteNodeLists[j].PageStartAddress) && (HitPoint<(pMyDataProtectCell->Cell.PteNodeLists[j].PageStartAddress+0x1000)) )
			{
				//ѭ������ Pte��ĵ�
				//pMyDataProtectCell.Cell.PteNodeLists[j].PointCount �����Pte��Point���ܸ���
				for(k=0;k<(LONG)(pMyDataProtectCell->Cell.PteNodeLists[j].PointCount);k++)
				{
					//�ҵ����Ǹ���
					if( HitPoint==(LONG)(pMyDataProtectCell->Cell.PteNodeLists[j].ProtectPoints[k]) )
					{
						FindEThreadAndSetEIpFromAccessLoops(ItemIndex,EThread,EIp,HitPoint,1);
						DbgPrint("_________________________________\r\n");
						KeReleaseSpinLock(&g_List_Lock,Irql);	
						return 1;
					}
					//����Ѿ������������Ľ�β,����������Ĳ���
					if( k==(pMyDataProtectCell->Cell.PteNodeLists[j].PointCount-1) )
					{
						break;	
					}
					
				}

				FindEThreadAndSetEIpFromAccessLoops(ItemIndex,EThread,EIp,HitPoint,0);	
				KeReleaseSpinLock(&g_List_Lock,Irql);	
				return 1;
			}
		}
	}
	//����㲻�����Cell�ڣ���Cell�����ƶ�
	//���²���һֱ���ҵ�����ͷ
	else
	{
		pNext = pNext->Flink;
		if(pNext != pFirst)
		{
			pMyDataProtectCell = CONTAINING_RECORD(pNext, TDataProtectCell, CellNext);
			goto FindNext;
		}
		else
		{
			//Cell�Ѿ����ҵ��˽�β�����治����û��ʣ��ĵ㶼��������
			KeReleaseSpinLock(&g_List_Lock,Irql);	
			return 0;	//����ֵΪ0  ��ʾ����㲻��������Ҫ�����ĵ�
		}

	}
	
	KeReleaseSpinLock(&g_List_Lock,Irql);	
	
	return 0;	
}

///////////////(0E)Find Sub Func
//������һ��EThread  ��  һ��EIP����������Ҫ������û�����EThread,Ȼ��������Ӧ��ֵ
ULONG
FindEThreadAndSetEIpFromAccessLoops(ULONG ItemIndex,ULONG EThread,ULONG EIp,ULONG ErrorAddress,ULONG IsOurPoints)
{
	LONG	i = 0;
	ULONG	AccessCount = 0;

	AccessCount = g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessCount;


	for(i=0;i<(LONG)AccessCount;i++)
	{
		//�ҵ�����ͬ��EThread���ͽ����滻EIP
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentThread == EThread)
		{
			g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentEIp 		= EIp;
			g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentCr2 		= ErrorAddress;
			g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].IsOurPoints		= IsOurPoints;
			return 1;		
		}
	}
	//û���ҵ���������һ��
	if(i==AccessCount)
	{
		g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessCount++;
		g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentThread 	= EThread;
		g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentEIp 		= EIp;
		g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentCr2 		= ErrorAddress;
		g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].IsOurPoints		= IsOurPoints;
	}

	return 1;
}	




//��Trap01 ���õĽӿ�
//ͨ��EProcess�õ�PF��EIP
//����1 ��ʾ�ɹ�   ����0��ʾʧ��  2��ʾʹ���Ǳ�ǵĵ�
ULONG
FindCellList_GetEIp(PVOID EProcess,ULONG* DB_EIp,ULONG EThread,ULONG* ErrorAddress)
{
	ULONG		RetValue = 0;
	LONG		ItemIndex = 0;
	KIRQL		Irql;
	KeAcquireSpinLock(&g_List_Lock,&Irql);
	
	for(ItemIndex=0;ItemIndex<(LONG)(g_lMyDataProtectInfoIndex);ItemIndex++)
	{
		//�ҵ���EProcess
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].lpvEProcess == EProcess)
		{
			break;	
		}
	}
	//û���ҵ�
	if(ItemIndex==g_lMyDataProtectInfoIndex)
	{
		KeReleaseSpinLock(&g_List_Lock,Irql);
		return 0;//����û���������
	}
	
	RetValue = FindEThreadAndGetEIpFromAccessLoops(ItemIndex,EThread,DB_EIp,ErrorAddress);
	
	KeReleaseSpinLock(&g_List_Lock,Irql);
	return RetValue;
}

///////////////(01)Get Sub Func
//������һ��EThread  Ȼ����в��ң������ҵ��������EIp��ֵ
ULONG
FindEThreadAndGetEIpFromAccessLoops(ULONG ItemIndex,ULONG EThread,ULONG* EIp,ULONG* ErrorAddress)
{
	LONG		i = 0;
	ULONG		AccessCount = 0;
	ULONG		EIpTemp = 0;
	ULONG		ErrorAddressTemp = 0;
	ULONG		ulRet = 0;
	
	AccessCount = g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessCount;
	LOG_INFO("AccessCount Is :%d \r\n",AccessCount);

	for(i=0;i<(LONG)AccessCount;i++)
	{
		//�ҵ�����ͬ��EThread���ͽ����滻EIP
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentThread == EThread)
		{
			EIpTemp = g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentEIp;
			ErrorAddressTemp = g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentCr2;
			LOG_INFO("AccessEIpTemp Is :%x \r\n",EIpTemp);
			LOG_INFO("AccessErrorAddressTemp Is :%x\r\n",ErrorAddressTemp);
			if( (EIpTemp==0) && (ErrorAddressTemp==0) )
			{
				break;
			}
			else
			{
				*EIp = EIpTemp;
				*ErrorAddress = ErrorAddressTemp;

				if((g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].IsOurPoints)==1)
					ulRet = 2;
				else
					ulRet = 1;
				
				//����ģ���ж�
				//DataProtect_QuicklyJudge(EIpTemp);
				//DataProtect_NormalJudge(EIpTemp);
				//���ҽ���������
				g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].IsOurPoints = 0;
				g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentCr2 = 0;
				g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentEIp = 0;
			}

			
			return ulRet;		
		}
	}
	//û���ҵ����򷵻�0
	if(i==AccessCount)
	{
		*EIp = 0;
		*ErrorAddress = 0;
		return 0;
	}

	return 0;
}










/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														ͨ���Ӻ���																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//������һ��EProcess
//����ֵΪ���EProcess��Cr3
ULONG
GetCr3FromEProcess_XPSP3(PVOID EProcess)
{
	ULONG		Cr3 = 0;
	Cr3 = *(PULONG)((ULONG)EProcess+0x18);
	return Cr3;	
}


//������һ��EProcess���õ����EProcess�����
//����ֵΪ-1��ʾû���ҵ����EProcess
LONG
GetDataProtectItemsIndexFromEProcess(PVOID EProcess)
{
	LONG		ItemIndex = 0;
	
	for(ItemIndex=0;ItemIndex<(LONG)(g_lMyDataProtectInfoIndex);ItemIndex++)
	{
		//�ҵ���EProcess
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].lpvEProcess == EProcess)
			return ItemIndex;	
	}
	//û���ҵ�
	if( ItemIndex==(LONG)(g_lMyDataProtectInfoIndex) )
		return -1;//����û���������
	
	return -1;//����û���������	
}


//������һ��������׵�ַ������ĸ���
//��������ɹ�
NTSTATUS
SortInputArrayQuickly(PVOID AddressPointerList[],ULONG ulCount)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	ULONG			ulPointerTemp = 0;
	LONG 			i 	 = 0;
	LONG			j 	 = 0;

	for(i=0;i<(LONG)ulCount;i++)
	{
		for(j=0;j<(LONG)(ulCount-1-i);j++)
		{
			//��ַ��С��������
			if(AddressPointerList[j]>AddressPointerList[j+1])
			{
				ulPointerTemp = (ULONG)AddressPointerList[j];
				AddressPointerList[j] = AddressPointerList[j+1];
				(ULONG)AddressPointerList[j+1] = ulPointerTemp;
			}		
		}		
	}//Sort End

	return Status;
}


//������һ��EProcess
//����0��ʾ�����˱���     ����1��ʾ�ر��˱���
ULONG
__stdcall
TheDataProtectIsClose(PVOID EProcess)
{
	LONG			Item = 0;
	Item = GetDataProtectItemsIndexFromEProcess(EProcess);
	if(Item==-1)
		return 0;

	//�ر��˱���
	if ( g_pMyDataProtectInfo->DataProtectItems[Item].IsCloseProtect == 1  )
		return 1;

	//�������ǿ�����
	return	0;
}


/*
1��ʾ�ҵ��˽���
0��ʾ�������ǵĽ���
*/
ULONG
__stdcall
FilterProcess(ULONG Cr3)
{
	LONG		ItemIndex = 0;

	for(ItemIndex=0;ItemIndex<(LONG)g_lMyDataProtectInfoIndex;ItemIndex++)
	{
		//�ҵ���EProcess
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].Cr3 == Cr3)
			return 1;	
	}
	//û���ҵ�
	if(ItemIndex==g_lMyDataProtectInfoIndex)
		return 0;//����û���������

	return 0;
}