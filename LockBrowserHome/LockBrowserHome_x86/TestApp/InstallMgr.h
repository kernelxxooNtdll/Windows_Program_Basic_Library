#pragma once

#include <string>
using namespace std;


typedef
BOOL
(WINAPI *PFNWow64DisableWow64FsRedirection)(
	__out PVOID *OldValue
	);

typedef
BOOL
(WINAPI *PFNWow64RevertWow64FsRedirection)(
	__in PVOID OlValue
	);

typedef 
BOOL 
(__stdcall * LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);



class CInstallMgr
{
public:

	CInstallMgr();
	virtual ~CInstallMgr();


	bool InstallDriver();

	bool UninstallDriver();

protected:

	//
	//	�ж��Ƿ���WOW64����
	//
	static bool IsX64System();
	static bool ReleaseResFile(LPCWSTR strFileName, UINT wResID, LPCWSTR strFileType);
	static DWORD QueryServiceStatusByName(LPCWSTR SvcName);

private:

	//
	//	�ļ��ض������
	//
	void DisableFsRedirection();
	void RevertFsRedirection();

	PFNWow64DisableWow64FsRedirection m_pfnWow64DisableWow64FsRedirection;
	PFNWow64RevertWow64FsRedirection m_pfnWow64RevertWow64FsRedirection;

	bool m_bWow64;
	PVOID m_FsRedirectionOldValue;
	wstring m_driversPath;
};
