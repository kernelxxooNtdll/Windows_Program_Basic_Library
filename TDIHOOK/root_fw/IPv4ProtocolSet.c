#include "stdafx.h"
#include "IPv4ProtocolSet.h"


//////////////////////////////////////////////////////////////////////////
// IPv4�����豸����
PDEVICE_OBJECT g_tcpFltDevice = NULL;
PDEVICE_OBJECT g_udpFltDevice = NULL;
PDEVICE_OBJECT g_rawIpFltDevice = NULL;
PDEVICE_OBJECT g_ipFltDevice = NULL;
PDEVICE_OBJECT g_igmpFltDevice = NULL;


//////////////////////////////////////////////////////////////////////////
// ����: AttachIPv4Filter
// ˵��: �ҽ�IPv4�豸
// ���: DriverObject ��������ָ��
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS AattachIPv4Filters( IN PDRIVER_OBJECT DriverObject )
{
    // TCP
    AttachFilter( DriverObject, &g_tcpFltDevice, DEVICE_TCP4_NAME );

    // UDP
    AttachFilter( DriverObject, &g_udpFltDevice, DEVICE_UDP4_NAME );

    // RawIp
    AttachFilter( DriverObject, &g_rawIpFltDevice, DEVICE_RAWIP4_NAME );

    // IP
    AttachFilter( DriverObject, &g_ipFltDevice, DEVICE_IPV4_NAME );

    // IGMP
    AttachFilter( DriverObject, &g_igmpFltDevice, DEVICE_IPMULTICAST4_NAME );

    if ( g_tcpFltDevice && g_udpFltDevice && 
         g_rawIpFltDevice && g_ipFltDevice && 
         g_igmpFltDevice )
    {
        return STATUS_SUCCESS;
    }

    DbgMsg(__FILE__, __LINE__, "Warning: Attach IPv4 failed.\n");
    return STATUS_UNSUCCESSFUL;
}


//////////////////////////////////////////////////////////////////////////
// ����: DetachIPv4Filters
// ˵��: ж��IPv4�豸
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachIPv4Filters()
{
    DetachFilter( g_ipFltDevice );
    DetachFilter( g_tcpFltDevice );
    DetachFilter( g_udpFltDevice );
    DetachFilter( g_rawIpFltDevice );
    DetachFilter( g_igmpFltDevice );
}


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
    )
{
    UNICODE_STRING uniDeviceName;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pNextStackDevice;
    PTDI_DEVICE_EXTENSION DeviceExt;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // ��ȡ�豸����
    RtlInitUnicodeString( &uniDeviceName, pszDeviceName );
    status = IoGetDeviceObjectPointer( &uniDeviceName, 
        FILE_ATTRIBUTE_NORMAL, 
        &pFileObject, 
        &pNextStackDevice 
        );

    if ( !NT_SUCCESS(status) )
    {
        DbgMsg(__FILE__, __LINE__, "IoGetDeviceObjectPointer failed DeviceName: %ws\n", pszDeviceName);
        return status;
    }

    // ���������豸����
    status = IoCreateDevice( DriverObject, 
        sizeof(TDI_DEVICE_EXTENSION), 
        NULL, 
        pNextStackDevice->DeviceType, 
        pNextStackDevice->Characteristics, 
        FALSE,          // �Ƕ�ռ�豸
        fltDeviceObject 
        );

    if ( !NT_SUCCESS(status) )
    {
        DbgMsg(__FILE__, __LINE__, "IoCreateDevice failed DeviceName: %ws, Status: %08X\n", 
            pszDeviceName, status);

        ObDereferenceObject( pFileObject );
        return status;
    }

    // ʹ���豸ֱ�Ӷ�ȡ��־λ
    (*fltDeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;
    (*fltDeviceObject)->Flags |= (*fltDeviceObject)->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO);

    // �����²��豸����
    DeviceExt = (PTDI_DEVICE_EXTENSION)(*fltDeviceObject)->DeviceExtension;
    RtlZeroMemory( DeviceExt, sizeof(*DeviceExt) );

    // ���豸�ҽӵ��豸ջ,������ײ��豸����
    DeviceExt->NextStackDevice = IoAttachDeviceToDeviceStack( *fltDeviceObject, pNextStackDevice );
    DeviceExt->FileObject = pFileObject;
    DeviceExt->FltDeviceObject = *fltDeviceObject;

    DbgMsg(__FILE__, __LINE__, "Attach success: %ws\n", pszDeviceName);
    return status;
}


//////////////////////////////////////////////////////////////////////////
// ����: DetachFilter
// ˵��: ж�ز�ɾ���豸����
// ���: DeviceObject Ҫɾ�����豸����
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachFilter( IN PDEVICE_OBJECT DeviceObject )
{
    PTDI_DEVICE_EXTENSION DeviceExt;

    if ( NULL == DeviceObject )
        return;

    DeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    if ( NULL == DeviceExt )
    {
        DbgMsg(__FILE__, __LINE__, "Warring: DetachFilter DeviceExtension == NULL.\n");
        return;
    }

    // �����²��ļ����������
    if ( NULL != DeviceExt->FileObject )
    {
        ObReferenceObject( DeviceExt->FileObject );
    }

    // ժ���ҽ��豸
    if ( NULL != DeviceExt->NextStackDevice )
    {
        IoDetachDevice( DeviceExt->NextStackDevice );
    }

    // ɾ���豸
    if ( NULL != DeviceExt->FltDeviceObject )
    {
        IoDeleteDevice( DeviceExt->FltDeviceObject );
    }
}



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
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ( DeviceObject == g_tcpFltDevice )
        *ProtocolType = IPPROTO_TCP;
    else if ( DeviceObject == g_udpFltDevice )
        *ProtocolType = IPPROTO_UDP;
    else if ( DeviceObject == g_rawIpFltDevice )
    {
        // ��ȡԭʼ�ӿ�Э���
        status = TiGetProtocolNumber( &FileObject->FileName, ProtocolType );
        if ( !NT_SUCCESS(status) )
        {
            DbgMsg(__FILE__, __LINE__, "TiGetProtocolNumber Raw IP protocol number is invalid.\n");
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else if ( DeviceObject == g_ipFltDevice )
        *ProtocolType = IPPROTO_IP;
    else if ( DeviceObject == g_igmpFltDevice )
        *ProtocolType = IPPROTO_IGMP;
    else
        status = STATUS_INVALID_PARAMETER;

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: TiGetProtocolNumber
// ˵��: �����ļ�������Э���
// ���: FileName �ļ�������
// ����: Protocol ����Э���
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TiGetProtocolNumber( IN PUNICODE_STRING FileName, OUT PULONG Protocol )
{
    UNICODE_STRING us;
    NTSTATUS Status;
    ULONG Value;
    PWSTR Name;

    Name = FileName->Buffer;

    if ( *Name++ != (WCHAR)L'\\' )
        return STATUS_UNSUCCESSFUL;

    if ( *Name == L'\0' )
        return STATUS_UNSUCCESSFUL;

    RtlInitUnicodeString(&us, Name);

    Status = RtlUnicodeStringToInteger(&us, 10, &Value);
    if ( !NT_SUCCESS(Status) || ((Value > IPPROTO_RAW)) )
        return STATUS_UNSUCCESSFUL;

    *Protocol = Value;
    return STATUS_SUCCESS;
}



