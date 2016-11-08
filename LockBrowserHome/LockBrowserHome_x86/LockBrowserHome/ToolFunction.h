#ifndef __TOOL_FUNCTION__
#define __TOOL_FUNCTION__

#include <ntddk.h>
#include <Ntstrsafe.h>


typedef enum _ESystemVersion
{
	e_Windows_XP,
	e_Windows_7,
	e_Windows_NotSupport
}ESystemVersion, *PESystemVersion;


//
// 系统未导出接口声明
//
#define MAX_PATH 260
typedef struct _IMAGE_NAME_INFO{
	UNICODE_STRING Name;
	WCHAR Buffer[MAX_PATH + 20];
}IMAGE_NAME_INFO, *PIMAGE_NAME_INFO;


NTSTATUS
NTSYSAPI
ZwQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

#define PROCESS_QUERY_INFORMATION (0x0400)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))

NTSTATUS 
LhpGetProcessIdByName(
	IN PWCHAR ProcessName,
	OUT PHANDLE ProcessId
	);

NTSTATUS
LhpGetSystemVersion(
	ESystemVersion *Version
	);

NTSTATUS
NTAPI
LhpGetProcessImageNameByProcessId(
	IN HANDLE ProcessId,
	OUT LPWSTR lpszImageName,
	IN OUT PULONG Length
	);

NTSTATUS 
LhpConvertDeviceName(
	IN LPCWSTR Ring0FileName,
	OUT LPCWSTR Ring3FileName,
	IN ULONG Ring3FileNameLength
	);

NTSTATUS
LhpRepairImportTable(
	IN PVOID ImageBase,
	IN ULONG ImageSize,
	IN PUNICODE_STRING ModuleFile
	);

#endif