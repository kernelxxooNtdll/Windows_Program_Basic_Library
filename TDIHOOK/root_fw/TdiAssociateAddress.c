#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// ����: AssociateAddressCompleteRoutine
// ˵��: ������ַ�����������(����������ַ��������������)
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context ��ַ��������ָ��
// ����: STATUS_SUCCESS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS AssociateAddressCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PCONNECTION_ITEM ConnectItem;
    PIO_STACK_LOCATION IrpSP;
    PADDRESS_ITEM AddressItem = (PADDRESS_ITEM)Context;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    // �ļ�����������,��ô�͵ô�����������������ɾ��
    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
        NULL != TdiDeviceExt && 
        NULL != AddressItem && 
        TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // ��ȡ���������Ľṹ
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // ���������ַ������������
        if ( ConnectItem != NULL && AddressItem != NULL )
        {
            ConnectItem->AddressItemPtr = AddressItem;
        }
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// ����: DisAssociateAddressCompleteRoutine
// ˵��: ��ַ�ļ�������������ȡ����������
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context ����������ָ��
// ����: STATUS_SUCCESS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DisAssociateAddressCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCONNECTION_ITEM ConnectionItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
         ConnectionItem != NULL && 
         TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // ȡ����ַ�����������ĵĹ���
        if ( ConnectionItem != NULL )
        {
            ConnectionItem->AddressItemPtr = NULL;
        }
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispAssociateAddress
// ˵��: ���������Ĺ�����ַ�ļ�����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispAssociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION IrpSP;
    PFILE_OBJECT FileObject;
    NTSTATUS status;
    PTDI_REQUEST_KERNEL_ASSOCIATE AssociateParam;
    PADDRESS_ITEM AddressItem = NULL;
    PTDI_DEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        AssociateParam = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSP->Parameters;

        status = ObReferenceObjectByHandle(AssociateParam->AddressHandle, 
            0, 
            *IoFileObjectType, 
            KernelMode, 
            (PVOID*)&FileObject, 
            NULL 
            );

        if ( NT_SUCCESS(status) )
        {
            // �ӵ�ַ�����в���
            AddressItem = FindAddressItemByFileObj( FileObject );
            ObDereferenceObject( FileObject );
        }

        // �����������
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, AssociateAddressCompleteRoutine, AddressItem, TRUE, TRUE, TRUE );
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
// ����: TdiDispDisassociateAddress
// ˵��: ȡ���������������ַ�ļ�����Ĺ���
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTAUTS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisassociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status;
    PCONNECTION_ITEM ConnectItem;
    PIO_STACK_LOCATION IrpSP;
    PTDI_DEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // �����������ȡ����ַ�����������ĵĹ���
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, DisAssociateAddressCompleteRoutine, ConnectItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }

    return status;
}


