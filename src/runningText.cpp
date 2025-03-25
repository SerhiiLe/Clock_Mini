// работа с бегущим текстом

#include <Arduino.h>

#include "runningText.h"
#include "fontsV.h"
#include "defines.h"
#include "leds_max.h"

// **************** НАСТРОЙКИ ****************
#define LET_WIDTH 5       // ширина буквы шрифта
#define LET_HEIGHT 8      // высота буквы шрифта
#define SPACE 1           // пробел

#define MAX_LENGTH 1024
// --------------------- ДЛЯ РАЗРАБОТЧИКОВ ----------------------

int16_t currentOffset = LEDS_IN_ROW;
uint8_t _currentColor = 1;

char _runningText[MAX_LENGTH]; // текст, который будет крутиться
bool runningMode; // режим: true - разово вывести то, что поместилось., false - прокрутить текст

// ------------- СЛУЖЕБНЫЕ ФУНКЦИИ --------------

// Отрисовка буквы с учётом выхода за край экрана
// letter - буква, которую надо отобразить
// offset - позиция на экране. Может быть отрицательной, если буква уже уехала или больше ширины, если ещё не доехала
// color - режим цвета (1 - обычный, 0 - инверсия)
int16_t drawLetter(uint32_t letter, int16_t offset, uint8_t color) {
	uint16_t cn = 0;
	uint8_t metric;
	const uint8_t* pointer;

	// костыль для отрисовки нестандартными шрифтами
	if(letter >= 1 && letter <= 9) { // заменители двоеточия
		metric = 0x84;
		pointer = fontSemicolon[letter - 1];
		goto m1; // пропустить проверку на стандартные шрифты
	}
	else if( letter == 0x7f ) { // заменитель пробела
		letter = 32;
	}

	// определение номера буквы в массиве шрифта
	if( letter < 0x7f ) // для английских букв и символов
		cn = letter-32;
	else if( letter >= 0xd090 && letter <= 0xd0bf ) // А-Яа-п (utf-8 символы идут не по порядку, надо собирать из кусков)
		cn = letter - 0xd090 + 95;
	else if( letter >= 0xd180 && letter <= 0xd18f ) // р-я
		cn = letter - 0xd180 + 143;
	else if( letter == 0xd081 ) // Ё
		cn = 159;
	else if( letter == 0xd191 ) // ё
		cn = 160;
	else if( letter >= 0xd084 && letter <= 0xd087 ) // Є-Ї
		cn = letter - 0xd084 + 161;
	else if( letter >= 0xd194 && letter <= 0xd197 ) // є-ї
		cn = letter - 0xd194 + 165;
	else if( letter == 0xd290 || letter == 0xd291 ) // Ґґ
		cn = letter - 0xd290 + 169;
	else if( letter == 0xc2b0 ) // °
		cn = 171;
	else if( letter == 0xc2ab || letter == 0xc2bb || (letter >= 0xe2809c && letter <= 0xe2809f) ) // "
		cn = 2;
	else if( letter >= 0xe28098 && letter <= 0xe2809b ) // '
		cn = 7;
	else if( letter >= 0xe28090 && letter <= 0xe28095 ) // -
		cn = 13;
	else if( letter == 0xe280a6 ) // ...
		cn = 172;
	else if( letter == 0xc2a0 ) // "NO-BREAK SPACE"
		cn = 0;
	else if( letter == 0xe28496 ) // №
		cn = 3; // 3 - # или 46 - N
	else
		cn = 162; // символ не найден, вывести пустой прямоугольник

	// с номером буквы в массиве шрифта определились, получаем указатель на неё и метрику
	metric = pgm_read_byte(&(fontVar[cn][LET_WIDTH]));
	pointer = fontVar[cn];

	m1:

	uint8_t LW = metric & 0xF; // ширина буквы
	uint8_t LH = metric >> 4; // высота буквы
	if (LH > LEDS_IN_COL) LH = LEDS_IN_COL;

	// отрисовка буквы
	drawChar(pointer, offset, TEXT_BASELINE, LW, LH, color);

	return LW;
}

// отрисовка содержимого экрана с учётом подготовленных данных:
// runningMode: true - разовый вывод, false - прокрутка текста
// _runningText: буфер который надо отобразить
// currentOffset: позиция с которой надо отобразить
// screenIsFree: отрисовка завершена, экран свободен для нового задания
void drawString() {
	int16_t i = 0, delta = 0;
	uint32_t c;
	while (_runningText[i] != '\0' && i < MAX_LENGTH) {
		// Выделение символа UTF-8
		// 0xxxxxxx - 7 бит 1 байт, 110xxxxx - 10 бит 2 байта, 1110xxxx - 16 бит 3 байта, 11110xxx - 21 бит 4 байта
		c = (byte)_runningText[i++];
		if( c > 127  ) {
			if( c >> 5 == 6 ) {
				c = (c << 8) | (byte)_runningText[i++];
			} else if( c >> 4 == 14 ) {
				c = (c << 8) | (byte)_runningText[i++];
				c = (c << 8) | (byte)_runningText[i++];
			} else if( c >> 3 == 30 ) {
				c = (c << 8) | (byte)_runningText[i++];
				c = (c << 8) | (byte)_runningText[i++];
				c = (c << 8) | (byte)_runningText[i++];
			}
		}
		delta += drawLetter(c, currentOffset + delta, _currentColor) + SPACE;
	}

	if(runningMode) {
		screenIsFree = true;
		_runningText[0] = 0;
		// currentOffset = LEDS_IN_ROW + LET_WIDTH;
	} else {
		currentOffset--;
		if(currentOffset < -delta) { // строка убежала
			screenIsFree = true;
			_runningText[0] = 0;
			last_time_display = millis();
		}
	}
}

void initRunning(uint8_t color, int16_t posX) {
	_currentColor = color;
	runningMode = posX >= 0 && posX <= LEDS_IN_ROW;
	currentOffset = runningMode ? posX: LEDS_IN_ROW;
	if(_runningText[0]==32) currentOffset -= LET_WIDTH >> 1;
	screenIsFree = false;
	itsTinyText = false;
}
// Инициализация строки, которая будет отображаться на экране
// txt - сама строка
// color - цвет (1 - вкл. 0 - инверсия)
// posX - стартовая позиция строки, если есть, то режим без прокрутки
void initRString(const char *txt, uint8_t color, int16_t posX) {
	strncpy(_runningText, txt, MAX_LENGTH);
	initRunning(color, posX);
}
void initRString(const __FlashStringHelper *txt, uint8_t color, int16_t posX) {
	strncpy_P(_runningText, (const char*)txt, MAX_LENGTH);
	initRunning(color, posX);
}
void initRString(String txt, uint8_t color, int16_t posX) {
	txt.toCharArray(_runningText, MAX_LENGTH);
	initRunning(color, posX);
}
