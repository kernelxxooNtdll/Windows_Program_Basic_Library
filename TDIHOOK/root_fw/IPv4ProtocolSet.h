#pragma once


//////////////////////////////////////////////////////////////////////////
// IPv4 �豸����
// ����IPv4�豸����
// ICMP
#define DEVICE_IPV4_NAME            L"\\Device\\Ip"
// Stream
#define DEVICE_TCP4_NAME            L"\\Device\\Tcp"
// Datagram
#define DEVICE_UDP4_NAME            L"\\Device\\Udp"
// Raw
#define DEVICE_RAWIP4_NAME          L"\\Device\\RawIp"
// IGMP
#define DEVICE_IPMULTICAST4_NAME    L"\\Device\\IPMULTICAST"


extern PDEVICE_OBJECT g_tcpFltDevice;
extern PDEVICE_OBJECT g_udpFltDevice;
extern PDEVICE_OBJECT g_rawIpFltDevice;
extern PDEVICE_OBJECT g_ipFltDevice;
extern PDEVICE_OBJECT g_igmpFltDevice;



//////////////////////////////////////////////////////////////////////////
// ����: AttachFilter
// ˵��: �����豸�������������豸����
// ���: DriverObject ��������
//       fltDeviceObj �����豸����
//       DeviceName Ҫ�ҽӵ�Ŀ���豸��
// ����: ״ֵ̬
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS AttachFilter( 
    IN PDRIVER_OBJECT DriverObject, 
    OUT PDEVICE_OBJECT* fltDeviceObject, 
    IN PWCHAR pszDeviceName 
    );


//////////////////////////////////////////////////////////////////////////
// ����: DetachFilter
// ˵��: ж�ز�ɾ���豸����
// ���: DeviceObject Ҫɾ�����豸����
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachFilter( IN PDEVICE_OBJECT DeviceObject );


// ����TDI�������IPv4
NTSTATUS AattachIPv4Filters( IN PDRIVER_OBJECT DriverObject );


//////////////////////////////////////////////////////////////////////////
// ����: DetachIPv4Filters
// ˵��: ж��IPv4�豸
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachIPv4Filters();



//////////////////////////////////////////////////////////////////////////
// ����: GetProtocolType
// ˵��: ���ݵ�ǰ���豸�����ȡЭ������
// ���: DeviceObject ��ǰ������豸����
//       FileObject IRPջ�е��ļ�����,������RawIpЭ�����ͽ��л�ȡ����
// ����: ProtocolType Э������
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS GetProtocolType( IN PDEVICE_OBJECT DeviceObject, 
    IN PFILE_OBJECT FileObject, 
    OUT PULONG ProtocolType 
    );



//////////////////////////////////////////////////////////////////////////
// ����: GetRawProtocolType
// ˵��: RawIp��Э�����ͻ�ȡ����
// ���: FileObject ��Ӧ���ļ�����ָ��
// ����: Э������
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
UCHAR GetRawProtocolType( IN PFILE_OBJECT FileObject );



//////////////////////////////////////////////////////////////////////////
// ����: TiGetProtocolNumber
// ˵��: �����ļ�������Э���
// ���: FileName �ļ�������
// ����: Protocol ����Э���
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TiGetProtocolNumber( IN PUNICODE_STRING FileName, OUT PULONG Protocol );



