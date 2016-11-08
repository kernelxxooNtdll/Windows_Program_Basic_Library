#include "stdafx.h"
#include "ObjListManager.h"


#define ADDRESSFILE_TAG             'file'
#define CONNECTIONENDPOINT_TAG      'conn'



// �����ַ�ļ��������
LIST_ENTRY g_AddressFileListHead;
KSPIN_LOCK g_AddressFileListLock;

// �����������ļ��������
LIST_ENTRY g_ConnectionEndpointListHead;
KSPIN_LOCK g_ConnectionEndpointListLock;


KIRQL g_AddressFileOldIrql;
KIRQL g_ConnectionEndpointOldIrql;

// �ڴ��
NPAGED_LOOKASIDE_LIST g_AddressFileListPool;
NPAGED_LOOKASIDE_LIST g_ConnectionEndpointListPool;


// ͬ�������Ķ���
#define AddressFileList_Lock()  \
    KeAcquireSpinLock( &g_AddressFileListLock, &g_AddressFileOldIrql )

#define AddressFileList_Unlock()    \
    KeReleaseSpinLock( &g_AddressFileListLock, g_AddressFileOldIrql )

#define ConnectionEndpointList_Lock()   \
    KeAcquireSpinLock( &g_ConnectionEndpointListLock, &g_ConnectionEndpointOldIrql )

#define ConnectionEndpointList_Unlock() \
    KeReleaseSpinLock( &g_ConnectionEndpointListLock, g_ConnectionEndpointOldIrql )



//////////////////////////////////////////////////////////////////////////
// ����: InitializeListManager
// ˵��: ��ʼ������
// ����: ��ʼ���ṹ,����TRUE��ʾ��ʼ�����
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID InitializeListManager()
{
    InitializeListHead( &g_AddressFileListHead );
    KeInitializeSpinLock( &g_AddressFileListLock );

    InitializeListHead( &g_ConnectionEndpointListHead );
    KeInitializeSpinLock( &g_ConnectionEndpointListLock );

    // ��ʼ���ڴ������
    ExInitializeNPagedLookasideList( &g_AddressFileListPool, NULL, NULL, 0,
        sizeof(ADDRESS_ITEM), ADDRESSFILE_TAG, 0 );

    ExInitializeNPagedLookasideList( &g_ConnectionEndpointListPool, NULL, NULL,
        0, sizeof(CONNECTION_ITEM), CONNECTIONENDPOINT_TAG, 0 );
}



//////////////////////////////////////////////////////////////////////////
// ����: UninitializeListManager
// ˵��: ɾ�������������
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID UninitializeListManager()
{
    PLIST_ENTRY pListEntry;
    PADDRESS_ITEM pAddressItem;
    PCONNECTION_ITEM pConnectionItem;

    // ��մ����ַ����
    AddressFileList_Lock();
    while ( !IsListEmpty(&g_AddressFileListHead) )
    {
        pListEntry = RemoveTailList( &g_AddressFileListHead );
        pAddressItem = CONTAINING_RECORD( pListEntry, ADDRESS_ITEM, ListEntry );
        FreeAddressItem( pAddressItem );
    }
    ExDeleteNPagedLookasideList( &g_AddressFileListPool );
    AddressFileList_Unlock();

    // �����������������
    ConnectionEndpointList_Lock();
    while ( !IsListEmpty(&g_ConnectionEndpointListHead) )
    {
        pListEntry = RemoveTailList( &g_ConnectionEndpointListHead );
        pConnectionItem = CONTAINING_RECORD( pListEntry, CONNECTION_ITEM, ListEntry );
        FreeConnectionItem( pConnectionItem );
    }
    ExDeleteNPagedLookasideList( &g_ConnectionEndpointListPool );
    ConnectionEndpointList_Unlock();

    // �Ƴ�����ͷ��
    if ( RemoveHeadList( &g_AddressFileListHead ) )
    {
        DbgMsg(__FILE__, __LINE__, "Remove AddressFileList Success.\n");
    }

    if ( RemoveHeadList( &g_ConnectionEndpointListHead ) )
    {
        DbgMsg(__FILE__, __LINE__, "Remove ConnectionEndpointList Success.\n");
    }
}



//////////////////////////////////////////////////////////////////////////
// ����: MallocConnectionItem
// ˵��: �������������Ľṹ���ڴ�
// ����: ��������������Ľṹ���ڴ��ַ
// ��ע: �������ʧ��,����ֵΪNULL
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PCONNECTION_ITEM MallocConnectionItem()
{
    PCONNECTION_ITEM pConnectionItem = ExAllocateFromNPagedLookasideList( &g_ConnectionEndpointListPool );

    if ( NULL != pConnectionItem )
    {
        RtlZeroMemory( pConnectionItem, sizeof(CONNECTION_ITEM) );
        InitializeListHead( &pConnectionItem->ListEntry );

        // ��ʼ��δ֪����״̬
        pConnectionItem->ConnectType = UNKNOWN_STATE;
    }

    return pConnectionItem;
}


//////////////////////////////////////////////////////////////////////////
// ����: MallocAddressItem
// ˵��: ���䴫���ַ�ṹ���ڴ�
// ����: ����Ĵ����ַ�ṹ���ڴ��ַ
// ��ע: �������ʧ��,����ֵΪNULL
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PADDRESS_ITEM MallocAddressItem()
{
    PADDRESS_ITEM pAddressItem = ExAllocateFromNPagedLookasideList( &g_AddressFileListPool );

    if ( NULL != pAddressItem )
    {
        RtlZeroMemory( pAddressItem, sizeof(ADDRESS_ITEM) );
        InitializeListHead( &pAddressItem->ListEntry );
    }

    return pAddressItem;
}



//////////////////////////////////////////////////////////////////////////
// ����: FreeAddressItem
// ˵��: �ͷŴ����ַ�����ṹ���ڴ�
// ���: pAddressItem ��ַ�ṹ���ڴ�
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID FreeAddressItem( IN PADDRESS_ITEM pAddressItem )
{
    ASSERT( pAddressItem != NULL );

    ExFreeToNPagedLookasideList( &g_AddressFileListPool, pAddressItem );
}


//////////////////////////////////////////////////////////////////////////
// ����: FreeConnectionItem
// ˵��: �ͷ�������������ؽṹ���ڴ�
// ���: pConnectionitem ���������Ľṹ���ڴ�
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID FreeConnectionItem( IN PCONNECTION_ITEM pConnectionItem )
{
    ASSERT( NULL != pConnectionItem );

    ExFreeToNPagedLookasideList( &g_ConnectionEndpointListPool, pConnectionItem );
}


//////////////////////////////////////////////////////////////////////////
// ����: InsertAddressItemToList
// ˵��: ���봫���ַ������ַ���������
// ���: pAddressItem �����ַ�ṹ��ָ��
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID InsertAddressItemToList( IN PADDRESS_ITEM pAddressItem )
{
    ASSERT( NULL != pAddressItem );

    AddressFileList_Lock();
    InsertTailList( &g_AddressFileListHead, &pAddressItem->ListEntry );
    AddressFileList_Unlock();
}



//////////////////////////////////////////////////////////////////////////
// ����: InsertConnectionItemToList
// ˵��: ���뵽����������������
// ���: pConnectionItem ���������Ľṹ���ڴ�
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID InsertConnectionItemToList( IN PCONNECTION_ITEM pConnectionItem )
{
    ASSERT( NULL != pConnectionItem );

    ConnectionEndpointList_Lock();
    InsertTailList( &g_ConnectionEndpointListHead, &pConnectionItem->ListEntry );
    ConnectionEndpointList_Unlock();
}



//////////////////////////////////////////////////////////////////////////
// ����: DeleteConnectionItemFromList
// ˵��: ��������ɾ�����������Ľṹ���ڴ�
// ���: pConnectionItem Ҫɾ��������������ָ��
// ����: �����Ľ��
// ��ע: Ԫ��ֻ���������е�ʱ�����Ч��
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
BOOLEAN DeleteConnectionItemFromList( IN PCONNECTION_ITEM pItem )
{
    ASSERT( NULL != pItem );

    if ( IsListEmpty(&pItem->ListEntry) )
    {
        DbgMsg(__FILE__, __LINE__, "DeleteConnectionItemFromList List is empty...\n");
        return FALSE;
    }

    // ���������Ƴ�����
    ConnectionEndpointList_Lock();
    RemoveEntryList( &pItem->ListEntry );
    FreeConnectionItem( pItem );
    ConnectionEndpointList_Unlock();

    return TRUE;
}



//////////////////////////////////////////////////////////////////////////
// ����: DeleteAddressItemFromList
// ˵��: ��������ɾ�������ַ�ṹ���ڴ�
// ���: pAddressItem Ҫɾ���Ĵ����ַ�ṹ���ڴ�ָ��
// ����: �����Ľ��
// ��ע: Ԫ��ֻ���������е�ʱ�����Ч��
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
BOOLEAN DeleteAddressItemFromList( IN PADDRESS_ITEM pAddrItem )
{
    ASSERT( NULL != pAddrItem );

    if ( IsListEmpty(&pAddrItem->ListEntry) )
    {
        DbgMsg(__FILE__, __LINE__, "DeleteAddressItemFromList List is empty...\n");
        return FALSE;
    }

    // ���������Ƴ�����
    AddressFileList_Lock();
    RemoveEntryList( &pAddrItem->ListEntry );
    FreeAddressItem( pAddrItem );
    AddressFileList_Unlock();

    return TRUE;
}



//////////////////////////////////////////////////////////////////////////
// ����: FindAddressItemByFileObj
// ˵��: ����FileObject���Ҷ�Ӧ�Ĵ����ַ�ṹ��ָ��
// ���: FileObject �����ַ�ļ�����ָ��
// ����: ��FileObject��Ӧ�Ĵ����ַ�ṹ��ָ��
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PADDRESS_ITEM FindAddressItemByFileObj( IN PFILE_OBJECT FileObject )
{
    PLIST_ENTRY pLink = NULL;
    PADDRESS_ITEM pResult = NULL;

    if ( IsListEmpty(&g_AddressFileListHead) )
    {
        DbgMsg(__FILE__, __LINE__, "FindAddressItem AddressFileList is empty...\n");
        return NULL;
    }

    // �������������ַ�ṹ������
    AddressFileList_Lock();
    for ( pLink = g_AddressFileListHead.Flink; 
          pLink != &g_AddressFileListHead; 
          pLink = pLink->Flink )
    {
        PADDRESS_ITEM pAddressItem = CONTAINING_RECORD( pLink, ADDRESS_ITEM, ListEntry );

        if ( pAddressItem->FileObject == FileObject )
        {
            // ������ҽ��
            pResult = pAddressItem;
            break;
        }
    }
    AddressFileList_Unlock();

    return pResult;
}


//////////////////////////////////////////////////////////////////////////
// ����: FindConnectItemByFileObj
// ˵��: ����FileObject���Ҷ�Ӧ�����������Ľṹ��ָ��
// ���: FileObject �����������ļ�����ָ��
// ����: ��FileObject ��Ӧ�����������Ľṹ��ָ��
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PCONNECTION_ITEM FindConnectItemByFileObj( IN PFILE_OBJECT FileObject )
{
    PLIST_ENTRY pLink = NULL;
    PCONNECTION_ITEM pResult = NULL;

    if ( IsListEmpty(&g_ConnectionEndpointListHead) )
    {
        DbgMsg(__FILE__, __LINE__, "FindConnectionItem ConnectionEndpointList is empty...\n");
        return NULL;
    }

    // �����������������Ľṹ������
    ConnectionEndpointList_Lock();
    for ( pLink = g_ConnectionEndpointListHead.Flink;
          pLink != &g_ConnectionEndpointListHead;
          pLink = pLink->Flink )
    {
        PCONNECTION_ITEM pConnectionItem = CONTAINING_RECORD( pLink, CONNECTION_ITEM, ListEntry );

        if ( pConnectionItem->FileObject == FileObject )
        {
            // ������ҽ��
            pResult = pConnectionItem;
            break;
        }
    }
    ConnectionEndpointList_Unlock();

    return pResult;
}


//////////////////////////////////////////////////////////////////////////
// ����: FindConnectItemByConnectContext
// ˵��: ����ConnectionContext(����������ָ��)���Ҷ�Ӧ�����������Ľṹ��ָ��
// ���: ConnectionContext ����������ָ��
// ����: ��ConnectionContext ��Ӧ�����������Ľṹ��ָ��
// ��ע: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PCONNECTION_ITEM FindConnectItemByConnectContext( IN CONNECTION_CONTEXT ConnectionContext )
{
    PLIST_ENTRY pLink = NULL;
    PCONNECTION_ITEM pResult = NULL;

    if ( IsListEmpty(&g_ConnectionEndpointListHead) )
    {
        DbgMsg(__FILE__, __LINE__, "FindConnectionItem ConnectionEndpointList is empty...\n");
        return NULL;
    }

    // �����������������Ľṹ������
    ConnectionEndpointList_Lock();
    for ( pLink = g_ConnectionEndpointListHead.Flink;
        pLink != &g_ConnectionEndpointListHead;
        pLink = pLink->Flink )
    {
        PCONNECTION_ITEM pConnectionItem = CONTAINING_RECORD( pLink, CONNECTION_ITEM, ListEntry );

        if ( pConnectionItem->ConnectionContext == ConnectionContext )
        {
            // ������ҽ��
            pResult = pConnectionItem;
            break;
        }
    }
    ConnectionEndpointList_Unlock();

    return pResult;
}



