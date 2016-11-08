#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispSendDatagram
// ˵��: TDI_SEND_DATAGRAM �������������ӵ����ݰ�
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: ���ؾͷ��� STATUS_INVALID_ADDRESS
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSendDatagram( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION IrpSP;
    PTDI_REQUEST_KERNEL_SENDDG SenddgRequest;
    PCONNECTION_ITEM ConnectItem;
    PUCHAR pSendData;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        SenddgRequest = (PTDI_REQUEST_KERNEL_SENDDG)(&IrpSP->Parameters);
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );
        pSendData = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

        if ( ConnectItem != NULL )
        {
            DbgMsg(__FILE__, __LINE__, "SendDatagram Data: %08X, SendLength: %d.\n", 
                pSendData, SenddgRequest->SendLength );
        }
    }

    // �ύ���²��豸������
    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispSend
// ˵��: TDI_SEND �������ݰ�
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: ���ؾͷ��� STATUS_FILE_CLOSED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSend( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION IrpSP;
    PTDI_REQUEST_KERNEL_SEND param;
    PUCHAR pSendData;
    PCONNECTION_ITEM ConnectItem;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        param = (PTDI_REQUEST_KERNEL_SEND)(&IrpSP->Parameters);
        pSendData = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // ��ӡ����������Ϣ
        if ( ConnectItem != NULL )
        {
            DbgMsg(__FILE__, __LINE__, "Send Data: %08X, SendLength: %d\n", 
                pSendData, param->SendLength);
        }
    }

    // �ύ���²��豸�����ʹ���
    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}
