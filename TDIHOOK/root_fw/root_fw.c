#include "root_fw.h"
#include "TdiFltDisp.h"
#include "TdiSetEventHandler.h"


// ����ǽ��R3�����豸����
PDEVICE_OBJECT g_cmdhlpDevice = NULL;


// TDI��ǲ������
TDI_REQUEST_TABLE TdiRequestTable[] = {
    { TDI_ASSOCIATE_ADDRESS, TdiDispAssociateAddress },             // (0x01)
    { TDI_DISASSOCIATE_ADDRESS, TdiDispDisassociateAddress },       // (0x02)
    { TDI_CONNECT, TdiDispConnect },        // (0x03)
    { TDI_LISTEN, NULL },                   // (0x04)
    { TDI_ACCEPT, NULL },                   // (0x05)
    { TDI_DISCONNECT, TdiDispDisConnect },  // (0x06)
    { TDI_SEND, TdiDispSend },              // (0x07)
    { TDI_RECEIVE, NULL },                  // (0x08)
    { TDI_SEND_DATAGRAM, TdiDispSendDatagram },                     // (0x09)
    { TDI_RECEIVE_DATAGRAM, NULL },         // (0x0A)
    { TDI_SET_EVENT_HANDLER, TdiDispSetEventHandler },              // (0x0B)
    { TDI_QUERY_INFORMATION, NULL },        // (0x0C)
    { TDI_SET_INFORMATION, NULL },          // (0x0D)
    { TDI_ACTION, NULL },                   // (0x0E)

    { TDI_DIRECT_SEND, NULL },              // (0x27)
    { TDI_DIRECT_SEND_DATAGRAM, NULL },     // (0x29)
    { TDI_DIRECT_ACCEPT, NULL }             // (0x2A)
};



//////////////////////////////////////////////////////////////////////////
// ����: DriverEntry
// ˵��: ������ڵ�
// ���: ��׼����
// ����: ��׼����
// ����: ��׼����ֵ
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT DriverObject, 
    IN PUNICODE_STRING RegistryPath 
    )
{
    ULONG nIndex;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

#ifndef DBG
    DriverObject->DriverUnload = NULL;
#else
    DriverObject->DriverUnload = DriverUnload;
#endif

    // ʹ��Ĭ�ϵ���ǲ����
    for ( nIndex = 0; nIndex < IRP_MJ_MAXIMUM_FUNCTION; ++nIndex )
    {
        DriverObject->MajorFunction[nIndex] = OnDispatchRoutine;
    }

    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = OnDispatchInternal;

    // �����豸����
    status = CreateFlthlpDevice( DriverObject );
    if ( !NT_SUCCESS(status) )
    {
        return status;
    }

    // ����TDI�������IPv4
    status = AattachIPv4Filters( DriverObject );
    if ( !NT_SUCCESS(status) )
    {
        DriverUnload( DriverObject );
        return status;
    }

    // ��ʼ�������������
    InitializeListManager();

    DbgMsg(__FILE__, __LINE__, "root_fw DriverEntry success.\n");
    return status;
}


//////////////////////////////////////////////////////////////////////////
// ����: OnUnload
// ˵��: �豸ж������
// ��ע: ֪ͨTDIBase����ʼ��
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID 
DriverUnload( IN PDRIVER_OBJECT DriverObject )
{
    DbgMsg(__FILE__, __LINE__, "root_fw DriverUnload.\n");

    // ж��IPv4Э�鼯�Ĺ��ز���
    DetachIPv4Filters();

    // ɾ�������������
    UninitializeListManager();

    if ( NULL != g_cmdhlpDevice )
    {
        // ɾ������ǽ�豸
        UNICODE_STRING uniSymblicLinkName;
        RtlInitUnicodeString( &uniSymblicLinkName, SYMBLIC_LINK_NAME );

        IoDeleteSymbolicLink( &uniSymblicLinkName );

        IoDeleteDevice( g_cmdhlpDevice );
    }
}


//////////////////////////////////////////////////////////////////////////
// ����: Dispatch ����IRP_MJ_INTERNAL_DEVICE_CONTROL������IOCTL����
// ˵��: ������ǲ����
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS 
OnDispatchRoutine( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // ����ǽIRP����
    if ( DeviceObject == g_cmdhlpDevice )
    {
        status = DispatchCmdhlpIrp( DeviceObject, Irp );
    }
    else if ( DeviceObject == g_tcpFltDevice || 
              DeviceObject == g_udpFltDevice || 
              DeviceObject == g_rawIpFltDevice || 
              DeviceObject == g_ipFltDevice )
    {
        // IPv4 �豸������
        status = DispatchFilterIrp( DeviceObject, Irp );
    }
    else
    {
        DbgMsg(__FILE__, __LINE__, "Warning: OnDeviceControl unknown DeviceObject: %08X.\n", DeviceObject);
        status = IRPUnFinish( Irp );
    }

    return status;
}


//////////////////////////////////////////////////////////////////////////
// ����: DispatchInternal
// ˵��: IRP_MJ_INTERNAL_DEVICE_CONTROL��ǲ����
// ��ע: ������ֻ�й�����TDI�豸�ϵĲ��з��͸�IRP
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS 
OnDispatchInternal( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    int nIndex;
    PTDI_DEVICE_EXTENSION DevExt;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpSP;

    IrpSP = IoGetCurrentIrpStackLocation( Irp );

    if ( DeviceObject == g_ipFltDevice || 
         DeviceObject == g_tcpFltDevice || 
         DeviceObject == g_udpFltDevice || 
         DeviceObject == g_rawIpFltDevice )
    {
        DevExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

        // ������ǲ���������������ָ���Ļص�����
        for ( nIndex = 0; nIndex < _countof(TdiRequestTable); ++nIndex )
        {
            if ( TdiRequestTable[nIndex].RequestCode == IrpSP->MinorFunction )
            {
                PDRIVER_DISPATCH pDispatchFunc = TdiRequestTable[nIndex].DispatchFunc;

                // ����ǲ��������
                if ( NULL != pDispatchFunc )
                {
                    // ���ö�Ӧ����ǲ����
                    status = pDispatchFunc( DeviceObject, Irp );
                }
                else
                {
                    // ��ȡʵ�ʲ������豸����
                    IoSkipCurrentIrpStackLocation( Irp );
                    status = IoCallDriver( DevExt->NextStackDevice, Irp );
                }

                break;
            }
        }
    }
    else
    {
        DbgMsg(__FILE__, __LINE__, "Warning: OnInternalDeviceCotrol unknown DeviceObject: %08X\n", DeviceObject);
        status = IRPUnFinish( Irp );        // ������ɸ�����
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: CreateFlthlpDevice
// ˵��: ��������ǽ�豸(���豸�������ں�R3ͨ��)
// ���: DriverObject Windows�ṩ����������
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CreateFlthlpDevice( IN PDRIVER_OBJECT DriverObject )
{
    UNICODE_STRING uniDeviceName;
    UNICODE_STRING uniSymblicLinkName;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT DeviceObject;

    RtlInitUnicodeString( &uniDeviceName, DEVICE_NAME );
    status = IoCreateDevice( DriverObject, 
        0, 
        &uniDeviceName, 
        FILE_DEVICE_UNKNOWN, 
        0, 
        TRUE,       // ��ռ����
        &DeviceObject 
        );

    if ( !NT_SUCCESS(status) )
        return status;

    // �����﷨������
    RtlInitUnicodeString( &uniSymblicLinkName, SYMBLIC_LINK_NAME );
    status = IoCreateSymbolicLink( &uniSymblicLinkName, &uniDeviceName );
    if ( !NT_SUCCESS(status) )
    {
        IoDeleteDevice( DeviceObject );
    }
    else 
    {
        g_cmdhlpDevice = DeviceObject;
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: DispatchCmdhlpIrp
// ˵��: ����R3�Է���ǽ����(���豸�������ں�R3ͨ��)
// ���: DeviceObject ����ǽͨѶ�豸����
//       Irp IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchCmdhlpIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );

    switch ( IrpSP->MajorFunction )
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        status = IRPFinish( Irp );
        break;

        // IOCTL
    case IRP_MJ_DEVICE_CONTROL:
        break;

    default:
        status = IRPUnFinish( Irp );
        break;
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: IRPUnFinish
// ˵��: �޷������IRP����(status == STATUS_UNSUCCESSFUL)
// ���: Irp �����IRPָ��
// ����: STATUS_UNSUCCESSFUL
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPUnFinish( IN PIRP Irp )
{
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return STATUS_UNSUCCESSFUL;
}



//////////////////////////////////////////////////////////////////////////
// ����: IRPFinish
// ˵��: ���IRP����(status == STATUS_SUCCESS)
// ���: Irp �����IRPָ��
// ����: STATUS_SUCCESS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPFinish( IN PIRP Irp )
{
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// ����: TdiSendIrpSynchronous
// ˵��: �ȴ��²��豸�������(ͬ��)
// ���: NextDevice �²��豸����
//       Irp ��Ҫ������IRPָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiSendIrpSynchronous( IN PDEVICE_OBJECT NextDevice, IN PIRP Irp )
{
    KEVENT kEvent;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    // �����¼�֪ͨ�������
    KeInitializeEvent( &kEvent, NotificationEvent, FALSE );
    Irp->UserEvent = &kEvent;
    Irp->UserIosb = &iosb;

    status = IoCallDriver( NextDevice, Irp );

    // �ȴ��¼�������ź�
    if ( status == STATUS_PENDING )
    {
        KeWaitForSingleObject( &kEvent, Executive, KernelMode, FALSE, NULL );
        status = iosb.Status;
    }

    return status;
}



