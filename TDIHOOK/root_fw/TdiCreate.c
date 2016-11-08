#include "TdiFltDisp.h"
#include "root_fw.h"



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
    )
{
    PFILE_FULL_EA_INFORMATION EaInfo;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    PAGED_CODE()

    EaInfo = (PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    // ���󴴽�Address File
    if ( FindEaData( EaInfo, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH ) )
    {
        // ������Address File����
        status = DispatchTransportAddress( DeviceObject, Irp, TdiDeviceExt, IrpSP );
    }
    else if ( FindEaData( EaInfo, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH) )
    {
        // ���������������ĵ�����
        CONNECTION_CONTEXT ConnectionContext = *(CONNECTION_CONTEXT *)(EaInfo->EaName + EaInfo->EaNameLength + 1);

        status = DispatchConnectionContext( DeviceObject, 
            Irp, 
            TdiDeviceExt, 
            IrpSP, 
            ConnectionContext 
            );
    }
    else
    {
        // ���󴴽�����ͨ��
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: TransportAddressCompleteRoutine
// ˵��: ���ɴ����ַ����������� IRP_MJ_CREATE TransportAddress
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context ���õ���������������(TDI_DEVICE_EXTENSIONָ��)
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TransportAddressCompleteRoutine( IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context 
    )
{
    NTSTATUS status;
    UCHAR LocalAddress[TA_ADDRESS_MAX];
    PADDRESS_ITEM AddressItem = (PADDRESS_ITEM)Context;

    // ����Pendingλ
    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( AddressItem == NULL )
        return STATUS_SUCCESS;

    // ������豸����
    if ( !NT_SUCCESS(Irp->IoStatus.Status) )
    {
        FreeAddressItem( AddressItem );
        return STATUS_SUCCESS;
    }

    // ��ѯ��ַ��Ϣ
    status = TdiQueryAddressInfo( AddressItem->FileObject, sizeof(LocalAddress), LocalAddress );
    if ( !NT_SUCCESS(status) )
    {
        DbgMsg(__FILE__, __LINE__, "TdiQueryAddressInfo failed FileObject: %08X, Status: %08X.\n", 
            AddressItem->FileObject, status );

        FreeAddressItem( AddressItem );
        return STATUS_SUCCESS;
    }

    PrintIpAddressInfo( "TransportAddressCompleteRoutine LocalAddress:", (PTA_ADDRESS)LocalAddress );

    // �����ַ��Ϣ
    RtlCopyMemory( &AddressItem->LocalAddress, LocalAddress, sizeof(AddressItem->LocalAddress) );

    // ���뵽�����ַ������
    InsertAddressItemToList( AddressItem );
    return STATUS_SUCCESS;
}



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
    )
{
    NTSTATUS status;
    ULONG ProtocolType;
    PADDRESS_ITEM pAddressItem;

    // ��ȡЭ���
    status = GetProtocolType( DeviceObject, IrpSP->FileObject, &ProtocolType );

    if ( Irp->CurrentLocation > 1 && NT_SUCCESS(status) )
    {
        pAddressItem = MallocAddressItem();
        if ( pAddressItem != NULL )
        {
            pAddressItem->FileObject = IrpSP->FileObject;
            pAddressItem->ProtocolType = ProtocolType;

            // ���浱ǰ���̵�ID
            pAddressItem->ProcessID = PsGetCurrentProcessId();
        }

        // �����������
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, TransportAddressCompleteRoutine, pAddressItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else 
    {
        // ���������豸ջ
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



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
    )
{
    PIRP Irp = NULL;
    PMDL mdl = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PTDI_ADDRESS_INFO pAddrInfo = NULL;
    ULONG nBufSize = sizeof(TDI_ADDRESS_INFO) + AddressLength;

    // ���豸ջ����ʼ���´���
    PDEVICE_OBJECT RelatedDevice = IoGetRelatedDeviceObject( FileObject );

    RtlZeroMemory( pIPAddress, AddressLength );

    __try 
    {
        // ����IRP����
        Irp = IoAllocateIrp( RelatedDevice->StackSize, FALSE );
        if ( Irp == NULL )
        {
            DbgMsg(__FILE__, __LINE__, "TdiQueryAddressInfo IoAllocateIrp == NULL, Insufficient Resources.\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        // �����ڴ�
        pAddrInfo = kmalloc( nBufSize );
        if ( pAddrInfo == NULL )
        {
            DbgMsg(__FILE__, __LINE__, "TdiQueryAddressInfo kmalloc == NULL, Insufficient Resources.\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory( pAddrInfo, nBufSize );

        // �����ڴ�������
        mdl = IoAllocateMdl( pAddrInfo, nBufSize, FALSE, FALSE, NULL );
        if ( mdl == NULL )
            __leave;

        MmProbeAndLockPages( mdl, KernelMode, IoModifyAccess );

        // ���ɲ�ѯ����
        TdiBuildQueryInformation( Irp, RelatedDevice, FileObject, EventNotifyCompleteRoutine, 
            NULL, TDI_QUERY_ADDRESS_INFO, mdl );

        // �����²��豸���ȴ������
        status = TdiSendIrpSynchronous( RelatedDevice, Irp );

        if ( NT_SUCCESS(status) )
        {
            PTA_ADDRESS pTAddress = pAddrInfo->Address.Address;

            if ( pTAddress->AddressLength > AddressLength )
            {
                DbgMsg(__FILE__, __LINE__, "Warning: TdiQueryAddressInfo AddressLength OverSize: %d.\n", pTAddress->AddressLength);
                __leave;
            }

            // ���汾�ص�ַ
            RtlCopyBytes( pIPAddress, pTAddress, AddressLength );
        }
    }
    __finally
    {
        if ( mdl )
        {
            MmUnlockPages( mdl );
            IoFreeMdl( mdl );
        }

        if ( pAddrInfo )
        {
            kfree( pAddrInfo );
        }

        if ( Irp )
        {
            IoFreeIrp( Irp );
        }
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// ����: ConnectionContextCompleteRoutine
// ˵��: ��������������ַ����������� IRP_MJ_CREATE ConnectionContext
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
//       Context ���õ���������������(TDI_DEVICE_EXTENSIONָ��)
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ConnectionContextCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PCONNECTION_ITEM pConnectionItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( pConnectionItem == NULL )
        return STATUS_SUCCESS;

    // ���뵽����������������
    if ( NT_SUCCESS(Irp->IoStatus.Status) )
    {
        InsertConnectionItemToList( pConnectionItem );
    }
    else
    {
        FreeConnectionItem( pConnectionItem );
    }

    // �������ϲ�����������
    return STATUS_SUCCESS;
}



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
    )
{
    NTSTATUS status;
    PCONNECTION_ITEM pConnectionItem = NULL;

    if ( Irp->CurrentLocation > 1 )
    {
        // ľ���ҵ�������ϣ��
        pConnectionItem = MallocConnectionItem();
        if ( NULL != pConnectionItem )
        {
            // ����ϣ��
            pConnectionItem->FileObject = IrpSP->FileObject;

            // ��������������ָ��
            pConnectionItem->ConnectionContext = ConnectionContext;
        }

        // �����������
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, ConnectionContextCompleteRoutine, pConnectionItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}




