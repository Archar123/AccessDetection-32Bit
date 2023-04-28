#ifndef _DATAACCESSDETECTION_MODE_H_
#define _DATAACCESSDETECTION_MODE_H_

#include		<Ntifs.h>
#include		<ntimage.h>
#include		<stdio.h>
#include		"DataAccessDetection_Game.h"

//主要提供外部接口

/////////////////////////////////////////////////////////////////////////////////
//外部初始化
NTSTATUS
DataAccessDetection_Mode_Init();

//外部释放
NTSTATUS   
DataAccessDetection_Mode_UnInit();
//////////////////////////////////////////////////////////////////////////////////



//外部接口 是否开启监控
NTSTATUS
DataAccessDetection_OpenOrNot(PGameInfor Infor);


//开启游戏保护 只需要传进来EProcess 和点数组
NTSTATUS
OpenDataProtectByEProcess(PVOID EProcess,PVOID AddressPointerList[],ULONG ulCount);


//关闭游戏保护 只需要传进来EProcess 我们就关闭这个进程的游戏数据保护
NTSTATUS
CloseDataProtectByEProcess(PVOID	EProcess);



#endif