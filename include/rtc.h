#ifndef rtc_h
#define rtc_h

uint8_t rtc_init();
void rtc_setSYS();
void rtc_saveTIME(time_t t);
time_t getRTCTimeU();

#endif