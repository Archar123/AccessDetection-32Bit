#ifndef _IDTHOOK_H_
#define	_IDTHOOK_H_

#include <NTIFS.H>


#define	MAKELONG(a,b)		((unsigned long)(((unsigned short)(a))|((unsigned long)((unsigned short)(b))) << 16))


#pragma pack(push)
#pragma pack(1) // 1�ֽڶ���
typedef struct _IDTR //IDT��ַ
{
	USHORT IDT_limit;				//��Χռλ//��С
	USHORT IDT_LOWbase;	//����ַռλ_IDT_ENTRY����ָ��
	USHORT IDT_HIGbase;
}IDTR,*PIDTR;

typedef struct _IDT_ENTRY
{
	USHORT LowOffset;	//�жϴ�������ַ��λ
	USHORT selector;		//ѡ���
	UCHAR  reserved;
	UCHAR  type:4;				//4λ
	UCHAR  always0:1;        //1λ
	UCHAR  dpl:2;                //2λ
	UCHAR  present:1;        //1λ
	USHORT HigOffset;	//�жϴ�������ַ��λ
}IDTENTRY,*PIDTENTRY;//��ȡ��ַʵ�������������
#pragma pack(pop)




//////////////////////////////////////////
//
typedef struct _INT01_STACK
{
  ULONG SaveEip;
  ULONG SaveCS;
  ULONG SaveEFLAGS;
} INT01_STACK, *PINT01_STACK;

typedef struct _FRAME_CONTEXT
{
	USHORT	cs;
	USHORT	ss;
	USHORT	ds;
	USHORT  es;
	USHORT	fs;
	USHORT  gs;
};





//DR6
typedef struct _DR6INFO
{
  ULONG B0:1;  //B0
  ULONG B1:1;  //B1
  ULONG B2:1;  //B2
  ULONG B3:1;  //B3
  ULONG Reserved1:9;  //reserved
  ULONG BD:1;  //BD
  ULONG BS:1;  //BS
  ULONG BT:1;  //BT
  ULONG Reserved2:16;  //Reserved
}DR6INFO, *PDR6INFO;

typedef struct _DR6 
{
	union  {
		ULONG 	Long;
		DR6INFO	Dr6Info;
	} u;
}DR6,*PDR6;



//DR7
typedef struct _DR7INFO
{
  ULONG L0:1;  //L0
  ULONG G0:1;  //G0
  ULONG L1:1;  //L1
  ULONG G1:1;  //G1
  ULONG L2:1;  //L2
  ULONG G2:1;  //G2
  ULONG L3:1;  //L3
  ULONG G3:1;  //G3
  ULONG LE:1;  //LE
  ULONG GE:1;  //GE
  ULONG Reserved1:3;  //reserved
  ULONG GD:1;  //GD
  ULONG Reserved2:2;  //reserved
  ULONG RW0:2;  //R/W0
  ULONG LEN0:2;  //LEN0
  ULONG RW1:2;  //R/W1
  ULONG LEN1:2;  //LEN1
  ULONG RW2:2;  //R/W2
  ULONG LEN2:2;  //LEN2
  ULONG RW3:2;  //R/W3
  ULONG LEN3:2;  //LEN3
}DR7INFO, *PDR7INFO;

typedef struct _DR7 
{
	union  {
		ULONG 	Long;
		DR7INFO	Dr7Info;
	} u;
}DR7,*PDR7;




//�Լ����жϴ������
///////////////////////////////////////////////////////////
ULONG
__stdcall
MyOneStep(ULONG* TrapFrame);


ULONG
__stdcall
MyPageFault(ULONG* TrapFrame);




//////////////////////////////////////////////////////////////////////////////////////
//
//HOOKIDT ֱ��ʹ�ýӿ� HOOK��01 �� 0E;
VOID
DataAccessDetection_IDT_Init();

VOID
DataAccessDetection_IDT_UnInit();



//�ⲿ���ýӿ�
VOID
DataAccessDetection_HOOKIDT(ULONG *OldTrapAddr,ULONG InterruptIndex,ULONG NewInterruptFunc);

VOID
DataAccessDetection_UNHOOKIDT(ULONG InterruptIndex,ULONG OldInterruptFunc);




//
VOID
HookInterrupt(ULONG uCPU, ULONG InterruptIndex, ULONG NewInterruptFunc);


VOID 
HookIdtDpc(
	IN struct _KDPC  *Dpc,
	IN PVOID  DeferredContext,
	IN PVOID  SystemArgument1,
	IN PVOID  SystemArgument2);



VOID
Z_HookIdtByDpc(ULONG InterruptIndex, ULONG NewInterruptFunc);


ULONG 
GetInterruptFuncAddress(ULONG	InterruptIndex);

//######################################
#endif