
#include "Arduino.h"

#define	RESPONCE_OK            "OK\n"
#define	RESPONCE_ERROR         "ERRO\n"
#define	RESPONCE_BUSY          "BUSY\n"
#define	RESPONCE_ERROR_HSPEED  "ERRO_HSPEED\n"
#define	RESPONCE_ERROR_STEPS   "ERRO_STEPS\n"
#define	RESPONCE_NOTSUPPORTED  "NOT_SUPPORTED\n"


enum ActiveState {
	STATE_EHLO = 0,
	STATE_IDLE,
	STATE_ROTT,
	STATE_ALARM
};

#define rotate_cw  true
#define rotate_ccw false

void ProcessCommand();

