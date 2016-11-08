#include "stdafx.h"
#include "TdiSetEventHandler.h"


// �¼������ṹ�嶨��
TDI_EVENT_DISPATCH_TABLE TdiEventDispatchTable[] = {
    { TDI_EVENT_CONNECT, ConnectEventHandler }, 
    { TDI_EVENT_DISCONNECT, DisconnectEventHandler }, 
    { TDI_EVENT_ERROR, NULL }, 
    { TDI_EVENT_RECEIVE, ReceiveEventHandler }, 
    { TDI_EVENT_RECEIVE_DATAGRAM, ReceiveDatagramEventHandler }, 
    { TDI_EVENT_RECEIVE_EXPEDITED, ReceiveEventHandler }, 
    { TDI_EVENT_SEND_POSSIBLE, NULL }, 
    { TDI_EVENT_CHAINED_RECEIVE, ChainedReceiveEventHandler }, 
    { TDI_EVENT_CHAINED_RECEIVE_DATAGRAM, ChainedReceiveDatagramEventHandler }, 
    { TDI_EVENT_CHAINED_RECEIVE_EXPEDITED, ChainedReceiveEventHandler }, 
    { TDI_EVENT_ERROR_EX, NULL }
};



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispSetEventHandler
// ˵��: TDI_SET_EVENT_HANDLER ��ǲ����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSetEventHandler( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    ULONG nIndex;
    PIO_STACK_LOCATION IrpSP;
    PADDRESS_ITEM AddressItem;
    PTDI_REQUEST_KERNEL_SET_EVENT param;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IrpSP = IoGetCurrentIrpStackLocation( Irp );
    param = (PTDI_REQUEST_KERNEL_SET_EVENT)(&IrpSP->Parameters);

    // ���洦������Ϣ
    AddressItem = FindAddressItemByFileObj( IrpSP->FileObject );

    // û�в�������,���߳������¼�����ķ�Χ����������
    if ( param->EventHandler == NULL || 
         param->EventType < 0 || 
         param->EventType >= MAX_EVENT || 
         AddressItem == NULL )
    {
        IoSkipCurrentIrpStackLocation( Irp );
        return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    for ( nIndex = 0; nIndex < _countof(TdiEventDispatchTable); ++nIndex )
    {
        if ( TdiEventDispatchTable[nIndex].EventType != param->EventType )
            continue;

        if ( TdiEventDispatchTable[nIndex].EventHandler != NULL )
        {
            PTDI_EVENT_CONTEXT pEventContext = &AddressItem->EventContext[nIndex];

            // ����ɵĻص�������Ϣ
            pEventContext->FileObject = IrpSP->FileObject;
            pEventContext->OldHandler = param->EventHandler;
            pEventContext->OldContext = param->EventContext;

            // �����µĻص��������Ͳ���
            param->EventHandler = TdiEventDispatchTable[nIndex].EventHandler;
            param->EventContext = pEventContext;
        }

        break;
    }

    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}




