#include "Predef.h"
#include "ProtectSelf.h"
#include "Processes.h"

/*
//************�궨��*************************************************
#define  PROCESS_TERMINATE 0x0001
#define  PROCESS_CREATE_PROCESS 0x0080
typedef  (* KIINSERTQUEUEAPC)(
						IN PKAPC Apc,
						IN KPRIORITY Increment);

//************���ݶ���*************************************************
BYTE OriginalHead[5]={0};
BYTE ReplaceHead[5]={0xE9,0,0,0,0};
extern POBJECT_TYPE *PsProcessType;
extern PDEVICE_EXTENSION  g_DeviceExtension;
KIINSERTQUEUEAPC g_KiInsertQueueApc;
//************�ڲ���������*********************************************
VOID
FASTCALL
DatourMyKiInsertQueueApc (
		IN PKAPC Apc,
		IN KPRIORITY Increment);
VOID
FASTCALL
OriginalKiInsertQueueApc(
	    IN PKAPC Apc,
	    IN KPRIORITY Increment);

//
ULONG GetFunctionAddr( IN PCWSTR FunctionName)     //PCWSTR����ָ�룬ָ��16λUNICODE
{
	UNICODE_STRING UniCodeFunctionName;
	RtlInitUnicodeString( &UniCodeFunctionName, FunctionName );
	return (ULONG)MmGetSystemRoutineAddress( &UniCodeFunctionName );   
}
//
ULONG GetKiInsertQueueApcAddr()
{
	ULONG sp_code1=0x28,sp_code2=0xe8,sp_code3=0xd88a;  //������,sp_code3 windbg��ʾ����Ӧ��Ϊd88a
	ULONG address=0;
	PUCHAR addr;
	PUCHAR p;
	addr=(PUCHAR)GetFunctionAddr(L"KeInsertQueueApc");
	for(p=addr;p<p+PAGE_SIZE;p++)
	{
		if((*(p-1)==sp_code1)&&(*p==sp_code2)&&(*(PUSHORT)(p+5)==sp_code3))
		{
			address=*(PULONG)(p+1)+(ULONG)(p+5);
			break;
		}
	}
	KdPrint(("[KeInsertQueueApc] addr %x\n",(ULONG)addr));
	KdPrint(("[KiInsertQueueApc] address %x\n",address));
	return address;
}

/************************************************************************
* ��������:HookKiInsertQueueApc
* ��������:��װ����KiInsertQueueApc����
* �����б�:
* ���� ֵ:��
*************************************************************************/
void HookKiInsertQueueApc()
{

	g_KiInsertQueueApc=(KIINSERTQUEUEAPC)GetKiInsertQueueApcAddr();
	RtlCopyMemory(OriginalHead,(BYTE *)g_KiInsertQueueApc,5);
	*(ULONG *)(ReplaceHead+1)=(ULONG)DatourMyKiInsertQueueApc-((ULONG)g_KiInsertQueueApc+5);
	KIRQL Irql;
	Irql=WOFF();
	//д���µĺ���ͷ
	RtlCopyMemory((BYTE *)g_KiInsertQueueApc,ReplaceHead,5);
	WON(Irql);
	
};
/************************************************************************
* ��������:DatourMyKiInsertQueueApc
* ��������:Hook�ص�����
* �����б�:

* ���� ֵ:״̬
*************************************************************************/
VOID FASTCALL DatourMyKiInsertQueueApc(
								  IN PKAPC Apc,
								  IN KPRIORITY Increment)
{
	NTSTATUS status=STATUS_SUCCESS;
	WCHAR TarProcPath;
	BOOL bRet;
	ULONG ulThread;
	ULONG ulEp;
	PEPROCESS pEprocess;
	pEprocess=PsGetCurrentProcess();
	if (pEprocess!=g_DeviceExtension->pEprocess)
	{
		if(MmIsAddressValid((PULONG)((ULONG)Apc+0x008)))    //��ַ��֤ KAPC�ṹ+008--->kthread
			ulThread=*((PULONG)((ULONG)Apc+0x008));
		else
			return ;
		if(MmIsAddressValid((PULONG)((ULONG)ulThread+0x044))) //kthread+30-->KAPC_STATE+10-->eprocess
			ulEp=*((PULONG)((ULONG)ulThread+0x044));
		else
			return ;
		if(((PEPROCESS)ulEp==g_DeviceExtension->pEprocess)&&(Increment==2))
		{
			return;

		}    
	}
	OriginalKiInsertQueueApc(Apc,Increment);
}

_declspec (naked) VOID FASTCALL OriginalKiInsertQueueApc(
	IN PKAPC Apc,
	IN KPRIORITY Increment)
{
	_asm
	{
		mov edi,edi
		push ebp
		mov ebp,esp
		mov eax,g_KiInsertQueueApc
		add eax,5
		jmp eax

	}
}

/************************************************************************
* ��������:UnHookProcessCreate
* ��������:ж������
* �����б�:
* ���� ֵ:��
*************************************************************************/
void UnHookKiInsertQueueApc()
{
	KIRQL Irql;
	Irql=WOFF();
	//д��ԭ���ĺ���ͷ
	RtlCopyMemory((BYTE *)g_KiInsertQueueApc,OriginalHead,5);
	WON(Irql);
};

