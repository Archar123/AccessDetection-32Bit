#ifndef _DATAACCESSDETECTION_STRUCT_H_
#define _DATAACCESSDETECTION_STRUCT_H_

#include		<Ntifs.h>
#include		<stdio.h>


typedef	struct _ACCESSNODE
{
	ULONG				CurrentThread;								//��ǰ���߳�(��Ϊ�жϲ�������ͬһ���߳�һ���жϣ����ԾͲ����ڹ�����Դ���ƻ�������)
	ULONG				CurrentEIp;									//��ǰ��EIP(ȱҳ��ʱ���EIP)
	ULONG				CurrentCr2;									//��ǰ�Ĵ����ַ
	ULONG				IsOurPoints;									//0��ʾ�ⲻ�����Ǳ����ĵ㣬1��ʾ�������Ǳ����ĵ�
}TAccessNode,*PAccessNode;


//����ͷ��㣬������ EProcess ImageName ����ָ��Cell��ָ��
typedef struct _DATAPROTECTITEM
{
	LIST_ENTRY		CellsHead;									//ָ��TDataProtectCell����������ͷ
	ULONG				Cr3;												//�������
	PVOID					lpvEProcess;								//�������̵�EProcess
	TAccessNode	AccessLoops[1024];				//���Ϊ256��һ�����飬��Ϊһ����������̸߳���Ӧ�ò��ᳬ��256��
	ULONG				AccessCount;							//���Բ�ͬEThread���ʵ��ܸ���
	ULONG				IsCloseProtect;							//0��ʾ��������   1��ʾ�رձ���
}TDataProtectItem,*PDataProtectItem;



typedef struct _DATAPROTECTINFO 
{   
	ULONG 						ulCount;								//�������Ľ��̸���
	TDataProtectItem 	DataProtectItems[1];		//����ṹ
}TDataProtectInfo, *PDataProtectInfo;



///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////
//Pte�ڵ�
typedef struct _PTENODE
{
	ULONG					PageStartAddress;				//��ҳ�����ʼ��ַ
	ULONG					PteEx;										//����PAEģʽ����չ��4���ֽڲ���
	ULONG					Pte;											//Pte
	PMDL						pMDL;									//���������ͽ�����MDL
	ULONG					PointCount;							//�������ĵ�ĸ���
	ULONG					ProtectPoints[64];				//�������ĵ㣨���֧��64����
}TPteNode,*PPteNode;


//����Ľṹ
typedef struct _CELL
{
	ULONG					PageAddress;							//ҳ�����׵�ַ
	ULONG					PageSize;									//ҳ���Ĵ�С
	ULONG					PageCount;								//ҳ�����
	ULONG					UsedPageCount;						//����ʹ�õ�ҳ�����
	TPteNode				PteNodeLists[16];					//Pte�Ľṹ����	�����֧��16����
}TCell,*PCell;

//����ڵ���������ŵ���ҳ��������
typedef struct _DATAPROTECTCELL  
{
 	LIST_ENTRY   		CellNext;									//��һ���ṹ	
	ULONG					CellIndex;									//Cell�ڵ�����(��һ���ڵ�Ϊ0)   ������Ҫ�ڳ�ʼ�����޸ĵ�ʱ����������޸�
	TCell						Cell;												//����Ľṹ
}TDataProtectCell,*PDataProtectCell;



//��ʼ��
VOID
DataAccessDetection_Struct_Init();

//����ʼ��
VOID
DataAccessDetection_Struct_UnInit();


////////////////////////////////////////	INIT
//ѡ���ĸ�ͷ��㣬���������ʼ��
NTSTATUS
InitCellList(ULONG ulIndex);

////////////////////////////////////////	DEL
//ɾ�����еĽڵ�
NTSTATUS
DeleteCellList_DelAll(ULONG ulIndex);





//DataProtect_Mode�ӿ�
//������һ��Point�������ʱ��,��������Ҫ�����������Ȼ��ҳ�����зָ�
NTSTATUS
PointsInit_SortAndCut(PVOID EProcess,PVOID AddressPointerList[],ULONG ulCount);








/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														IDT ���ýӿ�																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//0E�Ľӿ�
//������һ���㣬�ж���û�������ǵĽṹ��
ULONG
FindCellList_FindPoint(PVOID EProcess,ULONG HitPoint,ULONG EIp,ULONG EThread);

///////////////(0E)Find Sub Func
//�����õ��Ӻ���
ULONG
FindEThreadAndSetEIpFromAccessLoops(ULONG ItemIndex,ULONG EThread,ULONG EIp,ULONG ErrorAddress,ULONG IsOurPoints);


//01�Ľӿ�
//����һ��EProcess��Ȼ��õ�һ��EIP
ULONG
FindCellList_GetEIp(PVOID EProcess,ULONG* DB_EIp,ULONG EThread,ULONG* ErrorAddress);

///////////////(01)Get Sub Func
//�����õ��Ӻ���
ULONG
FindEThreadAndGetEIpFromAccessLoops(ULONG ItemIndex,ULONG EThread,ULONG* EIp,ULONG* ErrorAddress);







/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														ͨ���Ӻ���																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//������һ��EProcess
//����ֵΪ���EProcess��Cr3
ULONG
GetCr3FromEProcess_XPSP3(PVOID EProcess);


//������һ��EProcess���������EProcess�����
//����ֵΪ-1��ʾû���ҵ����EProcess
LONG
GetDataProtectItemsIndexFromEProcess();


//������һ��������׵�ַ������ĸ���
//��������ɹ�
NTSTATUS
SortInputArrayQuickly(PVOID AddressPointerList[],ULONG ulCount);


//������һ��EProcess
//����0��ʾ�����˱���     ����1��ʾ�ر��˱���
ULONG
__stdcall
TheDataProtectIsClose(PVOID EProcess);

//�������˽���
ULONG
__stdcall
FilterProcess(ULONG Cr3);

#endif