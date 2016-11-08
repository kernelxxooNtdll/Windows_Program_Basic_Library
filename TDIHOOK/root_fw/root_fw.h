#pragma once

#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// ����: CreateFlthlpDevice
// ˵��: ��������ǽ�豸(���豸�������ں�R3ͨ��)
// ���: DriverObject Windows�ṩ����������
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CreateFlthlpDevice( IN PDRIVER_OBJECT DriverObject );



//////////////////////////////////////////////////////////////////////////
// ����: DispatchCmdhlpIrp
// ˵��: ����R3�Է���ǽ����(���豸�������ں�R3ͨ��)
// ���: DeviceObject ����ǽͨѶ�豸����
//       Irp IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchCmdhlpIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: IRPUnFinish
// ˵��: �޷������IRP����(status == STATUS_UNSUCCESSFUL)
// ���: Irp �����IRPָ��
// ����: STATUS_UNSUCCESSFUL
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPUnFinish( IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: IRPFinish
// ˵��: ���IRP����(status == STATUS_SUCCESS)
// ���: Irp �����IRPָ��
// ����: STATUS_SUCCESS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPFinish( IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: TdiSendIrpSynchronous
// ˵��: �ȴ��²��豸�������(ͬ��)
// ���: NextDevice �²��豸����
//       Irp ��Ҫ������IRPָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiSendIrpSynchronous( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



// ������Ҫ��������ǲ����
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH OnDispatchRoutine;

__drv_dispatchType(IRP_MJ_INTERNAL_DEVICE_CONTROL)
DRIVER_DISPATCH OnDispatchInternal;


#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, OnDispatchRoutine)
    #pragma alloc_text( PAGE, OnDispatchInternal)
    #pragma alloc_text( PAGE, DriverUnload)
#endif // ALLOC_PRAGMA


