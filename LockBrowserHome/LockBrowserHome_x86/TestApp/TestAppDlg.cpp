// TestAppDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestApp.h"
#include "TestAppDlg.h"
#include "InstallMgr.h"

#include <WinIoCtl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HANDLE ShellCode_C(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
	);

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CTestAppDlg dialog




CTestAppDlg::CTestAppDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestAppDlg::IDD, pParent)
	, m_lockUrl(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_URL, m_lockUrl);
}

BEGIN_MESSAGE_MAP(CTestAppDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_LOCK, &CTestAppDlg::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CTestAppDlg::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CTestAppDlg message handlers

BOOL CTestAppDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	m_lockUrl = L"http://www.baidu.com";
	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTestAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTestAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTestAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//
// ¸Éµô360
//
#define LOCK_IO_CTL_KILL_360	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Ëø¶¨ä¯ÀÀÆ÷Ê×Ò³
//
#define LOCK_IO_CTL_LOCK_HOME_PAGE	\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAX_URL_LENGTH 200
typedef struct _IO_PACKET_LOCK_HOMEPAGE
{
	WCHAR LockedURL[MAX_URL_LENGTH];
}IO_PACKET_LOCK_HOMEPAGE, *PIO_PACKET_LOCK_HOMEPAGE;



BOOL CTestAppDlg::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
	{
		return TRUE;      //ÆÁ±ÎµôEsc¼ü
	}
	if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		return TRUE;      //ÆÁ±ÎµôEnter¼ü
	}

	return CDialog::PreTranslateMessage( pMsg );
}


void CTestAppDlg::OnBnClickedBtnLock()
{
	UpdateData(TRUE);

	m_lockUrl.TrimLeft();
	m_lockUrl.TrimRight();
	
	if (m_lockUrl.IsEmpty())
	{
		MessageBox(L"ÇëÌîÐ´Ò»¸öÓÐÐ§µÄURL!");
	}
	else
	{
		CInstallMgr mgr;
		if (!mgr.InstallDriver())
		{
			MessageBox(L"Ëø¶¨Ê§°Ü");
			return;
		}

		IO_PACKET_LOCK_HOMEPAGE IoPacket;
		ZeroMemory(&IoPacket, sizeof(IoPacket));
		wcscpy(IoPacket.LockedURL, (LPCWSTR)m_lockUrl);

		HANDLE hDevice = CreateFile(L"\\\\.\\LockBrowserHome",
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			CString strInfo;
			strInfo.Format(L"Can't open driver's device. GetLastError() = %d\n", GetLastError());
			MessageBox(strInfo);
			return;
		}

		BOOL bRet = FALSE;
		DWORD BytesReturned = 0;

		bRet = DeviceIoControl(hDevice,
			LOCK_IO_CTL_LOCK_HOME_PAGE,
			&IoPacket,
			sizeof(IoPacket),
			NULL,
			0,
			&BytesReturned,
			NULL);

		CloseHandle(hDevice);

		if (bRet){
			MessageBox(L"Ëø¶¨³É¹¦!");
		}else{
			MessageBox(L"Ëø¶¨Ê§°Ü!");
		}
	}
}

void CTestAppDlg::OnBnClickedBtnUnlock()
{
	// TODO: Add your control notification handler code here

	CInstallMgr mgr;
	if (mgr.UninstallDriver()){
		MessageBox(L"½âËø³É¹¦!");
	}else{
		MessageBox(L"½âËøÊ§°Ü!");
	}
}

