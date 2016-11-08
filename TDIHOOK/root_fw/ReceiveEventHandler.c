#include "stdafx.h"
#include "TdiSetEventHandler.h"



//////////////////////////////////////////////////////////////////////////
// ����: ReceiveEventHandlerEx (TDI_EVENT_RECEIVE and TDI_EVENT_RECEIVE_EXPEDITED)
// ˵��: ���ݽ����¼��ݲ������� (��������)
// ���: ��� MSDN ClientEventReceive �� ClientEventReceiveExpedited
// ����: ��� MSDN ClientEventReceive �� ClientEventReceiveExpedited
// ����: ��� MSDN ClientEventReceive �� ClientEventReceiveExpedited
// ��ע: ���������,�򷵻� STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ReceiveEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN ULONG  ReceiveFlags, 
    IN ULONG  BytesIndicated, 
    IN ULONG  BytesAvailable, 
    OUT ULONG  *BytesTaken, 
    IN PVOID  Tsdu, 
    OUT PIRP  *IoRequestPacket 
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSP;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    // ����ԭ���Ĵ�����
    status = ((PTDI_IND_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
        ConnectionContext, ReceiveFlags, BytesIndicated, BytesAvailable, 
        BytesTaken, Tsdu, IoRequestPacket );

    DbgMsg(__FILE__, __LINE__, "ReceiveEventHandler Status: %08X, BytesTaken: %u, ReceiveFlags: 0x%X.\n", 
        status, *BytesTaken, ReceiveFlags );

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: ChainedReceiveEventHandler (TDI_EVENT_CHAINED_RECEIVE and TDI_EVENT_CHAINED_RECEIVE_EXPEDITED)
// ˵��: ��ʽ���ݽ��ղ������� (��������)
// ���: ��� MSDN ClientEventChainedReceive �� ClientEventChainedReceiveExpedited
// ����: ��� MSDN ClientEventChainedReceive �� ClientEventChainedReceiveExpedited
// ����: ��� MSDN ClientEventChainedReceive �� ClientEventChainedReceiveExpedited
// ��ע: ��������,�򷵻� STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ChainedReceiveEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN ULONG  ReceiveFlags, 
    IN ULONG  ReceiveLength, 
    IN ULONG  StartingOffset, 
    IN PMDL  Tsdu, 
    IN PVOID  TsduDescriptor 
    )
{
    NTSTATUS status;
    ULONG uDataLength;
    PUCHAR pData = NULL;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    status = ((PTDI_IND_CHAINED_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
        ConnectionContext, ReceiveFlags, ReceiveLength, StartingOffset, Tsdu, TsduDescriptor );

    // ��ȡ���յ���������Ϣ
    pData = (PUCHAR)MmGetSystemAddressForMdlSafe( Tsdu, NormalPagePriority );
    uDataLength = MmGetMdlByteCount( Tsdu );

    DbgMsg(__FILE__, __LINE__, "ChainedReceiveEventHandler Status: %08X, DataLength: %u, ReceiveFlags: 0x%X.\n",
        status, uDataLength, ReceiveFlags);

    return status;
}
