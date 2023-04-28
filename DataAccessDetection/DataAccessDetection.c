#include "DataAccessDetection.h"
#include "DataAccessDetection_Mode.h"
#include "DataAccessDetection_Game.h"
#include "Universal.h"
#include "SysCallBack.h"
#include "IDTHook.h"
#include "ModuleJudge.h"
#include "Process.h"
#include "InitSys.h"


PPROCESSINFO_SYS		ProcessInfo_Sys = NULL;
PPROCESSINFO				NormalProcessInfo = NULL;
PPROCESSINFO				HideProcessInfo = NULL;

//##########################################################################
//									MAIN
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject,PUNICODE_STRING RegisterPath)
{
	NTSTATUS       			Status;
	UNICODE_STRING	uniDeviceName;
	UNICODE_STRING	uniLinkName;
	PDEVICE_OBJECT		DeviceObject = NULL;
	LONG						i = 0;

	RtlInitUnicodeString(&uniDeviceName,DP_DEVICE_NAME);
	RtlInitUnicodeString(&uniLinkName,DP_LINK_NAME);

	for (i=0;i<IRP_MJ_MAXIMUM_FUNCTION;i++)
		DriverObject->MajorFunction[i] = DispatchFunction;

	DriverObject->DriverUnload = UnloadDriver;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;

	Status = IoCreateDevice(DriverObject,0,&uniDeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,&DeviceObject);
	if (!NT_SUCCESS(Status))
		return STATUS_UNSUCCESSFUL;

	Status = IoCreateSymbolicLink(&uniLinkName,&uniDeviceName);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(DeviceObject);
		return STATUS_UNSUCCESSFUL;
	}

	//主要给枚举进程使用
	MainInit();

	DbgPrint("[DataProtect]#驱动加载!#\r\n");

	//*****************************************************//
	DataAccessDetection_Mode_Init();
	DataAccessDetection_SysCallBack_Init();
	DataAccessDetection_IDT_Init();
	DbgPrint("[DataAccessDetection]#驱动初始化完成!#\r\n");

	return STATUS_SUCCESS;
}


VOID
UnloadDriver(PDRIVER_OBJECT  DriverObject)
{
	UNICODE_STRING  uniLinkName;

	RtlInitUnicodeString(&uniLinkName,DP_LINK_NAME);
	IoDeleteSymbolicLink(&uniLinkName);
	if (DriverObject->DeviceObject!=NULL)
		IoDeleteDevice(DriverObject->DeviceObject);

	//****************************************************//

	DataAccessDetection_IDT_UnInit();
	DataAccessDetection_SysCallBack_UnInit(); 
	DataAccessDetection_Mode_UnInit();
	DbgPrint("[DataAccessDetection]#驱动卸载!#\r\n");

	MainUnInit();
}


NTSTATUS
	DispatchControl(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{
	NTSTATUS  Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION   IrpSp;
	PVOID			InputBuffer = NULL;
	PVOID			OutputBuffer = NULL;
	ULONG		InputSize  = 0;
	ULONG		OutputSize = 0;
	ULONG		IoControlCode = 0;


	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;
	InputSize    = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
	InputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
	OutputBuffer = Irp->UserBuffer;
	OutputSize   = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;



	switch(IoControlCode)
	{
	case DP_IOCTL(DP_IOCTL_DATAACCESSDETECTION_OPEN):
		{
			DbgPrint("[DataAccessDetection] ----------------------------Open Detection!  \r\n");
			Status = DataAccessDetection_OpenOrNot((PGameInfor)InputBuffer);
			Irp->IoStatus.Information = 0;
			Status = Irp->IoStatus.Status = Status;
			break;
		}
	case DP_IOCTL(DP_IOCTL_DATAACCESSDETECTION_CLOSE):
		{
			DbgPrint("[DataAccessDetection] ----------------------------Close Detection!  \r\n");
			Status = DataAccessDetection_OpenOrNot((PGameInfor)InputBuffer);
			Irp->IoStatus.Information = 0;
			Status = Irp->IoStatus.Status = Status;
			break;
		}
		//返回进程
		case DP_IOCTL(DP_IOCTL_GET_PROCESS):
		{
			DbgPrint("CTL_LIST_PROCESS!");
			//申请内存用来存放进程的信息

			EnumProcessByForce();
			GetNormalProcessList( NormalProcessInfo, HideProcessInfo);

			if(InputBuffer==NULL)
			{
				Status	=	Irp->IoStatus.Status	=	STATUS_INVALID_BUFFER_SIZE;
				break;
			}

			memcpy(InputBuffer,NormalProcessInfo,sizeof(PROCESSINFO)*256);  //拷贝数据到用户模式下的内存中

			Irp->IoStatus.Information	=	OutputSize;

			Status	=	STATUS_SUCCESS;

			break;
		}
	default:
		{
			Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Irp->IoStatus.Information = 0;
			break;
		}
	}

	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return Status;
}


NTSTATUS
DispatchFunction(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


NTSTATUS
MainInit()
{
	//判断PC 版本
	InitGlobalVariable();

	//申请内存
	ProcessInfo_Sys = (PPROCESSINFO_SYS)ExAllocatePoolWithTag(NonPagedPool,sizeof(PROCESSINFO_SYS)*256,'XXXX');
	if (!ProcessInfo_Sys)
		return STATUS_UNSUCCESSFUL;
	memset(ProcessInfo_Sys,0,sizeof(PROCESSINFO_SYS)*256);

	//用来显示进程的信息
	NormalProcessInfo = (PPROCESSINFO)ExAllocatePoolWithTag(NonPagedPool,sizeof(PROCESSINFO)*256,'NORL');
	if (!NormalProcessInfo)
	{
		DbgPrint("Init ProcessInfo Failed");
		return STATUS_UNSUCCESSFUL;
	}
	memset(NormalProcessInfo,0,sizeof(PROCESSINFO)*256);


	//用来记录隐藏进程
	HideProcessInfo = (PPROCESSINFO)ExAllocatePoolWithTag(NonPagedPool,(sizeof(PROCESSINFO)+sizeof(SAFESYSTEM_PROCESS_INFORMATION))*120,'HIDE');
	if (!HideProcessInfo)
	{
		DbgPrint("Init ProcessInfo Failed");
		return STATUS_SUCCESS;
	}

	memset(HideProcessInfo,0,(sizeof(PROCESSINFO)+sizeof(SAFESYSTEM_PROCESS_INFORMATION))*120);

	return STATUS_SUCCESS;
}

NTSTATUS
MainUnInit()
{
	ExFreePool(ProcessInfo_Sys);
	ExFreePool(NormalProcessInfo);
	ExFreePool(HideProcessInfo);
	return STATUS_SUCCESS;
}
