﻿/*
Stepvoice Recorder
Copyright (C) 2004-2016 Andrey Firsov
*/

#include "stdafx.h"
#include <htmlhelp.h>

#include "MP3_Recorder.h"
#include "MainFrm.h"

#include "UrlWnd.h"
#include "version.h"
#include "FileUtils.h"
#include <versionhelpers.h> //IsWindowsVistaOrGreater etc. functions
//#include <vld.h> //Header file from "Visual Leak Detector" - detecting memory leaks.

#include "tinyxml2.h"
#include "NewVersionDlg.h"
#include "version.h"

#include <wininet.h> //for DeleteUrlCacheEntry
#pragma comment(lib, "wininet.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////
extern TCHAR g_command_line[MAX_PATH];

////////////////////////////////////////////////////////////////////////////////
// About dialog
////////////////////////////////////////////////////////////////////////////////
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	static bool m_bRecIcon;

	//{{AFX_MSG(CAboutDlg)
	afx_msg void OnPlayerIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	ON_BN_CLICKED(IDC_PLAYERICON, OnPlayerIcon)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
bool CAboutDlg::m_bRecIcon = false;

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	m_bRecIcon = true;
}
//---------------------------------------------------------------------------

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------

void CAboutDlg::OnPlayerIcon()
{
	CStatic* wndRecIcon = (CStatic*)GetDlgItem(IDC_PLAYERICON);
	if (m_bRecIcon)
	{
		SetTimer(1, 900, NULL);
		wndRecIcon->SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME2));
	}
	else
	{
		KillTimer(1);
		wndRecIcon->SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME));
	}

	m_bRecIcon = !m_bRecIcon;
}
//---------------------------------------------------------------------------

void CAboutDlg::OnTimer(UINT nIDEvent)
{
	static bool bRecIcon = false;
	if (nIDEvent == 1)
	{
		CStatic* wndRecIcon = (CStatic*)GetDlgItem(IDC_PLAYERICON);
		if (bRecIcon)
			wndRecIcon->SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME2));
		else
			wndRecIcon->SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME));
		bRecIcon = !bRecIcon;
	}
	CDialog::OnTimer(nIDEvent);
}
//---------------------------------------------------------------------------

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString l_thanks_to[] = {
		_T("MPEG Layer-3 Audio Codec Lame 3.98.2\r\n(http://www.mp3dev.org/mp3)"),
		_T("BASS library (bass.dll) by Ian Luck\r\n(http://www.un4seen.com)"),
		_T("TinyXML-2 library by Lee Thomason\r\n(http://www.grinninglizard.com)"),
		_T("Ruben Marchan"),
		_T("")
	};
	CString l_wnd_text;

	int i = 0;
	while (l_thanks_to[i] != _T(""))
	{
		if (i > 0)
			l_wnd_text += _T("\r\n\r\n");
		l_wnd_text += l_thanks_to[i++];
	}

	CEdit* pThanksEdit = (CEdit*)GetDlgItem(IDC_THANKSTO);
	pThanksEdit->SetWindowText(l_wnd_text);

	// Preparing version string.

	CString l_format_string;
	GetDlgItemText(IDC_STATIC_VERSION, l_format_string);

	CString l_version_string;
	l_version_string.Format(l_format_string, STRFILEVER);
	SetDlgItemText(IDC_STATIC_VERSION, l_version_string);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
//---------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// Recorder application class
////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CMP3_RecorderApp, CWinApp)
	//{{AFX_MSG_MAP(CMP3_RecorderApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_OPENLOGFOLDER, OnHelpOpenLogFolder)
	ON_COMMAND(IDM_HELP_EMAIL, OnHelpEmail)
	ON_COMMAND(IDM_HELP_DOC, OnHelpDoc)
	ON_COMMAND(IDA_HELP_DOC, OnHelpDoc)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_CHECKFORUPDATES, &CMP3_RecorderApp::OnHelpCheckforupdates)
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
CMP3_RecorderApp theApp;

////////////////////////////////////////////////////////////////////////////////
CMP3_RecorderApp::CMP3_RecorderApp()
	:m_is_vista(::IsWindowsVistaOrGreater())
{
}

////////////////////////////////////////////////////////////////////////////////
static const UINT UWM_ARE_YOU_ME = ::RegisterWindowMessage(
	_T("UWM_ARE_YOU_ME-{B87861B4-8BE0-4dc7-A952-E8FFEEF48FD3}"));

static const UINT UWM_PARSE_LINE = ::RegisterWindowMessage(
	_T("UWM_PARSE_LINE-{FE0907E6-B77E-46da-8D2B-15F41F32F440}"));

////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK CMP3_RecorderApp::searcher(HWND hWnd, LPARAM lParam)
{
	DWORD result;
	LRESULT ok = ::SendMessageTimeout(hWnd, UWM_ARE_YOU_ME, 0, 0,
		SMTO_BLOCK | SMTO_ABORTIFHUNG, 200,
		&result);
	if (ok == 0)
		return TRUE; // ���������� ��� ���� � ����������

	if (result == UWM_ARE_YOU_ME)
	{ /* ����� */
		HWND* target = (HWND*)lParam;
		*target = hWnd;
		return FALSE; // ����������� �����
	} /* ����� */

	return TRUE; // ���������� �����
} // CMyApp::searcher


bool CMP3_RecorderApp::IsNeedOneInstance()
{
	return !RegistryConfig::GetOption(_T("General\\Multiple instances"), false);
}

bool CMP3_RecorderApp::IsAlreadyRunning()
{
	HANDLE hMutexOneInstance = ::CreateMutex(NULL, FALSE,
		_T("SVREC-169A0B91-77B7-4533-9C25-59FCB08FCD614"));

	bool l_already_running = (::GetLastError() == ERROR_ALREADY_EXISTS ||
		::GetLastError() == ERROR_ACCESS_DENIED);
	// ����� ���������� ERROR_ACCESS_DENIED, ���� ������� ��� ������
	// � ������ ���������������� ������, �.�. � �������� ���������
	// SECURITY_ATTRIBUTES ��� �������� �������� ���������� NULL

	return l_already_running;
}

////////////////////////////////////////////////////////////////////////////////
BOOL CMP3_RecorderApp::InitInstance()
{
	//New SetRegistryKey commented - must be called once.
	SetRegistryKey(_T("StepVoice Software"));
	//RegistryConfig::SetRegistryKey(_T("StepVoice Software"));

	// Variables for command line
	CString l_cmd_line(m_lpCmdLine);
	const int CMD_LENGTH = l_cmd_line.GetLength();

	if (this->IsAlreadyRunning() && this->IsNeedOneInstance())
	{
		HWND hOther = NULL;
		EnumWindows(searcher, (LPARAM)&hOther);
		if (hOther != NULL)
		{
			if (CMD_LENGTH > 0)
			{
				_tcsncpy_s(g_command_line, MAX_PATH,
					l_cmd_line.GetBuffer(CMD_LENGTH), CMD_LENGTH);
				::PostMessage(hOther, UWM_PARSE_LINE, 0, 0);
			}
			else
			{
				::SetForegroundWindow(hOther);
				::ShowWindow(hOther, SW_RESTORE);
			}
			return FALSE; // window exist, exiting this instance
		}
		//return FALSE; // exiting
	}

	//Initializing logger after the running instance check (avoid log creation error).
	InitLogger();
	LOG_INFO() << "Program started.";
	LOG_INFO() << "Operation system: " << GetWindowsVersionString() << '.';

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	pFrame->DragAcceptFiles(TRUE);
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	if (CMD_LENGTH > 0)
	{
		_tcsncpy_s(g_command_line, MAX_PATH,
			l_cmd_line.GetBuffer(CMD_LENGTH), CMD_LENGTH);
		pFrame->PostMessage(UWM_PARSE_LINE, 0, 0);
	}

	return TRUE;
}
//---------------------------------------------------------------------------

int CMP3_RecorderApp::ExitInstance()
{
	LOG_INFO() << "Program closed.";
	return 0;
}
//---------------------------------------------------------------------------

void CMP3_RecorderApp::InitLogger()
{
	//Checking stepvoice data folder.

	using namespace FileUtils;
	const CString appDataFolder = GetSpecialFolder(CSIDL_COMMON_APPDATA);
	const CString svrecDataFolder = CombinePath(appDataFolder, _T("Stepvoice"));

	if (!FolderExists(svrecDataFolder) && !::CreateDirectory(svrecDataFolder, NULL))
	{
		AfxMessageBox(_T("Unable to create log folder: ") + svrecDataFolder);
		return;
	}

	//Keeping only last 3 log files in folder (in case of some problem,
	//user reopen svrec and send report with current-normal and previous-
	//problematic logs). Log names are in this format:
	//	SvRec_1.log.txt <-- oldest
	//	SvRec_2.log.txt
	//	SvRec_3.log.txt <-- newest

	const CString log1Path = CombinePath(svrecDataFolder, _T("SvRec_1.log.txt"));
	const CString log2Path = CombinePath(svrecDataFolder, _T("SvRec_2.log.txt"));
	const CString log3Path = CombinePath(svrecDataFolder, _T("SvRec_3.log.txt"));

	const bool haveLog1 = FileExists(log1Path);
	const bool haveLog2 = FileExists(log2Path);
	const bool haveLog3 = FileExists(log3Path);

	//When all files present - remove the oldest log and shift names.
	if (haveLog3)
	{
		if (haveLog1)
			::DeleteFile(log1Path);
		if (haveLog2)
			::MoveFile(log2Path, log1Path);
		::MoveFile(log3Path, log2Path);
	}

	//Ok, previous logs processed, creating logger with appropriate path.

	if (haveLog3 || haveLog2)
		CLog::Open(log3Path);
	else
		if (haveLog1)
			CLog::Open(log2Path);
		else
			CLog::Open(log1Path);
}
//---------------------------------------------------------------------------

CString CMP3_RecorderApp::GetWindowsVersionString() const
{
	CString version = _T("?");

	//if (::IsWindows10OrGreater())
	//	version = _T("10");
	if (::IsWindows8Point1OrGreater())
		version = _T("8.1");
	else
		if (::IsWindows8OrGreater())
			version = _T("8.0");
		else
			if (::IsWindows7SP1OrGreater())
				version = _T("7 (SP1)");
			else
				if (::IsWindows7OrGreater())
					version = _T("7");
				else
					if (::IsWindowsVistaSP2OrGreater())
						version = _T("Vista (SP2)");
					else
						if (::IsWindowsVistaSP1OrGreater())
							version = _T("Vista (SP1)");
						else
							if (::IsWindowsVistaOrGreater())
								version = _T("Vista");

	CString resultString;
	resultString.Format(_T("Windows %s (or greater) %s"), version, IsWow64() ? _T("x64") : _T("x32"));
	return resultString;
}
//---------------------------------------------------------------------------

bool CMP3_RecorderApp::IsWow64() const
{
	BOOL bIsWow64 = FALSE;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.

	typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			//handle error
		}
	}
	return bIsWow64 ? true : false;
}

////////////////////////////////////////////////////////////////////////////////
void CMP3_RecorderApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
//---------------------------------------------------------------------------

void CMP3_RecorderApp::OnHelpOpenLogFolder()
{
	const CString appDataFolder = FileUtils::GetSpecialFolder(CSIDL_COMMON_APPDATA);
	const CString svrecDataFolder = FileUtils::CombinePath(appDataFolder, _T("Stepvoice"));
	ShellExecute(::GetDesktopWindow(), _T("open"), svrecDataFolder, NULL, NULL, SW_SHOW);
}
//---------------------------------------------------------------------------

void CMP3_RecorderApp::OnHelpEmail()
{
	CString mailString;
	mailString.Format(_T("mailto:support@stepvoice.com?subject=[svrec %s]&body=(Please attach log files, accessible via program menu: Help | Open log folder)"),
		STRFILEVER);

	mailString.Replace(_T(" "), _T("%20"));
	ShellExecute(0, NULL, mailString, NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------

void CMP3_RecorderApp::OnHelpDoc()
{
	CMainFrame* pFrame = (CMainFrame*)m_pMainWnd;

	using namespace FileUtils;
	CString helpFile = CombinePath(GetProgramFolder(), _T("svrec.chm::/stepvoice_recorder/overview.html"));
	::HtmlHelp(GetDesktopWindow(), helpFile, HH_DISPLAY_TOPIC, NULL);
}
//---------------------------------------------------------------------------

bool CMP3_RecorderApp::GetUpdateInformation(CString& latestVersion, CString& downloadLink)
{
	LOG_DEBUG() << __FUNCTION__ << " ::1";
	CWaitCursor waitCursor;

	using namespace FileUtils;
	const CString appDataFolder = GetSpecialFolder(CSIDL_COMMON_APPDATA);
	const CString svrecDataFolder = CombinePath(appDataFolder, _T("Stepvoice"));

	const CString updateFileName = L"StepvoiceRecorderUpdate.xml";
	const CString remoteFilePath = L"http://stepvoice.com/download/" + updateFileName;
	const CString localFilePath = CombinePath(svrecDataFolder, updateFileName);

	DeleteUrlCacheEntry(remoteFilePath);
	HRESULT hr = URLDownloadToFile(NULL, remoteFilePath, localFilePath, 0, NULL);
	if (hr != S_OK) {
		LOG_DEBUG() << __FUNCTION__ << " ::2, error, hr=" << int(hr);
		return false;
	}

	CW2A localFilePathAnsi(localFilePath);
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError err = doc.LoadFile(localFilePathAnsi);
	if (err != tinyxml2::XML_SUCCESS) {
		LOG_DEBUG() << __FUNCTION__ << " ::3, error, unable to load xml file.";
		return false;
	}

	latestVersion = doc.FirstChildElement()->FirstChildElement("LastVersion")->GetText();
	downloadLink = doc.FirstChildElement()->FirstChildElement("DownloadLink")->GetText();
	::DeleteFile(localFilePath);
	LOG_DEBUG() << __FUNCTION__ << " ::4, latestVersion=" << latestVersion << ", downloadLink=" << downloadLink;
	return true;
}
//---------------------------------------------------------------------------

void CMP3_RecorderApp::OnHelpCheckforupdates()
{
	const CString caption = L"Stepvoice Recorder";
	const CString msgLatest = L"You are running the latest version!";
	const CString msgError = L"Failed to retrieve update information.";

	CString latestVersion, downloadLink;
	if (!GetUpdateInformation(latestVersion, downloadLink)) {
		::MessageBox(m_pMainWnd->GetSafeHwnd(), msgError, caption, MB_OK | MB_ICONSTOP);
		return;
	}

	const CString curVersion(STRFILEVER);
	if (curVersion >= latestVersion) {
		::MessageBox(m_pMainWnd->GetSafeHwnd(), msgLatest, caption, MB_OK | MB_ICONINFORMATION);
		return;
	}

	CNewVersionDlg dlg(m_pMainWnd);
	dlg.DoModal(latestVersion, downloadLink);
}
//---------------------------------------------------------------------------

BOOL CMP3_RecorderApp::OnIdle(LONG lCount)
{
	// Letting the base class make all needed work
	if (CWinApp::OnIdle(lCount))
	{
		return TRUE;
	}

	// Updating the main window interface
	CMainFrame* l_main_wnd = (CMainFrame*)AfxGetMainWnd();
	ASSERT(l_main_wnd);
	l_main_wnd->UpdateInterface();

	return FALSE;	// No more idle time needed
}
//---------------------------------------------------------------------------
