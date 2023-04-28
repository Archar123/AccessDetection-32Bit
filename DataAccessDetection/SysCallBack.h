#ifndef _SYSCALLBACK_H_
#define	_SYSCALLBACK_H_

#include		<Ntifs.h>

//-----------------------------------------------------------------------
//注册系统回调

//外部调用接口
VOID
DataAccessDetection_SysCallBack_Init();

VOID
DataAccessDetection_SysCallBack_UnInit();



VOID 
MyCreateThreadNotify(
	IN HANDLE  ProcessId,
	IN HANDLE  ThreadId,
	IN BOOLEAN  Create);


VOID
MyCreateProcessNotify(
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create);


ULONG
IsOurDetectionGames(char* szProcName,HANDLE ProcessId,BOOLEAN IsCreate);



#endif