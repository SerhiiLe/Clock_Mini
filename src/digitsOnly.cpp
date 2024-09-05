/*
Отрисовка только части циферблата средним по размеру шрифтом 8x4, в комбинации с 5x3 шрифтом даёт крупные часи с секундами.
Только цифры и разделители двоеточие и пробел
*/

#include <Arduino.h>
#include "defines.h"
#include "leds_max.h"
#include "digitsOnly.h"
#include "ntp.h"

const byte fontSemicolon[][4] PROGMEM = {
	{0x00, 0x22, 0x44, 0x00}, // : 1 1st 0
	{0x00, 0x44, 0x22, 0x00}, // : 2 2st 1
	{0x00, 0x20, 0x04, 0x00}, // : 3 1st 2
	{0x00, 0x04, 0x20, 0x00}, // : 4 2st 3
	{0x00, 0x46, 0x46, 0x00}, // : 5 1st 4
	{0x00, 0x4A, 0x4A, 0x00}, // : 6 2st
	{0x00, 0x52, 0x52, 0x00}, // : 7 3 st
	{0x00, 0x62, 0x62, 0x00}, // : 8 4st 7
	{0x00, 0x66, 0x66, 0x00} // : 9 ":"
};

const byte fontSpace[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ' ' 32 0

const byte fontHight[][5] PROGMEM = {
	{0x7E, 0x81, 0x81, 0x81, 0x7E}, // 0 48
	{0x00, 0x82, 0xFF, 0x80, 0x00}, // 1
	{0x82, 0xC1, 0xA1, 0x91, 0x8E}, // 2
	{0x41, 0x81, 0x89, 0x95, 0x63}, // 3
	{0x30, 0x2C, 0x22, 0xFF, 0x20}, // 4
	{0x4F, 0x89, 0x89, 0x89, 0x71}, // 5
	{0x7C, 0x92, 0x91, 0x91, 0x60}, // 6
	{0x01, 0xF1, 0x09, 0x05, 0x03}, // 7
	{0x76, 0x89, 0x89, 0x89, 0x76}, // 8
	{0x0E, 0x91, 0x91, 0x51, 0x3E}, // 9 57
	{0x00, 0x66, 0x66, 0x00, 0x00}  // : 58
};
const byte fontBold[][6] PROGMEM = {
	{0x7E, 0xFF, 0x81, 0x81, 0xFF, 0x7E}, // 0 48
	{0x00, 0x84, 0xFE, 0xFF, 0x80, 0x00}, // 1
	{0xC2, 0xE3, 0xB1, 0x99, 0x8F, 0x86}, // 2
	{0x41, 0xC1, 0x89, 0x9D, 0xFF, 0x63}, // 3
	{0x38, 0x3C, 0x26, 0x23, 0xFF, 0xFF}, // 4
	{0x4F, 0xCF, 0x89, 0x89, 0xF9, 0x71}, // 5
	{0x7E, 0xFF, 0x91, 0x91, 0xF1, 0x60}, // 6
	{0x01, 0x01, 0xF1, 0xF9, 0x0F, 0x07}, // 7
	{0x76, 0xFF, 0x89, 0x89, 0xFF, 0x76}, // 8
	{0x0E, 0x9F, 0x91, 0x91, 0xFF, 0x7E}, // 9 57
	{0x00, 0x66, 0x66, 0x00, 0x00, 0x00} // : 58
};
const byte fontWide[][6] PROGMEM = {
	{0x7E, 0xA1, 0x91, 0x89, 0x85, 0x7E}, // 0 48
	{0x00, 0x84, 0x82, 0xFF, 0x80, 0x80}, // 1
	{0x82, 0xC1, 0xA1, 0x91, 0x89, 0x86}, // 2
	{0x41, 0x81, 0x81, 0x89, 0x95, 0x63}, // 3
	{0x30, 0x28, 0x24, 0x22, 0xFF, 0x20}, // 4
	{0x4F, 0x89, 0x89, 0x89, 0x89, 0x71}, // 5
	{0x7C, 0x92, 0x91, 0x91, 0x91, 0x60}, // 6
	{0x01, 0xE1, 0x11, 0x09, 0x05, 0x03}, // 7
	{0x76, 0x89, 0x89, 0x89, 0x89, 0x76}, // 8
	{0x0E, 0x91, 0x91, 0x91, 0x51, 0x3E}, // 9 57
	{0x00, 0x66, 0x66, 0x00, 0x00, 0x00} // : 58
};
const byte fontMediumScript[][4] PROGMEM = {
	{0x7E, 0x81, 0x81, 0x7E}, // 0 48
	{0x00, 0x82, 0xFF, 0x80}, // 1
	{0xC2, 0xA1, 0x91, 0x8E}, // 2
	{0x42, 0x81, 0x89, 0x76}, // 3
	{0x0F, 0x10, 0x10, 0xFF}, // 4
	{0x4F, 0x89, 0x89, 0x71}, // 5
	{0x7C, 0x92, 0x91, 0x60}, // 6
	{0x01, 0xF1, 0x09, 0x07}, // 7
	{0x76, 0x89, 0x89, 0x76}, // 8
	{0x0E, 0x91, 0x51, 0x3E}, // 9 57
	{0x00, 0x24, 0x00, 0x00}  // : 58
};
const byte fontMediumScript2[][4] PROGMEM = {
	{0x3E, 0x41, 0x41, 0x3E}, // 0 48
	{0x00, 0x42, 0x7F, 0x40}, // 1
	{0x62, 0x51, 0x49, 0x46}, // 2
	{0x22, 0x41, 0x49, 0x36}, // 3
	{0x07, 0x08, 0x08, 0x7F}, // 4
	{0x4F, 0x49, 0x49, 0x31}, // 5
	{0x3E, 0x49, 0x49, 0x30}, // 6
	{0x01, 0x79, 0x05, 0x03}, // 7
	{0x36, 0x49, 0x49, 0x36}, // 8
	{0x06, 0x49, 0x49, 0x3E}, // 9 57
	{0x00, 0x24, 0x00, 0x00}  // : 58
};
const byte fontMediumDigit[][4] PROGMEM = {
	{0x7F, 0x41, 0x41, 0x7F}, // 0 48
	{0x00, 0x00, 0x7F, 0x00}, // 1
	{0x79, 0x49, 0x49, 0x4F}, // 2
	{0x41, 0x49, 0x49, 0x7F}, // 3
	{0x0F, 0x08, 0x08, 0x7F}, // 4
	{0x4F, 0x49, 0x49, 0x79}, // 5
	{0x7F, 0x49, 0x49, 0x79}, // 6
	{0x01, 0x01, 0x01, 0x7F}, // 7
	{0x7F, 0x49, 0x49, 0x7F}, // 8
	{0x4F, 0x49, 0x49, 0x7F}, // 9 57
	{0x00, 0x22, 0x00, 0x00}  // : 58
};
const byte fontMediumDigit2[][4] PROGMEM = {
	{0x7F, 0x7F, 0x41, 0x7F}, // 0 48
	{0x44, 0x7E, 0x7F, 0x40}, // 1
	{0x7B, 0x7B, 0x49, 0x4F}, // 2
	{0x63, 0x6B, 0x49, 0x7F}, // 3
	{0x0F, 0x0F, 0x08, 0x7F}, // 4
	{0x6F, 0x6F, 0x49, 0x79}, // 5
	{0x7F, 0x7F, 0x49, 0x79}, // 6
	{0x03, 0x03, 0x01, 0x7F}, // 7
	{0x7F, 0x7F, 0x49, 0x7F}, // 8
	{0x6F, 0x6F, 0x49, 0x7F}, // 9 57
	{0x00, 0x22, 0x00, 0x00}  // : 58
};

// отрисовка одной буквы нестандартным шрифтом
int16_t drawMedium(const char c, int16_t x) {
	byte dots;
	uint8_t cn = 0, cw = 4;
	uint8_t fontWidth = 4;
	byte* font;
	if(c >= 1 && c <= 9) {
		// заменители для двоеточия
		cn = c - 1;
		font = (byte*)fontSemicolon;
	} else if(c == 0x7f) {
		// заменитель пробела, для мигания двоеточия, 4 в широких шрифтах и 3 в узких
		font = (byte*)fontSpace;
		if(gs.tiny_clock >= FONT_NARROW) cw = 3;
	} else if(c == ' ') {
		// реальный пробел, 4 в широких шрифтах и 1 в узких
		font = (byte*)fontSpace;
		if(gs.tiny_clock >= FONT_NARROW) cw = 1;
	} else {
		// сопоставление ascii кода символа и номера в таблице символов
		if(c >= '0' && c <= ':') // 0-9: maps to 0-10
			cn = c - '0';
		if(c == ':') cw = 3;
		// выбор шрифта
		switch (gs.tiny_clock) {
			case FONT_HIGHT:
				font = (byte*)fontHight;
				fontWidth = 5;
				cw = c == ':' ? 4: 5;
				break;
			case FONT_BOLD:
			case FONT_WIDE:
				font = gs.tiny_clock == FONT_BOLD ? (byte*)fontBold: (byte*)fontWide;
				fontWidth = 6;
				cw = c == ':' ? 4: 6;
				break;
			case FONT_NARROW:
				font = (byte*)fontMediumScript;
				break;
			case FONT_NARROW2:
				font = (byte*)fontMediumScript2;
				break;
			case FONT_DIGIT:
				font = (byte*)fontMediumDigit;
				break;
			case FONT_DIGIT2:
				font = (byte*)fontMediumDigit2;
				break;
			default:
				return 0;
				break;
		}
	} 

	for(uint8_t col = 0; col < cw; col++) {
		if(col + x > LEDS_IN_ROW) return cw;
		dots = pgm_read_byte(font + cn * fontWidth + col);
		for(uint8_t row = 0; row < 8; row++) {
			if(row >= 0 && row < LEDS_IN_COL)
				drawPixelXY(x + col, row, dots & (1 << row) ? 1: 0);
		}
	}
	return cw;
}

// отрисовка циферблата нестандартными шрифтами
int16_t printMedium(const char* txt, int16_t pos, uint8_t limit) {
	clearALL();
	int16_t i = 0;
	if( txt[0]==' ' ) pos += gs.tiny_clock==FONT_BOLD || gs.tiny_clock==FONT_WIDE ? -1:
		gs.tiny_clock==FONT_HIGHT ? -2: 1;
	while (txt[i] != '\0' && i<limit) {
		pos += drawMedium(txt[i++], pos) + 1;
	}
	return pos;
}

// изменение символа ":" между часами и минутами на другой стиль
const char* changeDots(char* txt) {
	static uint8_t stage = 1;
	switch (gs.dots_style) {
		case 1: // Обычный 1
			txt[2] = stage & 2 ? ':': ' ';
			break;
		case 2: // Качели 0.5
			txt[2] = txt[2] == ':' ? 1: 2;
			break;
		case 3: // Качели 1
			txt[2] = stage & 2 ? 1: 2;
			break;
		case 4: // Биение 0.5
			txt[2] = txt[2] == ':' ? 3: 4;
			break;
		case 5: // Биение 1
			txt[2] = stage & 2 ? 3: 4;
			break;
		case 6: // Семафор 0.5
			txt[2] = txt[2] == ':' ? 5: 8;
			break;
		case 7: // Семафор 1
			txt[2] = stage & 2 ? 5: 8;
			break;
		case 8: // Капля
			txt[2] = stage + 4;
			break;
		case 9: // Четверть
			txt[2] = txt[2] == ':' ? getTime().tm_sec/15 + 5: 0x7f;
			break;
		case 10: // Мозаика
			txt[2] = stage == 1 ? 9:
					stage == 2 ? 1: 
					stage == 3 ? 9: 2;
			break;
		case 11: // Статичный
			txt[2] = ':';
			break;
	}
	stage++;
	if(stage>4) stage = 1;
	return txt;
}