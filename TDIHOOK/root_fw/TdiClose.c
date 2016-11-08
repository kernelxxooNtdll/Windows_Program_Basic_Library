#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// ����: CleanupConnectionFileRoutine
// ˵��: TDI_CLEANUP ɾ���ļ����������������ļ��������
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context ���õ���������������(TDI_DEVICE_EXTENSIONָ��)
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupConnectionFileRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    NTSTATUS status = Irp->IoStatus.Status;
    PCONNECTION_ITEM pConnectionItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    // �ļ�����������,��ô�͵ô�����������������ɾ��
    if ( NT_SUCCESS(status) && NULL != pConnectionItem )
    {
        DeleteConnectionItemFromList( pConnectionItem );
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// ����: CleanupAddressFileRoutine
// ˵��: TDI_CLEANUP ɾ�������ַ�ļ��������
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context ���õ���������������(TDI_DEVICE_EXTENSIONָ��)
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupAddressFileRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    NTSTATUS status = Irp->IoStatus.Status;
    PADDRESS_ITEM pAddressItem = (PADDRESS_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    // �ļ�����������,��ô�͵ôӵ�ַ�����еĶ���
    if ( NT_SUCCESS(status) && NULL != pAddressItem )
    {
        DeleteAddressItemFromList( pAddressItem );
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// ����: TdiClose
// ˵��: TDI IRP_MJ_CLOSE ��ǲ��������
// ���: DeviceObject �����豸����
//       Irp IRP����ָ��
//       TdiDeviceExt �����豸�������չ
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiClose( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt
    )
{
    PAGED_CODE()

    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}



//////////////////////////////////////////////////////////////////////////
// ����: CleanupTransportAddressFile
// ˵��: �������ַ�ļ�
// ���: DeviceObject �����豸����
//       Irp IRP����ָ��
//       TdiDeviceExt �����豸������չָ��
//       IrpSP �����豸����ջָ��
// ����: 
// ����: 
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupTransportAddressFile( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt, 
    IN PIO_STACK_LOCATION IrpSP 
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PADDRESS_ITEM pAddressItem = FindAddressItemByFileObj( IrpSP->FileObject );

    if ( Irp->CurrentLocation > 1 && pAddressItem != NULL )
    {
        // ׼���ص�����
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, CleanupAddressFileRoutine, pAddressItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: CleanupConnectionFile
// ˵��: ���������������ļ�����
// ���: DeviceObject �����豸����
//       Irp IRP����ָ��
//       TdiDeviceExt �����豸������չָ��
//       IrpSP �����豸����ջָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupConnectionFile( IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt,  
    IN PIO_STACK_LOCATION IrpSP 
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PCONNECTION_ITEM pConnectionItem = FindConnectItemByFileObj( IrpSP->FileObject );

    // ������
    if ( Irp->CurrentLocation > 1 && pConnectionItem != NULL )
    {
        // ׼���ص�����
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, CleanupConnectionFileRoutine, pConnectionItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: TdiCleanup
// ˵��: TDI IRP_MJ_CLEANUP ��ǲ��������
// ���: DeviceObject �����豸����
//       Irp IRP����ָ��
//       TdiDeviceExt �����豸�������չ
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiCleanup( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE()

    // ����豸ջ�����
    if ( Irp->CurrentLocation > 0 && IrpSP->FileObject->FsContext2 != NULL )
    {
        switch ( (ULONG)IrpSP->FileObject->FsContext2 )
        {
            // �رյ���һ�����͵�ַ�ļ�
        case TDI_TRANSPORT_ADDRESS_FILE:
            status = CleanupTransportAddressFile( DeviceObject, Irp, TdiDeviceExt, IrpSP );
            break;

            // �رյ���һ������������
        case TDI_CONNECTION_FILE:
            status = CleanupConnectionFile( DeviceObject, Irp, TdiDeviceExt, IrpSP );
            break;

        default:    // TDI_CONTROL_CHANNEL_FILE
            IoSkipCurrentIrpStackLocation( Irp );
            status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
            break;
        }
    }
    else
    {
        // ���²��豸ջ����
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



