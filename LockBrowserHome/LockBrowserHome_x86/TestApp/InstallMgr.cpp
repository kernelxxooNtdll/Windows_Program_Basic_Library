
#include "stdafx.h"
#include "InstallMgr.h"
#include "resource.h"

#include <Winsvc.h>


CInstallMgr::CInstallMgr()
:m_bWow64(false)
,m_FsRedirectionOldValue(NULL)
,m_pfnWow64DisableWow64FsRedirection(NULL)
,m_pfnWow64RevertWow64FsRedirection(NULL)
{
	WCHAR tmpPath[MAX_PATH] = {0};

	m_bWow64 = IsX64System();
	if (m_bWow64){

		m_pfnWow64DisableWow64FsRedirection = (PFNWow64DisableWow64FsRedirection)GetProcAddress(GetModuleHandleW(L"kernel32"), "Wow64DisableWow64FsRedirection");
		m_pfnWow64RevertWow64FsRedirection = (PFNWow64RevertWow64FsRedirection)GetProcAddress(GetModuleHandleW(L"kernel32"), "Wow64RevertWow64FsRedirection");

		GetWindowsDirectoryW(tmpPath, MAX_PATH);
		m_driversPath.assign(tmpPath);
		m_driversPath.append(L"\\System32\\drivers\\");

	}else{

		GetWindowsDirectoryW(tmpPath, MAX_PATH);
		m_driversPath.assign(tmpPath);
		m_driversPath.append(L"\\System32\\drivers\\");
	}
}

CInstallMgr::~CInstallMgr()
{

}

bool CInstallMgr::InstallDriver()
{
	bool bRet = false;
	SC_HANDLE scManager = NULL;
	SC_HANDLE sysHandle = NULL;

	do 
	{
		if (QueryServiceStatusByName(L"LockBrowserHome") == SERVICE_RUNNING)
		{
			bRet = true;
			break;
		}


		//
		// 1.释放驱动文件
		//
		if (m_bWow64)
		{
			//
			// WOW64环境下，驱动要放在system32\drivers目录下，故需要关闭文件重定向
			//
			DisableFsRedirection();
			bRet = ReleaseResFile((m_driversPath + L"LockBrowserHome.sys").c_str(), IDR_SYS_64, L"SYS");
			if (!bRet){
				break;
			}
			RevertFsRedirection();
		}
		else
		{
			bRet = ReleaseResFile((m_driversPath + L"LockBrowserHome.sys").c_str(), IDR_SYS_32, L"SYS");
			if (!bRet){
				break;
			}
		}

		//
		// 2.创建驱动服务
		//
		scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
		if (!scManager){
			break;
		}

		if ((sysHandle = OpenServiceW(scManager, L"LockBrowserHome", SERVICE_ALL_ACCESS)) == NULL)
		{
			DWORD TagId = 0;
			sysHandle = CreateServiceW(scManager, 
				L"LockBrowserHome", 
				L"LockBrowserHome",
				SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER,
				SERVICE_SYSTEM_START,
				SERVICE_ERROR_IGNORE,
				(m_driversPath + L"LockBrowserHome.sys").c_str(),
				L"qqihoo",
				&TagId,
				NULL,
				NULL,
				NULL);
			if (!sysHandle){
				bRet = false;
				break;
			}
		}

		//
		// 3.启动驱动服务并删除文件
		//
		bRet = StartServiceW(sysHandle, 0, NULL);

		if (bRet)
		{
			if (m_bWow64){
				DisableFsRedirection();
			}

			DeleteFileW((m_driversPath + L"LockBrowserHome.sys").c_str());

			if (m_bWow64){
				RevertFsRedirection();
			}
		}

	} while (false);

	if (scManager){
		CloseServiceHandle(scManager);
		scManager = NULL;
	}

	if (sysHandle){
		CloseServiceHandle(sysHandle);
		sysHandle = NULL;
	}

	return bRet;
}

bool CInstallMgr::UninstallDriver()
{
	bool bRet = false;
	SC_HANDLE scManager = NULL;
	SC_HANDLE sysHandle = NULL;

	do 
	{
		if (QueryServiceStatusByName(L"LockBrowserHome") != SERVICE_RUNNING)
		{
			bRet = true;
			break;
		}

		//
		// 停止驱动服务
		//
		scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!scManager){
			bRet = false;
			break;
		}

		if ((sysHandle = OpenServiceW(scManager, L"LockBrowserHome", SERVICE_ALL_ACCESS)) == NULL)
		{
			bRet = false;
			break;
		}

		SERVICE_STATUS SvcState;
		ZeroMemory(&SvcState, sizeof(SvcState));
		bRet = (bool)ControlService(sysHandle, SERVICE_CONTROL_STOP, &SvcState);
		if (!bRet){
			break;
		}

		bRet = DeleteService(sysHandle);

	} while (false);

	if (scManager){
		CloseServiceHandle(scManager);
		scManager = NULL;
	}

	if (sysHandle){
		CloseServiceHandle(sysHandle);
		sysHandle = NULL;
	}

	return bRet;
}


bool CInstallMgr::ReleaseResFile(LPCWSTR strFileName, UINT wResID, LPCWSTR strFileType)
{
	// 资源大小
	DWORD dwWrite = 0;         

	// 创建文件
	HANDLE  hFile = CreateFile(strFileName, GENERIC_WRITE,FILE_SHARE_WRITE,NULL,
		CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	if ( hFile == INVALID_HANDLE_VALUE)
		return false;

	// 查找资源文件中、加载资源到内存、得到资源大小   
	HRSRC   hrsc =  FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(wResID), strFileType);
	HGLOBAL hG = LoadResource(AfxGetInstanceHandle(), hrsc);
	DWORD   dwSize = SizeofResource(AfxGetInstanceHandle(),  hrsc);

	// 写入文件
	WriteFile(hFile,hG,dwSize,&dwWrite,NULL);
	CloseHandle( hFile );

	return true;
}


bool CInstallMgr::IsX64System()
{
	BOOL bIsX64System = false;
	LPFN_ISWOW64PROCESS pfnIsWow64 = NULL;

	do 
	{
		pfnIsWow64 = (LPFN_ISWOW64PROCESS)::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "IsWow64Process");
		if (!pfnIsWow64){
			break;
		}

		if (!pfnIsWow64(::GetCurrentProcess(), &bIsX64System)) {
			break;
		}

	} while (false);

	return bIsX64System;
}


void CInstallMgr::DisableFsRedirection()
{
	if (m_bWow64){
		m_pfnWow64DisableWow64FsRedirection(&m_FsRedirectionOldValue);
	}
}

void CInstallMgr::RevertFsRedirection()
{
	if (m_bWow64){
		m_pfnWow64RevertWow64FsRedirection(&m_FsRedirectionOldValue);
	}
}



DWORD CInstallMgr::QueryServiceStatusByName(LPCWSTR SvcName)
{
	DWORD nRet = 0;
	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_QUERY_LOCK_STATUS); 
	if (schSCManager == NULL) 
	{
		nRet = GetLastError();
		return 0;
	}

	SC_HANDLE schService = OpenService( schSCManager, SvcName, SERVICE_QUERY_STATUS );
	if (schService == 0) 
	{
		nRet = GetLastError();
	}
	else
	{
		SERVICE_STATUS serviceStatus;
		if (::QueryServiceStatus(schService, &serviceStatus))
		{
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);	
			return serviceStatus.dwCurrentState;
		}

		CloseServiceHandle(schService); 
	}

	CloseServiceHandle(schSCManager);	

	return 0;
}
