#include "stdafx.h"
#include "TdiSetEventHandler.h"


//////////////////////////////////////////////////////////////////////////
// ����: ConnectEventHandler
// ˵��: �����¼���������(��������Ϊ��������ʱ��,����Զ�̵�����)
// ���: ��� MSDN ConnectEventHandler
// ����: ��� MSDN ConnectEventHandler
// ����: ��� MSDN ConnectEventHandler
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ConnectEventHandler( IN PVOID  TdiEventContext, 
    IN LONG  RemoteAddressLength, 
    IN PVOID  RemoteAddress, 
    IN LONG  UserDataLength, 
    IN PVOID  UserData, 
    IN LONG  OptionsLength, 
    IN PVOID  Options, 
    OUT CONNECTION_CONTEXT  *ConnectionContext, 
    OUT PIRP  *AcceptIrp 
    )
{
    NTSTATUS status;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    // ����ԭ���Ĵ�����
    status = ((PTDI_IND_CONNECT)(pEventContext->OldHandler))( pEventContext->OldContext, 
        RemoteAddressLength, RemoteAddress, UserDataLength, UserData, OptionsLength, 
        Options, ConnectionContext, AcceptIrp );

    if ( MmIsAddressValid(*AcceptIrp) )
    {
        // �����������
        PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( *AcceptIrp );
        PCONNECTION_ITEM ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );
        PADDRESS_ITEM AddressItem = FindAddressItemByFileObj( pEventContext->FileObject );

        // ��վ״̬
        ConnectItem->ConnectType = IN_SITE_STATE;

        // ��ⳤ��
        if ( RemoteAddressLength > sizeof(ConnectItem->RemoteAddress) )
        {
            DbgMsg(__FILE__, __LINE__, "ConnectEventHandler RemoteAddressLength OverSize: %d.\n", RemoteAddressLength);
            return status;
        }

        // ������ַ����
        ConnectItem->AddressItemPtr = AddressItem;

        // ����Զ�̵�ַ
        RtlCopyMemory( ConnectItem->RemoteAddress, ((TRANSPORT_ADDRESS *)RemoteAddress)->Address, RemoteAddressLength );

        PrintIpAddressInfo( "ConnectEventHandler RemoteAddress", ((TRANSPORT_ADDRESS *)RemoteAddress)->Address );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: DisconnectEventHandler
// ˵��: �Ͽ������¼��������� (��������Ϊ��������ʱ��,�Ͽ�Զ������������)
// ���: ���MSDN ClientEventDisconnect
// ����: ���MSDN ClientEventDisconnect
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DisconnectEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN LONG  DisconnectDataLength, 
    IN PVOID  DisconnectData, 
    IN LONG  DisconnectInformationLength, 
    IN PVOID  DisconnectInformation, 
    IN ULONG  DisconnectFlags 
    )
{
    NTSTATUS status;
    PCONNECTION_ITEM ConnectItem;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    status = ((PTDI_IND_DISCONNECT)(pEventContext->OldHandler))( pEventContext->OldContext, 
        ConnectionContext, DisconnectDataLength, DisconnectData, DisconnectInformationLength, 
        DisconnectInformation,  DisconnectFlags );

    // ���������������ҵ����Ӷ���ṹ��ָ��
    ConnectItem = FindConnectItemByConnectContext( ConnectionContext );

    if ( ConnectItem != NULL )
    {
        // ����Ϊ��״̬
        ConnectItem->ConnectType = UNKNOWN_STATE;

        // ȡ�����ַ����İ�
        ConnectItem->AddressItemPtr = NULL;

        // ���Զ�̵�ַ
        RtlZeroMemory( ConnectItem->RemoteAddress, sizeof(ConnectItem->RemoteAddress) );
    }

    DbgMsg(__FILE__, __LINE__, "DisconnectEventHandler Result status: %08X.\n", status);
    return status;
}

