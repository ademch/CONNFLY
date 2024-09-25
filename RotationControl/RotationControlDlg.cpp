// RotationControlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RotationControl.h"
#include "RotationControlDlg.h"
#include "ControllerCommands.h"
#include <ctime>
#include <direct.h>
#include <atlbase.h>
#include <dbt.h>

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

#pragma warning(disable : 4244)

extern int round(float x);

SolarProgramState solarProgramState;

// CRotationControlDlg dialog
CRotationControlDlg::CRotationControlDlg(CWnd* pParent /*=NULL*/)
	               : CDialog(CRotationControlDlg::IDD, pParent)
				   , m_iJoggingSpeed(120)
				   , m_iLeftLimit(-110)
				   , m_iRightLimit(110)
				   , m_iSolarPeriod(45)
				   , m_iOrbitalCycles(3)
				   , m_iServoRatio(6)
				   , m_iWarningTimeSec(180)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_bConsoleVisible = FALSE;
	pFileLog = NULL;

	asynchCommunicator = new AsynchCommunicator(this);
}

CRotationControlDlg::~CRotationControlDlg()
{
	for (unsigned int i=0; i< vecToolTips.size(); i++)
		delete vecToolTips[i];
	vecToolTips.clear();

	delete asynchCommunicator;
	asynchCommunicator = NULL;
}


void CRotationControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO_PORTS,          m_comboPorts);
	DDX_Control(pDX, IDC_BUTTON_CONNECT,       m_btnConnect);
	DDX_Control(pDX, IDC_BUTTON_DISCONNECT,    m_btnDisconnect);
	DDX_Control(pDX, IDC_STATIC_PLOT,          m_Plot);
	DDX_Control(pDX, IDC_BUTTON_JOG_LEFT,      m_btnJogLeft);
	DDX_Control(pDX, IDC_BUTTON_JOG_RIGHT,     m_btnJogRight);
	DDX_Control(pDX, IDC_BUTTON_GOTO_ZERO,     m_btnGotoZero);
	DDX_Control(pDX, IDC_BUTTON_SETZERO,       m_btnSetZero);
	DDX_Control(pDX, IDC_RADIO_START,          m_btnProgramStart);
	DDX_Control(pDX, IDC_RADIO_STOP,           m_btnProgramStop);
	DDX_Control(pDX, IDC_CHECK_HARDSTOP,       m_btnHardStop);
	DDX_Control(pDX, IDC_CHECK_POWERON,        m_btnPowerOn);
	DDX_Control(pDX, IDC_EDIT_JOGGING_SPEED,   m_editJoggingSpeed);
	DDX_Control(pDX, IDC_EDIT_LEFT_LIMIT,      m_editLeftLimit);
	DDX_Control(pDX, IDC_EDIT_RIGHT_LIMIT,     m_editRightLimit);
	DDX_Control(pDX, IDC_EDIT_SOLAR_PERIOD,    m_editSolarPeriod);
	DDX_Control(pDX, IDC_EDIT_ORBITAL_CYCLES,  m_editOrbitalCycles);
	DDX_Control(pDX, IDC_CHECK_IGNORE_ENCODER, m_checkIgnoreEncoder);
	DDX_Control(pDX, IDC_STATIC_CYCLE,         m_textProgram);
	DDX_Control(pDX, IDC_BUTTON_LOG,           m_btnLog);
	DDX_Control(pDX, IDC_STATIC_MINUTES,       m_textMinutes);
	DDX_Control(pDX, IDC_STATIC_SECONDS,       m_textSeconds);
	DDX_Control(pDX, IDC_EDIT_WARNING_PERIOD,  m_editWarningPeriod);

	DDX_Text(pDX,    IDC_EDIT_WARNING_PERIOD,  m_iWarningTimeSec);
	DDX_Text(pDX,    IDC_EDIT_JOGGING_SPEED,   m_iJoggingSpeed);
	DDX_Text(pDX,    IDC_EDIT_LEFT_LIMIT,      m_iLeftLimit);
	DDX_Text(pDX,    IDC_EDIT_RIGHT_LIMIT,     m_iRightLimit);
	DDX_Text(pDX,    IDC_EDIT_SOLAR_PERIOD,    m_iSolarPeriod);
	DDX_Text(pDX,    IDC_EDIT_ORBITAL_CYCLES,  m_iOrbitalCycles);
	DDX_Text(pDX,    IDC_EDIT_SERVO_RATIO,     m_iServoRatio);
}

BEGIN_MESSAGE_MAP(CRotationControlDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	ON_WM_DEVICECHANGE()
	ON_WM_CLOSE()
	ON_COMMAND(IDC_BUTTON_JOG_LEFT  | 1 << 13,  OnBnDownButtonJogLeft)
	ON_COMMAND(IDC_BUTTON_JOG_LEFT  | 1 << 14,  OnBnUpButtonJogLeft)
	ON_COMMAND(IDC_BUTTON_JOG_RIGHT | 1 << 13,  OnBnDownButtonJogRight)
	ON_COMMAND(IDC_BUTTON_JOG_RIGHT | 1 << 14,  OnBnUpButtonJogRight)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_CONNECT,    		OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, 		OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_CHECK_HARDSTOP,    		OnBnClickedCheckHardstop)
	ON_BN_CLICKED(IDC_CHECK_POWERON,     		OnBnClickedCheckEnableDriver)
	ON_BN_CLICKED(IDC_RADIO_STOP,        		OnBnClickedRadioStop)
	ON_BN_CLICKED(IDC_RADIO_START,       		OnBnClickedRadioStart)
	ON_BN_CLICKED(IDC_BUTTON_SETZERO,    		OnBnClickedButtonSetZero)
	ON_BN_CLICKED(IDC_BUTTON_GOTO_ZERO,  		OnBnClickedButtonGotoZero)
	ON_MESSAGE(WM_ASYNCH_FINISHED,              OnAsynchFinished)
	ON_BN_CLICKED(IDC_CHECK_IGNORE_ENCODER,     OnBnClickedCheckIgnoreEncoder)
	ON_BN_CLICKED(IDC_BUTTON_LOG,               OnBnClickedButtonLog)
END_MESSAGE_MAP()


void CRotationControlDlg::printfConsole(const char* fmt, ...)
{
	char text[256] = {0};
	va_list ap;

	va_start(ap, fmt);
		vsprintf(text, fmt, ap);
	va_end(ap);

	printf(text);
	printf("\n");
}


void CRotationControlDlg::printfLog(const char* fmt, ...)
{
	if (!pFileLog) return;

	SYSTEMTIME systime;
	GetSystemTime(&systime);
	fprintf(pFileLog, "%04d-%02d-%02d %02d:%02d:%02d\t",
		    systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond);

	va_list ap;
	va_start(ap, fmt);
		vfprintf(pFileLog, fmt, ap);
	va_end(ap);

	fflush(pFileLog);
}

void CRotationControlDlg::CreateOrOpenLogFile()
{
	char strPath[MAX_PATH];
	GetModuleFileName(NULL, strPath, MAX_PATH);

	std::string str(strPath);
	size_t pos = str.find_last_of('\\');
	str.resize(pos);

	str += "\\Log";

	if (_chdir(str.c_str()) != 0)
		_mkdir(str.c_str());
		
	str += "\\log.txt";

	pFileLog = fopen(str.c_str(), "a");

	if (pFileLog)
	{	
		fprintf(pFileLog, "\n");
		printfLog("Software started____________________________________\n");
	}
}

void CRotationControlDlg::EnableDlgButtons(BOOL bEnable)
{
	m_btnJogLeft.EnableWindow(bEnable);
	m_btnJogRight.EnableWindow(bEnable);
	m_btnSetZero.EnableWindow(bEnable);
	m_btnGotoZero.EnableWindow(bEnable);
	m_btnProgramStart.EnableWindow(bEnable);
	m_btnProgramStop.EnableWindow(bEnable);
	m_btnHardStop.EnableWindow(bEnable);
	m_btnPowerOn.EnableWindow(bEnable);
	m_checkIgnoreEncoder.EnableWindow(bEnable);
}

void CRotationControlDlg::EnableCommandButtons(BOOL bEnable)
{
	m_btnJogLeft.EnableWindow(bEnable);
	m_btnJogRight.EnableWindow(bEnable);
	m_btnSetZero.EnableWindow(bEnable);
	m_btnGotoZero.EnableWindow(bEnable);
	m_btnProgramStart.EnableWindow(bEnable);
	m_btnProgramStop.EnableWindow(bEnable);
	m_checkIgnoreEncoder.EnableWindow(bEnable);
}

void CRotationControlDlg::EnablePositioningButtons(BOOL bEnable)
{
	m_btnJogLeft.EnableWindow(bEnable);
	m_btnJogRight.EnableWindow(bEnable);
	m_btnSetZero.EnableWindow(bEnable);
	m_btnGotoZero.EnableWindow(bEnable);
}

void CRotationControlDlg::LoadControlBitmaps()
{
	m_btnJogLeft.SetBitmap(  	 LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_LEFT))  );
	m_btnJogRight.SetBitmap( 	 LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_RIGHT)) );
	m_btnSetZero.SetBitmap(  	 LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_ZERO))  );
	m_btnGotoZero.SetBitmap( 	 LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_TOP))   );
	m_btnProgramStart.SetBitmap( LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_START)) );
	m_btnProgramStop.SetBitmap(  LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_STOP))  );
	m_btnHardStop.SetBitmap(     LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_HARDSTOP)));
	m_btnPowerOn.SetBitmap(      LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_POWER)));
	m_btnLog.SetBitmap(          LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_BITMAP_LOG))   );
}


void CRotationControlDlg::RegisterToolTip(CWnd* pWnd, UINT nIDText)
{
	CToolTipCtrl* m_pToolTip = new CToolTipCtrl();

	m_pToolTip->Create(this);
	m_pToolTip->AddTool(pWnd, nIDText);
	m_pToolTip->Activate(TRUE);

	vecToolTips.push_back(m_pToolTip);
}


void CRotationControlDlg::CreateGUIToolTips()
{
	RegisterToolTip(&m_btnHardStop,     	  IDS_STRING_HARDSTOP);
	RegisterToolTip(&m_btnPowerOn,      	  IDS_STRING_ENABLE_DRIVER);
	RegisterToolTip(&m_btnJogLeft,            IDS_STRING_JOG_LEFT);
	RegisterToolTip(&m_btnJogRight,      	  IDS_STRING_JOG_RIGHT);
	RegisterToolTip(&m_btnSetZero,            IDS_STRING_SET_ZERO);
	RegisterToolTip(&m_btnGotoZero,     	  IDS_STRING_GOTO_ZERO);
	RegisterToolTip(&m_btnProgramStart, 	  IDS_STRING_START_PROGRAM);
	RegisterToolTip(&m_btnProgramStop,  	  IDS_STRING_STOP_PROGRAM);
	RegisterToolTip(&m_checkIgnoreEncoder,    IDS_STRING_IGNORE_ENCODER);
	RegisterToolTip(&m_btnLog,                IDS_STRING_OPENLOG);
	RegisterToolTip(&m_editJoggingSpeed,      IDS_STRING_JOGGING_SPEED);
	RegisterToolTip(&m_editWarningPeriod,     IDS_STRING_WARNING);
}


// CRotationControlDlg message handlers

BOOL CRotationControlDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	int dpiY = GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);

	font14.CreateFont(-MulDiv(14, dpiY, 72),
					   0,0,0,FW_NORMAL,
		               0,0,0,
		               DEFAULT_CHARSET,0,0,0,
					   0, _T("MS Shell Dlg"));
	font36.CreateFont(-MulDiv(36, dpiY, 72),
					   0,0,0,FW_NORMAL,
		               0,0,0,
		               DEFAULT_CHARSET,0,0,0,
					   0, _T("MS Shell Dlg"));
	font28.CreateFont(-MulDiv(28, dpiY, 72),
					   0,0,0,FW_NORMAL,
		               0,0,0,
		               DEFAULT_CHARSET,0,0,0,
					   0, _T("MS Shell Dlg"));
	GetDlgItem(IDC_STATIC_CYCLE)->SetFont(&font14);
	GetDlgItem(IDC_STATIC_MINUTES)->SetFont(&font36);
	GetDlgItem(IDC_STATIC_COLON)->SetFont(&font28);
	GetDlgItem(IDC_STATIC_SECONDS)->SetFont(&font28);

	LoadControlBitmaps();

	CreateGUIToolTips();

	EnableDlgButtons(FALSE);

	UpdateLegend();
	RegistryGetCOMPortNames();

	printfConsole(_T("Scanning COM ports..."));

		int iCONNFLYport = -1;
		std::vector<CString> ports = asynchCommunicator->EnumSerialPorts();
		for (std::vector<CString>::iterator it = ports.begin(); it != ports.end(); it++)
		{
			// Identify COM port number
			unsigned int iPortNumber=1;
			sscanf(*it, "COM%d", &iPortNumber);

			if ((iPortNumber < 100) && strCOMportFriendlyName[iPortNumber])
			{
				m_comboPorts.AddString(*it + " (CONNFLY)");
				
				// make this port selected by default
				iCONNFLYport = m_comboPorts.GetCount()-1;

				m_btnConnect.EnableWindow(TRUE);
			}
			else
				m_comboPorts.AddString(*it);
		}

		if (iCONNFLYport < 0)
			// CONNFLY was not found
			iCONNFLYport = m_comboPorts.GetCount()-1;

		if (iCONNFLYport >= 0)
			m_comboPorts.SetCurSel(iCONNFLYport);

	printfConsole("%i ports found", ports.size());

	CreateOrOpenLogFile();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRotationControlDlg::OnPaint() 
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
		//CStatic* plot = (CStatic*) GetDlgItem(IDC_STATIC_PLOT);
		//CDC *dc = plot->GetDC();

		CDialog::OnPaint();
	}
}

void CRotationControlDlg::UpdateLegend()
{
	char aCaption[200];

	if (!solarProgramState.bProgramInProgress)
	{
		sprintf(aCaption, "Cycle: Ready");

		m_Plot.plotState = plotStateInit;
	}
	else
	{
		if (solarProgramState.iProgramSequence == PROGRAM_JOG_TO_START)
		{
			sprintf(aCaption, "Cycle: Initializing");

			m_Plot.plotState = plotStateInit;
			m_Plot.m_iLeftLimitDeg  = m_iLeftLimit;
			m_Plot.m_iRightLimitDeg = m_iRightLimit;

			printfLog("Starting experiment ->\n");
			printfLog("\tLeft limit: %d deg, Right Limit: %d deg\n", m_iLeftLimit, m_iRightLimit);
			printfLog("\tSolar period: %d min, Number of cycles: %d\n", m_iSolarPeriod, m_iOrbitalCycles);
			printfLog("\tJogging speed: %d 1.8deg/min, Servo ratio %d:1\n", m_iJoggingSpeed, m_iServoRatio);
		}
		else if ((solarProgramState.iProgramSequence >= PROGRAM_JOG_TO_START_COMPLETE) &&
                 (solarProgramState.iProgramSequence <  PROGRAM_ROTATION_SEQ_COMPLETE))
		{
			// (0xFF) remains 0,1,2,3,4..., (>> 1) converts (0,1 -> 0; 1,2 -> 1...)
			int iCycle = ((solarProgramState.iProgramSequence & 0xFF) >> 1) + 1;
			sprintf(aCaption, "Cycle: %d", iCycle);

			if (solarProgramState.iProgramSequence  % 2)
			{
				m_Plot.plotState = plotStateShade;
				printfLog("Cycle %d: Shade trajecotry started\n", iCycle);

			}
			else
			{
				m_Plot.plotState = plotStateSun;
				printfLog("Cycle %d: Sun trajectory started\n", iCycle);
			}
		}
		else if (solarProgramState.iProgramSequence > PROGRAM_ROTATION_SEQ_COMPLETE)
		{
			sprintf(aCaption, "Cycle: Finalizing" );

			m_Plot.plotState = plotStateInit;
			printfLog("Experiment completed\n");
		}
	}
	m_textProgram.SetWindowText(aCaption);
}


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRotationControlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
// The system calls this function when device list updates
BOOL CRotationControlDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
	if ( ((nEventType == DBT_DEVICEARRIVAL) || (nEventType == DBT_DEVICEREMOVECOMPLETE)) &&
		 (!asynchCommunicator->serialPortIsOpen()) ) // react only if we have not already connected
	{
		PDEV_BROADCAST_HDR devHdr = (PDEV_BROADCAST_HDR)dwData;
		if (devHdr->dbch_devicetype == DBT_DEVTYP_PORT)
		{
			//PDEV_BROADCAST_PORT portInterface; TODO handle disconnection of connected interface
			//portInterface = (PDEV_BROADCAST_PORT)devHdr;

			RegistryFreeCOMPortNames();

			RegistryGetCOMPortNames();

			UpdateCOMPortList();
		}
	}

	return TRUE;
}

//void CRotationControlDlg::OnNMCustomdrawCustom2(NMHDR *pNMHDR, LRESULT *pResult)
//{
//	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
//	// TODO: Add your control notification handler code here
//	
//	int cxIcon = GetSystemMetrics(SM_CXICON);
//
//	*pResult = 0;
//}

BOOL CRotationControlDlg::PreTranslateMessage(MSG* pMsg)
{
	// Process tooltips
	for (unsigned int i=0; i< vecToolTips.size(); i++)
		vecToolTips[i]->RelayEvent(pMsg);

	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_OEM_3))	// `
	{
		if (HWND hwnd = GetConsoleWindow())
		{
			if (m_bConsoleVisible)
				::ShowWindow(hwnd, FALSE);
			else
				::ShowWindow(hwnd, TRUE);

			m_bConsoleVisible = !m_bConsoleVisible;

			::SetForegroundWindow(m_hWnd);
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}


void CRotationControlDlg::OnBnClickedButtonConnect()
{
	// read data from GUI
	UpdateData();

	char strName[20];
	int iPortNumber=1;
	if (m_comboPorts.GetCount() > 0)
	{
		m_comboPorts.GetLBText(m_comboPorts.GetCurSel(), strName);
		sscanf(strName, "COM%d", &iPortNumber);

		printfConsole("Connecting to %s...", strName);

			if (!asynchCommunicator->serialPortOpen(iPortNumber)) return;

		printfConsole(_T("done"));

		m_btnConnect.EnableWindow(FALSE);
		m_btnDisconnect.EnableWindow(TRUE);

		asynchCommunicator->SetTimeouts();

		printfConsole(_T("Waiting for a greeting from Connfly..."));

		asynchCommunicator->StartAsynchThread();

		// Set Com thread command
		asynchCommunicator->SetAsynchCommand(asynchHello);
	}
}


void CRotationControlDlg::OnBnClickedButtonDisconnect()
{
	if (asynchCommunicator->serialPortIsOpen())
	{
		printfConsole(_T("Closing COM port..."));

			m_btnHardStop.SetCheck(BST_CHECKED);
			OnBnClickedCheckHardstop();
			Sleep(500);	// wait for commands to be processed before thread kill ! do not delete

			asynchCommunicator->StopAsynchThread();
			Sleep(500); // finalize

			asynchCommunicator->serialPortClose();

		printfConsole(_T("done"));

		m_btnConnect.EnableWindow(TRUE);
		m_btnDisconnect.EnableWindow(FALSE);

		EnableDlgButtons(FALSE);

		// only now asynchHardStop return GUI callback will be served
	}
}


void CRotationControlDlg::OnClose()
{
	m_btnHardStop.SetCheck(BST_CHECKED);
	OnBnClickedCheckHardstop();

	asynchCommunicator->StopAsynchThread();
	Sleep(500);

	asynchCommunicator->serialPortClose();

	if (!pFileLog) fclose(pFileLog);

	CDialog::OnClose();
	RegistryFreeCOMPortNames();
}

LRESULT CRotationControlDlg::OnAsynchFinished(WPARAM wParam, LPARAM lParam)
{
	
	switch (lParam)
	{
	case asynchHello:
		{
			// open up further functionality
			m_btnHardStop.EnableWindow(TRUE);
			m_btnHardStop.SetCheck(BST_CHECKED);
		}
		break;

	case asynchCheckAlarm:
		{
			if ( wParam == 0)
				m_btnPowerOn.EnableWindow(TRUE);
			else
			{
				// Set pushed, meaning alarm
				m_btnHardStop.SetCheck(BST_CHECKED);

				// Set unpushed, meaning disabled
				m_btnPowerOn.SetCheck(BST_UNCHECKED);
				m_btnPowerOn.EnableWindow(FALSE);

				EnableCommandButtons(FALSE);
			}
		}
		break;
	case asynchEnable:
		{
			EnableCommandButtons(TRUE);
			m_btnProgramStop.SetCheck(BST_CHECKED);
			m_btnProgramStart.SetCheck(BST_UNCHECKED);

		}
		break;
	case asynchDisable:
		{
			EnableCommandButtons(FALSE);

		}
		break;
	case asynchRotateGetStatus:
		{
			if (wParam == RES_ROT_IN_PROGRESS)	// rotation is in progress
			{
				m_Plot.Invalidate();
			}
			else	// RES_ROT_FINISHED
			{
				if (solarProgramState.bProgramInProgress)
				{
					// Progressing through solarProgramState.iProgramSequence is fully managed here
					// There is no feedback from asynchThread on what exactly has been complete
					if (solarProgramState.iProgramSequence == 0)
					{
						solarProgramState.iProgramSequence |= PROGRAM_JOG_TO_START_COMPLETE;
						asynchCommunicator->SetAsynchCommand(asynchRotationProgram, solarProgramState.iProgramSequence);
					}
					else if (solarProgramState.iProgramSequence < solarProgramState.iLastOperationMarker)
					{
						solarProgramState.iProgramSequence++;
						asynchCommunicator->SetAsynchCommand(asynchRotationProgram, solarProgramState.iProgramSequence);
					}
					else if (solarProgramState.iProgramSequence == solarProgramState.iLastOperationMarker)
					{
						solarProgramState.iProgramSequence |= PROGRAM_ROTATION_SEQ_COMPLETE;
						asynchCommunicator->SetAsynchCommand(asynchRotationProgram, solarProgramState.iProgramSequence);
					}
					else
					{
						solarProgramState.bProgramInProgress = false;

						// the last state update, intermediate states are updated at program start callback
						UpdateLegend();
	
						m_btnProgramStart.SetCheck(BST_UNCHECKED);
						m_btnProgramStop.SetCheck(BST_CHECKED);

						EnablePositioningButtons(TRUE);
					}

					m_Plot.Invalidate();
				}
				else
				{
					// SOME CODE
				}
			}
		}
		break;
	case asynchHardStop:
		{
			EnableCommandButtons(FALSE);
		}
		break;

	case asynchRotationProgram:
		{	
			if (wParam == PROGRAM_JOG_TO_START)
			{
				EnablePositioningButtons(FALSE);
			}
			else if (wParam == PROGRAM_FAILED_START)
			{
				solarProgramState.bProgramInProgress = false;

				m_btnProgramStart.SetCheck(BST_UNCHECKED);
				m_btnProgramStop.SetCheck(BST_CHECKED);

				EnablePositioningButtons(TRUE);
			}
			else if ((wParam >= PROGRAM_JOG_TO_START_COMPLETE) &&
				     (wParam <  PROGRAM_ROTATION_SEQ_COMPLETE))
			{
				// Rotation sequence
				if (wParam % 2)
				{
					// Calculate speed up coefficient (Solar -> 90min then Shade ?, Shade = 360 - Solar)
					float fCoefficinet = 360.0f/(m_iRightLimit - m_iLeftLimit) - 1.0f;
					StartProgramCountdownTimer(m_iSolarPeriod*fCoefficinet*60);
				}
				else
				{
					StartProgramCountdownTimer(m_iSolarPeriod*60);
				}
			}

			UpdateLegend();
		}
		break;
	case asynchRotationProgramStop:
		{
			EnablePositioningButtons(TRUE);

			UpdateLegend();
			m_Plot.Invalidate();
		}
		break;

	}	// switch

	return 0;
}



void CRotationControlDlg::OnBnDownButtonJogLeft()
{
	// read data from GUI
	UpdateData();
	asynchCommunicator->SetAsynchCommand(asynchJog, ROTATE_CCW);

}

void CRotationControlDlg::OnBnUpButtonJogLeft()
{
	asynchCommunicator->SetAsynchCommand(asynchJogStop);
}

void CRotationControlDlg::OnBnDownButtonJogRight()
{
	// read data from GUI
	UpdateData();
	asynchCommunicator->SetAsynchCommand(asynchJog, ROTATE_CW);
}

void CRotationControlDlg::OnBnUpButtonJogRight()
{
	asynchCommunicator->SetAsynchCommand(asynchJogStop);
}


void CRotationControlDlg::OnBnClickedCheckHardstop()
{
	if (m_btnHardStop.GetCheck() == BST_UNCHECKED)
	{
		// Get ready to work
		asynchCommunicator->SetAsynchCommand(asynchCheckAlarm);
	}
	else
	{
		// kill timer because it inteferes with GUI disabling logic
		KillTimer(TIMER_GETSTATE_ID);
		KillTimer(TIMER_COUNTDOWN);

		// who else?
		if (solarProgramState.bProgramInProgress)
		{
			printfLog("Program stopped by user clicking \"Hard stop\"");
			solarProgramState.bProgramInProgress = false;
		}
		UpdateLegend();

		m_btnProgramStart.SetCheck(BST_UNCHECKED);
		m_btnProgramStop.SetCheck(BST_CHECKED);

		// Shut down
		if (m_btnPowerOn.GetCheck() == BST_CHECKED)
		{
			m_btnPowerOn.SetCheck(BST_UNCHECKED);

			asynchCommunicator->SetAsynchCommand(asynchHardStop);
		}
	
		m_btnPowerOn.EnableWindow(FALSE);
	}
}

void CRotationControlDlg::OnBnClickedCheckEnableDriver()
{
	if (m_btnPowerOn.GetCheck() == BST_CHECKED)
		asynchCommunicator->SetAsynchCommand(asynchEnable);

	else
		asynchCommunicator->SetAsynchCommand(asynchDisable);

}


void CRotationControlDlg::OnBnClickedRadioStart()
{
	// read data from GUI
	UpdateData();

	if (solarProgramState.bProgramInProgress) return;

	ZeroMemory(&solarProgramState, sizeof(solarProgramState));
	solarProgramState.bProgramInProgress = true;
	solarProgramState.iLastOperationMarker  = (m_iOrbitalCycles*2 - 1) | PROGRAM_JOG_TO_START_COMPLETE;

	// solarProgramState.iProgramSequence is zero, still we pass it to follow general logic
	asynchCommunicator->SetAsynchCommand(asynchRotationProgram, solarProgramState.iProgramSequence);
}

void CRotationControlDlg::OnBnClickedRadioStop()
{
	solarProgramState.bProgramInProgress = false;
	KillTimer(TIMER_COUNTDOWN);

	printfLog("Program stopped by user clicking \"Program stop\" button\n");

	asynchCommunicator->SetAsynchCommand(asynchRotationProgramStop);
}

void CRotationControlDlg::StartProgramCountdownTimer(int iCountdownSeconds)
{
	FILETIME ft0;
	GetSystemTimeAsFileTime(&ft0);
	time0.LowPart  = ft0.dwLowDateTime;
	time0.HighPart = ft0.dwHighDateTime;

	m_iCountdownSeconds = iCountdownSeconds;

	// update data at the start to save initial value, but not at timer callback
	UpdateData();
	m_iWarningSecSaved = m_iWarningTimeSec;

	SetTimer(TIMER_COUNTDOWN, TIMER_COUNTDOWN_MS, NULL);
}

void CRotationControlDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == TIMER_GETSTATE_ID)
	{
		asynchCommunicator->SetAsynchCommand(asynchRotateGetStatus);
	}
	else if (nIDEvent == TIMER_COUNTDOWN)
	{
		FILETIME ft1;
		GetSystemTimeAsFileTime(&ft1);
		ULARGE_INTEGER time1 = {ft1.dwLowDateTime, ft1.dwHighDateTime};

		unsigned int timeSecondsElapsed = (unsigned int)((time1.QuadPart - time0.QuadPart)/10000000);

		int iSecondsRemain = m_iCountdownSeconds - timeSecondsElapsed;

		// Sound
		if      (iSecondsRemain==9) Beep(697,200);
		else if (iSecondsRemain==8) Beep(621,200);
		else if (iSecondsRemain==7) Beep(586,200);
		else if (iSecondsRemain==6) Beep(521,200);
		else if (iSecondsRemain==5) Beep(463,200);
		else if (iSecondsRemain==4) Beep(410,200);
		else if (iSecondsRemain==3) Beep(387,200);
		else if (iSecondsRemain==2) Beep(346,200);
		else if (iSecondsRemain==1) Beep(308,200);
		else if (iSecondsRemain==0) Beep(1570,100);

		// Log
		if ((iSecondsRemain % 10) == 0)
			printfLog("\tAngle (deg):\t%3d\n", round(m_Plot.AngleGlobalCoordinatesDeg()) );

		// Countdown dipslay
		int iMinutes = iSecondsRemain / 60;	
		int iSeconds = iSecondsRemain % 60;

		char strMinutes[5] = {0};
		char strSeconds[5] = {0};

		sprintf(strMinutes, "%02d", iMinutes);
		sprintf(strSeconds, "%02d", iSeconds);

		m_textMinutes.SetWindowText(strMinutes);
		m_textSeconds.SetWindowText(strSeconds);

		if (iSecondsRemain <= m_iWarningSecSaved)
		{
			m_textMinutes.SetTextColor(RGB(255,0,0));
			m_textSeconds.SetTextColor(RGB(255,0,0));
		}
		else
		{
			m_textMinutes.SetTextColor(RGB(0,0,0));
			m_textSeconds.SetTextColor(RGB(0,0,0));
		}

		// automatic shut down
		if (iSecondsRemain <= 0)
		{
			m_textMinutes.SetTextColor(RGB(0,0,0));
			m_textSeconds.SetTextColor(RGB(0,0,0));

			KillTimer(TIMER_COUNTDOWN);
		}

	}
	// CDialog::OnTimer(nIDEvent);
}


void CRotationControlDlg::OnBnClickedButtonSetZero()
{
	m_Plot.m_AngleStepDriverZeroDeg = m_Plot.m_AngleStepDriverCurrentDeg;
	m_Plot.Invalidate();
}

void CRotationControlDlg::OnBnClickedButtonGotoZero()
{
	// read data from GUI
	UpdateData();

	asynchCommunicator->SetAsynchCommand(asynchJogToZeroPos);
}


void CRotationControlDlg::OnBnClickedCheckIgnoreEncoder()
{
	// by Default steprdiver sends encoder feedback on GSTAT

	if (m_checkIgnoreEncoder.GetCheck() == BST_CHECKED)
		asynchCommunicator->SetAsynchCommand(asynchSwitchToStepPulses);
	else
		asynchCommunicator->SetAsynchCommand(asynchSwitchToEncoder);
}

void CRotationControlDlg::OnBnClickedButtonLog()
{
	char strPath[MAX_PATH];
	GetModuleFileName(NULL, strPath, MAX_PATH);

	std::string str(strPath);
	size_t pos = str.find_last_of('\\');
	str.resize(pos);

	str += "\\Log";

	if (str.length() == 0) return;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	ShellExecute(NULL, "open", str.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void CRotationControlDlg::RegistryGetCOMPortNames()
{
	memset(strCOMportFriendlyName, 0, 100*sizeof(TCHAR*));
	
	CRegKey Key;
	CString strRegPath(_T("SYSTEM\\CurrentControlSet\\Enum\\USB\\Vid_1a86&Pid_7523"));
	if (Key.Open(HKEY_LOCAL_MACHINE, strRegPath, KEY_READ) == ERROR_SUCCESS)
	{
		DWORD len = MAX_PATH;
		TCHAR strName[MAX_PATH] = {0};
		for (int i = 0; Key.EnumKey(i, strName, &len) == ERROR_SUCCESS; i++)
		{
			CRegKey KeyChild;
			if (KeyChild.Open(HKEY_LOCAL_MACHINE, strRegPath + _T("\\") + strName, KEY_READ) == ERROR_SUCCESS)
			{
				ULONG lenChild = MAX_PATH;
				TCHAR strValue[MAX_PATH] = {0};
				KeyChild.QueryStringValue("FriendlyName", strValue, &lenChild);

				char* ptr = strstr(strValue, "(COM");
				if (ptr != NULL)
				{
					ptr += 4;	// the length of "(COM" 
					unsigned int iNumber = atoi(ptr);
					if (iNumber < 100)
					{
						strCOMportFriendlyName[iNumber] = new char[strlen(strValue)+1];
						strcpy(strCOMportFriendlyName[iNumber], strValue);
					}
				}
			}
			KeyChild.Close();
			len = MAX_PATH;
		}
		Key.Close();
	}
}

void CRotationControlDlg::RegistryFreeCOMPortNames()
{
	for (int i=0; i<100; i++) {
		delete strCOMportFriendlyName[i];
		strCOMportFriendlyName[i] = 0;
	}
}

// Call RegistryGetCOMPortNames beforehand
void CRotationControlDlg::UpdateCOMPortList()
{
	int iCONNFLYport = -1;
	std::vector<CString> ports = asynchCommunicator->EnumSerialPorts();

	// ComboBox is updated dynamically on device connect/disconnect
	m_comboPorts.ResetContent();

	for (std::vector<CString>::iterator it = ports.begin(); it != ports.end(); it++)
	{
		// Identify COM port number
		unsigned int iPortNumber=1;
		sscanf(*it, "COM%d", &iPortNumber);

		// if the port is found in the list of Connfly registered devices
		if ((iPortNumber < 100) && strCOMportFriendlyName[iPortNumber])
		{
			m_comboPorts.AddString(*it + " (CONNFLY)");
			
			// make this port selected by default
			iCONNFLYport = m_comboPorts.GetCount()-1;

			m_btnConnect.EnableWindow(TRUE);
		}
		else
			m_comboPorts.AddString(*it);
	}

	if (iCONNFLYport < 0)
	{
		// CONNFLY was not found
		iCONNFLYport = m_comboPorts.GetCount()-1;
		m_btnConnect.EnableWindow(FALSE);
	}

	if (iCONNFLYport >= 0)
		m_comboPorts.SetCurSel(iCONNFLYport);
}
