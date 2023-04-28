#ifndef		_UNIVERSAL_H_
#define		_UNIVERSAL_H_

#include		<Ntifs.h>
#include		<ntimage.h>
#include		<stdio.h>


#define SEC_IMAGE    		0x01000000


#define TRAPFRAME_DR6  	1		//Dr6

#define TRAPFRAME_CR2  	0		//ҳ�����ַ
#define TRAPFRAME_CR3  	1		//����
#define TRAPFRAME_EDI		2		//pushad
#define TRAPFRAME_ESI		3
#define TRAPFRAME_EBP		4
#define TRAPFRAME_ESP		5
#define TRAPFRAME_EBX		6
#define TRAPFRAME_EDX		7
#define TRAPFRAME_ECX		8
#define TRAPFRAME_EAX  	9
#define TRAPFRAME_EFLG	10	//pushfd
#define TRAPFRAME_XEAX 	11	//push eax
#define TRAPFRAME_TERC 	12	//TrapFrame 	ErrorCode   push edx
#define TRAPFRAME_TEIP		13							//EIP
#define TRAPFRAME_TCS		14							//CS
#define	TRAPFRAME_TFLG	15							//EFlags

#define TRAPFRAME_DEIP	13							//EIP
#define TRAPFRAME_DCS		14							//CS
#define	TRAPFRAME_DFLG	15							//EFlags


/////////////////////////////////////////////////////////////////////////////////////////
//�Ƿ���Log

//#define  LOG_INFO DbgPrint
#define		LOG_INFO //_
#define		ERR_INFO	 DbgPrint


/////////////////////////////////////////////////////////////////////////////////////////
//�����л�
CHAR 
ChangePreMode(PETHREAD EThread);

VOID 
RecoverPreMode(PETHREAD EThread, CHAR PreMode);

//��������   ��ȫ��������
BOOLEAN 
MyCopyMemory( PVOID pDestination, PVOID pSourceAddress, SIZE_T SizeOfCopy );

//�����ڴ溯��
BOOLEAN
MyLockMemory(PVOID VirtualAddress,SIZE_T SizeofOnePage,PVOID EProcess,PMDL* pMdl);

//�����ڴ溯��
BOOLEAN
MyUnLockMemory(PVOID EProcess,PVOID pMdl);


//ͨ���������Ƶõ�������ַ
ULONG
GetFuncAddrssFromFile(char* FunName,PUNICODE_STRING DllName);

//���OEP
PVOID 
GetOEPByImageBase(PVOID ImageBase);

VOID 	
UnicodeToChar(PUNICODE_STRING dst, char *src);


ULONG
GetImageBaseFromEProcess_Win7SP1(PVOID EProcess);


void
PageProtectOn();

void
PageProtectOff();



NTSTATUS 
MyCopyFile(PUNICODE_STRING DestinationFileName,PUNICODE_STRING SourceFileName);



extern 
UCHAR* 
PsGetProcessImageFileName(PEPROCESS EProcess);


PVOID 
GetFunctionAddressByName(WCHAR *szFunction);


VOID	
CharToWchar(PCHAR src,PWCHAR dst);

#endif