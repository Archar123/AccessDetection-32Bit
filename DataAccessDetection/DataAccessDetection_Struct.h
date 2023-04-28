#ifndef _DATAACCESSDETECTION_STRUCT_H_
#define _DATAACCESSDETECTION_STRUCT_H_

#include		<Ntifs.h>
#include		<stdio.h>


typedef	struct _ACCESSNODE
{
	ULONG				CurrentThread;								//当前的线程(因为中断不可能是同一个线程一起中断，所以就不存在共享资源被破坏的问题)
	ULONG				CurrentEIp;									//当前的EIP(缺页的时候的EIP)
	ULONG				CurrentCr2;									//当前的错误地址
	ULONG				IsOurPoints;									//0表示这不是我们保护的点，1表示这是我们保护的点
}TAccessNode,*PAccessNode;


//定义头结点，里面存放 EProcess ImageName 还有指向Cell的指针
typedef struct _DATAPROTECTITEM
{
	LIST_ENTRY		CellsHead;									//指向TDataProtectCell是这个链表的头
	ULONG				Cr3;												//方便查找
	PVOID					lpvEProcess;								//保护进程的EProcess
	TAccessNode	AccessLoops[1024];				//最大为256的一个数组，因为一个进程里的线程个数应该不会超过256。
	ULONG				AccessCount;							//来自不同EThread访问的总个数
	ULONG				IsCloseProtect;							//0表示开启保护   1表示关闭保护
}TDataProtectItem,*PDataProtectItem;



typedef struct _DATAPROTECTINFO 
{   
	ULONG 						ulCount;								//被保护的进程个数
	TDataProtectItem 	DataProtectItems[1];		//数组结构
}TDataProtectInfo, *PDataProtectInfo;



///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////
//Pte节点
typedef struct _PTENODE
{
	ULONG					PageStartAddress;				//本页面的起始地址
	ULONG					PteEx;										//开启PAE模式下扩展的4个字节部分
	ULONG					Pte;											//Pte
	PMDL						pMDL;									//用于锁定和解锁的MDL
	ULONG					PointCount;							//被保护的点的个数
	ULONG					ProtectPoints[64];				//被保护的点（最大支持64个）
}TPteNode,*PPteNode;


//定义的结构
typedef struct _CELL
{
	ULONG					PageAddress;							//页面块的首地址
	ULONG					PageSize;									//页面块的大小
	ULONG					PageCount;								//页面个数
	ULONG					UsedPageCount;						//还在使用的页面个数
	TPteNode				PteNodeLists[16];					//Pte的结构数组	（最大支持16个）
}TCell,*PCell;

//定义节点对象，里面存放的是页面块的属性
typedef struct _DATAPROTECTCELL  
{
 	LIST_ENTRY   		CellNext;									//下一个结构	
	ULONG					CellIndex;									//Cell节点的序号(第一个节点为0)   这个序号要在初始化和修改的时候进行重新修改
	TCell						Cell;												//定义的结构
}TDataProtectCell,*PDataProtectCell;



//初始化
VOID
DataAccessDetection_Struct_Init();

//反初始化
VOID
DataAccessDetection_Struct_UnInit();


////////////////////////////////////////	INIT
//选择哪个头结点，进行链表初始化
NTSTATUS
InitCellList(ULONG ulIndex);

////////////////////////////////////////	DEL
//删除所有的节点
NTSTATUS
DeleteCellList_DelAll(ULONG ulIndex);





//DataProtect_Mode接口
//当输入一个Point的数组的时候,我们首先要对其进行排序，然后按页面块进行分割
NTSTATUS
PointsInit_SortAndCut(PVOID EProcess,PVOID AddressPointerList[],ULONG ulCount);








/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														IDT 调用接口																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//0E的接口
//给定的一个点，判断有没有在我们的结构内
ULONG
FindCellList_FindPoint(PVOID EProcess,ULONG HitPoint,ULONG EIp,ULONG EThread);

///////////////(0E)Find Sub Func
//被调用的子函数
ULONG
FindEThreadAndSetEIpFromAccessLoops(ULONG ItemIndex,ULONG EThread,ULONG EIp,ULONG ErrorAddress,ULONG IsOurPoints);


//01的接口
//给定一个EProcess，然后得到一个EIP
ULONG
FindCellList_GetEIp(PVOID EProcess,ULONG* DB_EIp,ULONG EThread,ULONG* ErrorAddress);

///////////////(01)Get Sub Func
//被调用的子函数
ULONG
FindEThreadAndGetEIpFromAccessLoops(ULONG ItemIndex,ULONG EThread,ULONG* EIp,ULONG* ErrorAddress);







/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														通用子函数																			  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//传进来一个EProcess
//返回值为这个EProcess的Cr3
ULONG
GetCr3FromEProcess_XPSP3(PVOID EProcess);


//传进来一个EProcess，返回这个EProcess的序号
//返回值为-1表示没有找到这个EProcess
LONG
GetDataProtectItemsIndexFromEProcess();


//传进来一个数组的首地址和数组的个数
//返回排序成功
NTSTATUS
SortInputArrayQuickly(PVOID AddressPointerList[],ULONG ulCount);


//传进来一个EProcess
//返回0表示开启了保护     返回1表示关闭了保护
ULONG
__stdcall
TheDataProtectIsClose(PVOID EProcess);

//用来过滤进程
ULONG
__stdcall
FilterProcess(ULONG Cr3);

#endif