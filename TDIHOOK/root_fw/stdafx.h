#pragma once


//////////////////////////////////////////////////////////////////////////
// Thanks to tdifw1.4.4
// Thanks to ReactOS 0.3.13
//////////////////////////////////////////////////////////////////////////


#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						


#include "VisualDDKHelpers.h"
#include <ntddk.h>
#include <ntddstor.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <tdi.h>
#include <tdiinfo.h>
#include <TdiKrnl.h>
#include <tdistat.h>
#include <ntstrsafe.h>

#include <stdlib.h>


// �ṹ�嶨���ͷ�ļ�
#include "structs.h"

// ���紫�����Ͷ���
#include "Sock.h"

// IPv4�豸��
#include "fw_ioctl.h"
#include "IPv4ProtocolSet.h"
#include "ObjListManager.h"


// ����δ�����ĺ���(����EPROCESS ��ȡ������)
//UCHAR* PsGetProcessImageFileName( PEPROCESS Process );


// �ڴ�����궨��
#define kmalloc(size)           ExAllocatePoolWithTag( NonPagedPool, size, 'root' )
#define kfree(ptr)              ExFreePoolWithTag( ptr, 'root' )


// ��ӡIP��ַ��Ϣ
void PrintIpAddressInfo( PCHAR pszMsg, PTA_ADDRESS pTaAddress );

void DbgMsg(char *lpszFile, int Line, char *lpszMsg, ...);
