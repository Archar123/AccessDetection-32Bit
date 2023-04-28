#include "InitSys.h"
#include "GetWindowsVersion.h" 


//全局变量
ULONG_PTR    g_ImageFileName  = 0;				//EProcess中 ImageName的偏移

WIN_VERSION  g_WinVersion = WINDOWS_UNKNOW;//获取进程路径使用到


ULONG_PTR	  g_SectionObjectOfEProcess = 0;

ULONG_PTR    g_ObjectHeaderSize = 0;
ULONG_PTR    g_ObjectTypeOffsetOf_Object_Header = 0;
ULONG_PTR    g_ObjectTableOffsetOf_EPROCESS = 0;
ULONG_PTR    g_PreviousModeOffsetOf_KTHREAD = 0;

VOID
InitGlobalVariable()
{
	WIN_VERSION  WinVersion;

	WinVersion = GetWindowsVersion();
	g_WinVersion = WinVersion;

	switch(WinVersion)
	{
	case WINDOWS_7:
		{
			g_ImageFileName = 0x2e0;

			g_SectionObjectOfEProcess = 0x268;

			g_ObjectTableOffsetOf_EPROCESS = 0x200;
			g_PreviousModeOffsetOf_KTHREAD = 0x1f6;

			break;
		}

	case WINDOWS_XP:
		{
			g_ImageFileName = 0x174;

			g_SectionObjectOfEProcess  = 0x138;

			g_ObjectHeaderSize = 0x18;
			g_ObjectTypeOffsetOf_Object_Header = 0x8;

			g_ObjectTableOffsetOf_EPROCESS = 0x0c4;
			g_PreviousModeOffsetOf_KTHREAD = 0x140;
			break;
		}
	}


}