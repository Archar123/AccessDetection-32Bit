#include "DataAccessDetection_Mode.h"
#include "DataAccessDetection_Struct.h"
#include "DataAccessDetection_Game.h"
#include "Universal.h"
#include "MemoryPage.h"
#include "ModuleJudge.h"



//_______________________________________
//						Mode
//_______________________________________

NTSTATUS
DataAccessDetection_Mode_Init()
{
	NTSTATUS  Status = STATUS_SUCCESS;
	DbgPrint("[DataAccessDetection][INTERFACE] Initialize!\r\n");
	DataAccessDetection_Struct_Init();
	ModuleJudge_Init();
	return Status;	
}


NTSTATUS   
DataAccessDetection_Mode_UnInit()
{
	NTSTATUS  Status = STATUS_SUCCESS;
	ModuleJudge_UnInit();
	DataAccessDetection_Struct_UnInit();
	DbgPrint("[DataAccessDetection][INTERFACE] Closed!\r\n");
	return Status;	
}





//是否开启游戏数据保护接口函数
NTSTATUS
DataAccessDetection_OpenOrNot(PGameInfor Infor)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PEPROCESS	TargetEProcess = NULL;

	//通过Id得到EProcess
	Status = PsLookupProcessByProcessId(Infor->GamePID, &TargetEProcess);

	//打开
	if(Infor->IsOpenOrClose == 1)
		Status = OpenDataProtectByEProcess((PVOID)TargetEProcess,(PVOID)Infor->PointList,Infor->PointNum);
	else
		Status = CloseDataProtectByEProcess((PVOID)TargetEProcess);

	ObfDereferenceObject(TargetEProcess);
	return	Status;
}



//开启游戏保护 只需要传进来EProcess 和点数组
NTSTATUS
OpenDataProtectByEProcess(PVOID EProcess,PVOID AddressPointerList[],ULONG ulCount)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PointsInit_SortAndCut((PVOID)EProcess,AddressPointerList,ulCount);
	return	Status;
}



//关闭游戏保护 只需要传进来EProcess 我们就关闭这个进程的游戏数据保护
NTSTATUS
CloseDataProtectByEProcess(PVOID	EProcess)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	LONG			Item = 0;

	Item = GetDataProtectItemsIndexFromEProcess(EProcess);
	if(Item==-1)
	{
		Status = STATUS_UNSUCCESSFUL;
		return Status;
	}

	//将所有的点都删除
	DeleteCellList_DelAll(Item);

	return	Status;
}





