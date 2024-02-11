/*
	Работа с модулем часов.
	Модули бывают разные, по этому блок выделен отдельно, чтобы было удобно менять под разные платы
*/
#include <Arduino.h>
#include <RTClib.h>
#include <sys/time.h>
#include "defines.h"
#include "rtc.h"

RTC_DS1307 rtc;

bool rtc_enable = false;

/*
У чипа DS1307 есть проблема в том, что счётчик времени имеет только 32 бита, что приводит к проблеме 38-го года, когда наступит переполнение.
Ждать 38-го года не интересно, по этому в коде сразу делается поправка и время считается с 10-го января 2020-го года, таким образом
превращая проблему 38-го года в проблему 88-го года.
*/

uint8_t rtc_init() {
	if( ! rtc.begin()) return 0;
	rtc_enable = true;
	if( ! rtc.isrunning()) {
		// плата rtc после смены батарейки или если была остановлена не работает.
		// этот кусок запускает отсчёт времени датой компиляции прошивки. 
		LOG(println, PSTR("RTC is NOT running, let's set the time!"));
		// When time needs to be set on a new device, or after a power loss, the
		// following line sets the RTC to the date & time this sketch was compiled
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
		// This line sets the RTC with an explicit date & time, for example to set
		// January 21, 2014 at 3am you would call:
		// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
		return 2;
	}
	// часы запущены, установка времени системы по RTC
	rtc_setSYS();
	return 1;
}

// Установка времени системы по значениям из RTC
void rtc_setSYS() {
	if( ! rtc_enable ) return;
	DateTime now = rtc.now();
/*
	// код для получения unixtime из отдельных частей даты
	struct tm tm;
	tm.tm_hour = now.hour();
	tm.tm_min = now.minute();
	tm.tm_year = now.year() - 1900;
	tm.tm_mon = now.month() - 1;
	tm.tm_mday = now.day();
	tm.tm_sec = now.second();
	tm.tm_isdst = tz_dst;
	time_t t2 = mktime(&tm);
	LOG(printf_P,"RTC time (old): %llu\n",t2);
*/
	time_t t = now.unixtime();
	LOG(printf_P, PSTR("RTC time: %lli\n"), t);
	t += (gs.tz_shift * 3600) + (gs.tz_dst * 3600); 
	// set the system time
	timeval tv = { t, 0 };
	settimeofday(&tv, nullptr);
}

// запись времени системы в RTC
void rtc_saveTIME(time_t t) {
	if( ! rtc_enable ) return;
	DateTime now = rtc.now();
	// записать новое время только если оно не совпадает с текущим в RTC
	if(t != now.unixtime()) {
		rtc.adjust(DateTime(t));
		LOG(printf_P, PSTR("adjust RTC from %lli to %lli\n"), now.unixtime(), t);
	}
}

time_t getRTCTimeU() {
	if( ! rtc_enable ) return 0;
	DateTime now = rtc.now();
	return now.unixtime();
}

/*
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
*/
