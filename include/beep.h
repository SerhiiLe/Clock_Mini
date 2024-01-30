#ifndef beep_h
#define beep_h

void beep_init();
void beep_start(uint8_t melody_number=0, bool loop=false);
void beep_stop();
void beep_process();

extern bool fl_beep_active;
extern uint8_t melody_count;

#endif