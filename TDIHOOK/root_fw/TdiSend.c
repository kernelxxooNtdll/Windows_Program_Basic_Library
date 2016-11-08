#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSendDatagram
// 说明: TDI_SEND_DATAGRAM 发送面向无连接的数据包
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 拦截就返回 STATUS_INVALID_ADDRESS
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

    // 提交给下层设备对象发送
    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSend
// 说明: TDI_SEND 发送数据包
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 拦截就返回 STATUS_FILE_CLOSED
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

        // 打印发送数据信息
        if ( ConnectItem != NULL )
        {
            DbgMsg(__FILE__, __LINE__, "Send Data: %08X, SendLength: %d\n", 
                pSendData, param->SendLength);
        }
    }

    // 提交给下层设备对象发送处理
    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}
