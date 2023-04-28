#ifndef _DATAACCESSDETECTION_MODE_H_
#define _DATAACCESSDETECTION_MODE_H_

#include		<Ntifs.h>
#include		<ntimage.h>
#include		<stdio.h>
#include		"DataAccessDetection_Game.h"

//��Ҫ�ṩ�ⲿ�ӿ�

/////////////////////////////////////////////////////////////////////////////////
//�ⲿ��ʼ��
NTSTATUS
DataAccessDetection_Mode_Init();

//�ⲿ�ͷ�
NTSTATUS   
DataAccessDetection_Mode_UnInit();
//////////////////////////////////////////////////////////////////////////////////



//�ⲿ�ӿ� �Ƿ������
NTSTATUS
DataAccessDetection_OpenOrNot(PGameInfor Infor);


//������Ϸ���� ֻ��Ҫ������EProcess �͵�����
NTSTATUS
OpenDataProtectByEProcess(PVOID EProcess,PVOID AddressPointerList[],ULONG ulCount);


//�ر���Ϸ���� ֻ��Ҫ������EProcess ���Ǿ͹ر�������̵���Ϸ���ݱ���
NTSTATUS
CloseDataProtectByEProcess(PVOID	EProcess);



#endif