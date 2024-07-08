
#include "stdafx.h"
#include "AsynchCommunicator.h"
#include "ControllerCommands.h"
#include <deque>
#include <math.h>

#pragma warning(disable : 4244)

std::deque<AsynchMessage> messageQueue;

CWinThread* threadCOMResponse = NULL;
BOOL bThreadTerminate = false;

extern int round(float x);

int EnsureRange(int iVal, int iMin, int iMax)
{
	if (iVal < iMin) iVal = iMin;
	else if (iVal > iMax) iVal = iMax;

	return iVal;
}

float EnsureRange(float fVal, float fMin, float fMax)
{
	if (fVal < fMin) fVal = fMin;
	else if (fVal > fMax) fVal = fMax;

	return fVal;
}

char *asynchCommadHuman[] =
{ 
	"asynchNone",
    "asynchHello",
	"asynchCheckAlarm",
	"asynchEnable",
	"asynchDisable",
	"asynchJog",
    "asynchJogStop",
	"asynchJogToZeroPos",
	"asynchHardStop",
	"asynchRotationProgram",
	"asynchRotationProgramStop",
	"asynchRotateGetStatus",
	"asynchSwitchToEncoder",
	"asynchSwitchToStepPulses"
};


BOOL AsynchCommunicator::serialPortOpen(int iPort)
{
	serialPort.Open(iPort);
	if (!serialPort.IsOpen()) return FALSE;

	return TRUE;
}

void AsynchCommunicator::serialPortClose() {
	serialPort.Close();
}

BOOL AsynchCommunicator::serialPortIsOpen() {
	return serialPort.IsOpen();
}

// Single entry point for commands scheduler from a single thread
void AsynchCommunicator::SetAsynchCommand(AsynchState _asynchCommand, int param1)
{
	messageQueue.push_back( AsynchMessage(_asynchCommand, param1) );

	//dlg->printfLog("Command queued: %s\n", asynchCommadHuman[_asynchCommand]);
}

std::vector<CString> AsynchCommunicator::EnumSerialPorts() {
	return serialPort.EnumSerialPorts();
}


// forward declaration, put here to keep source code tidy
UINT COMResponseFunc(LPVOID param);

void AsynchCommunicator::StartAsynchThread()
{
	if (!threadCOMResponse)
	{
		bThreadTerminate = FALSE;
		messageQueue.clear();

		threadCOMResponse = AfxBeginThread(COMResponseFunc, this);
	}
}

void AsynchCommunicator::StopAsynchThread()
{
	if (threadCOMResponse)
	{
		bThreadTerminate  = TRUE;
		threadCOMResponse = NULL;
	}
}

void AsynchCommunicator::SetTimeouts()
{
	// Set timeouts
	COMMTIMEOUTS timeouts;
	// interval between successive bytes starting from the first one
	timeouts.ReadIntervalTimeout         = 500;  // ms
	// time allocated for each byte
	timeouts.ReadTotalTimeoutMultiplier  = 500;  // ms
	// extra time
	timeouts.ReadTotalTimeoutConstant    = 500;  // ms
	// time allocated for each byte
	timeouts.WriteTotalTimeoutMultiplier = 500;  // ms
	// extra time
	timeouts.WriteTotalTimeoutConstant   = 500;  // ms

	serialPort.SetTimeouts(timeouts);
}

BOOL AsynchCommunicator::StepDriverQueryStateStr(CString &str)
{
	dlg->printfConsole(_T("..Get state command sent..."));
	
	serialPort.Write(COMMAND_GET_STATE, DWORD(strlen(COMMAND_GET_STATE)));
	str = serialPort.ReadUntil('\n');

	return IsResponseValid(str);
}

BOOL AsynchCommunicator::StepDriverReadStateStr(CString &str)
{
	str = serialPort.ReadUntil('\n');

	return IsResponseValid(str);
}

BOOL AsynchCommunicator::StepDriverRotateRaw(float fRotationSpeed, int iAngle_1_8)
{
	int iServoRatio = EnsureRange(dlg->m_iServoRatio, 1, 10);

	fRotationSpeed *= iServoRatio;
	iAngle_1_8     *= iServoRatio;

	// can not get lower than    1 because of StepDriver timer limit
	// can not get higher than 280 because of rotation safety (1800 if the limit of the error in StepDriver)
	fRotationSpeed = EnsureRange(fRotationSpeed, 1.0f, 280.0f*iServoRatio);

	dlg->printfConsole(_T("..Rotate command sent [vel=%.3f(1.8deg/min) st=%d(1.8deg)] %d:1"),
		                                          fRotationSpeed,      iAngle_1_8,    iServoRatio);

	CString strCommand;
	strCommand.Format(COMMAND_ROTATE, fRotationSpeed, iAngle_1_8);
	serialPort.Write(strCommand, strCommand.GetLength());

	CString str = serialPort.ReadUntil('\n');

	return ( IsResponseValid(str) && (str == RESPONSE_OK) );
}



BOOL AsynchCommunicator::StepDriverJog(int cw)
{
	int      iAngle_1_8 = 30;
	if (!cw) iAngle_1_8 = -iAngle_1_8;

	return StepDriverRotateRaw(dlg->m_iJoggingSpeed, iAngle_1_8);
}


BOOL AsynchCommunicator::StepDriverJogToZeroPos()
{
	// 1deg to 1.8deg
	float fAngle_1_8 = (0.0f - dlg->m_Plot.AngleGlobalCoordinatesDeg()) / 1.8f;
	int   iAngle_1_8 = round(fAngle_1_8) % 200;	// Remember only one rotation
	
	return StepDriverRotateRaw(dlg->m_iJoggingSpeed, iAngle_1_8);
}

BOOL AsynchCommunicator::StepDriverJogToRightLimit()
{
	// 1deg to 1.8deg
	float fAngle_1_8 = (dlg->m_iRightLimit - dlg->m_Plot.AngleGlobalCoordinatesDeg()) / 1.8f;
	int   iAngle_1_8 = round(fAngle_1_8) % 200;	// Remember only one rotation

	return StepDriverRotateRaw(dlg->m_iJoggingSpeed, iAngle_1_8);
}

BOOL AsynchCommunicator::StepDriverRotateSolar()
{
	// rotating ccw, because started from the right limit
	float fAngle_1_8 = (dlg->m_iLeftLimit - dlg->m_Plot.AngleGlobalCoordinatesDeg()) / 1.8f;
	int   iAngle_1_8 = round(fAngle_1_8) % 200;

	float fSpeed1_8DegPerMin = fabs(fAngle_1_8)/dlg->m_iSolarPeriod;

	return StepDriverRotateRaw(fSpeed1_8DegPerMin, iAngle_1_8);
}

BOOL AsynchCommunicator::StepDriverRotateShade()
{
	// rotating cw, because we continue from the left limit
	float fAngle_1_8 = (dlg->m_iRightLimit - dlg->m_Plot.AngleGlobalCoordinatesDeg()) / 1.8f;
	int   iAngle_1_8 = round(fAngle_1_8) % 200;

	// Calculate speed up coefficient (Solar -> 90min then Shade ?, Shade = 360 - Solar)
	float fCoefficinet = 360.0f/(dlg->m_iRightLimit - dlg->m_iLeftLimit) - 1.0;
	float fSpeed1_8DegPerMin = fabs(fAngle_1_8)/( float(dlg->m_iSolarPeriod)*fCoefficinet );

	return StepDriverRotateRaw(fSpeed1_8DegPerMin, iAngle_1_8);
}


BOOL AsynchCommunicator::StepDriverRotateStop()
{
	dlg->printfConsole(_T("..Rotate Stop command sent..."));

	serialPort.Write(COMMAND_ROTATE_STOP, DWORD(strlen(COMMAND_ROTATE_STOP)));
	CString str = serialPort.ReadUntil('\n');

	return ( IsResponseValid(str) && (str == RESPONSE_OK) );
}

BOOL AsynchCommunicator::StepDriverRotateStopHard()
{
	dlg->printfConsole(_T("..Rotate Stop Hard command sent..."));

	serialPort.Write(COMMAND_ROTATE_STOPHARD, DWORD(strlen(COMMAND_ROTATE_STOPHARD)));
	CString str = serialPort.ReadUntil('\n');

	return ( IsResponseValid(str) && (str == RESPONSE_OK) );
}

BOOL AsynchCommunicator::StepDriverSwitchToEncoder()
{
	dlg->printfConsole(_T("..Switch to Encoder command sent..."));

	serialPort.Write(COMMAND_SAMPLE_ENCODER, DWORD(strlen(COMMAND_SAMPLE_ENCODER)));
	CString str = serialPort.ReadUntil('\n');

	return ( IsResponseValid(str) && (str == RESPONSE_OK) );
}

BOOL AsynchCommunicator::StepDriverSwitchToDrivePulses()
{
	dlg->printfConsole(_T("..Switch to Driver Pulses command sent..."));

	serialPort.Write(COMMAND_SAMPLE_PULSES, DWORD(strlen(COMMAND_SAMPLE_PULSES)));
	CString str = serialPort.ReadUntil('\n');

	return ( IsResponseValid(str) && (str == RESPONSE_OK) );
}

BOOL AsynchCommunicator::StepDriverEnable()
{
	dlg->printfConsole(_T("..StepDriver enable command sent..."));

	serialPort.Write(COMMAND_ENABLE, DWORD(strlen(COMMAND_ENABLE)));
	CString str = serialPort.ReadUntil('\n');

	return ( IsResponseValid(str) && (str == RESPONSE_OK) );
}

BOOL AsynchCommunicator::StepDriverDisable()
{
	dlg->printfConsole(_T("..StepDriver disable command sent..."));

	serialPort.Write(COMMAND_DISABLE, DWORD(strlen(COMMAND_DISABLE)));
	CString str = serialPort.ReadUntil('\n');

	return ( IsResponseValid(str) && (str == RESPONSE_OK) );
}

BOOL AsynchCommunicator::IsResponseValid(CString strResponce)
{
	dlg->printfConsole("....%i bytes response received (%s)", strResponce.GetLength(), strResponce);

	if (!strResponce.IsEmpty()) return TRUE;

	return FALSE;
}

// Convert 4000 pulses per revolution into 360 degree angle
float strPulses2Degrees(const CString &strPulses)
{
	float fDeg = (float)strtol(strPulses, NULL, 10);
	fDeg = fDeg/float(MICROSTEPS) * 1.8f;

	return fDeg;
}

UINT COMResponseFunc(LPVOID param)
{
	AsynchCommunicator *asynCommun = (AsynchCommunicator*)param;
	CRotationControlDlg* dlg       = asynCommun->dlg;

	AsynchMessage asynchMessage;
	AsynchState   asynchCommand;

	while (!bThreadTerminate)
	{
		if (!messageQueue.size()) {	Sleep(50); continue; }

		asynchMessage = messageQueue.front(); messageQueue.pop_front();
		// shortcut
		asynchCommand = asynchMessage.asynchCommand;

		dlg->printfConsole("\nRequested command: %s Param1=%d", asynchCommadHuman[asynchCommand], asynchMessage.param1);

		CString strDriverState;

		if (asynchCommand == asynchHello)
		{
			// Give time for stepdriver to reboot
			Sleep(2000);

			// speacial case: await for an answer without requesting it
			if (asynCommun->StepDriverReadStateStr(strDriverState) &&
			   (strDriverState == RESPONSE_STATE_HELLO))
			{
				dlg->printfConsole(_T("....StepDriver connection established!"));

				AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, RES_DEFAULT, asynchHello);

				//asynchCommand = asynchNone;
				continue;
			}
		}
		
		if (asynchCommand != asynchNone)
		{
			//FILETIME ft0;
			//GetSystemTimeAsFileTime(&ft0);

			// Before issuing any command read device state
			if (!asynCommun->StepDriverQueryStateStr(strDriverState))
			{
				// invalid responce on a basic command

				continue;
			}

			//FILETIME ft1;
			//GetSystemTimeAsFileTime(&ft1);

			//float iNano  = float(ft1.dwLowDateTime - ft0.dwLowDateTime)/10000000;
			//dlg->printfConsole("Nanos: %f", iNano);


			// Alarm can happen during rotation, we will know it through Timer polling for rotation status
			// Special state for alarm is not needed, we handle it through GUI hardstop pushbutton
			if (strDriverState == RESPONSE_STATE_ALARM)
			{
				AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, RES_ALARM, asynchCheckAlarm);
				
				// StepDriver disabled ENABLE signal on its own, no need to wait for PC to micromanage

				dlg->KillTimer(TIMER_GETSTATE_ID);

				continue;
			}
		}

		switch (asynchCommand)
		{
		case asynchCheckAlarm:
			{
				// if we got here, there is no alarm
				AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, RES_NO_ALARM, asynchCheckAlarm);
			}

			break;
		case asynchEnable:
			{
				if (!strDriverState.Find(RESPONSE_STATE_IDLE) &&
					 asynCommun->StepDriverEnable())
				{
					AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, RES_DEFAULT, asynchEnable);
				}
			}
			break;
		case asynchDisable:
			{
				// Ignores driver state

				if (asynCommun->StepDriverDisable())
				{
					AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, 0, asynchDisable);
				}
			}
			break;
		case asynchRotateGetStatus:
			{
				if (!strDriverState.Find(RESPONSE_STATE_ROTT))
				{
					CString strPulses = strDriverState.Mid( strDriverState.Find(' ') );
					dlg->m_Plot.m_AngleStepDriverCurrentDeg = strPulses2Degrees(strPulses)/float(dlg->m_iServoRatio);

					AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, RES_ROT_IN_PROGRESS, asynchRotateGetStatus);
				}
				else
				{
					// Nice thing: the last rott coordinates
					// unknown whether state could be smth other than idle
					if (!strDriverState.Find(RESPONSE_STATE_IDLE))
					{
						CString strPulses = strDriverState.Mid( strDriverState.Find(' ') );
						dlg->m_Plot.m_AngleStepDriverCurrentDeg = strPulses2Degrees(strPulses)/float(dlg->m_iServoRatio);
					}

					dlg->KillTimer(TIMER_GETSTATE_ID);
					
					AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, RES_ROT_FINISHED, asynchRotateGetStatus);
				}
			}
			break;
		case asynchJog:
			{
				if ( (strDriverState.Find(RESPONSE_STATE_IDLE) == 0) ||
					 (strDriverState.Find(RESPONSE_STATE_ROTT) == 0) )	// CatchUp deccelerating motor
				{
					if (asynCommun->StepDriverJog(asynchMessage.param1))
					{
						// setup timer for periodic poll of rotation state
						dlg->SetTimer(TIMER_GETSTATE_ID, TIMER_GETSTATE_MS, NULL);
					}
				}
			}
			break;
		case asynchJogToZeroPos:
			{
				if (!strDriverState.Find(RESPONSE_STATE_IDLE) &&
				 	 asynCommun->StepDriverJogToZeroPos())
				{
					// setup timer for periodic poll of rotation state
					dlg->SetTimer(TIMER_GETSTATE_ID, TIMER_GETSTATE_MS, NULL);
				}
			}
			break;
		case asynchJogStop:
			{
				if (strDriverState.Find(RESPONSE_STATE_ROTT) == 0)
				{
					asynCommun->StepDriverRotateStop();
				}

				break;
			}
		case asynchHardStop:
			{
				// Ignore driver state

				asynCommun->StepDriverRotateStopHard();
				asynCommun->StepDriverDisable();

				AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, 0, asynchHardStop);

				break;
			}
		case asynchRotationProgram:
			{
				if (!strDriverState.Find(RESPONSE_STATE_IDLE))
				{
					// Jog to start
					if (asynchMessage.param1 == PROGRAM_JOG_TO_START)
					{
						if (asynCommun->StepDriverJogToRightLimit())
						{
							AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, asynchMessage.param1, asynchRotationProgram);

							// setup timer for periodic poll of rotation state
							dlg->SetTimer(TIMER_GETSTATE_ID, TIMER_GETSTATE_MS, NULL);
						}
					}
					// Rotation sequence
					else if ((asynchMessage.param1 >= PROGRAM_JOG_TO_START_COMPLETE) &&
						     (asynchMessage.param1 <  PROGRAM_ROTATION_SEQ_COMPLETE))
					{
						if (asynchMessage.param1 % 2)
						{
							if (asynCommun->StepDriverRotateShade())
							{
								AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, asynchMessage.param1, asynchRotationProgram);

								// setup timer for periodic poll of rotation state
								dlg->SetTimer(TIMER_GETSTATE_ID, TIMER_GETSTATE_MS_SLOW, NULL);
							}
						}
						else
						{
							if (asynCommun->StepDriverRotateSolar())
							{
								AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, asynchMessage.param1, asynchRotationProgram);

								// setup timer for periodic poll of rotation state
								dlg->SetTimer(TIMER_GETSTATE_ID, TIMER_GETSTATE_MS_SLOW, NULL);
							}
						}
					}
					// JogToZeroPos
					else
					{
						if (asynCommun->StepDriverJogToZeroPos())
						{
							AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, asynchMessage.param1, asynchRotationProgram);
							// setup timer for periodic poll of rotation state
							dlg->SetTimer(TIMER_GETSTATE_ID, TIMER_GETSTATE_MS, NULL);
						}
					}

				}
				else
				{
					// not IDLE

					AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, PROGRAM_FAILED_START, asynchRotationProgram);
				}
			}
			break;
		case asynchRotationProgramStop:
			{
				if (asynCommun->StepDriverRotateStop())
				{
					AfxGetMainWnd()->PostMessage(WM_ASYNCH_FINISHED, RES_DEFAULT, asynchRotationProgramStop);
				}
			}
			break;
		case asynchSwitchToEncoder:
			{
				asynCommun->StepDriverSwitchToEncoder();
			}
			break;
		case asynchSwitchToStepPulses:
			{
				asynCommun->StepDriverSwitchToDrivePulses();
			}
			break;

		} // switch

	}	// while 

	return 0;
}


