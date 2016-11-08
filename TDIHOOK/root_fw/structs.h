#pragma once

// 结构体定义头文件
#include <ntddk.h>


#define MAX_EVENT                   (TDI_EVENT_ERROR_EX + 1)

// 最大地址结构体
#define TDI_ADDRESS_MAX_LENGTH	    TDI_ADDRESS_LENGTH_OSI_TSAP
#define TA_ADDRESS_MAX			    (sizeof(TA_ADDRESS) - 1 + TDI_ADDRESS_MAX_LENGTH)
#define TDI_ADDRESS_INFO_MAX	    (sizeof(TDI_ADDRESS_INFO) - 1 + TDI_ADDRESS_MAX_LENGTH)



// TDI设备对象的设备扩展结构体
typedef struct _TDI_DEVICE_EXTENSION
{
    PFILE_OBJECT  FileObject;           // 文件对象
    PDEVICE_OBJECT  FltDeviceObject;    // 本层设备对象
    PDEVICE_OBJECT  NextStackDevice;    // 下层设备对象
} TDI_DEVICE_EXTENSION, *PTDI_DEVICE_EXTENSION;



// TDI请求处理派遣表结构体
typedef struct _TDI_REQUEST_TABLE
{
    UCHAR  RequestCode;                 // IRP IOCTL
    PDRIVER_DISPATCH  DispatchFunc;     // 派遣处理函数指针
} TDI_REQUEST_TABLE, *PTDI_REQUEST_TABLE;



// TDI事件请求列表
typedef struct _TDI_EVENT_DISPATCH_TABLE
{
    ULONG  EventType;                   // event type
    PVOID  EventHandler;                // 处理函数地址指针
} TDI_EVENT_DISPATCH_TABLE, *PTDI_EVENT_DISPATCH_TABLE;


// TDI 事件结构体
typedef struct _TDI_EVENT_CONTEXT
{
    PFILE_OBJECT  FileObject;   // 保存地址文件对象
    PVOID  OldHandler;          // 保存旧操作函数指针
    PVOID  OldContext;          // 保存旧操作函数参数上下文
} TDI_EVENT_CONTEXT, *PTDI_EVENT_CONTEXT;


// 地址信息结构体
typedef struct _ADDRESS_ITEM
{
    LIST_ENTRY  ListEntry;                  // 地址信息结构体链表
    PFILE_OBJECT  FileObject;               // 文件指针(索引)

    HANDLE  ProcessID;                      // 进程ID
    ULONG  ProtocolType;                    // 协议类型

    UCHAR  LocalAddress[TA_ADDRESS_MAX];    // 本地地址信息

    TDI_EVENT_CONTEXT  EventContext[MAX_EVENT]; // 事件处理的结构体上下文
} ADDRESS_ITEM, *PADDRESS_ITEM;



// 连接上下文对象指针
typedef struct _CONNECTION_ITEM
{
    LIST_ENTRY  ListEntry;                  // 连接上下文对象链表
    PFILE_OBJECT  FileObject;               // 文件指针(索引)
    CONNECTION_CONTEXT  ConnectionContext;  // 连接上下文指针(索引 事件处理函数使用)

    UCHAR  ConnectType;                     // 连接类型状态(入站 or 出站)
    UCHAR  RemoteAddress[TA_ADDRESS_MAX];   // 远程地址信息

    PADDRESS_ITEM  AddressItemPtr;          // 关联的地址信息结构体
} CONNECTION_ITEM, *PCONNECTION_ITEM;



