/*
	Опрос барометра/термометра BMP
	это самый дешёвый модуль BMP180 (BMP085 почти тоже, но без линейного стабилизатора 3.3V)

	Имеет неприятное свойство саморазогревания, из-за чего завышает температуру.
*/

#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include "defines.h"

Adafruit_BMP085 bmp;

bool fl_barometer = false; // флаг наличия барометра 
float Temperature = 0.0f; // температура последнего опроса
int32_t Pressure = 0; // давление последнего опроса
unsigned long lastTempTime = 0; // время последнего опроса

bool barometer_init() {
	if( ! bmp.begin(BMP085_STANDARD)) return false;
	// if( ! bmp.begin(BMP085_ULTRALOWPOWER)) return false;
	fl_barometer = true;
	return true;
}

int32_t getPressure() {
	if(!fl_barometer) return 0;
	return bmp.readPressure() + gs.bar_cor + (gs.bar_cor * 100);
}

float getTemperature() {
	if(!fl_barometer) return -100.0f;
	return bmp.readTemperature() + gs.term_cor;
}

const char* currentPressureTemp (char *a, bool fl_tiny) {
	if(fl_barometer) {
		if(millis() - lastTempTime > 1000ul * gs.term_pool || lastTempTime == 0) {
			Temperature = bmp.readTemperature() + gs.term_cor;
			Pressure = bmp.readPressure()/100 + gs.bar_cor;
			lastTempTime = millis();
		}
		if(fl_tiny)
		sprintf_P(a, PSTR(" %+1.1f\xc2\xb0\x43\n%4i hPa"), Temperature, Pressure);
		else
		sprintf_P(a, PSTR("%+1.1f\xc2\xb0\x43 %i hPa"), Temperature, Pressure);
		return a;
	}
	sprintf_P(a, PSTR("unknown"));
	return a;
}


/*
Serial.print("Temperature = ");
Serial.print(bmp.readTemperature());
Serial.println(" *C");

Serial.print("Pressure = ");
Serial.print(bmp.readPressure());
Serial.println(" Pa");

// Calculate altitude assuming 'standard' barometric
// pressure of 1013.25 millibar = 101325 Pascal
Serial.print("Altitude = ");
Serial.print(bmp.readAltitude());
Serial.println(" meters");

Serial.print("Pressure at sealevel (calculated) = ");
Serial.print(bmp.readSealevelPressure());
Serial.println(" Pa");

// you can get a more precise measurement of altitude
// if you know the current sea level pressure which will
// vary with weather and such. If it is 1015 millibars
// that is equal to 101500 Pascals.
Serial.print("Real altitude = ");
Serial.print(bmp.readAltitude(101500));
Serial.println(" meters");

Serial.println();
*/