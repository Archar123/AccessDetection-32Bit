#include "Process.h"

#include "Process.h"
#include "Universal.h"
#include "GetWindowsVersion.h"
#include "InitSys.h"



//暴力枚举进程
extern		ULONG_PTR  g_ImageFileName;

extern		PPROCESSINFO_SYS		ProcessInfo_Sys;

BOOLEAN
	EnumProcessByForce()
{
	NTSTATUS   Status;
	ULONG_PTR  i = 0; 
	ULONG_PTR  Count = 0;

	PEPROCESS  EProcess = NULL;

	for (i=0;i<100000;i+=4)
	{
		Status = PsLookupProcessByProcessId((HANDLE)i,&EProcess);    

		if (Status==STATUS_SUCCESS)
		{
			if (!IsRealProcess(EProcess))
			{
				ObfDereferenceObject(EProcess);
			}
			else
			{
				Count++;

				ProcessInfo_Sys->ProcessIdAndEProcess[Count].ulPid = i;
				ProcessInfo_Sys->ProcessIdAndEProcess[Count].EProcess = (ULONG_PTR)EProcess;

				DbgPrint("%s\r\n",(char*)((ULONG_PTR)EProcess+g_ImageFileName));
			}

			ObfDereferenceObject(EProcess);
		}


	}

	ProcessInfo_Sys->ulCount = (Count + 1);

	return	TRUE;
}


//获取路径
extern WIN_VERSION  g_WinVersion;

extern
	ULONG_PTR	  
	g_SectionObjectOfEProcess;

BOOLEAN
	GetProcessPathBySectionObject(ULONG_PTR ulProcessID,WCHAR* wzProcessPath)
{
	PEPROCESS         EProcess = NULL;
	PSECTION_OBJECT   SectionObject   = NULL;
	PSECTION_OBJECT64 SectionObject64 = NULL;
	PSEGMENT        Segment   = NULL;
	PSEGMENT64      Segment64 = NULL;
	PCONTROL_AREA   ControlArea = NULL;
	PCONTROL_AREA64 ControlArea64 = NULL;
	PFILE_OBJECT    FileObject  = NULL;
	BOOLEAN         bGetPath = FALSE;

	if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)ulProcessID, &EProcess)))
	{

		switch(g_WinVersion)
		{
		case WINDOWS_XP:
			{
				//g_SectionObjectOfEProcess  = 0x138;

				if (g_SectionObjectOfEProcess!=0&&MmIsAddressValid((PVOID)((ULONG_PTR)EProcess + g_SectionObjectOfEProcess)))
				{
					SectionObject = *(PSECTION_OBJECT*)((ULONG_PTR)EProcess + g_SectionObjectOfEProcess);

					if (SectionObject && MmIsAddressValid(SectionObject))
					{

						Segment = (PSEGMENT)SectionObject->Segment;
						if (Segment && MmIsAddressValid(Segment))
						{
							ControlArea = Segment->ControlArea;
							if (ControlArea && MmIsAddressValid(ControlArea))
							{
								FileObject = ControlArea->FilePointer;

								if (FileObject&&MmIsAddressValid(FileObject))
								{
									bGetPath = GetPathByFileObject(FileObject, wzProcessPath);
									if (!bGetPath)
									{
										DbgPrint("SectionObject: 0x%08X, FileObject: 0x%08X\n", SectionObject, FileObject);
									}
								}
							}
						}
					}
				}
				break;
			}

		case WINDOWS_7:
			{
				//g_SectionObjectOfEProcess = 0x268;

				if (g_SectionObjectOfEProcess!=0&&MmIsAddressValid((PVOID)((ULONG_PTR)EProcess + g_SectionObjectOfEProcess)))
				{
					SectionObject64 = *(PSECTION_OBJECT64*)((ULONG_PTR)EProcess + g_SectionObjectOfEProcess);



					if (SectionObject64 && MmIsAddressValid(SectionObject64))
					{

						Segment64 = (PSEGMENT64)(SectionObject64->Segment);
						if (Segment64 && MmIsAddressValid(Segment64))
						{
							ControlArea64 = (PCONTROL_AREA64)Segment64->ControlArea;
							if (ControlArea64 && MmIsAddressValid(ControlArea64))
							{
								FileObject = (PFILE_OBJECT)ControlArea64->FilePointer;

								if (FileObject&&MmIsAddressValid(FileObject))
								{
									FileObject = (PFILE_OBJECT)((ULONG_PTR)FileObject & 0xFFFFFFFFFFFFFFF0);
									bGetPath = GetPathByFileObject(FileObject, wzProcessPath);
									if (!bGetPath)
									{
										DbgPrint("SectionObject: 0x%08X, FileObject: 0x%08X\n", SectionObject, FileObject);
									}
								}
							}
						}
					}
				}
				break;
			}
		}

	}

	if (bGetPath==FALSE)
	{
		wcscpy(wzProcessPath,L"Unknow");
	}


	return bGetPath;

}

//通过FileObject得到
BOOLEAN 
	GetPathByFileObject(PFILE_OBJECT FileObject, WCHAR* wzPath)
{
	BOOLEAN bGetPath = FALSE;
	CHAR szIoQueryFileDosDeviceName[] = "IoQueryFileDosDeviceName";
	CHAR szIoVolumeDeviceToDosName[] = "IoVolumeDeviceToDosName";
	CHAR szRtlVolumeDeviceToDosName[] = "RtlVolumeDeviceToDosName";

	POBJECT_NAME_INFORMATION ObjectNameInformation = NULL;
	__try
	{
		if (FileObject && MmIsAddressValid(FileObject) && wzPath)
		{

			if (NT_SUCCESS(IoQueryFileDosDeviceName(FileObject,&ObjectNameInformation)))   //注意该函数调用后要释放内存
			{
				wcsncpy(wzPath,ObjectNameInformation->Name.Buffer,ObjectNameInformation->Name.Length);

				bGetPath = TRUE;

				ExFreePool(ObjectNameInformation);
			}


			if (!bGetPath)
			{

				if (IoVolumeDeviceToDosName||RtlVolumeDeviceToDosName)
				{
					NTSTATUS  Status = STATUS_UNSUCCESSFUL;
					ULONG_PTR ulRet= 0;
					PVOID     Buffer = ExAllocatePoolWithTag(PagedPool,0x1000,'PATH');

					if (Buffer)
					{
						// ObQueryNameString : \Device\HarddiskVolume1\Program Files\VMware\VMware Tools\VMwareTray.exe
						memset(Buffer, 0, 0x1000);
						Status = ObQueryNameString(FileObject, (POBJECT_NAME_INFORMATION)Buffer, 0x1000, &ulRet);
						if (NT_SUCCESS(Status))
						{
							POBJECT_NAME_INFORMATION Temp = (POBJECT_NAME_INFORMATION)Buffer;

							WCHAR szHarddiskVolume[100] = L"\\Device\\HarddiskVolume";

							if (Temp->Name.Buffer!=NULL)
							{
								if (Temp->Name.Length / sizeof(WCHAR) > wcslen(szHarddiskVolume) &&
									!_wcsnicmp(Temp->Name.Buffer, szHarddiskVolume, wcslen(szHarddiskVolume)))
								{
									// 如果是以 "\\Device\\HarddiskVolume" 这样的形式存在的，那么再查询其卷名。
									UNICODE_STRING uniDosName;

									if (NT_SUCCESS(IoVolumeDeviceToDosName(FileObject->DeviceObject, &uniDosName)))
									{
										if (uniDosName.Buffer!=NULL)
										{

											wcsncpy(wzPath, uniDosName.Buffer, uniDosName.Length);
											wcsncat(wzPath, Temp->Name.Buffer + wcslen(szHarddiskVolume) + 1, Temp->Name.Length - (wcslen(szHarddiskVolume) + 1));
											bGetPath = TRUE;
										}	

										ExFreePool(uniDosName.Buffer);
									}

									else if (NT_SUCCESS(RtlVolumeDeviceToDosName(FileObject->DeviceObject, &uniDosName)))
									{
										if (uniDosName.Buffer!=NULL)
										{

											wcsncpy(wzPath, uniDosName.Buffer, uniDosName.Length);
											wcsncat(wzPath, Temp->Name.Buffer + wcslen(szHarddiskVolume) + 1, Temp->Name.Length - (wcslen(szHarddiskVolume) + 1));
											bGetPath = TRUE;
										}	

										ExFreePool(uniDosName.Buffer);
									}

								}
								else
								{
									// 如果不是以 "\\Device\\HarddiskVolume" 这样的形式开头的，那么直接复制名称。
									wcsncpy(wzPath, Temp->Name.Buffer, Temp->Name.Length);
									bGetPath = TRUE;
								}
							}
						}

						ExFreePool(Buffer);
					}
				}
			}
		}
	}
	__except(1)
	{
		DbgPrint("GetPathByFileObject Catch __Except\r\n");
		bGetPath = FALSE;
	}

	return bGetPath;
}



//得到Idle的EProcess
extern
	ULONG_PTR g_BuildNumber;


PEPROCESS 
	GetIdleProcess()
{
	ULONG_PTR uIdleAddr = 0;
	ULONG_PTR PsInitialSystemProcessAddress = (ULONG_PTR)&PsInitialSystemProcess;

	DbgPrint("%x\r\n",PsInitialSystemProcessAddress);
	if (g_BuildNumber == 7601)
	{
		if (PsInitialSystemProcessAddress && MmIsAddressValid((PVOID)((ULONG_PTR)PsInitialSystemProcessAddress + 0xA0)))
		{
			uIdleAddr = *(PULONG_PTR)((ULONG_PTR)PsInitialSystemProcessAddress + 0xA0);
		}
	}

	if (g_BuildNumber==2600)
	{
		if (PsInitialSystemProcessAddress && MmIsAddressValid((PVOID)((ULONG_PTR)PsInitialSystemProcessAddress - 0x78B4)))
		{
			uIdleAddr = *(PULONG_PTR)((ULONG_PTR)PsInitialSystemProcessAddress - 0x78B4);
		}
	}


	DbgPrint("IdleEProcess:%p\r\n",uIdleAddr);

	return	 (PEPROCESS)uIdleAddr;

}




///////////////////////////////////     接口
//

BOOLEAN 
	GetNormalProcessList(PPROCESSINFO Info,PPROCESSINFO HideInfo)
{
	ULONG_PTR		i = 0;
	char  lpszString[256];

	//
	Info->ulCount =  ProcessInfo_Sys->ulCount;

	i = 0;
	Info->ProcessInfo[i].IntHideType = 0;
	Info->ProcessInfo[i].ulPid = 0;
	Info->ProcessInfo[i].EProcess = (ULONG)GetIdleProcess();  //获得Idle进程

	memset(Info->ProcessInfo[i].lpwzFullProcessPath,0,sizeof(Info->ProcessInfo[i].lpwzFullProcessPath));
	wcsncat(Info->ProcessInfo[i].lpwzFullProcessPath,L"System Idle",wcslen(L"System Idle"));

	memset(Info->ProcessInfo[i].ImageName,0,sizeof(Info->ProcessInfo[i].ImageName));
	wcsncat(Info->ProcessInfo[i].ImageName,L"System Idle",wcslen(L"System Idle"));

	Info->ProcessInfo[i].ulKernelOpen = 1;
	Info->ProcessInfo[i].ulInheritedFromProcessId = 0;

	//以上处理的是Idle进程

	i++;

	__try
	{
		for(i=1;i<(Info->ulCount);i++)
		{
			Info->ProcessInfo[i].IntHideType = 0;
			Info->ProcessInfo[i].ulPid = ProcessInfo_Sys->ProcessIdAndEProcess[i].ulPid;
			Info->ProcessInfo[i].EProcess = ProcessInfo_Sys->ProcessIdAndEProcess[i].EProcess;

			//ImageName和 Path
			if((Info->ProcessInfo[i].ulPid)==4)
			{
				memset(Info->ProcessInfo[i].lpwzFullProcessPath,0,sizeof(Info->ProcessInfo[i].lpwzFullProcessPath));
				wcsncat(Info->ProcessInfo[i].lpwzFullProcessPath,L"System",wcslen(L"System"));

				memset(Info->ProcessInfo[i].ImageName,0,sizeof(Info->ProcessInfo[i].ImageName));
				wcsncat(Info->ProcessInfo[i].ImageName,L"System",wcslen(L"System"));
			}
			else
			{
				strcpy(lpszString,(char*)((Info->ProcessInfo[i].EProcess)+g_ImageFileName));
				CharToWchar(lpszString,Info->ProcessInfo[i].ImageName);

				//获得路径
				memset(Info->ProcessInfo[i].lpwzFullProcessPath,0,sizeof(Info->ProcessInfo[i].lpwzFullProcessPath));
				//GetProcessPathByPeb((PEPROCESS)(Info->ProcessInfo[i].EProcess),Info->ProcessInfo[i].lpwzFullProcessPath);
				GetProcessPathBySectionObject(Info->ProcessInfo[i].ulPid,Info->ProcessInfo[i].lpwzFullProcessPath);
			}


			//获得内核访问状态  和 父进程句柄
			Info->ProcessInfo[i].ulKernelOpen = KernelStatus((HANDLE)Info->ProcessInfo[i].ulPid);
			Info->ProcessInfo[i].ulInheritedFromProcessId = GetInheritedProcessPid((PEPROCESS)Info->ProcessInfo[i].EProcess);
		}

	}__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("ListProcess Error By Force !\r\n");
	}


	return	TRUE;
}






//////////////////////////////////////////////////////////////////////////
//辅助函数

extern ULONG_PTR    g_ObjectHeaderSize;
extern ULONG_PTR    g_ObjectTypeOffsetOf_Object_Header;
extern ULONG_PTR    g_ObjectTableOffsetOf_EPROCESS;
extern ULONG_PTR    g_PreviousModeOffsetOf_KTHREAD;

BOOLEAN
	IsRealProcess(PEPROCESS EProcess)
{
	ULONG_PTR ObjectType; 
	ULONG_PTR    ObjectTypeAddress; 
	BOOLEAN bRet = FALSE;

	ULONG_PTR ProcessType = ((ULONG_PTR)*PsProcessType);

	if (ProcessType && MmIsAddressValid && EProcess && MmIsAddressValid((PVOID)(EProcess)))
	{ 
		ObjectType = KeGetObjectType((PVOID)EProcess);

		if (ObjectType && ProcessType == ObjectType && !IsProcessDie(EProcess))
		{
			bRet = TRUE; 
		}
	} 

	return bRet; 
}

ULONG_PTR 
	KeGetObjectType(PVOID Object)
{
	ULONG_PTR ObjectType = 0;
	pfnObGetObjectType        ObGetObjectType = NULL;    


	if (!Object||!MmIsAddressValid(Object))
	{
		return 0;
	}

	if (g_BuildNumber < 6000)
	{
		ULONG SizeOfObjectHeader = 0, ObjectTypeOffset = 0, ObjectTypeAddress = 0;

		ObjectTypeAddress =(ULONG) ((ULONG_PTR)Object - g_ObjectHeaderSize + g_ObjectTypeOffsetOf_Object_Header);

		if (MmIsAddressValid((PVOID)ObjectTypeAddress))
		{ 
			ObjectType = *(ULONG_PTR*)ObjectTypeAddress;
		}
	}
	else
	{
		//高版本使用函数

		ObGetObjectType = (pfnObGetObjectType)GetFunctionAddressByName(L"ObGetObjectType");

		if (ObGetObjectType)
		{
			ObjectType = ObGetObjectType(Object);
		}
	}

	return ObjectType;
}

BOOLEAN 
	IsProcessDie(PEPROCESS EProcess)
{
	BOOLEAN bDie = FALSE;

	if (MmIsAddressValid &&
		EProcess && 
		MmIsAddressValid(EProcess) &&
		MmIsAddressValid((PVOID)((ULONG_PTR)EProcess + g_ObjectTableOffsetOf_EPROCESS)))
	{
		PVOID ObjectTable = *(PVOID*)((ULONG_PTR)EProcess + g_ObjectTableOffsetOf_EPROCESS );

		if (!ObjectTable||!MmIsAddressValid(ObjectTable) )
		{
			DbgPrint("Process is Die\r\n");
			bDie = TRUE;
		}
	}
	else
	{
		bDie = TRUE;
	}

	return bDie;
}



//
BOOLEAN 
	KernelStatus(HANDLE hPid)
{
	HANDLE hProcess;
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	CLIENT_ID ClientId={0};
	BOOLEAN bRetOK = FALSE;

	ClientId.UniqueProcess = hPid;
	InitializeObjectAttributes(&ObjectAttributes,NULL,0,NULL,NULL);

	status=ZwOpenProcess(&hProcess,PROCESS_ALL_ACCESS,&ObjectAttributes,&ClientId);

	if (NT_SUCCESS(status))
	{
		bRetOK = TRUE;
		ZwClose(hProcess);
	}

	return bRetOK;
}

//得到父进程的PID
ULONG_PTR
	GetInheritedProcessPid(PEPROCESS Eprocess)
{
	NTSTATUS Status;
	PROCESS_BASIC_INFORMATION pbi;
	HANDLE hProcess;
	ULONG_PTR Pid=0;
	BOOLEAN bInit = FALSE;

	if (!MmIsAddressValid(Eprocess))
		return 0;

	Status = ObOpenObjectByPointer(
		Eprocess,         
		OBJ_KERNEL_HANDLE,     
		NULL,               
		PROCESS_ALL_ACCESS,       
		*PsProcessType,     
		KernelMode,            
		&hProcess);

	if (!NT_SUCCESS(Status))
		return 0;

	Status = ZwQueryInformationProcess(hProcess,
		ProcessBasicInformation,
		(PVOID)&pbi,
		sizeof(PROCESS_BASIC_INFORMATION),
		NULL );
	if (!NT_SUCCESS(Status))
	{
		ZwClose(hProcess);

		return 0;
	}

	Pid = pbi.InheritedFromUniqueProcessId;
	ZwClose(hProcess);
	return Pid;
}