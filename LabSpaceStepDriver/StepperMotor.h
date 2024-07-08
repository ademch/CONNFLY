
#include "Arduino.h"
#include "PinsConfig.h"

#define STEP_MICROSTEP            20                 // microstepts in 1.8 deg step
#define TIMER_TICKS_PERSECOND    (16000000 / 1024)

// Acceleration/decelaration profile:
//
// * Max speed of 48 pulses per second being multiplied by 1/STEP_DIVIDER gives real speed of:
//   E.g. 48*1.8/8  = 6*1.8 deg per s
//
// * Acceleration time is fixed to 1s
// * Max speed is achieved in STEP_NUMBER steps
//    24/8 = in 3*1.8 deg
//

enum MotorState { IDLE, ACC, MARCH, DECC };

class StepMotor
{
private:
	int      iStepsAcc, iStepsDec, iStepsMarch;
	int      iSeq;

	uint8_t  pin_step, pin_dir, pin_boost, pin_en;


	int      iRotationDir;

	uint16_t iACC_STEP_NUMBER;	// Number of acceleration/decceleration steps

public:

	MotorState    mState;

	unsigned int  i1_8_StepCount;
	float         fVelocity;
	bool          bCW;

	StepMotor(void(*_RotationDoneCallback)() = NULL):
   		      RotationDoneCallback(_RotationDoneCallback)
	{
		pin_step  = PIN_STEPDRIVER_STEP;
		pin_dir   = PIN_STEPDRIVER_DIR;
		pin_en    = PIN_STEPDRIVER_EN;

		iSeq   = 0;
		mState = IDLE;

		iRotationDir = +1;
		iCurPos = 0;

		iACC_STEP_NUMBER   = 0;
		i1_8_StepCount     = 0;
		bCW                = false;
	};

	int      iCurPos;		// signed

	void MarchStopPremature();
	void MarchStopHard();
	void CatchUpDecceleratingMotor();

	void MarchNSteps(unsigned int _i1_8_StepCount, float _fVelocity, bool _bCW);
//	void MarchNStepsSteady(unsigned int i1_8_StepCount, float _fVelocity, bool bCW);

	void timer_routine();	// timer compare interrupt service routine

private:
	void SendStepPulseToDrive();
	void SetTimerVal(unsigned int val);

	void (*RotationDoneCallback)();
};	


