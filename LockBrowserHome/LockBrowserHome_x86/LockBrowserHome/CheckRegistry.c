
#include "CheckRegistry.h"
#include <Ntstrsafe.h>


#define MAX_PATH 260
typedef unsigned long DWORD;
typedef struct _REGISTRY_VALUE_ITEM
{
	PWSTR ValueName;
	UCHAR ValueType;
	union
	{
		PWSTR ValueString;
		DWORD ValueDword;
	}ValueData;
}REGISTRY_VALUE_ITEM, *PREGISTRY_VALUE_ITEM;

static REGISTRY_VALUE_ITEM g_DriverRegistryValues[]=
{
	{L"Description", REG_SZ},
	{L"DisplayName", REG_SZ},
	{L"ErrorControl", REG_DWORD},
	{L"ImagePath", REG_EXPAND_SZ},
	{L"Start", REG_DWORD},
	{L"Tag", REG_DWORD},
	{L"Type", REG_DWORD},
};

enum ERegistryServiceValueType
{
	e_bd505_Description,
	e_bd505_DisplayName,
	e_bd505_ErrorControl,
	e_bd505_ImagePath,
	e_bd505_Start,
	e_bd505_Tag,
	e_bd505_Type,
};

WCHAR g_DriverFilePath[MAX_PATH] = {0};

NTSTATUS LhpInitializeRegistryData(PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	HANDLE hKey = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"ImagePath");
	PKEY_VALUE_PARTIAL_INFORMATION ValuePartialInfo = NULL;
	ULONG ReturnLength = 0;

	do 
	{
		ValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool, 500);
		if (ValuePartialInfo == NULL)
			break;

		RtlZeroMemory(ValuePartialInfo, 200);
		InitializeObjectAttributes(&ObjectAttributes, RegistryPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

		Status = ZwOpenKey(&hKey, KEY_QUERY_VALUE, &ObjectAttributes);
		if (!NT_SUCCESS(Status))
			break;

		Status = ZwQueryValueKey(hKey, &ValueName, KeyValuePartialInformation, ValuePartialInfo, 200, &ReturnLength);
		if (!NT_SUCCESS(Status))
			break;

		Status = RtlStringCbCopyNW(g_DriverFilePath, sizeof(g_DriverFilePath), (PWSTR)ValuePartialInfo->Data, ValuePartialInfo->DataLength);

	} while (FALSE);

	if (hKey)
	{
		ZwClose(hKey);
		hKey = NULL;
	}

	return Status;
}


NTSTATUS LhpCheckAndRepairRegistry()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING RegistryPath;
	UNICODE_STRING ValueName;
	WCHAR Buffer[MAX_PATH] = {0};
	HANDLE hServiceKey = NULL;
	DWORD LoadValue = 0;
	ULONG Index = 0;
	BOOLEAN bRepairOK = FALSE;

	do 
	{
		//
		// create service registry item
		//
		Status = RtlStringCbCopyW(Buffer, sizeof(Buffer), L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\LockBrowserHome");
		if (!NT_SUCCESS(Status))
			break;

		RtlInitUnicodeString(&RegistryPath, Buffer);
		InitializeObjectAttributes(&ObjectAttributes, &RegistryPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
		Status = ZwCreateKey(&hServiceKey, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
		if (!NT_SUCCESS(Status))
			break;


		g_DriverRegistryValues[e_bd505_Description].ValueData.ValueString = L"LockBrowserHome";
		g_DriverRegistryValues[e_bd505_DisplayName].ValueData.ValueString = L"LockBrowserHome";
		g_DriverRegistryValues[e_bd505_ErrorControl].ValueData.ValueDword = 1;
		g_DriverRegistryValues[e_bd505_ImagePath].ValueData.ValueString = g_DriverFilePath;
		g_DriverRegistryValues[e_bd505_Start].ValueData.ValueDword = 1;
		g_DriverRegistryValues[e_bd505_Tag].ValueData.ValueDword = 2;
		g_DriverRegistryValues[e_bd505_Type].ValueData.ValueDword = 0x01;

		for (Index = 0; Index < sizeof(g_DriverRegistryValues)/sizeof(g_DriverRegistryValues[0]); Index++)
		{
			RtlInitUnicodeString(&ValueName, g_DriverRegistryValues[Index].ValueName);

			if (g_DriverRegistryValues[Index].ValueType == REG_DWORD)
			{
				Status = ZwSetValueKey(hServiceKey, 
					&ValueName, 
					0, 
					REG_DWORD, 
					&g_DriverRegistryValues[Index].ValueData.ValueDword, 
					sizeof(DWORD)
					);
			}
			else if(g_DriverRegistryValues[Index].ValueType == REG_SZ || g_DriverRegistryValues[Index].ValueType == REG_EXPAND_SZ)
			{
				Status = ZwSetValueKey(hServiceKey, 
					&ValueName, 
					0, 
					g_DriverRegistryValues[Index].ValueType, 
					g_DriverRegistryValues[Index].ValueData.ValueString,
					(wcslen(g_DriverRegistryValues[Index].ValueData.ValueString) + 1) * sizeof(WCHAR)
					);
			}
			if (!NT_SUCCESS(Status))
				break;
		}

		if (Index == sizeof(g_DriverRegistryValues)/sizeof(g_DriverRegistryValues[0]))
			bRepairOK = TRUE;

		KdPrint((L"/-----LhpCheckAndRepairRegistry ok-------\n"));

	} while (FALSE);

	//clean:
	if (hServiceKey)
		ZwClose(hServiceKey);

	return bRepairOK ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

