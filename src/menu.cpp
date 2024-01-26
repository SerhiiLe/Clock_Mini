#include <Arduino.h>
#include "leds_max.h"
#include "runningText.h"
#include "menu.h"
#include "rtc.h"
#include "ntp.h"
#include "defines.h"

#define POS_1 2     // позиция первой цифры (часы)
#define POS_2 19    // позиция второй цифры (минуты)
#define POS_C 14	// позиция раздельного двоеточия (время)
#define POS_D 15	// позиция точки (дата)

// структура времени, которая будет меняться через меню
tm wt;

// правила отображения/изменения текущего показания
struct {
	uint8_t val = 0;
	uint8_t min = 0;
	uint8_t max = 0;
	uint8_t pos = 0;
} cur_d;

// что сейчас меняется
// 0 - часы
// 1 - минуты
// 2 - день
// 3 - месяц
// 4 - год
uint8_t cur_edit = 0;

// флаг того, что есть что менять, иначе время не трогать
bool fl_menuChanged = false;

const char two_digit[] PROGMEM = "%02u";

void show_d(uint8_t d, uint8_t color, uint8_t pos) {
	char s[6];
	sprintf_P(s, two_digit, d);
	initRString(s, color, pos);
	display_tick(false);
}

void show_screen(uint8_t cur_edit) {
	char s[11];
	switch (cur_edit) {
		case 0: // выбраны часы
			clearALL();
			cur_d.val = wt.tm_hour;
			cur_d.min = 0;
			cur_d.max = 23;
			cur_d.pos = POS_1;
			show_d(wt.tm_hour, 0, cur_d.pos);
			initRString(":", 1, POS_C);
			display_tick(false);
			show_d(wt.tm_min, 1, POS_2);
			break;
		case 1: // выбраны минуты
			clearALL();
			cur_d.val = wt.tm_min;
			cur_d.min = 0;
			cur_d.max = 59;
			cur_d.pos = POS_2;
			show_d(wt.tm_hour, 1, POS_1);
			initRString(":", 1, POS_C);
			display_tick(false);
			show_d(wt.tm_min, 0, cur_d.pos);
			break;
		case 2: // день
			clearALL();
			cur_d.val = wt.tm_mday;
			cur_d.min = 1;
			cur_d.max = 31;
			cur_d.pos = POS_1;
			show_d(cur_d.val, 0, cur_d.pos);
			initRString(".", 1, POS_D);
			display_tick(false);
			show_d(wt.tm_mon + 1, 1, POS_2);
			break;
		case 3: // месяц
			clearALL();
			cur_d.val = wt.tm_mon + 1;
			cur_d.min = 1;
			cur_d.max = 12;
			cur_d.pos = POS_2;
			show_d(wt.tm_mday, 1, POS_1);
			initRString(".", 1, POS_D);
			display_tick(false);
			show_d(cur_d.val, 0, cur_d.pos);
			break;
		case 4: // год
			clearALL();
			cur_d.val = wt.tm_year - 100;
			cur_d.min = 0;
			cur_d.max = 99;
			cur_d.pos = 18;
			sprintf_P(s, PSTR(".%04u"), wt.tm_year + 1900);
			initRString(s, 1, 2);
			display_tick(false);
			show_d(cur_d.val, 0, cur_d.pos);
			break;
	}
}

void menu_init() {
	wt = getTime();
	cur_edit = 0;
	show_screen(cur_edit);
}

void save_time() {
	if(! fl_menuChanged) return;
	wt.tm_sec = 2; // "обнуление" секунд
	time_t t = mktime(&wt);
	LOG(printf_P,"RTC time: %llu\n",t);
	// set the system time
	timeval tv = { t, 0 };
	settimeofday(&tv, nullptr);
	rtc_saveTIME(t);
	// чистка экрана, а то в начале есть артефакты
	clearALL();
}

void save_current(uint8_t sel) {
	switch( sel ) {
		case 0: // часы настроены
			wt.tm_hour = cur_d.val;
			break;
		case 1: //  минуты настроены
			wt.tm_min = cur_d.val;
			break;
		case 2: // дни настроены
			wt.tm_mday = cur_d.val;
			break;
		case 3: // месяцы настроены
			wt.tm_mon = cur_d.val - 1;
			break;
		case 4: // годы настроены
			wt.tm_year = cur_d.val + 100;
			break;
	}
}

// работа меню
bool menu_process(bool sel_hold, uint16_t sel_clicks, bool set_hold, uint16_t set_clicks) {
	if( sel_hold ) {
		save_current(cur_edit);
		save_time();
		return false;
	}
	if( sel_clicks ) {
		save_current(cur_edit++);
		if( cur_edit > 4 ) {
			save_time();
			return false;
		}
		show_screen(cur_edit);
	}
	if( set_clicks ) {
		cur_d.val = (cur_d.val + set_clicks) % (cur_d.max + 1);
		if( cur_d.val < cur_d.min ) cur_d.val = cur_d.min;
		show_d(cur_d.val, 0, cur_d.pos);
		fl_menuChanged = true;
	}

	return true;
}