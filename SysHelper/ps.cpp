
//////////////////////////////////////////////////////////////////////////
// 提权
//////////////////////////////////////////////////////////////////////////
bool EnableDebugPrivilege()   
{   
	HANDLE hToken;   
	LUID sedebugnameValue;   
	TOKEN_PRIVILEGES tkp;   
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{   
		return   FALSE;   
	}   
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))  
	{   
		CloseHandle(hToken);   
		return false;   
	}   
	tkp.PrivilegeCount = 1;   
	tkp.Privileges[0].Luid = sedebugnameValue;   
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;   
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) 
	{   
		CloseHandle(hToken);   
		return false;   
	}   
	return true;   
}


//////////////////////////////////////////////////////////////////////////
// 根据进程ID得到进程名
//////////////////////////////////////////////////////////////////////////
wstring GetProcessNameById(DWORD ProcessId)
{
	wstring strProcessName = L"";
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapShot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 pe32;
		ZeroMemory(&pe32, sizeof(pe32));
		pe32.dwSize = sizeof(pe32);

		BOOL bOk = Process32First(hSnapShot, &pe32);
		while (bOk)
		{
			if (pe32.th32ProcessID == ProcessId)
			{
				strProcessName = pe32.szExeFile;
				break;
			}
			bOk = Process32Next(hSnapShot, &pe32);
		}

		CloseHandle(hSnapShot);
	}

	return strProcessName;
}
