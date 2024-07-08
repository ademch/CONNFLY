
#ifndef CROTATIONCONTROLDLG_H
#define CROTATIONCONTROLDLG_H


#include "afxwin.h"
#include "afxcmn.h"
#include "DownButton.h"
#include "Plot.h"
#include "AsynchCommunicator.h"
#include <vector>

#define TIMER_GETSTATE_ID  				1     // OS id
#define TIMER_GETSTATE_MS  				500	  // ms
#define TIMER_GETSTATE_MS_SLOW			1000  // ms

#define TIMER_COUNTDOWN    				2     // OS id
#define TIMER_COUNTDOWN_MS 				1000	 // ms

#define PROGRAM_JOG_TO_START            0
#define PROGRAM_JOG_TO_START_COMPLETE   256
#define PROGRAM_ROTATION_SEQ_COMPLETE   512
#define PROGRAM_FAILED_START            1024

struct SolarProgramState
{
	bool bProgramInProgress;

	int iProgramSequence;		// bits 0-5	define solar/shade rotation
	int iLastOperationMarker;	// 
};

class AsynchCommunicator;

// CRotationControlDlg dialog
class CRotationControlDlg : public CDialog
{
// Construction
public:
	CRotationControlDlg(CWnd* pParent = NULL);	// standard constructor
	~CRotationControlDlg();

// Dialog Data
	enum { IDD = IDD_ROTATIONCONTROL_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP();

	void LoadControlBitmaps();
	void CreateGUIToolTips();

	AsynchCommunicator *asynchCommunicator;

public:
	CComboBox		m_comboPorts;
	CRichEditCtrl	m_reLog;
	CButton			m_btnConnect;
	CButton			m_btnDisconnect;
	CButton			m_btnHardStop;
	CButton			m_btnPowerOn;
	CDownButton		m_btnJogLeft;
	CDownButton		m_btnJogRight;
	CStaticPlot		m_Plot;
	CButton			m_btnGotoZero;
	CButton			m_btnSetZero;
	CButtonSpecial  m_btnProgramStart;
	CButtonSpecial	m_btnProgramStop;
	CEdit           m_editLeftLimit;
	CEdit 			m_editRightLimit;
	CEdit 			m_editSolarPeriod;
	CEdit 			m_editOrbitalCycles;
	CEdit 			m_editJoggingSpeed;
	CStatic			m_textProgram;
	CButton         m_btnLog;
	CStaticSpecial  m_textMinutes;
	CStaticSpecial  m_textSeconds;
	CEdit			m_editWarningPeriod;

	void printfConsole(const char* fmt, ...);
	void printfLog(const char* fmt, ...);

	void EnableDlgButtons(BOOL bEnable);
	void EnableCommandButtons(BOOL bEnable);
	void EnablePositioningButtons(BOOL bEnable);

	//afx_msg void OnNMCustomdrawCustom2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonDisconnect();
	afx_msg void OnBnDownButtonJogLeft();
	afx_msg void OnBnUpButtonJogLeft();
	afx_msg void OnBnDownButtonJogRight();
	afx_msg void OnBnUpButtonJogRight();
	afx_msg void OnBnClickedCheckHardstop();
	afx_msg void OnBnClickedCheckEnableDriver();

	LRESULT OnAsynchFinished(WPARAM wParam, LPARAM lParam);

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnClose();

	afx_msg void OnBnClickedCheckIgnoreEncoder();
	afx_msg void OnBnClickedRadioStop();
	afx_msg void OnBnClickedRadioStart();
	afx_msg void OnBnClickedButtonSetZero();
	afx_msg void OnBnClickedButtonGotoZero();
	afx_msg void OnBnClickedButtonLog();

	int m_iJoggingSpeed;
	int m_iLeftLimit;
	int m_iRightLimit;
	int m_iSolarPeriod;
	int m_iOrbitalCycles;

	CButton m_checkIgnoreEncoder;

	void StartProgramCountdownTimer(int iCountdownSeconds);

	ULARGE_INTEGER time0;
	int m_iCountdownSeconds;
	int m_iWarningSecSaved;

	int m_iServoRatio;

private:
	CFont font14;
	CFont font28;
	CFont font36;

	FILE* pFileLog;

	int m_iWarningTimeSec;

	void RegisterToolTip(CWnd* pWnd, UINT nIDText);

	BOOL m_bConsoleVisible;

	std::vector<CToolTipCtrl*> vecToolTips;

	void UpdateLegend();
	void CreateOrOpenLogFile();

};

#endif

