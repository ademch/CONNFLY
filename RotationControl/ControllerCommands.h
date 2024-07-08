
// JOG is fast operation
// ROTATE is long operation

#define COMMAND_ENABLE			"ENBLE\n"
#define COMMAND_DISABLE			"DSBLE\n"
#define COMMAND_ROTATE			"ROTATE %.3f %d\n"   // speed[1.8deg/min] steps[1.8deg]
#define COMMAND_ROTATE_STOP		"RSTOP\n"
#define COMMAND_ROTATE_STOPHARD	"RSTPHARD\n"
#define COMMAND_GET_STATE		"GSTAT\n"
#define COMMAND_SAMPLE_PULSES	"USEPULSES\n"
#define COMMAND_SAMPLE_ENCODER	"USENCODER\n"


// response has to be strictly 5 bytes
#define RESPONSE_OK            "OK"
#define RESPONSE_ERROR         "ERRO"
#define RESPONSE_ERROR_HSPEED  "ERRO_HSPEED"
#define RESPONSE_NOTSUP        "NOT_SUPPORTED"

#define RESPONSE_STATE_HELLO   "EHLO"
#define RESPONSE_STATE_IDLE    "IDLE"	// followed by number of pulses
#define RESPONSE_STATE_ROTT    "ROTT"	// followed by number of pulses
#define RESPONSE_STATE_ALARM   "ALRM"
