#ifndef __IO_CONTROL_H__
#define __IO_CONTROL_H__

#include "LockCommandLine.h"

//
// �ɵ�360
//
#define LOCK_IO_CTL_KILL_360	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
// �����������ҳ
//
#define LOCK_IO_CTL_LOCK_HOME_PAGE	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define MAX_URL_LENGTH 200
typedef struct _IO_PACKET_LOCK_HOMEPAGE
{
	WCHAR LockedURL[MAX_URL_LENGTH];
}IO_PACKET_LOCK_HOMEPAGE, *PIO_PACKET_LOCK_HOMEPAGE;



#endif //__IO_CONTROL_H__