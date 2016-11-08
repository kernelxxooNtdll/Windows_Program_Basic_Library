
#ifndef __CHECK_REGISTRY_H__
#define __CHECK_REGISTRY_H__

#include <ntddk.h>

NTSTATUS LhpInitializeRegistryData(PUNICODE_STRING RegistryPath);
NTSTATUS LhpCheckAndRepairRegistry();

#endif //__CHECK_REGISTRY_H__