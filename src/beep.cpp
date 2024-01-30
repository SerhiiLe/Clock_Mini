/*
Управление пищалкой

используется активный зумер, это удобно потому, что у esp8266 фактически нет апаратного PWM, а программная эмуляция ужасна
как следствие нельзя играть разными тонами, остаётся только "морзянка"
*/

#include <Arduino.h>
#include "defines.h"
#include "beep.h"

bool fl_beep_active = false;
uint8_t active_melody = 0;
uint8_t active_step = 0;
bool fl_infinity = false;
unsigned long start_millis = 0;
uint8_t melody_count = 0;

/*
Определение "мелодий", они же "морзянка".
Нечётные числа - длительность включения пищалки
Чётные - длительность паузы
ноль - окончание последовательности, обязательно должен быть завершающим
*/
const uint8_t ts_0[] PROGMEM = {2, 100, 0}; // редкие щелчки
const uint8_t ts_1[] PROGMEM = {10, 50, 0}; // один короткий бип
const uint8_t ts_2[] PROGMEM = {10, 50, 10, 100, 0}; // два коротких
const uint8_t ts_3[] PROGMEM = {10, 50, 10, 50, 10, 100, 0}; // три коротких
const uint8_t ts_4[] PROGMEM = {100, 200, 0}; // один длинный бип
const uint8_t ts_5[] PROGMEM = {100, 100, 100, 200, 0}; // два длинных
const uint8_t ts_6[] PROGMEM = {100, 100, 100, 100, 100, 200, 0}; // три длинных
const uint8_t ts_7[] PROGMEM = {10, 50, 10, 50, 10, 20, 10, 20, 10, 50, 10, 20, 10, 20, 10, 20, 10, 50, 10, 20, 10, 150, 0}; // непонятно что
const uint8_t ts_8[] PROGMEM = {5, 10, 0}; // быстрая трещётка

const uint8_t* melody[] PROGMEM = {ts_0, ts_1, ts_2, ts_3, ts_4, ts_5, ts_6, ts_7, ts_8};
// конец определения "мелодий"

#undef ON
#undef OFF
#if BUZZER_LOW
	#define ON 0
	#define OFF 1
#else
	#define ON 1
	#define OFF 0
#endif

// первичная настройка пищалки
void beep_init() {
	pinMode(PIN_BUZZER, OUTPUT);
	melody_count = sizeof(melody) / sizeof(uint8_t*);
}

void beep_stop() {
	fl_beep_active = false;
	digitalWrite(PIN_BUZZER, OFF);
	LOG(println,PSTR("stop tone"));
}

// основной цикл пищалки
void beep_process() {
	if(!fl_beep_active) return;
	unsigned long now = millis();
	if( now - start_millis < 10ul * pgm_read_byte(&melody[active_melody][active_step]) ) return;
	active_step++;
	if( pgm_read_byte(&melody[active_melody][active_step]) == 0 ) {
		if( fl_infinity ) active_step = 0;
		else {
			beep_stop();
			return;
		}
	}
	digitalWrite(PIN_BUZZER, active_step & 1 ? OFF: ON);
	start_millis = now;
	LOG(printf_P,PSTR("step=%u, pause=%u\n"),active_step,pgm_read_byte(&melody[active_melody][active_step]));
}

void beep_start(uint8_t melody_number, bool loop) {
	if( melody_number >= melody_count ) return;
	active_melody = melody_number;
	active_step = 0;
	fl_infinity = loop;
	start_millis = millis();
	digitalWrite(PIN_BUZZER, ON);
	fl_beep_active = true;
	LOG(println,PSTR("start tone"));
}

uint8_t beep_isPlay() {
	return fl_beep_active;
}