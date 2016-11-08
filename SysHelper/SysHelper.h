#pragma once
#include <windows.h>

#include <string>
using namespace std;


namespace SysHelper
{
	BOOL GetCurrentUserSID(wstring& strUserSID);
	DWORD FindProcessByName(LPCWSTR ProcessName);
	BOOL GetAccountSidByName(LPTSTR AccountName, PSID *Sid);
}
