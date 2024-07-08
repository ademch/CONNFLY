
#ifndef ASYNCH_COMMUNICATOR_H
#define ASYNCH_COMMUNICATOR_H

#include "resource.h"			// main symbols
#include "RotationControlDlg.h"
#include "SerialPort.h"

#define WM_ASYNCH_FINISHED    WM_APP + 1

#define ROTATE_CW                1
#define ROTATE_CCW               0

#define RES_DEFAULT              0
#define RES_ROT_IN_PROGRESS      0
#define RES_ROT_FINISHED         1
#define RES_NO_ALARM             0
#define RES_ALARM                1

#define MICROSTEPS               20

// Asynch command state
enum AsynchState
{	asynchNone = 0,
	asynchHello,
	asynchCheckAlarm,
	asynchEnable,
	asynchDisable,
	asynchJog,
	asynchJogStop,
	asynchJogToZeroPos,
	asynchHardStop,
	asynchRotationProgram,
	asynchRotationProgramStop,
	asynchRotateGetStatus,
	asynchSwitchToEncoder,
	asynchSwitchToStepPulses
};

struct AsynchMessage
{
public:
	AsynchMessage(AsynchState _asynchCommand = asynchNone, int _param1 = 0)
	{
		asynchCommand = _asynchCommand;
		param1 = _param1;
	}

	AsynchState asynchCommand;
	int param1;
};



class CRotationControlDlg;

class AsynchCommunicator
{
public:
	AsynchCommunicator(CRotationControlDlg* _dlg)
	{
		dlg = _dlg;
	}

	virtual ~AsynchCommunicator() {
	}

	CRotationControlDlg* dlg;

	BOOL StepDriverRotateRaw(float fRotationSpeed, int iAngle_1_8);

	BOOL StepDriverQueryStateStr(CString &str);
	BOOL StepDriverReadStateStr(CString &str);

	BOOL StepDriverDisable();
	BOOL StepDriverEnable();

	BOOL StepDriverRotateSolar();
	BOOL StepDriverRotateShade();
	BOOL StepDriverRotateStop();

	BOOL StepDriverJog(int cw);
	BOOL StepDriverJogToZeroPos();
	BOOL StepDriverJogToRightLimit();
	BOOL StepDriverJogStop();
	BOOL StepDriverRotateStopHard();

	BOOL StepDriverSwitchToEncoder();
	BOOL StepDriverSwitchToDrivePulses();

	//BOOL IsResponseValid(char* lpBuffer, DWORD dwbytesRead);
	BOOL IsResponseValid(CString strResponce);

	BOOL serialPortOpen(int iPor);
	void serialPortClose();
	BOOL serialPortIsOpen();
	std::vector<CString> EnumSerialPorts();
	void SetTimeouts();
	void SetAsynchCommand(AsynchState _asynchCommand, int param1 = 0);

	void StartAsynchThread();
	void StopAsynchThread();

protected:
	CSerialPort serialPort;
};

#endif

