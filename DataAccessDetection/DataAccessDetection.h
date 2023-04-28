#ifndef _DATAACCESSDETECTION_H_
#define _DATAACCESSDETECTION_H_

#include		<Ntifs.h>
#include		<devioctl.h>


#define CTL_CODE( DeviceType, Function, Method, Access ) (  ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )

#define DP_IOCTL(i)			CTL_CODE(FILE_DEVICE_UNKNOWN,i,METHOD_NEITHER,FILE_ANY_ACCESS)


enum DP_ENUM_IOCTL
{
	DP_IOCTL_DATAACCESSDETECTION = 0x100,					
	DP_IOCTL_DATAACCESSDETECTION_OPEN,
	DP_IOCTL_DATAACCESSDETECTION_CLOSE,
	DP_IOCTL_GET_PROCESS,	
};

#define DP_DEVICE_NAME				L"\\Device\\DataAccessDetection_Device"				    // Driver Name
#define DP_LINK_NAME					L"\\DosDevices\\DataAccessDetection_Link"                // Win32 Link Name



NTSTATUS
DispatchFunction(PDEVICE_OBJECT  DeviceObject,PIRP Irp);

NTSTATUS
DispatchControl(PDEVICE_OBJECT  DeviceObject,PIRP Irp);

//Çý¶¯Ð¶ÔØ
VOID
UnloadDriver(PDRIVER_OBJECT  DriverObject);


NTSTATUS
MainInit();

NTSTATUS
MainUnInit();

#endif