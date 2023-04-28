#ifndef	 _DATAACCESSDETECTION_GAME_H_
#define   _DATAACCESSDETECTION_GAME_H_

#include <NTIFS.h>


typedef struct _GAMEINFOR_
{
	UCHAR		szProcName[256];		//游戏名称
	ULONG		EProcess;						//游戏EProcess
	ULONG		GameID;					
	ULONG		PointList[16];				
	ULONG		PointNum;						//保护的点的个数			
	HANDLE		GamePID;					
	ULONG		IsDoubleProcess;		
	ULONG		IsOpenOrClose;			//1表示开启保护		0表示关闭保护
}TGameInfor,*PGameInfor;


#endif