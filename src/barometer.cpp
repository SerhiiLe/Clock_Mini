/*
	Опрос барометра/термометра BMP
	это самый дешёвый модуль BMP180 (BMP085 почти тоже, но без линейного стабилизатора 3.3V)

	Имеет неприятное свойство саморазогревания, из-за чего завышает температуру.
*/

#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include "defines.h"
#include "forecaster.h"

Adafruit_BMP085 bmp;

uint8_t fl_barometerIsInit = false; // флаг наличия барометра 
float Temperature = 0.0f; // температура последнего опроса
int32_t Pressure = 0; // давление последнего опроса
unsigned long lastTempTime = 0; // время последнего опроса

bool barometer_init() {
	if( ! bmp.begin(BMP085_STANDARD)) return false;
	// if( ! bmp.begin(BMP085_ULTRALOWPOWER)) return false;
	fl_barometerIsInit = 1;
	return true;
}

int32_t getPressure(bool fl_cor) {
	if(!fl_barometerIsInit) return 0;
	return bmp.readPressure() + (fl_cor ? ws.bar_cor * 100: 0);
}

float getTemperature(bool fl_cor) {
	if(!fl_barometerIsInit) return -100.0f;
	return bmp.readTemperature() + (fl_cor ? ws.term_cor: 0);
}

const char* currentPressureTemp (char *a, bool fl_tiny) {
	if(fl_barometerIsInit) {
		if(millis() - lastTempTime > 1000ul * ws.term_pool || lastTempTime == 0) {
			Temperature = getTemperature(false);
			Pressure = getPressure(false)/100;
			lastTempTime = millis();
		}
		float t = Temperature + ws.term_cor;
		int32_t p = Pressure + ws.bar_cor;
		char ft[100];
		ft[0] = 0;
		if(ws.forecast) {
			int16_t trend = forecaster_getTrend();
			int8_t cast = forecaster_getCast();
			if(fl_tiny)
				sprintf_P(ft, PSTR("\n%+i %i"), trend, cast);
			else
				sprintf_P(ft, PSTR(" trend:%+i cast:%i"), trend, cast);
		}
		if(fl_tiny)
		sprintf_P(a, PSTR(" %+1.1f\xc2\xb0\x43\n%4i hPa%s"), t, p, ft);
		else
		sprintf_P(a, PSTR("%+1.1f\xc2\xb0\x43 %i hPa%s"), t, p, ft);
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

