#pragma once

#include "stdafx.h"


//////////////////////////////////////////////////////////////////////////
// ����: TdiCreate
// ˵��: TDI IRP_MJ_CREATE ��ǲ��������
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       TdiDeviceExt ������TDI�豸�����ϵ���չ
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiCreate( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt 
    );



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
    );



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispConnect
// ˵��: TDI_CONNECT�ڲ���ǲ����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispConnect(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispDisConnect
// ˵��: TDI_DISCONNECT �Ͽ�����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisConnect( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispAssociateAddress
// ˵��: ���������Ĺ�����ַ�ļ�����
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispAssociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispDisassociateAddress
// ˵��: ȡ���������������ַ�ļ�����Ĺ���
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTAUTS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisassociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispSend
// ˵��: TDI_SEND �����������ӵ����ݰ�
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSend( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: TdiDispSendDatagram
// ˵��: TDI_SEND_DATAGRAM �������������ӵ����ݰ�
// ���: DeviceObject �����豸����
//       Irp �����IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSendDatagram( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



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
    );


//////////////////////////////////////////////////////////////////////////
// ����: DispatchFilterIrp
// ˵��: ����TDI ��ͨ���� IRP_MJ_DEVICE_CONTROL
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchFilterIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// ����: DispatchTransportAddress
// ˵��: TDI ���ɴ����ַ���� IRP_MJ_CREATE TransportAddress
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       TdiDeviceExt ������TDI�豸�����ϵ���չָ��
//       IrpSP ��ǰIRPջ����IO_STACK_LOCATIONָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchTransportAddress( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt, 
    IN PIO_STACK_LOCATION IrpSP 
    );



//////////////////////////////////////////////////////////////////////////
// ����: DispatchConnectionContext
// ˵��: TDI ������������������ IRP_MJ_CREATE ConnectionContext
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       TdiDeviceExt ������TDI�豸�����ϵ���չָ��
//       IrpSP ��ǰIRPջ����IO_STACK_LOCATIONָ��
//       ConnectionContext ����������ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchConnectionContext( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt, 
    IN PIO_STACK_LOCATION IrpSP, 
    IN CONNECTION_CONTEXT ConnectionContext 
    );



//////////////////////////////////////////////////////////////////////////
// ����: EventCompleteRoutine
// ˵��: ֪ͨ�¼����������
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context ���õ���������������(TDI_DEVICE_EXTENSIONָ��)
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
IO_COMPLETION_ROUTINE 
EventNotifyCompleteRoutine;



//////////////////////////////////////////////////////////////////////////
// ����: FindEaData
// ˵��: ��֤FILE_FULL_EA_INFORMATION��������
// ���: FileEaInfo FILE_FULL_EA_INFORMATIONָ��
//       EaName �������Һ�ƥ���File EA����
//       NameLength EaName�ĳ���(ƥ����)
// ����: PFILE_FULL_EA_INFORMATION ���ҵ���ָ��File EA��Ϣָ��
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PFILE_FULL_EA_INFORMATION FindEaData( IN PFILE_FULL_EA_INFORMATION FileEaInfo, 
    IN PCHAR EaName, 
    IN ULONG NameLength 
    );



//////////////////////////////////////////////////////////////////////////
// ����: TdiQueryAddressInfo
// ˵��: ���²��豸�����ѯ������FileObject��ַ�Ͷ˿���Ϣ
// ���: FileObject �͵�ַ��Ϣ�������ļ�����
//       AddressLength ��ַ����������
//       pIPAddress IP��ַ��Ϣ
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiQueryAddressInfo( IN PFILE_OBJECT FileObject, 
    IN ULONG AddressLength, 
    OUT PUCHAR pIPAddress 
    );




