#include "Command.h"
#include "StepperMotor.h"
#include "PinsConfig.h"

//#define DEBUG

extern StepMotor stepMotor;
extern ActiveState activeState;
extern String aState[4];

extern int iEncoder;

bool bSendEncoderFeedback = false;

unsigned long iAlarmCounter;


void PrintHelp()
{
	Serial.println("Firmware build date:");
	Serial.println(__DATE__);
	Serial.println(__TIME__);
	Serial.println("Commands:");
	Serial.println("help, ?:   this help");
	Serial.println("GSTAT:     driver status");
	Serial.println("GENCODER:  get encoder value");
	Serial.println("USENCODER: GSTAT will report encoder values");
	Serial.println("USEPULSES: GSTAT will report pulse values (default)");
	Serial.println("ENBLE:     enable driver");
	Serial.println("DSBLE:     disable driver");
	Serial.println("ROTATE:    start rotation [.3f speed 1.8deg/min] steps[1.8deg]");
	Serial.println("RSTOP:     stop rotation");
	Serial.println("RSTPHARD:  stop now");
}

// Called 10 times per second in an infinite loop
void ProcessCommand()
{
	String strRequest;

	if (!digitalRead(PIN_STEPDRIVER_ALARM))	// => alarm [active low]
	{
		// For some reason PIN_STEPDRIVER_ALARM may go accidentally low
		// Knowing that the alarm is triggered only after 1s (~10 calls)
		iAlarmCounter++;
		if (iAlarmCounter > 10)
		{
			// PC knows we disable driver without command from "above"
			digitalWrite(PIN_STEPDRIVER_EN, LOW);

			stepMotor.MarchStopHard();

			activeState = STATE_ALARM;

			if (Serial.available())
			{
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
			
			return;
		}

		// There is no way to reset alarm state of the driver other than removing power from it,
		// that is why there is no way to reset software alarm
	}

	iAlarmCounter = 0;	// reset alarm counter

	if (Serial.available())
	{
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
		else if (strRequest == "GENCODER")	// TODO: delete?
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
			// STEP: parse velocity in 1.8 deg per minute
			strRequest.remove(0, 7);
			float fVelocity1_8 = strRequest.toFloat();
			fVelocity1_8 /= 60.0f;					     	// 1.8 deg per minute -> 1.8deg per second

			float fVelocityPPS;
			fVelocityPPS = fVelocity1_8 * STEP_MICROSTEP;	// 1.8 deg per second -> pulses per second

			if ((fVelocityPPS < 0.25f) || (fVelocityPPS > 90.0f * STEP_MICROSTEP))	// should we pass ratio to be able to check the angle???
			{
				Serial.print(RESPONCE_ERROR_HSPEED);
				return;
			}

			#ifdef DEBUG
				Serial.print("fVelocityPPS=");
				Serial.println(fVelocityPPS);
			#endif

			// STEP: parse number of 1.8 steps
			strRequest.remove(0, strRequest.indexOf(' ') + 1);
			int iSteps1_8 = strRequest.toInt();

			bool bCW = (iSteps1_8 > 0) ? true : false;
			iSteps1_8 = abs(iSteps1_8);

			if (iSteps1_8 > 3600)
			{
				Serial.print(RESPONCE_ERROR_STEPS);
				return;
			}

			// experimental block
			if (activeState == STATE_ROTT)
			{
				cli();

				// if the motor is deccelerating and the new request is with the same characteristics
				if ( (stepMotor.mState == DECC) &&
					((iSteps1_8 == stepMotor.i1_8_StepCount) &&
					(abs(fVelocityPPS - stepMotor.fVelocity) < 0.001) &&
					(stepMotor.bCW == bCW)) )
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
	// Make sure the logic stays clear- the state is changed to STATE_IDLE only from STATE_ROTT
	// If the state is STATE_ALARM or any other then make no state change
	if (activeState == STATE_ROTT)
		activeState = STATE_IDLE;
}

