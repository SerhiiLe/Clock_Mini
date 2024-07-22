/*
    Автономный прогноз погоды
*/

#include "forecaster.h"
#include "defines.h"
#include "barometer.h"
#include "rtc.h"
#include "webClient.h"

/*
https://github.com/GyverLibs/Forecaster
*/

Forecaster frc;

// void save_forecaster_data() {
// 	uint8_t csum = rtcWriteBlock(1, (uint8_t*)cond.dataDump(), sizeof(allForecasterData));
// 	rtcSetByte(0, csum);
// }

// обновление данных для предсказателя погоды, должно вызываться каждые пол часа.
void forecaster_update_data() {
	tm t = getTime();
	frc.setMonth(t.tm_mon+1);

	if(getPressure()>0) // если есть аппаратный датчик, то брать с него
		frc.addP(getPressure(), getTemperature());
	else if(ws.weather) // если нет аппаратного, но настроен интернет, брать с него
		frc.addP(weatherGetPressure(), weatherGetTemperature());
	else return; // или ничего не делать...

	uint8_t csum = rtcGetByte(0);
	LOG(printf_P, PSTR("read csum: %u\n"), csum);

	// сохранить дамп данных на случай перезагрузки
	// uint8_t 
	csum = rtcWriteBlock(1, (uint8_t*)frc.dataDump(), sizeof(allForecasterData));
	rtcSetByte(0, csum);
	LOG(println, PSTR("forecaster data update"));
	LOG(printf_P, PSTR("write csum: %u\n"), csum);
}

void forecaster_restore_data() {
	allForecasterData buf;
	uint8_t csum_real = rtcReadBlock(1, (uint8_t*)&buf, sizeof(allForecasterData));
	uint8_t csum = rtcGetByte(0);
	LOG(printf_P, PSTR("start read csum: %u, real %u\n"), csum, csum_real);
	uint32_t next = 0;
	if(csum_real == csum) {	// в памяти данные похожие на корректные
		// если последнее обновление меньше, чем 30 минут назад, то вычислить время следующего обновления
		int time_diff = getTimeU() - buf.last;
		if(time_diff < 1800) next = time_diff;
		// если больше 3 часа, то провести обычную процедуру старта
		if(time_diff > 10800) buf.start = false;
		// восстановить данные
		frc.restoreDump(&buf);
		LOG(println, PSTR("forecaster data restored"));
	} else {
		frc.setH(40);
		LOG(println, PSTR("new forecaster data"));
	}
	// датчик BMP180 выходит на рабочий режим примерно через 5 минут
	if(next < 300) next=300;
	forecasterTimer.setNext(next * 1000U);
	LOG(printf_P, PSTR("next forecaster update in %i sec\n"), next);
}

void forecaster_get_result(int16_t& trend, int8_t& cast) {
	trend = frc.getTrend();
	cast = static_cast<int8_t>(frc.getCast());
}