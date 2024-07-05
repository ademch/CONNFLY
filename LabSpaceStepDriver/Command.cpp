#include "Command.h"
#include "StepperMotor.h"
#include "PinsConfig.h"

//#define DEBUG


extern StepMotor stepMotor;
extern ActiveState activeState;
extern String aState[4];

extern int iEncoder;

bool bSendEncoderFeedback = false;


void PrintHelp()
{
	Serial.println("help, ?:   this help");
	Serial.println("GSTAT:     driver status");
	Serial.println("GENCODER:  get encoder value");
	Serial.println("USENCODER: GSTAT will report enoder values");
	Serial.println("USEPULSES: GSTAT will report pulse values (default)");
	Serial.println("ENBLE:     enable line");
	Serial.println("DSBLE:     disable line");
	Serial.println("ROTATE:    start rotation [.3f speed 1.8deg/min] steps[1.8deg]");
	Serial.println("RSTOP:     stop rotation");
	Serial.println("RSTPHARD:  stop now");
}

void ProcessCommand()
{
	if (!digitalRead(PIN_STEPDRIVER_ALARM))
	{
		if (activeState != STATE_ALARM)
			// PC knows we disable driver without command from "above"
			digitalWrite(PIN_STEPDRIVER_EN, LOW);

		stepMotor.MarchStopHard();

		activeState = STATE_ALARM;

		if (Serial.available())
		{
			String strRequest;
			strRequest = Serial.readStringUntil('\n');

			if (strRequest == "GSTAT")
			{
				// For a status request reply ALARM
				Serial.print(aState[activeState]);
			}
			else
				// For any other command (if any) reply error
				Serial.print(RESPONCE_ERROR);
		}
	}
	else
	if (Serial.available())
	{
		if (activeState == STATE_ALARM)
			activeState = STATE_IDLE;		// RESET alarm state

		String strRequest;
		strRequest = Serial.readStringUntil('\n');

		if (strRequest == "GSTAT")
		{
			if ((activeState == STATE_ROTT) || (activeState == STATE_IDLE))
			{
				char strBuff[20];
				sprintf(strBuff, aState[activeState].c_str(), (bSendEncoderFeedback) ? iEncoder*4 : stepMotor.iCurPos);
				//                                                                             ^ enoder provides only 1000 resolution per revolution, step driver provides 4000
				Serial.print(strBuff);
			}
			else
				Serial.print(aState[activeState]);
		}
		else if (strRequest == "GENCODER")
		{
			char strBuff[20];
			sprintf(strBuff, "%d\n", iEncoder);
			Serial.print(strBuff);
		}
		else if (strRequest == "USENCODER")
		{
			bSendEncoderFeedback = true;

			Serial.print(RESPONCE_OK);
		}
		else if (strRequest == "USEPULSES")
		{
			bSendEncoderFeedback = false;

			Serial.print(RESPONCE_OK);
		}
		else if (strRequest == "ENBLE")
		{
			digitalWrite(PIN_STEPDRIVER_EN, HIGH);

			Serial.print(RESPONCE_OK);
		}
		else if (strRequest == "DSBLE")
		{
			digitalWrite(PIN_STEPDRIVER_EN, LOW);

			Serial.print(RESPONCE_OK);
		}
		else if (strRequest == "RSTPHARD")
		{
			stepMotor.MarchStopHard();

			activeState = STATE_IDLE;

			Serial.print(RESPONCE_OK);
		}
		else if (strRequest.startsWith("ROTATE "))			// [%.3f speed 1.8deg/min] steps[1.8deg]
		{
			// parse velocity in 1.8 deg per minute
			strRequest.remove(0, 7);
			float fVelocity1_8 = strRequest.toFloat();
			fVelocity1_8 /= 60.0f;					     	// 1.8 deg per minute -> 1.8deg per second

			float fVelocityPPS;
			fVelocityPPS = fVelocity1_8 * STEP_MICROSTEP;	// 1.8 deg per second -> pulses per second

			if ((fVelocityPPS < 0.25f) || (fVelocityPPS > 90.0f * STEP_MICROSTEP))
			{
				Serial.print(RESPONCE_ERROR_HSPEED);
				return;
			}

			#ifdef DEBUG
				Serial.print("fVelocityPPS=");
				Serial.println(fVelocityPPS);
			#endif

			// parse number of 1.8 steps
			strRequest.remove(0, strRequest.indexOf(' ') + 1);
			int iSteps1_8 = strRequest.toInt();

			bool bCW = (iSteps1_8 > 0) ? true : false;
			iSteps1_8 = abs(iSteps1_8);

			if ((iSteps1_8 < 0) || (iSteps1_8 > 3600 ))
			{
				Serial.print(RESPONCE_ERROR_STEPS);
				return;
			}

			#ifdef DEBUG
				Serial.print("iSteps1_8=");
				Serial.println(iSteps1_8);
			#endif
			#ifdef DEBUG
				Serial.print("bCW=");
				Serial.println(bCW);
			#endif

			// experimental block
			if (activeState == STATE_ROTT)
			{
				cli();

				// if the motor is deccelerating and the new request is with the same characteristics
				if ( (stepMotor.mState == DECC) &&
					((iSteps1_8 == stepMotor.m1_8_StepCount) && (fVelocityPPS == stepMotor.fVelocity) && (stepMotor.mCW == bCW)) )
				{
					stepMotor.CatchUpDecceleratingMotor();

					sei();

					Serial.print(RESPONCE_OK);
					return;
				}
				sei();

				Serial.print(RESPONCE_BUSY);
				return;
			}

			activeState = STATE_ROTT;

			stepMotor.MarchNSteps(iSteps1_8, fVelocityPPS, bCW);

			Serial.print(RESPONCE_OK);
		}
		else if (strRequest == "RSTOP")
		{
			stepMotor.MarchStopPremature();

			// ---> the state is changed in callback 

			Serial.print(RESPONCE_OK);
		}
		else if ((strRequest == "help") || (strRequest == "?"))
		{
			PrintHelp();
		}
		else
		{
			Serial.print(RESPONCE_NOTSUPPORTED);
		}
	}
}


void RotationDoneCallback()
{
	if (digitalRead(PIN_STEPDRIVER_ALARM))	// active LOW
		activeState = STATE_IDLE;
}

