#include "stdafx.h"
#include "root_fw.h"



//////////////////////////////////////////////////////////////////////////
// ����: DisConnectCompleteRoutine
// ˵��: �Ͽ���Զ�̼�������ӵ��������
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context PCONNECTION_ITEMָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DisConnectCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PADDRESS_ITEM AddressItem;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCONNECTION_ITEM ConnectItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
         TdiDeviceExt != NULL && 
         ConnectItem != NULL && 
         TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // �޸�Ϊ��״̬
        ConnectItem->ConnectType = UNKNOWN_STATE;

        // ���Զ�̼������ַ��Ϣ
        RtlZeroMemory( ConnectItem->RemoteAddress, sizeof(ConnectItem->RemoteAddress) );
    }

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// ����: ConnectCompletionRoutine
// ˵��: ����Զ�̶˿��������
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context PCONNECTION_ITEMָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ConnectCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PTDI_REQUEST_KERNEL_CONNECT param;
    PIO_STACK_LOCATION IrpSP;
    PTA_ADDRESS RemoteAddr;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCONNECTION_ITEM ConnectItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
         ConnectItem != NULL && 
         ConnectItem->AddressItemPtr != NULL && 
         TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // ��ȡ������Ϣ
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        param = (PTDI_REQUEST_KERNEL_CONNECT)(&IrpSP->Parameters);

        if ( NULL == param )
        {
            DbgMsg(__FILE__, __LINE__, "ConnectCompleteRoutine: NULL == IrpSP->Parameters.\n");
            return STATUS_SUCCESS;
        }

        RemoteAddr = ((PTRANSPORT_ADDRESS)(param->RequestConnectionInformation->RemoteAddress))->Address;

        // ����ַ����
        if ( RemoteAddr->AddressLength > sizeof(ConnectItem->RemoteAddress) )
        {
            DbgMsg(__FILE__, __LINE__, "ConnectCompleteRoutine RemoteAddressLength OverSize: %d.\n", RemoteAddr->AddressLength );
            return STATUS_SUCCESS;
        }

        // ��վ״̬
        ConnectItem->ConnectType = OUT_SITE_STATE;

        // �������ӵ�Զ�̼������ַ��Ϣ
        RtlCopyBytes( ConnectItem->RemoteAddress, RemoteAddr, RemoteAddr->AddressLength );

        PrintIpAddressInfo( "ConnectCompleteRoutine Success RemoteIP:", RemoteAddr );
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispConnect
// ˵��: TDI_CONNECT�ڲ���ǲ����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: ���������,�ͷ��� STATUS_REMOTE_NOT_LISTENING
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispConnect(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status;
    PCONNECTION_ITEM ConnectItem;
    PTDI_DEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );
    PTDI_REQUEST_KERNEL_CONNECT param = (PTDI_REQUEST_KERNEL_CONNECT)(&IrpSP->Parameters);

    if ( Irp->CurrentLocation > 1 )
    {
        if ( param != NULL && param->RequestConnectionInformation->RemoteAddressLength > 0 )
        {
            // ��ӡ���ӵ�ַ
            PTA_ADDRESS TaRequestRemoteAddr = ((PTRANSPORT_ADDRESS)(param->RequestConnectionInformation->RemoteAddress))->Address;

            PrintIpAddressInfo( "TdiDispConnect RequestConnection RequestRemoteIP", TaRequestRemoteAddr );
        }

        // �����ļ������ҵ���Ӧ������������
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // �����������
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, ConnectCompleteRoutine, ConnectItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispDisConnect
// ˵��: TDI_DISCONNECT �Ͽ�����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisConnect( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSP;
    PCONNECTION_ITEM ConnectItem;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // �����������
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, DisConnectCompleteRoutine, ConnectItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}


;