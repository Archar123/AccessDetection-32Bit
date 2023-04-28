#include "DataAccessDetection_Struct.h"
#include "MemoryPage.h"
#include "Universal.h"
#include "ModuleJudge.h"


//全局数组 包含进程信息
PDataProtectInfo			g_pMyDataProtectInfo = NULL;
LONG								g_lMyDataProtectInfoIndex = 0;
KSPIN_LOCK					g_List_Lock;


//定义的临时变量
TCell 			tCellTemp 		= {0};
TPteNode 	tPteNodeTemp[64];


//初始化
VOID
DataAccessDetection_Struct_Init()
{
	//初始化锁
	KeInitializeSpinLock(&g_List_Lock);
	//用于维护 公共变量保护的结构
	g_pMyDataProtectInfo = ExAllocatePoolWithTag(NonPagedPool,sizeof(TDataProtectInfo)*16,'INIT');
	memset(g_pMyDataProtectInfo,0,sizeof(TDataProtectInfo)*16);
}

//反初始化
VOID
DataAccessDetection_Struct_UnInit()
{
	//释放内存
	if(g_pMyDataProtectInfo)
		ExFreePoolWithTag(g_pMyDataProtectInfo,'INIT');
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														一些内部接口																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//链表的初始化
NTSTATUS
InitCellList(ULONG ulIndex)
{	
	NTSTATUS Status = STATUS_SUCCESS;
	//初始化 第 uIndex 个数组的链表 
	InitializeListHead(&(g_pMyDataProtectInfo->DataProtectItems[ulIndex].CellsHead));
	return Status;
}


//删除所有的节点，并且去掉保护
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

	//通过索引得到EProcess
	EProcess = g_pMyDataProtectInfo->DataProtectItems[ulIndex].lpvEProcess;
	while (!IsListEmpty(&(g_pMyDataProtectInfo->DataProtectItems[ulIndex].CellsHead)))
	{
		next = RemoveHeadList(&(g_pMyDataProtectInfo->DataProtectItems[ulIndex].CellsHead));
		pCell = CONTAINING_RECORD(next, TDataProtectCell, CellNext);
		//将这个cell里的所有页面都变成valid
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

	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].IsCloseProtect = 1;//表示是关闭保护的
	g_pMyDataProtectInfo->DataProtectItems[ulIndex].Cr3 = 0;
	g_pMyDataProtectInfo->DataProtectItems[ulIndex].lpvEProcess = NULL;
	
	KeReleaseSpinLock(&g_List_Lock,Irql);	
	return Status;
}






/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												DataProtect_Mode 调用接口															  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//添加点
/////////////
//当输入一个Point的数组的时候，我们首先要对其进行排序，然后按页面块进行分割,最后联入链表中
//并且直接修改其Pte的页面属性
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

	//(1)首先进行排序
	SortInputArrayQuickly(AddressPointerList,ulCount);

	//(2)下面要按照页面进行分割
	for(i=0;i<(LONG)ulCount;i++)
	{
		ulPointerTemp = (ULONG)AddressPointerList[i];

		ulPageIndex = ulPointerTemp&0xFFFFF000;

		//不是第一个点
		if(PointIndex!=0)
		{
			//首先判断是不是在同一个页面里
			if( (tPteNodeTemp[PteIndex].PageStartAddress) == ulPageIndex )
			{
				//如果是同一个页面里
				tPteNodeTemp[PteIndex].ProtectPoints[PointIndex] = ulPointerTemp;
				tPteNodeTemp[PteIndex].PointCount++;
				PointIndex++;
			}
			else//不在同一个页面里
			{
				//变成 1页面 //点的序号重置
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
		else	//第一个点的时候，一定是第一个页面（0页面）
		{
			tPteNodeTemp[PteIndex].PageStartAddress = ulPageIndex;
			tPteNodeTemp[PteIndex].ProtectPoints[PointIndex] = ulPointerTemp;
			tPteNodeTemp[PteIndex].PointCount++;
			PointIndex++;

			//首先将这个页面锁定起来
			tPteNodeTemp[PteIndex].pMDL=NULL;
			MyLockMemory((PVOID)ulPageIndex,0x1000,EProcess,&tPteNodeTemp[PteIndex].pMDL);
			//设置页面的属性为 0
			DataProtect_SetPTEBySelf((PVOID)ulPageIndex,EProcess,0,0);

			if (Status!=STATUS_SUCCESS)
			{
				ERR_INFO("[DataProtect][Error] Pde Is Invalid \r\n");
			}
		}

	}

	//(3)按照页面分割完毕后，再将相邻的页面放在一个Cell里，然后插入到链表中

	InitCellList(g_lMyDataProtectInfoIndex);

	//PteIndex+1 表明了有多少个Pte
	PteCount = PteIndex+1;
	i = 0;
	PteIndex = 0;	//每个Cell中Pte的序号

	for(i=0;i<(LONG)PteCount;i++)
	{
		if(PteIndex!=0)
		{
			//说明前后两个Pte是紧挨着的
			if( (tPteNodeTemp[i-1].PageStartAddress + 0x1000) == (tPteNodeTemp[i].PageStartAddress) )
			{
				//放到同一个Cell中
				tCellTemp.PageCount++;
				tCellTemp.UsedPageCount++;
				tCellTemp.PageSize+=0x1000;
				tCellTemp.PteNodeLists[PteIndex] = tPteNodeTemp[i];
				PteIndex++;

				//如果已经是最后一项 那么说明这是同一块
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
				//表明不是挨着的了,所以应该是另一个Cell了，将这个Cell先插入到链表中，然后重新进行Cell的赋值
				//						数组的序号					Cell结构		Cell的序号
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
		else//表明是Cell中的第一个Pte	
		{
			//存放页面首地址
			(ULONG)tCellTemp.PageAddress = tPteNodeTemp[i].PageStartAddress;
			tCellTemp.PageCount++;
			tCellTemp.UsedPageCount++;
			tCellTemp.PageSize+=0x1000;
			tCellTemp.PteNodeLists[PteIndex] = tPteNodeTemp[i];

			//如果已经是最后一项 那么说明这是同一块
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
	//这个结构的Cr3和#PF中的查找相关，尽量在初始化完毕之后在进行设置，方便在进行监控的时候，添加保护点的操作已经完成了

	//这两个值主要是用来 快速判断进程的
	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].lpvEProcess = EProcess;
	//通过EProcess得到Cr3
	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].Cr3 = GetCr3FromEProcess_XPSP3(EProcess);

	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].IsCloseProtect = 0;//表示是开启保护的
	g_pMyDataProtectInfo->DataProtectItems[g_lMyDataProtectInfoIndex].AccessCount++;
	//这个其实是个数，再插入节点之后就是个数了
	g_lMyDataProtectInfoIndex++;
	//在触发#PF的时候，Index已经更新了

	//End
	return ns;
}











/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														IDT 调用接口																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//给Trap0E 调用的接口
//给定的一个点，判断有没有在我们的结构内
//return 0 表示没有找到相对应的点，return 1表明找到了相对应的点
ULONG
FindCellList_FindPoint(PVOID EProcess,ULONG HitPoint,ULONG EIp,ULONG EThread)
{
	//首先在数组里查找EProcess
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
		return 0;//根本没有这个进程
	}

	pFirst=&(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].CellsHead);
	//第一个Cell
	pNext = pFirst->Flink;
	pMyDataProtectCell = CONTAINING_RECORD(pNext, TDataProtectCell, CellNext);

FindNext:	
	//如果这个点在 这个Cell 的范围内
	if( (HitPoint>=pMyDataProtectCell->Cell.PageAddress) && (HitPoint<(pMyDataProtectCell->Cell.PageAddress + pMyDataProtectCell->Cell.PageSize)) )
	{
		//循环查找
		//pMyDataProtectCell->Cell.PageCount  是这个Cell里Pte的总个数
		for(j=0;j<(LONG)(pMyDataProtectCell->Cell.PageCount);j++)
		{
			//然后判断是不是在这个Pte里
			if( (HitPoint>=pMyDataProtectCell->Cell.PteNodeLists[j].PageStartAddress) && (HitPoint<(pMyDataProtectCell->Cell.PteNodeLists[j].PageStartAddress+0x1000)) )
			{
				//循环查找 Pte里的点
				//pMyDataProtectCell.Cell.PteNodeLists[j].PointCount 是这个Pte里Point的总个数
				for(k=0;k<(LONG)(pMyDataProtectCell->Cell.PteNodeLists[j].PointCount);k++)
				{
					//找到了那个点
					if( HitPoint==(LONG)(pMyDataProtectCell->Cell.PteNodeLists[j].ProtectPoints[k]) )
					{
						FindEThreadAndSetEIpFromAccessLoops(ItemIndex,EThread,EIp,HitPoint,1);
						DbgPrint("_________________________________\r\n");
						KeReleaseSpinLock(&g_List_Lock,Irql);	
						return 1;
					}
					//如果已经到了这个数组的结尾,则结束这个点的查找
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
	//这个点不在这个Cell内，则Cell向下移动
	//向下查找一直查找到链表头
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
			//Cell已经查找到了结尾，后面不管有没有剩余的点都不考虑了
			KeReleaseSpinLock(&g_List_Lock,Irql);	
			return 0;	//返回值为0  表示这个点不存在我们要保护的点
		}

	}
	
	KeReleaseSpinLock(&g_List_Lock,Irql);	
	
	return 0;	
}

///////////////(0E)Find Sub Func
//传进来一个EThread  和  一个EIP，首先我们要查找有没有这个EThread,然后设置相应的值
ULONG
FindEThreadAndSetEIpFromAccessLoops(ULONG ItemIndex,ULONG EThread,ULONG EIp,ULONG ErrorAddress,ULONG IsOurPoints)
{
	LONG	i = 0;
	ULONG	AccessCount = 0;

	AccessCount = g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessCount;


	for(i=0;i<(LONG)AccessCount;i++)
	{
		//找到了相同的EThread，就进行替换EIP
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentThread == EThread)
		{
			g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentEIp 		= EIp;
			g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentCr2 		= ErrorAddress;
			g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].IsOurPoints		= IsOurPoints;
			return 1;		
		}
	}
	//没有找到，则增加一个
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




//给Trap01 调用的接口
//通过EProcess得到PF的EIP
//返回1 表示成功   返回0表示失败  2表示使我们标记的点
ULONG
FindCellList_GetEIp(PVOID EProcess,ULONG* DB_EIp,ULONG EThread,ULONG* ErrorAddress)
{
	ULONG		RetValue = 0;
	LONG		ItemIndex = 0;
	KIRQL		Irql;
	KeAcquireSpinLock(&g_List_Lock,&Irql);
	
	for(ItemIndex=0;ItemIndex<(LONG)(g_lMyDataProtectInfoIndex);ItemIndex++)
	{
		//找到了EProcess
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].lpvEProcess == EProcess)
		{
			break;	
		}
	}
	//没有找到
	if(ItemIndex==g_lMyDataProtectInfoIndex)
	{
		KeReleaseSpinLock(&g_List_Lock,Irql);
		return 0;//根本没有这个进程
	}
	
	RetValue = FindEThreadAndGetEIpFromAccessLoops(ItemIndex,EThread,DB_EIp,ErrorAddress);
	
	KeReleaseSpinLock(&g_List_Lock,Irql);
	return RetValue;
}

///////////////(01)Get Sub Func
//传进来一个EThread  然后进行查找，若查找到，则进行EIp赋值
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
		//找到了相同的EThread，就进行替换EIP
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
				
				//进行模块判定
				//DataProtect_QuicklyJudge(EIpTemp);
				//DataProtect_NormalJudge(EIpTemp);
				//并且将这个项清空
				g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].IsOurPoints = 0;
				g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentCr2 = 0;
				g_pMyDataProtectInfo->DataProtectItems[ItemIndex].AccessLoops[i].CurrentEIp = 0;
			}

			
			return ulRet;		
		}
	}
	//没有找到，则返回0
	if(i==AccessCount)
	{
		*EIp = 0;
		*ErrorAddress = 0;
		return 0;
	}

	return 0;
}










/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														通用子函数																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//传进来一个EProcess
//返回值为这个EProcess的Cr3
ULONG
GetCr3FromEProcess_XPSP3(PVOID EProcess)
{
	ULONG		Cr3 = 0;
	Cr3 = *(PULONG)((ULONG)EProcess+0x18);
	return Cr3;	
}


//传进来一个EProcess，得到这个EProcess的序号
//返回值为-1表示没有找到这个EProcess
LONG
GetDataProtectItemsIndexFromEProcess(PVOID EProcess)
{
	LONG		ItemIndex = 0;
	
	for(ItemIndex=0;ItemIndex<(LONG)(g_lMyDataProtectInfoIndex);ItemIndex++)
	{
		//找到了EProcess
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].lpvEProcess == EProcess)
			return ItemIndex;	
	}
	//没有找到
	if( ItemIndex==(LONG)(g_lMyDataProtectInfoIndex) )
		return -1;//根本没有这个进程
	
	return -1;//根本没有这个进程	
}


//传进来一个数组的首地址和数组的个数
//返回排序成功
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
			//地址从小到大排序
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


//传进来一个EProcess
//返回0表示开启了保护     返回1表示关闭了保护
ULONG
__stdcall
TheDataProtectIsClose(PVOID EProcess)
{
	LONG			Item = 0;
	Item = GetDataProtectItemsIndexFromEProcess(EProcess);
	if(Item==-1)
		return 0;

	//关闭了保护
	if ( g_pMyDataProtectInfo->DataProtectItems[Item].IsCloseProtect == 1  )
		return 1;

	//保护还是开启的
	return	0;
}


/*
1表示找到了进程
0表示不是我们的进程
*/
ULONG
__stdcall
FilterProcess(ULONG Cr3)
{
	LONG		ItemIndex = 0;

	for(ItemIndex=0;ItemIndex<(LONG)g_lMyDataProtectInfoIndex;ItemIndex++)
	{
		//找到了EProcess
		if(g_pMyDataProtectInfo->DataProtectItems[ItemIndex].Cr3 == Cr3)
			return 1;	
	}
	//没有找到
	if(ItemIndex==g_lMyDataProtectInfoIndex)
		return 0;//根本没有这个进程

	return 0;
}