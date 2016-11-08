
#ifndef __INJECT_CODE_H__
#define __INJECT_CODE_H__

#include <ntddk.h>

NTSTATUS LhpLockCommandLine(HANDLE ProcessId, LPWSTR ProcessImageName);

NTSTATUS LhpLockCommandLineInternalXP(HANDLE ProcessId, LPWSTR ProcessImageName);
NTSTATUS LhpLockCommandLineInternaWin7(HANDLE ProcessId, LPWSTR ProcessImageName);


#endif // __INJECT_CODE_H__