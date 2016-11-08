#include "SysHelper.h"

#include <Tlhelp32.h>
#include <Sddl.h>

namespace SysHelper{

BOOL GetCurrentUserSID(wstring& strUserSID)
{
	BOOL bRet = FALSE;
	
	do 
	{
		DWORD pid = FindProcessByName(L"explorer.exe");
		if (pid == 0){
			break;
		}

		HANDLE ph = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); 
		if (!ph){
			break;
		}

		HANDLE token; 
		if (!OpenProcessToken(ph, TOKEN_QUERY | TOKEN_DUPLICATE, &token)){
			break;
		}

		/*ģ���¼�û��İ�ȫ������*/
		if (!ImpersonateLoggedOnUser(token)){
			break;
		}

		wchar_t szBuf[MAX_PATH] = L"";
		DWORD dwRet = MAX_PATH;
		if (GetUserNameW(szBuf, &dwRet)){
			RevertToSelf();
			break;
		}

		/*�ָ��Լ��û���ȫ������*/
		RevertToSelf();

		PSID pSid = NULL;
		LPWSTR sid = NULL;
		if (!GetAccountSidByName(szBuf, &pSid)){
			break;
		}

		ConvertSidToStringSid(pSid, &sid); //�ӽṹ���еõ�sid��
		if (sid){
			strUserSID.assign(sid);
			LocalFree(sid);
			bRet = TRUE;
		}

	} while (FALSE);
	
	return bRet;
}


//
// ���ҽ���pid
//
DWORD FindProcessByName(LPCWSTR ProcessName)
{
	HANDLE hHandle;
	DWORD i;
	BOOL Next;
	wchar_t szName[MAX_PATH];

	PROCESSENTRY32 p32 = {sizeof(p32)};
	hHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL); 
	Next = Process32First(hHandle, &p32);

	i = 0; 
	while (Next) 
	{ 
		wsprintf(szName, L"%s", p32.szExeFile);
		if (lstrcmpi(szName, ProcessName) == 0)
		{
			return p32.th32ProcessID ;
		}
		Next = Process32Next(hHandle, &p32); 
		i++; 
	} 
	CloseHandle(hHandle); 
	return 0;
}

//
//��ȡ�û�sid
//
BOOL GetAccountSidByName(LPTSTR AccountName, PSID *Sid)
{
	PSID pSID = NULL;
	DWORD cbSid = 0;
	LPTSTR DomainName = NULL;
	DWORD cbDomainName = 0;
	SID_NAME_USE SIDNameUse;
	BOOL  bDone = FALSE;

	try
	{
		if(!LookupAccountName(NULL,
			AccountName,
			pSID,
			&cbSid,
			DomainName,
			&cbDomainName,
			&SIDNameUse))
		{
			pSID = (PSID)malloc(cbSid);
			DomainName = (LPTSTR)malloc(cbDomainName * sizeof(TCHAR));
			if(!pSID || !DomainName)
			{
				throw;
			}
			if(!LookupAccountName(NULL,
				AccountName,
				pSID,
				&cbSid,
				DomainName,
				&cbDomainName,
				&SIDNameUse))
			{
				throw;
			}
			bDone = TRUE;
		}
	}
	catch(...)
	{
		//nothing
	}

	if(DomainName)
	{
		free(DomainName);
	}

	if(!bDone && pSID)
	{
		free(pSID);
	}
	if(bDone)
	{
		*Sid = pSID;
	}

	return bDone;
}

} // namespace SysHelper