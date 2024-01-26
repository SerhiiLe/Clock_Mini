#ifndef settings_init_h
#define settings_init_h

#include "defines.h"


Global_Settings gs;

cur_alarm alarms[MAX_ALARMS];
cur_text texts[MAX_RUNNING];

uint8_t sec_enable = 0;
uint8_t sec_curFile = 0;

#endif