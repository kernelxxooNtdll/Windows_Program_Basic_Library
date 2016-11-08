
#ifndef __MAIN_H__
#define __MAIN_H__


#include "IoControl.h"


VOID
DriverUnload(
	IN PDRIVER_OBJECT DriverObject
	);

NTSTATUS
LhpCheckSystem();


NTSTATUS
LhpCreateDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING DeviceName,
	IN PUNICODE_STRING SymbolLinkName,
	OUT PDEVICE_OBJECT *pDeviceObject
	);

NTSTATUS
LhpDeleteDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PUNICODE_STRING SymbolLinkName
	);

NTSTATUS
LhpSetDriverDispatch(
	IN PDRIVER_OBJECT DriverObject
	);

NTSTATUS
LhpDispatchCreateClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS
LhpDispatchDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS
LhpDispatchShutdown(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

VOID
LhpCreateProcessNotify(
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create
	);

VOID
LhpLoadImageNotify(
    IN PUNICODE_STRING FullImageName,
    IN HANDLE  ProcessId,
    IN PIMAGE_INFO  ImageInfo
    );

#endif // __MAIN_H__