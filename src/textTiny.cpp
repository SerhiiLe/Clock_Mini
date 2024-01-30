/*
Вывод циферблата или других надписей крошечным и корявым шрифтом 3х5
*/

#include <Arduino.h>
#include "defines.h"
#include "leds_max.h"
#include "textTiny.h"
#include "fontTini.h"

#define MAX_LENGTH 256	// максимальная длина пачки слайдов
#define SPACE 1			// отступ между буквами

char _outText[MAX_LENGTH]; // текст, который будет выводится в виде слайдов
int16_t _baseX = 0, _baseY = 0;
unsigned long _prevDraw = 0; // время последнего обновления экрана
bool _oneSlide = true; // вывести слайд без ожидания времени показа слайда
int16_t _curPosition = 0; // текущая позиция в тексте
int16_t _lastPosition = 0; // последняя позиция в тексте
int16_t _curY = 0; // текущее смещение по Y

int16_t drawTinyLetter(int16_t x, int16_t y, uint32_t c) {
	byte dots;
	uint8_t cn = 0;
	uint8_t fw = 3;
	if( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ) // A-Z maps to 1-26
		cn = c & 0x1F;
	else if (c >= 0x21 && c <= 0x40)
		cn = (c - 0x21) + 27;
	else if (c == ' ') // space
		cn = 0;
	else if (c>=0xd090 && c<=0xd0af) // А-Я
		cn = (c - 0xd090) + 59;
	else if (c>=0xd0b0 && c<=0xd0bf) // а-п
		cn = (c - 0xd0b0) + 59;
	else if (c>=0xd180 && c<=0xd18f) // р-я
		cn = (c - 0xd180) + 75;
	else if (c==0xd081 || c==0xd191) // Ёё
		cn = 64;
	else if (c==0xd084 || c==0xd194) // Єє
		cn = 91;
	else if (c==0xd086 || c==0xd196) // Іі
		cn = 9;
	else if (c==0xd087 || c==0xd197) // Її
		cn = 92;
	else if (c==0xd290 || c==0xd291) // Ґґ
		cn = 93;
	else if (c==0xb0) // °
		cn = 94;

	if( cn==27 || cn==33 || cn==38 || cn==40 || cn==52 || cn==53 )
		fw = 1;
	if( cn==34 || cn==35 || cn==0 )
		fw = 2;

	for(uint8_t col = 0; col < fw; col++) {
		if(col + x > LEDS_IN_ROW) return fw;
		dots = pgm_read_byte(&fontTiny[cn][col]);
		for(char row = 0; row < 5; row++) {
			if(y + row >= 0 && y + row < LEDS_IN_COL)
				drawPixelXY(x + col, y + row, dots & (1 << row) ? 1: 0);
		}
	}
	return fw;
}

bool drawSlide() {
	int16_t i = _curPosition, delta = 0;
	uint32_t c;
	bool fl_draw = false;
	if(_curY <= _baseY) {
		for(int16_t x=0; x<LEDS_IN_ROW; x++) {
			for(int16_t y=LEDS_IN_COL-1; y>0 || y>_curY+5; y--)
				drawPixelXY(x,y,getPixColorXY(x,y-1));
		}
		for(int16_t x=0; x<LEDS_IN_ROW; x++)
			drawPixelXY(x,0,0);
	}
	while (_outText[i] != '\0' && i < MAX_LENGTH && _outText[i] != '\n' && _curY <= _baseY) {
		// Выделение символа UTF-8
		// 0xxxxxxx - 7 бит 1 байт, 110xxxxx - 10 бит 2 байта, 1110xxxx - 16 бит 3 байта, 11110xxx - 21 бит 4 байта
		c = (byte)_outText[i++];
		if( c > 127  ) {
			if( c >> 5 == 6 ) {
				c = (c << 8) | (byte)_outText[i++];
			} else if( c >> 4 == 14 ) {
				c = (c << 8) | (byte)_outText[i++];
				c = (c << 8) | (byte)_outText[i++];
			} else if( c >> 3 == 30 ) {
				c = (c << 8) | (byte)_outText[i++];
				c = (c << 8) | (byte)_outText[i++];
				c = (c << 8) | (byte)_outText[i++];
			}
		}
		delta += drawTinyLetter(_baseX + delta, _curY, c) + SPACE;
		fl_draw = true;
	}

	if(_oneSlide) {
		screenIsFree = true;
		return true;
	}

	if(fl_draw) {
		_prevDraw = millis();
		_curY++;
		_lastPosition = i;
	}

	if(millis() - _prevDraw < 1000ul * gs.slide_show) return false;

	if(_outText[_lastPosition] != 0 && _outText[_lastPosition+1] != 0) {
		_curPosition = _lastPosition+1;
		_curY = _baseY - LEDS_IN_COL;
	} else {
		screenIsFree = true;
		return true;
	}

	return false;
}


void printTinyText(const char *txt, int16_t posX, bool instant) {
	strncpy(_outText, txt, MAX_LENGTH);
	_baseX = posX;
	if(instant) {
		if(_outText[0] == ' ') _baseX -= 1;
		if(_outText[0] == '1') _baseX -= 1;
	}
	_baseY = 1;
	_curY = instant ? _baseY: _baseY - LEDS_IN_COL;
	_curPosition = 0;
	itsTinyText = true;
	screenIsFree = false;
	_oneSlide = instant;
}