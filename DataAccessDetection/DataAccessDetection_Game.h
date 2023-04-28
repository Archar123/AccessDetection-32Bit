#ifndef	 _DATAACCESSDETECTION_GAME_H_
#define   _DATAACCESSDETECTION_GAME_H_

#include <NTIFS.h>


typedef struct _GAMEINFOR_
{
	UCHAR		szProcName[256];		//��Ϸ����
	ULONG		EProcess;						//��ϷEProcess
	ULONG		GameID;					
	ULONG		PointList[16];				
	ULONG		PointNum;						//�����ĵ�ĸ���			
	HANDLE		GamePID;					
	ULONG		IsDoubleProcess;		
	ULONG		IsOpenOrClose;			//1��ʾ��������		0��ʾ�رձ���
}TGameInfor,*PGameInfor;


#endif