#include 	"Universal.h"



CHAR 
ChangePreMode(PETHREAD EThread)
{
	//XP
	ULONG PreviousModeOffsetOf_KTHREAD = 0x140;
	CHAR PreMode = *(PCHAR)((ULONG_PTR)EThread + PreviousModeOffsetOf_KTHREAD);
	*(PCHAR)((ULONG_PTR)EThread + PreviousModeOffsetOf_KTHREAD) = KernelMode;
	return PreMode;
}

VOID 
RecoverPreMode(PETHREAD EThread, CHAR PreMode)
{	
	ULONG PreviousModeOffsetOf_KTHREAD = 0x140;
	*(PCHAR)((ULONG_PTR)EThread + PreviousModeOffsetOf_KTHREAD) = PreMode;
}


//辅助函数   安全拷贝函数
BOOLEAN 
MyCopyMemory( PVOID pDestination, PVOID pSourceAddress, SIZE_T SizeOfCopy )
{
    PMDL pMdl = NULL;
    PVOID pSafeAddress = NULL;
    pMdl = IoAllocateMdl( pSourceAddress, (ULONG)SizeOfCopy, FALSE, FALSE, NULL );
    if( !pMdl ) return FALSE;
    __try
    {
        MmProbeAndLockPages( pMdl, KernelMode, IoReadAccess );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        IoFreeMdl( pMdl );
        return FALSE;
    }
    pSafeAddress = MmGetSystemAddressForMdlSafe( pMdl, NormalPagePriority );
    if( !pSafeAddress ) return FALSE;
    RtlCopyMemory( pDestination, pSafeAddress, SizeOfCopy );
    MmUnlockPages( pMdl );
    IoFreeMdl( pMdl );
    return TRUE;
}

BOOLEAN
MyLockMemory(PVOID VirtualAddress,SIZE_T SizeofOnePage,PVOID EProcess,PMDL* pMdl)
{
    PMDL mdl=NULL;
	KAPC_STATE	ApcState;
	KeStackAttachProcess(EProcess, &ApcState);

	*pMdl=NULL;

	mdl = IoAllocateMdl(VirtualAddress,(ULONG)SizeofOnePage,FALSE,FALSE,NULL);

	__try
	{//申请非分页内存，然后进行内存锁定
		MmProbeAndLockPages( mdl, KernelMode, IoReadAccess );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		IoFreeMdl( mdl );
		return FALSE;
	}

	KeUnstackDetachProcess(&ApcState);

	*pMdl=mdl;

	DbgPrint("[DataAccessDetection]MyLockMemory:MDL=%p,VA: %p,%d\n",mdl,VirtualAddress,SizeofOnePage);
	return	TRUE;
}

BOOLEAN
MyUnLockMemory(PVOID EProcess,PVOID pMdl)
{
	KAPC_STATE ApcState;
	KeStackAttachProcess(EProcess,&ApcState);
	if (pMdl)
	{
		MmUnlockPages(pMdl);
		IoFreeMdl( pMdl );

		DbgPrint("[DataAccessDetection]MyUnLockMemory: %p \n",pMdl);
	}
	KeUnstackDetachProcess(&ApcState);
	return FALSE;
}


VOID UnicodeToChar(PUNICODE_STRING dst, char *src)
{
	ANSI_STRING		string;
	ULONG					Len;

	RtlUnicodeStringToAnsiString(&string,dst, TRUE);
	Len	=	(string.Length)>255 ? 255	:	(string.Length);
	strncpy(src,string.Buffer,Len);

	RtlFreeAnsiString(&string);
}



//通过ImageBase来得到驱动的基地址
PVOID GetOEPByImageBase(PVOID ImageBase)
{
	PIMAGE_DOS_HEADER			pDOSHeader;
	PIMAGE_NT_HEADERS			pNTHeader;
	PVOID									pEntryPoint;

	pDOSHeader = (PIMAGE_DOS_HEADER)ImageBase;
	pNTHeader = (PIMAGE_NT_HEADERS)((ULONG)ImageBase + pDOSHeader->e_lfanew);
	pEntryPoint = (PVOID)((ULONG)ImageBase + pNTHeader->OptionalHeader.AddressOfEntryPoint);

	return pEntryPoint;
}


ULONG
GetImageBaseFromEProcess_Win7SP1(PVOID EProcess)
{
	ULONG		ImageBase = 0;
	ImageBase = *(PULONG)((ULONG)EProcess+0x12C);
	return ImageBase;	
}


//通过函数名称得到函数地址
ULONG
GetFuncAddrssFromFile(char* FunName,PUNICODE_STRING DllName)
{
	//为动态DllName 创建一个内存映射
	//初始化一个 OBJECT_ATTRIBUTES 存放DllName 
	OBJECT_ATTRIBUTES		oa     = {0};
	IO_STATUS_BLOCK		Iosb;
	HANDLE						hFile  = NULL;
	NTSTATUS					Status = STATUS_SUCCESS; 
	HANDLE						hSection = NULL;
	PVOID							BaseAddr = NULL;
	ULONG							uSize = 0;
	ULONG*						ArrayOfFunctionAddress = NULL;
	ULONG*						ArrayOfFunctionNames  = NULL;
	unsigned short*			ArrayOfFunctionOrdinals = NULL;
	ULONG							uBase = 0;
	char*							FunctionName = NULL;
	ULONG							FunctionOrdinals = 0;
	ULONG							FunctionAddress = 0;
	ULONG							i = 0;

	PIMAGE_DOS_HEADER					DosHead;
	PIMAGE_OPTIONAL_HEADER		OpHead;
	PIMAGE_EXPORT_DIRECTORY		ExportTable;

	//创建一个oa
	InitializeObjectAttributes(&oa,DllName,OBJ_CASE_INSENSITIVE,0,0);

	//映射文件
	Status = ZwOpenFile(&hFile,FILE_EXECUTE|SYNCHRONIZE,&oa,&Iosb,FILE_SHARE_READ,FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(Status))
	{
		LOG_INFO("[DataProtect]GetFuncAddrss [ZwOpenFile] Failed!\r\n");
		return 0;
	}


	InitializeObjectAttributes(&oa,NULL,0,0,0);

	Status = ZwCreateSection(&hSection, SECTION_ALL_ACCESS, &oa,0,PAGE_EXECUTE, SEC_IMAGE, hFile);
	if (!NT_SUCCESS(Status))
	{
		ZwClose(hFile);
		LOG_INFO("[DataProtect]GetFuncAddrss [ZwCreateSection] Failed!\r\n");
		return 0;
	}

	Status = ZwMapViewOfSection(hSection, NtCurrentProcess(),&BaseAddr, 0, 1000, 0, &uSize, (SECTION_INHERIT)1, MEM_TOP_DOWN, PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	{
		ZwClose(hFile);
		LOG_INFO("[DataProtect]GetFuncAddrss [ZwMapViewOfSection] Failed!\r\n");
		return 0;
	}

	ZwClose(hFile);

	//在导出表中查找查找函数地址
	//Dos头
	DosHead = (PIMAGE_DOS_HEADER)BaseAddr;
	//选项头
	OpHead = (PIMAGE_OPTIONAL_HEADER)((ULONG)DosHead + DosHead->e_lfanew +24);
	//导出表
	ExportTable = (PIMAGE_EXPORT_DIRECTORY)(((ULONG)BaseAddr) + OpHead->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);


	//获得导出表中四个成员 
	//获得函数地址表
	ArrayOfFunctionAddress = (ULONG*)(((ULONG)BaseAddr)+ExportTable->AddressOfFunctions);

	//获得函数名表
	ArrayOfFunctionNames = (ULONG*)(((ULONG)BaseAddr)+ExportTable->AddressOfNames);

	//获得函数序数表
	ArrayOfFunctionOrdinals = (USHORT*)(((ULONG)BaseAddr)+ExportTable->AddressOfNameOrdinals);

	uBase = ExportTable->Base;
	for (i=0;i<ExportTable->NumberOfFunctions;i++)//检查每一个函数
	{
		// 得到函数名称
		FunctionName = (char*)((ULONG)BaseAddr+ArrayOfFunctionNames[i]);

		//获得函数的序数	
		FunctionOrdinals = ArrayOfFunctionOrdinals[i] + uBase -1;

		if (strncmp(FunctionName,FunName,strlen(FunName))==0)
		{
			//通过序数得到函数地址
			FunctionAddress = (ULONG)(((ULONG)BaseAddr) +ArrayOfFunctionAddress[FunctionOrdinals]);
			break;
		}
	}

	ZwClose(hSection);
	return FunctionAddress;
}


void PageProtectOn()
{
	//恢复内存保护
	__asm
	{
			mov eax,	cr0
			or	eax, 10000h
			mov cr0,	eax
			sti
	}
}

void PageProtectOff()
{
	//去掉内存保护
	__asm
	{
			cli
			mov eax, cr0
			and eax, not 10000h
			mov cr0, eax
	}
}


NTSTATUS 
MyCopyFile(PUNICODE_STRING DestinationFileName,PUNICODE_STRING SourceFileName)
{
   NTSTATUS 										status;
   HANDLE 											SourceFileHandle=NULL;
   HANDLE 											DestinationFileHandle=NULL;
   OBJECT_ATTRIBUTES 						ObjectAttributes;
   IO_STATUS_BLOCK 							IoStatusBlock;
   FILE_STANDARD_INFORMATION 		FileInfo;
   ULONG 												AllocationSize;
   PVOID 												FileBuffer=NULL;
   BOOLEAN 											bAllocateInVirtualMemory=FALSE;

   InitializeObjectAttributes(&ObjectAttributes,SourceFileName,OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,NULL,NULL);
          
                               
   status=IoCreateFile(&SourceFileHandle,GENERIC_READ|SYNCHRONIZE,&ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL,FILE_SHARE_READ,FILE_OPEN,FILE_SYNCHRONOUS_IO_NONALERT,NULL,0,CreateFileTypeNone,0,IO_NO_PARAMETER_CHECKING);
   if (!NT_SUCCESS(status))
   {
       LOG_INFO("IoCreateFile (%wZ) failed,eid=0x%08x\n",SourceFileName,status);
       goto cleanup;
   }


   status=ZwQueryInformationFile(SourceFileHandle,&IoStatusBlock,(PVOID)&FileInfo,sizeof(FileInfo),FileStandardInformation);
   if (!NT_SUCCESS(status))
   {
       LOG_INFO("ZwQueryFileInformation (%wZ) failed,eid=0x%08x\n",SourceFileName,status);
       goto cleanup;
   }

   AllocationSize=FileInfo.AllocationSize.LowPart;


   FileBuffer=ExAllocatePoolWithTag(PagedPool,AllocationSize,'CODE');
   if (!FileBuffer)
   {
       status=ZwAllocateVirtualMemory((HANDLE)(-1),(PVOID)&FileBuffer,0,&AllocationSize,MEM_COMMIT,PAGE_READWRITE);
       if (!NT_SUCCESS(status))
       {
           LOG_INFO("Cannot Allocate Such Large Buffer!\n");
           goto cleanup;
       }
       
       bAllocateInVirtualMemory=TRUE;
   }


   status=ZwReadFile(SourceFileHandle,NULL,NULL,NULL,&IoStatusBlock,FileBuffer,AllocationSize,NULL,NULL);
   if (!NT_SUCCESS(status))
   {
       LOG_INFO("ZwReadFile (%wZ) failed,eid=0x%08x\n",SourceFileName,status);
       goto cleanup;
   }
   
   
   InitializeObjectAttributes(&ObjectAttributes,DestinationFileName,OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,NULL,NULL);
   
   
   status=IoCreateFile(&DestinationFileHandle,GENERIC_READ|GENERIC_WRITE,&ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,FILE_OVERWRITE_IF,FILE_SYNCHRONOUS_IO_NONALERT,NULL,0,CreateFileTypeNone,NULL,IO_NO_PARAMETER_CHECKING);
   if (!NT_SUCCESS(status))
   {
       LOG_INFO("IoCreateFile (%wZ) failed,eid=0x%08x\n",DestinationFileName,status);
       goto cleanup;
   }

   status=ZwWriteFile(DestinationFileHandle,NULL,NULL,NULL,&IoStatusBlock,FileBuffer,AllocationSize,NULL,NULL);
   if (!NT_SUCCESS(status))
   {
       LOG_INFO("ZwWriteFile (%wZ) failed,eid=0x%08x\n",DestinationFileName,status);		
   }
   
   
cleanup:
   if (bAllocateInVirtualMemory)
       ZwFreeVirtualMemory((HANDLE)(-1),(PVOID)&FileBuffer,&AllocationSize,MEM_RELEASE);
   else if(FileBuffer)
       ExFreePoolWithTag(FileBuffer,'CODE');
   if(SourceFileHandle)
       ZwClose(SourceFileHandle);
   if (DestinationFileHandle)
       ZwClose(DestinationFileHandle);
   
   return status;
   
}


//通过 函数名称 得到函数地址
PVOID 
GetFunctionAddressByName(WCHAR *szFunction)
{
	UNICODE_STRING uniFunction;  
	PVOID AddrBase = NULL;

	if (szFunction && wcslen(szFunction) > 0)
	{
		RtlInitUnicodeString(&uniFunction, szFunction);
		AddrBase = MmGetSystemRoutineAddress(&uniFunction);
	}

	return AddrBase;
}


VOID	
CharToWchar(PCHAR src,PWCHAR dst)
{
	UNICODE_STRING	uString;
	ANSI_STRING	aString;

	RtlInitAnsiString(&aString,src);
	RtlAnsiStringToUnicodeString(&uString,&aString,TRUE);
	wcscpy(dst,uString.Buffer);
	RtlFreeUnicodeString(&uString);
}
