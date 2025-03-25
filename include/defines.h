#ifndef defines_h
#define defines_h

#define DEBUG // разрешение отладочных сообщений. Закомментировать, если не надо.

/*** описание констант, которые описывают конкретное "железо" ***/

// Пины подписаны не по надписям на плате, а по спецификации чипа. Для каждой платы надо смотреть её pinout

#if ESP32C3 == 1 // ESP32-c3
#define PIN_SS_MATRIX 7 // пин SS для SPI матрицы
#define PIN_PHOTO_SENSOR A0 // 0 // фоторезистор
#define PIN_BUZZER 5 // пин с пищалкой
#define PIN_BUTTON_SELECT 1 // кнопка выбора режима 16 // кнопка управления
#define PIN_BUTTON_SET 3 // кнопка установки
#define PIN_MOTION 10 // детектор движения
#define PIN_SPI_MOSI 6 // SPI MOSI
#define PIN_SPI_SCK 4 // SPI SCK
#elif ESP32 == 1 // ESP32
#define PIN_SS_MATRIX 5 // пин SS для SPI матрицы
#define PIN_PHOTO_SENSOR 36 // фоторезистор
#define PIN_BUZZER 19 // пин с пищалкой
#define PIN_BUTTON_SELECT 17 // кнопка выбора режима 16 // кнопка управления
#define PIN_BUTTON_SET 16 // кнопка установки
#define PIN_MOTION 26 // детектор движения
#define PIN_SPI_MOSI 23 // SPI MOSI
#define PIN_SPI_SCK 18 // SPI SCK
#else // ESP8266
#define PIN_SS_MATRIX 15 // пин SS для SPI матрицы
#define PIN_PHOTO_SENSOR A0 // фоторезистор
#define PIN_BUZZER 12 // пин с пищалкой
#define PIN_BUTTON_SELECT 0 // кнопка выбора режима 16 // кнопка управления
#define PIN_BUTTON_SET 2 // кнопка установки
#define PIN_MOTION 16 // детектор движения
#endif

#define SENSOR_BUTTON 0 // сенсорная кнопка - 1, обычная - 0

#define USE_NVRAM 1 // использовать отдельный чип на плате RTC, вместо flash esp8266/esp32. 0 - файлы, 1 - чип NVRAM
#define USE_RTC 1	// использовать аппаратный чип RTC (часы). 0 - только интернет, 1 - использовать
#define USE_BMP 1	// использовать датчик давления/температуры (BMP180). 0 - не использовать, 1 - использовать

#define BUZZER_LOW 0 // 1 - пищалка активна при нуле, 0 - при единице

/*** ограничение потребления матрицы ***/

#define BRIGHTNESS 15		// стандартная максимальная яркость (0-15)

/*** описание матрицы ***/

#define SEG_IN_ROW 4			// ширина матрицы
#define SEG_IN_COL 1			// высота матрицы
/*
 Это варианты исполнения самой матрицы. Кроме обычных уже матриц, где все сегменты идут последовательно и линейно, есть варианты с разворотом каждого модуля 8х8.
 А ещё есть самодельные матрицы. Для обычной китайской сборки ничего менять не надо.
*/
#define MATRIX_TYPE 0		// тип матрицы: 0 - зигзаг, 1 - параллельная
#define MATRIX_ROTATION 0	// поворот каждого сегмента: 0 - 0, 1 - 90, 2 - 180, 3 - 240 градусов по часовой стрелке
#define MATRIX_FLIP_X 0	    // зеркальное отражение по горизонтали: 0 - нет, 1 - да
#define MATRIX_FLIP_Y 0	    // зеркальное отражение по вертикали: 0 - нет, 1 - да
#define MATRIX_CONNECTION 0 // точка подключения: 0 - верхний левый угол направо, 1 - верхний правый угол налево, 2 - нижний правый налево, 3 нижний левый направо
							// 4 - верхний левый вниз, 5 - правый верхний вниз, 6 - правый нижний вверх, 7 - левый нижний вверх

#define CLOCK_SHIFT 2 		// сдвиг первой цифры при выводе часов. XX:YY = 5 символов + 4 пробела = 5*5+4 = 29. 32-29 = 3 или 2 светодиода перед и 1 после
#define TEXT_BASELINE 0		// высота, на которой выводится текст (от низа матрицы)

/*** часовой пояс и летнее время. Можно переопределить через Web настройки ***/

#define TIMEZONE 2 // временная зона по умолчанию
#define DSTSHIFT 0 // сдвиг летнего времени

/*** зарезервированное количество объектов в настройках. Занимают много места. ***/

#define MAX_ALARMS 9	// количество возможных будильников
#define MAX_RUNNING 9	// количество возможных бегущих строк (больше 9 может вызвать проблемы с памятью)
#define MAX_MESSAGES 3	// количество слотов для временных строк

/*** разное ***/

// номера временных строк, определяют приоритет вывода

#define MESSAGE_WEB 0       // номер сообщения отправленного через WEB или MQTT
#define MESSAGE_WEATHER 1   // номер сообщения с информацией о погоде
#define MESSAGE_QUOTE 2     // номер сообщения с цитатой

/*** дальше определение переменных, ничего не менять ***/

#include "define_vars.h"

#endif