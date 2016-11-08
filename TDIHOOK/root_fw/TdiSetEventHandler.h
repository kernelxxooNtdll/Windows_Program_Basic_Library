#pragma once



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispSetEventHandler
// ˵��: TDI_SET_EVENT_HANDLER ��ǲ����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSetEventHandler( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );




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
    );



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
    );






//////////////////////////////////////////////////////////////////////////
// ����: ReceiveEventHandler (TDI_EVENT_RECEIVE and TDI_EVENT_RECEIVE_EXPEDITED)
// ˵��: ���ݽ����¼��ݲ�������  (��������)
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
    );



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
    );



//////////////////////////////////////////////////////////////////////////
// ����: ReceiveDatagramEventHandler (TDI_EVENT_RECEIVE_DATAGRAM)
// ˵��: ���������պ��� (����������)
// ���: ��� MSDN ClientEventReceiveDatagram
// ����: ��� MSDN ClientEventReceiveDatagram
// ����: ��� MSDN ClientEventReceiveDatagram
// ��ע: ��������,�򷵻� STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ReceiveDatagramEventHandler( IN PVOID  TdiEventContext, 
    IN LONG  SourceAddressLength, 
    IN PVOID  SourceAddress, 
    IN LONG  OptionsLength, 
    IN PVOID  Options, 
    IN ULONG  ReceiveDatagramFlags, 
    IN ULONG  BytesIndicated, 
    IN ULONG  BytesAvailable, 
    OUT ULONG  *BytesTaken, 
    IN PVOID  Tsdu, 
    OUT PIRP  *IoRequestPacket 
    );



//////////////////////////////////////////////////////////////////////////
// ����: ChainedReceiveDatagramEventHandler (TDI_EVENT_CHAINED_RECEIVE_DATAGRAM)
// ˵��: ��ʽ�������������� (����������)
// ���: ��� MSDN ClientEventChainedReceiveDatagram
// ����: ��� MSDN ClientEventChainedReceiveDatagram
// ����: ��� MSDN ClientEventChainedReceiveDatagram
// ��ע: ��������,�򷵻� STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ChainedReceiveDatagramEventHandler( IN PVOID  TdiEventContext, 
    IN LONG  SourceAddressLength, 
    IN PVOID  SourceAddress, 
    IN LONG  OptionsLength, 
    IN PVOID  Options, 
    IN ULONG  ReceiveDatagramFlags, 
    IN ULONG  ReceiveDatagramLength, 
    IN ULONG  StartingOffset, 
    IN PMDL  Tsdu, 
    IN PVOID  TsduDescriptor 
    );
