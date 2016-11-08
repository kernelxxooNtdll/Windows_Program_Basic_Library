#include "stdafx.h"
#include "TdiSetEventHandler.h"



//////////////////////////////////////////////////////////////////////////
// 名称: ReceiveEventHandlerEx (TDI_EVENT_RECEIVE and TDI_EVENT_RECEIVE_EXPEDITED)
// 说明: 数据接收事件据操作函数 (面向连接)
// 入参: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 出参: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 返回: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 备注: 如果不接收,则返回 STATUS_DATA_NOT_ACCEPTED
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

    // 调用原来的处理函数
    status = ((PTDI_IND_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
        ConnectionContext, ReceiveFlags, BytesIndicated, BytesAvailable, 
        BytesTaken, Tsdu, IoRequestPacket );

    DbgMsg(__FILE__, __LINE__, "ReceiveEventHandler Status: %08X, BytesTaken: %u, ReceiveFlags: 0x%X.\n", 
        status, *BytesTaken, ReceiveFlags );

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: ChainedReceiveEventHandler (TDI_EVENT_CHAINED_RECEIVE and TDI_EVENT_CHAINED_RECEIVE_EXPEDITED)
// 说明: 链式数据接收操作函数 (面向连接)
// 入参: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 出参: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 返回: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 备注: 拦截数据,则返回 STATUS_DATA_NOT_ACCEPTED
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

    // 获取接收到的数据信息
    pData = (PUCHAR)MmGetSystemAddressForMdlSafe( Tsdu, NormalPagePriority );
    uDataLength = MmGetMdlByteCount( Tsdu );

    DbgMsg(__FILE__, __LINE__, "ChainedReceiveEventHandler Status: %08X, DataLength: %u, ReceiveFlags: 0x%X.\n",
        status, uDataLength, ReceiveFlags);

    return status;
}
