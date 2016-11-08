
#include <ntifs.h>
#include <ntimage.h>
#include "ToolFunction.h"

//ZwQuerySystemInformation
typedef enum _SYSTEM_INFORMATION_CLASS {  
	SystemBasicInformation=0,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation, 
	SystemModuleInformation,
	SystemLocksInformation, 
	SystemStackTraceInformation,
	SystemPagedPoolInformation, 
	SystemNonPagedPoolInformation,  
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation, 
	SystemSummaryMemoryInformation, 
	SystemNextEventIdInformation,
	SystemEventIdsInformation,
	SystemCrashDumpInformation,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemPlugPlayBusInformation,
	SystemDockInformation,
	SystemPowerInformation2,
	SystemProcessorSpeedInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation
}SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;


typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER SpareLi1;
	LARGE_INTEGER SpareLi2;
	LARGE_INTEGER SpareLi3;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ImageName;
	KPRIORITY BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
	ULONG HandleCount;
	ULONG SessionId;
	ULONG_PTR PageDirectoryBase;
	SIZE_T PeakVirtualSize;
	SIZE_T VirtualSize;
	ULONG PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER ReadOperationCount;
	LARGE_INTEGER WriteOperationCount;
	LARGE_INTEGER OtherOperationCount;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;



typedef struct _SYSTEM_MODULE_TABLE_ENTRY_INFO
{
	HANDLE Reserved[2];
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT Unknown;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	CHAR ImageName[256];
}SYSTEM_MODULE_TABLE_ENTRY_INFO,*PSYSTEM_MODULE_TABLE_ENTRY_INFO;
typedef struct SYSTEM_MODULE_INFORMATION
{
	ULONG ulNumberOfModules;
	SYSTEM_MODULE_TABLE_ENTRY_INFO smi[1];
}SYSTEM_MODULE_INFORMATION,*PSYSTEM_MODULE_INFORMATION;


NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation OPTIONAL,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

NTSYSAPI
UCHAR* 
NTAPI
PsGetProcessImageFileName(
	IN PEPROCESS Process
	);


//
// 通过进程名获取进程ID
//
NTSTATUS 
LhpGetProcessIdByName(
	IN PWCHAR ProcessName,
	OUT PHANDLE ProcessId
	)
{
	ULONG ulTempLen = 0;
	PUCHAR pRawProcess = NULL;
	PSYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
	PVOID pBuffer = NULL;
	ULONG ulTempOffset = 0;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		return Status;
	}


	Status = ZwQuerySystemInformation(SystemProcessInformation,
		NULL,
		0,
		&ulTempLen
		);
	if (!NT_SUCCESS(Status) && Status != STATUS_INFO_LENGTH_MISMATCH)
	{
		return Status;
	}

	pBuffer = (PVOID)ExAllocatePool(NonPagedPool, ulTempLen);
	if (!pBuffer)
	{
		return Status;
	}


	//申请的内存空间清零
	RtlZeroMemory(pBuffer, ulTempLen);

	Status = ZwQuerySystemInformation(SystemProcessInformation, 
		pBuffer, 
		ulTempLen, 
		NULL);
	if (!NT_SUCCESS(Status))
	{
		ExFreePool(pBuffer);
		return Status;
	}

	pRawProcess = (PUCHAR)pBuffer;
	do 
	{
		ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pRawProcess[ulTempOffset];
		ulTempOffset += ProcessInfo->NextEntryOffset;

		if (ProcessInfo->ImageName.Buffer != NULL && ProcessInfo->UniqueProcessId != 0)
		{
			WCHAR tmpProcessName[260] = {0};
			RtlStringCbCopyUnicodeString(tmpProcessName, sizeof(tmpProcessName), &ProcessInfo->ImageName);
			_wcslwr(tmpProcessName);
			if (wcsstr(tmpProcessName, ProcessName) != NULL)
			{
				*ProcessId = ProcessInfo->UniqueProcessId;
				Status = STATUS_SUCCESS;
				break;
			}
		}

	} while (ProcessInfo->NextEntryOffset != 0);

	ExFreePool(pBuffer);
	return Status;
}


//
// 获取系统版本
//
NTSTATUS
LhpGetSystemVersion(
	ESystemVersion *Version
	)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RTL_OSVERSIONINFOW OsVersion;
	RtlZeroMemory(&OsVersion, sizeof(OsVersion));
	OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);

	Status = RtlGetVersion(&OsVersion);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	if (OsVersion.dwMajorVersion == 5 && OsVersion.dwMinorVersion == 1)
	{
		*Version = e_Windows_XP;
	}
	else if (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion == 1)
	{
		*Version = e_Windows_7;
	}
	else
	{
		*Version = e_Windows_NotSupport;
	}

	Status = STATUS_SUCCESS;
	return Status;
}


//####################################################################################################
// 根绝进程ID获取进程全路径名
//####################################################################################################
NTSTATUS
NTAPI
LhpGetProcessImageNameByProcessId(
	IN HANDLE ProcessId,
	OUT LPWSTR lpszImageName,
	IN OUT PULONG Length
	)
{
	NTSTATUS Status = STATUS_INVALID_PARAMETER;
	PEPROCESS Process;
	HANDLE ProcessHandle;
	ULONG ReturnLength;
	ULONG BufferSize;
	PIMAGE_NAME_INFO pImageNameInfo;

	//输入参数判断
	if (!ProcessId ||
		!lpszImageName ||
		!Length ||
		!*Length
		)
	{
		return Status;
	}

	//由进程Id获取进程内核对象
	Status = PsLookupProcessByProcessId(ProcessId, &Process);

	if (!NT_SUCCESS(Status)){
		return Status;
	}

	//由进程内核对象获取进程句柄
	Status = ObOpenObjectByPointer(Process,
		OBJ_KERNEL_HANDLE,
		NULL,
		PROCESS_QUERY_INFORMATION,
		*PsProcessType,
		KernelMode,
		&ProcessHandle
		);
	if (!NT_SUCCESS(Status)){
		ObDereferenceObject(Process);
		return Status;
	}

	BufferSize = sizeof(UNICODE_STRING)+*Length * sizeof(WCHAR);
	pImageNameInfo = (PIMAGE_NAME_INFO)ExAllocatePool(NonPagedPool, BufferSize);
	if (!pImageNameInfo){
		ObDereferenceObject(Process);
		ZwClose(ProcessHandle);
		Status = STATUS_NO_MEMORY;
		return Status;
	}

	//由进程句柄获取进程信息（Image Name）
	ReturnLength = 0;
	Status = ZwQueryInformationProcess(ProcessHandle,
		ProcessImageFileName,
		pImageNameInfo,
		BufferSize - sizeof(WCHAR),
		&ReturnLength
		);
	if (!NT_SUCCESS(Status)){
		ObDereferenceObject(Process);
		ZwClose(ProcessHandle);
		if (Status == STATUS_INFO_LENGTH_MISMATCH){
			*Length = ReturnLength - sizeof(UNICODE_STRING);
		}
		else{
			*Length = 0;
		}
		ExFreePool(pImageNameInfo);
		return Status;
	}
	RtlCopyMemory(lpszImageName, pImageNameInfo->Name.Buffer, pImageNameInfo->Name.Length);
	*Length = pImageNameInfo->Name.Length / sizeof(WCHAR);
	ExFreePool(pImageNameInfo);
	ObDereferenceObject(Process);
	ZwClose(ProcessHandle);

	if (NT_SUCCESS(Status)){
		lpszImageName[*Length] = 0;
	}
	return Status;

}


//////////////////////////////////////////////////////////////////////////
//
// LhpConvertDeviceName
//
// 将形如 "\\Device\\HarddiskVolumeX\\xxxx的内核文件路径
// 转化为 DOS文件路径(c:\\xxxx\\xxx....)
//
//////////////////////////////////////////////////////////////////////////
NTSTATUS 
LhpConvertDeviceName(
	IN LPCWSTR Ring0FileName,
	OUT LPCWSTR Ring3FileName,
	IN ULONG Ring3FileNameLength
	)
{
	HANDLE  FileHandle;
	OBJECT_ATTRIBUTES oba; 
	IO_STATUS_BLOCK iosb ;
	UNICODE_STRING uniname;
	NTSTATUS  stat;
	PFILE_OBJECT FileObject; 


	RtlInitUnicodeString(&uniname, Ring0FileName);

	InitializeObjectAttributes(&oba, &uniname, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE , 0 , 0 );


	stat = IoCreateFile(&FileHandle, 
		FILE_READ_ATTRIBUTES, 
		&oba,
		&iosb, 
		0,
		FILE_ATTRIBUTE_NORMAL, 
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, 
		0,
		0,
		CreateFileTypeNone, 
		0,
		IO_NO_PARAMETER_CHECKING
		);

	if (!NT_SUCCESS(stat))
	{
		return stat ; 
	}

	stat = ObReferenceObjectByHandle(FileHandle, 
		GENERIC_READ, 
		*IoFileObjectType, 
		KernelMode, 
		&FileObject, 
		NULL);

	if (!NT_SUCCESS(stat) ||
		!FileObject ||
		!FileObject->FileName.Length ||
		!FileObject->FileName.Buffer)
	{
		ZwClose(FileHandle);
		return STATUS_INVALID_PARAMETER; 
	}

	//开始获取DosName
	{
		UNICODE_STRING VolumeName; 
		UNICODE_STRING FanalName; 
		BOOLEAN  WillFreeVolumeName = TRUE; 


		VolumeName.Buffer = NULL;
		VolumeName.Length = 0;
		VolumeName.MaximumLength = 0;


		stat = RtlVolumeDeviceToDosName(FileObject->DeviceObject , &VolumeName);

		if (!NT_SUCCESS(stat))
		{
			RtlInitUnicodeString(&VolumeName , L"\\" );
			WillFreeVolumeName = FALSE ; 
		}
		if ((ULONG)(FileObject->FileName.Length + VolumeName.Length) >= Ring3FileNameLength)
		{
			ObDereferenceObject(FileObject);
			ZwClose(FileHandle);
			if (WillFreeVolumeName && MmIsAddressValid(VolumeName.Buffer))
			{
				ExFreePool(VolumeName.Buffer);
			}
			return STATUS_INVALID_PARAMETER; 
		}

		RtlZeroMemory((PVOID)Ring3FileName , Ring3FileNameLength);
		RtlInitUnicodeString(&FanalName , Ring3FileName);
		FanalName.MaximumLength = (USHORT)Ring3FileNameLength;

		//byte 
		if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&FanalName , &VolumeName)))
		{
			ObDereferenceObject(FileObject);
			ZwClose(FileHandle);
			if (WillFreeVolumeName && MmIsAddressValid(VolumeName.Buffer))
			{
				ExFreePool(VolumeName.Buffer);
			}
			return STATUS_INVALID_PARAMETER;      
		}

		if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&FanalName , &FileObject->FileName)))
		{
			ObDereferenceObject(FileObject);
			ZwClose(FileHandle);
			if (WillFreeVolumeName && MmIsAddressValid(VolumeName.Buffer))
			{
				ExFreePool(VolumeName.Buffer);
			}
			return STATUS_INVALID_PARAMETER;
		}

		ObDereferenceObject(FileObject);
		ZwClose(FileHandle);
		if (WillFreeVolumeName && MmIsAddressValid(VolumeName.Buffer))
		{
			ExFreePool(VolumeName.Buffer);
		}

		return STATUS_SUCCESS;
	}
}


//////////////////////////////////////////////////////////////////////////
// 修复PE文件导入表
//////////////////////////////////////////////////////////////////////////
NTSTATUS
LhpRepairImportTable(
	IN PVOID ImageBase, 
	IN ULONG ImageSize,
	IN PUNICODE_STRING ModuleFile
	)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PIMAGE_DOS_HEADER DosHeader = NULL;
	PIMAGE_NT_HEADERS NtHeader = NULL;
	PIMAGE_OPTIONAL_HEADER OptionHeader = NULL;
	IMAGE_DATA_DIRECTORY ImportTableInfo = {0, 0};
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	LARGE_INTEGER ReadOffset;
	PVOID FileContent = NULL;

	InitializeObjectAttributes(&ObjectAttributes, ModuleFile, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);		

	do 
	{
		//DbgBreakPoint();

		//
		// 0.分配内存，准备存储文件内容
		//
		FileContent = ExAllocatePool(PagedPool, 0x1000);
		if (FileContent == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}


		//
		// 1.打开PE文件，读取导入表
		//
		Status = IoCreateFile(&hFile,
			FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0,
			CreateFileTypeNone,
			NULL,
			IO_NO_PARAMETER_CHECKING
			);
		if(!NT_SUCCESS(Status) || hFile == NULL)
			break;

		ReadOffset.HighPart = 0;
		ReadOffset.LowPart = 0;
		Status = ZwReadFile(hFile,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			FileContent,
			0x1000,
			&ReadOffset,
			NULL
			);
		if (!NT_SUCCESS(Status))
			break;

		DosHeader = (PIMAGE_DOS_HEADER)FileContent;
		NtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)FileContent + DosHeader->e_lfanew);
		OptionHeader = &(NtHeader->OptionalHeader);

		ImportTableInfo.VirtualAddress = OptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		ImportTableInfo.Size = OptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
	
	
		//
		// 2.将文件内容patch到内存对应位置
		//
		DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
		NtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)ImageBase + DosHeader->e_lfanew);
		OptionHeader = &(NtHeader->OptionalHeader);
	
		OptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = ImportTableInfo.VirtualAddress;
		OptionHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = ImportTableInfo.Size;

	} while (FALSE);


	if (hFile != NULL)
	{
		ZwClose(hFile);
		hFile = NULL;
	}
	
	if (FileContent != NULL)
	{
		ExFreePool(FileContent);
		FileContent = NULL;
	}

	return Status;
}


//////////////////////////////////////////////////////////////////////////
// 判断进程是否是WOW64进程
//////////////////////////////////////////////////////////////////////////
BOOLEAN
LhpIsWow64Process(
	IN LPWSTR ProcessImageName
	)
{
	BOOLEAN bRet = FALSE;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PIMAGE_DOS_HEADER DosHeader = NULL;
	PIMAGE_NT_HEADERS NtHeader = NULL;
	HANDLE hFile = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	LARGE_INTEGER ReadOffset;
	PVOID FileContent = NULL;
	UNICODE_STRING usProcessImageName;

	RtlInitUnicodeString(&usProcessImageName, ProcessImageName);
	InitializeObjectAttributes(&ObjectAttributes, &usProcessImageName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);		

	do 
	{
		if (sizeof(PVOID) == 4)
			break;

		//
		// 0.分配内存，准备存储文件内容
		//
		FileContent = ExAllocatePool(PagedPool, 0x1000);
		if (FileContent == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}


		//
		// 1.打开PE文件，读取导入表
		//
		Status = IoCreateFile(&hFile,
			FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OPEN,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0,
			CreateFileTypeNone,
			NULL,
			IO_NO_PARAMETER_CHECKING
			);
		if(!NT_SUCCESS(Status) || hFile == NULL)
			break;

		ReadOffset.HighPart = 0;
		ReadOffset.LowPart = 0;
		Status = ZwReadFile(hFile,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			FileContent,
			0x1000,
			&ReadOffset,
			NULL
			);
		if (!NT_SUCCESS(Status))
			break;

		DosHeader = (PIMAGE_DOS_HEADER)FileContent;
		NtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)FileContent + DosHeader->e_lfanew);

		if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
		{
			bRet = TRUE;
		}
		else
		{
			bRet = FALSE;
		}
	} while (FALSE);


	if (hFile != NULL)
	{
		ZwClose(hFile);
		hFile = NULL;
	}

	if (FileContent != NULL)
	{
		ExFreePool(FileContent);
		FileContent = NULL;
	}
	
	return bRet;
}