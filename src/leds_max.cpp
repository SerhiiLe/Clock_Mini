// разные функции для работы с LED матрицей

#include <Arduino.h>
#include "defines.h"
#include "leds_max.h"
#include "runningText.h"
#include "textTiny.h"
#include <max72xxPanelMini.h>

// работа в режиме эмуляции SPI, чтобы освободить пин MISO (PIO12/D6) для пищалки. Мало свободных выходов :(
// при старте все пины кроме аппаратных I2C и SPI подтянуты к + или земле и выдают сигнал, по этому чтобы не пищало при старте приходится использовать MISO
MAX7219<SEG_IN_ROW, SEG_IN_COL, PIN_SS_MATRIX, PIN_SPI_MOSI, PIN_SPI_SCK> mtrx;

bool screenIsFree = true; // экран свободен (текст полностью прокручен, слайды выведены)
bool itsTinyText = false; // сейчас время выводить слайды (tiny) вместо бегущей строки

// залить всё
void fillAll() {
	mtrx.fill();
}

// очистить всё
void clearALL() {
	mtrx.clear();
}

// отрисовать буфер сразу
void showALL(bool clear) {
	mtrx.update();
	if( clear ) mtrx.clear();
}

// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, uint8_t color) {
	if (x < 0 || x > LEDS_IN_ROW - 1 || y < 0 || y > LEDS_IN_COL - 1) return;
	if(gs.turn_display & 1) x = LEDS_IN_ROW - x - 1;
	if(gs.turn_display & 2) y = LEDS_IN_COL - y - 1;
	mtrx.dot(x, y, color);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(int8_t x, int8_t y) {
	if (x < 0 || x > LEDS_IN_ROW - 1 || y < 0 || y > LEDS_IN_COL - 1) return 0;
	if(gs.turn_display & 1) x = LEDS_IN_ROW - x - 1;
	if(gs.turn_display & 2) y = LEDS_IN_COL - y - 1;
	return mtrx.get(x ,y);
}

uint8_t led_brightness = 0;
void set_brightness(uint8_t br) {
	if(br != led_brightness) {
		LOG(printf_P, PSTR("new led_brightness = %i\n"), br);
		mtrx.setBright(br);
		led_brightness = br;
	}
}

void display_setup() {
    mtrx.begin();       // запускаем
    set_brightness(1);  // яркость 0..15
	#if(MATRIX_TYPE!=0)
	mtrx.setType(MATRIX_TYPE);
	#endif
	#if(MATRIX_FLIP_X || MATRIX_FLIP_Y)
	mtrx.setFlip(MATRIX_FLIP_X, MATRIX_FLIP_Y);
	#endif
	#if(MATRIX_ROTATION!=0)
	mtrx.setRotation(MATRIX_ROTATION);
	#endif
	#if(MATRIX_CONNECTION!=0)
	mtrx.setConnection(MATRIX_CONNECTION);
	#endif
}

void display_tick(bool clear) {
	if(screenIsFree) return;
	bool fl_clean = true;
	if(itsTinyText) fl_clean = drawSlide();
	else drawString();
	if(!fl_allowLEDS && alarmStartTime == 0) mtrx.clear();
	if(cur_motion && gs.show_move) drawPixelXY(LEDS_IN_ROW - 1, LEDS_IN_COL - 1);
	mtrx.update();
	// очистить буфер для заполнения новыми данными, чтобы не накладывались кадры
	if( clear && fl_clean ) mtrx.clear();
}
