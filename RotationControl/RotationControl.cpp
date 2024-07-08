// RotationControl.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "RotationControl.h"
#include "RotationControlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRotationControlApp

BEGIN_MESSAGE_MAP(CRotationControlApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()



CRotationControlApp::CRotationControlApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CRotationControlApp object

CRotationControlApp theApp;


// CRotationControlApp initialization

BOOL CRotationControlApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	//::AfxInitRichEdit2();
	CWinApp::InitInstance();

	AfxEnableControlContainer();

	AllocConsole();

	if (HWND hwnd = GetConsoleWindow())
	{
		if ( HMENU hMenu = GetSystemMenu(hwnd, FALSE) )
		{
			DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
		}
		ShowWindow(hwnd, FALSE);
	}

	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	CRotationControlDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	fclose(stderr);
	fclose(stdout);
	fclose(stdin);

	FreeConsole();


	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
