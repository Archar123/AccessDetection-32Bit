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





//�Ƿ�����Ϸ���ݱ����ӿں���
NTSTATUS
DataAccessDetection_OpenOrNot(PGameInfor Infor)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PEPROCESS	TargetEProcess = NULL;

	//ͨ��Id�õ�EProcess
	Status = PsLookupProcessByProcessId(Infor->GamePID, &TargetEProcess);

	//��
	if(Infor->IsOpenOrClose == 1)
		Status = OpenDataProtectByEProcess((PVOID)TargetEProcess,(PVOID)Infor->PointList,Infor->PointNum);
	else
		Status = CloseDataProtectByEProcess((PVOID)TargetEProcess);

	ObfDereferenceObject(TargetEProcess);
	return	Status;
}



//������Ϸ���� ֻ��Ҫ������EProcess �͵�����
NTSTATUS
OpenDataProtectByEProcess(PVOID EProcess,PVOID AddressPointerList[],ULONG ulCount)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PointsInit_SortAndCut((PVOID)EProcess,AddressPointerList,ulCount);
	return	Status;
}



//�ر���Ϸ���� ֻ��Ҫ������EProcess ���Ǿ͹ر�������̵���Ϸ���ݱ���
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

	//�����еĵ㶼ɾ��
	DeleteCellList_DelAll(Item);

	return	Status;
}





