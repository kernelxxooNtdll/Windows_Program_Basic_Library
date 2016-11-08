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


// 结构体定义的头文件
#include "structs.h"

// 网络传输类型定义
#include "Sock.h"

// IPv4设备集
#include "fw_ioctl.h"
#include "IPv4ProtocolSet.h"
#include "ObjListManager.h"


// 定义未公开的函数(根据EPROCESS 获取进程名)
//UCHAR* PsGetProcessImageFileName( PEPROCESS Process );


// 内存操作宏定义
#define kmalloc(size)           ExAllocatePoolWithTag( NonPagedPool, size, 'root' )
#define kfree(ptr)              ExFreePoolWithTag( ptr, 'root' )


// 打印IP地址信息
void PrintIpAddressInfo( PCHAR pszMsg, PTA_ADDRESS pTaAddress );

void DbgMsg(char *lpszFile, int Line, char *lpszMsg, ...);
