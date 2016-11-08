#include "root_fw.h"
#include "TdiFltDisp.h"


//////////////////////////////////////////////////////////////////////////
// ����: DispatchFilterIrp
// ˵��: ����TDI ��ͨ���� IRP_MJ_DEVICE_CONTROL
// ���: DeviceObject ���ص�TDI�ı����豸����
//       Irp IRP����ָ��
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchFilterIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );

    switch ( IrpSP->MajorFunction )
    {
        // ����һ���ļ�����
    case IRP_MJ_CREATE:
        status = TdiCreate( DeviceObject, Irp, TdiDeviceExt );
        break;

    case IRP_MJ_CLOSE:
        status = TdiClose( DeviceObject, Irp, TdiDeviceExt );
        break;

        // ���е��ļ�����ľ���ѹر�(���˱������������)
    case IRP_MJ_CLEANUP:
        status = TdiCleanup( DeviceObject, Irp, TdiDeviceExt );
        break;

    case IRP_MJ_DEVICE_CONTROL:
        status = TdiMapUserRequest(DeviceObject, Irp, IrpSP);
        if ( NT_SUCCESS(status) )
        {
            status = OnDispatchInternal( DeviceObject, Irp );
        }
        else
        {
            // ���������齻���²㴦��
            IoSkipCurrentIrpStackLocation( Irp );
            status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
        }
        break;

    default:
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
        break;
    }

    return status;
}



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
    )
{
    PFILE_FULL_EA_INFORMATION pNextEa = FileEaInfo;
    PFILE_FULL_EA_INFORMATION CurrentEaInfo = NULL;

    if ( NULL == pNextEa )
        return NULL;

    // ��������EA��Ϣ�ṹ
    while ( TRUE )
    {
        CurrentEaInfo = pNextEa;
        pNextEa += pNextEa->NextEntryOffset;    // ��һ��EA��Ϣ���ƫ��

        if ( CurrentEaInfo->EaNameLength == NameLength )
        {
            // �ҵ�EA���˳�ѭ��
            if ( RtlCompareMemory(CurrentEaInfo->EaName, EaName, NameLength) == NameLength )
                break;
        }

        // û����Ϣ��
        if ( !CurrentEaInfo->NextEntryOffset )
            return NULL;
    }

    return CurrentEaInfo;
}




//////////////////////////////////////////////////////////////////////////
// ����: EventCompleteRoutine
// ˵��: ֪ͨ�¼����������(֪ͨIRP�·��Ѿ��������)
// ���: DeviceObject �豸����
//       Irp IRP����ָ��
//       Context ���õ���������������(TDI_DEVICE_EXTENSIONָ��)
// ����: NTSTATUS
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS EventNotifyCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DeviceObject);

    // �����²�������
    Irp->UserIosb->Information = Irp->IoStatus.Information;
    Irp->UserIosb->Status = Irp->IoStatus.Status;

    KeSetEvent( Irp->UserEvent, IO_NETWORK_INCREMENT, FALSE );

    // �������ϻ���,�Լ������irp���뷵�����
    return STATUS_MORE_PROCESSING_REQUIRED;
}



