#ifndef barometer_h
#define barometer_h

bool barometer_init();
const char* currentPressureTemp (char *a, bool fl_tiny = false);
int32_t getPressure();
float getTemperature();

#endif