#ifndef define_vars_h
#define define_vars_h

/*
	Тут описаны различные глобальные переменные и настройки по умолчанию. Всё, что потом можно будет поменять через Web.
*/

extern uint8_t led_brightness; // текущая яркость
#define LEDS_IN_ROW SEG_IN_ROW * 8
#define LEDS_IN_COL SEG_IN_COL * 8

extern bool fs_isStarted;
extern bool wifi_isConnected;
extern bool wifi_isPortal;
extern String wifi_message;
extern bool ftp_isAllow;
extern bool fl_allowLEDS;
extern bool fl_timeNotSync;
extern bool fl_needStartTime;
extern bool fl_ntpRequestIsSend;
extern bool nvram_enable;

// таймеры должны быть доступны в разных местах
#include "timerMinim.h"
extern timerMinim scrollTimer;          // таймер скроллинга
extern timerMinim autoBrightnessTimer;  // Таймер отслеживания показаний датчика света при включенной авторегулировки яркости матрицы
extern timerMinim ntpSyncTimer;         // Таймер синхронизации времени с NTP-сервером
extern timerMinim scrollTimer;          // Таймер задержки между обновлениями бегущей строки, определяет скорость движения
extern timerMinim clockDate;            // Таймер периодичности вывода даты в виде бегущей строки (длительность примерно 15 секунд)
extern timerMinim textTimer[];          // Таймеры бегущих строк
extern timerMinim alarmStepTimer;
extern timerMinim showTermTimer;
extern timerMinim syncWeatherTimer;
extern timerMinim quoteUpdateTimer;

/*** определение глобальных перемененных, которые станут настройками ***/
// описания переменных в файле settings_init.h
// файл config.json

#define LENGTH_HELLO 100 // максимальная длина строки приветствия (для кириллицы реально в два раза меньше)
#define LENGTH_HOSTNAME 30 // максимальная длина названия часов в MDNS
#define LENGTH_LOGIN 20 // максимальная длина логина
#define LENGTH_PASSWORD 30 // максимальная длина пароля
#define LENGTH_TEXT_ALARM 100 // максимальная длина строки в будильнике
#define LENGTH_TEXT 250 // максимальная длина бегущей строки

struct Global_Settings {
	char str_hello[LENGTH_HELLO+1] = "Mini-Clock"; // строка которая выводится в момент запуска часов
	char str_hostname[LENGTH_HOSTNAME+1] = "mini"; // как будут анонсироваться часы в MDNS: http://mini.local
	uint8_t max_alarm_time = 5; // максимальное время работы будильника
	uint8_t run_allow = 0; // режим работы бегущей строки
	uint16_t run_begin = 0; // время начала работы бегущей строки
	uint16_t run_end = 1439; // время окончания работы бегущей строки
	uint8_t dsp_off = 0; // выключать дисплей во время ночного режима
	uint8_t show_move = 0; // включение светодиода датчика движения
	uint8_t delay_move = 5; // задержка срабатывания датчика движения (если есть ложные срабатывания)
	int8_t tz_shift = TIMEZONE; // временная зона, смещение локального времени относительно Гринвича
	uint8_t tz_dst = DSTSHIFT; // смещение летнего времени
	uint8_t sync_time_period = 8; // периодичность синхронизации ntp, в часах
	uint8_t tz_adjust = 0; // корректировать часовой пояс по серверу погоды
	uint8_t tiny_clock = 0; // выводить время крошечными цифрами (другими циферблатами)
	uint8_t dots_style = 0; // стиль мерцания двоеточия для циферблатов без секунд
	uint8_t show_date_short = 0; // показывать дату в коротком формате
	uint8_t tiny_date = 0; // выводить дату крошечными цифрами
	uint16_t show_date_period = 40; // периодичность вывода даты в секундах
	float latitude = 46.4857f; // географическая широта
	float longitude = 30.7438f; // географическая долгота
	uint8_t bright_mode = 1; // режим яркости матрицы (авто или ручной)
	uint8_t bright0 = 10; // яркость матрицы средняя (1-255)
	uint16_t bright_boost = 100; // усиление показателей датчика яркости в процентах (1-250) 100 - оригинальная
	uint8_t boost_mode = 0; // режим дополнительного увеличения яркости
	uint8_t bright_add = 1; // на сколько дополнительно увеличивать яркость
	uint16_t bright_begin = 0; // время начала дополнительного увеличения яркости
	uint16_t bright_end = 0; // время окончания дополнительного увеличения яркости
	uint8_t turn_display = 0; // перевернуть картинку
	uint16_t scroll_period = 40; // задержка между обновлениями бегущей строки, определяет скорость движения
	uint16_t slide_show = 2; // время показа одного слайда в режиме крошечных цифр
	char web_login[LENGTH_LOGIN+1] = "admin"; // логин для вэб
	char web_password[LENGTH_PASSWORD+1] = ""; // пароль для вэб
}; // 228 байт
extern Global_Settings gs;

struct cur_alarm {
	uint16_t settings = 0;	// настройки (побитовое поле)
	uint8_t hour = 0;	// часы
	uint8_t minute = 0;	// минуты
	uint8_t melody = 0;	// номер мелодии
	char text[LENGTH_TEXT_ALARM+1] = "";	// сам текст
}; // 954 байт на 9 записей
extern cur_alarm alarms[];

struct cur_text {
	char text[LENGTH_TEXT+1] = "";	// текст который надо отобразить
	uint16_t period = 90; // период повтора в секундах
	uint16_t repeat_mode = 0; // режим повтора (0 пока активно, 1 до конца дня, 2 день недели, 3 день месяца)
}; // 2304 байт на 9 записей
extern cur_text texts[];

struct temp_text {
	String text = "";	// текст который надо будет выводить
	timerMinim timer;	// таймер с отсчётом интервалов показа
	int16_t count = 0;	// число повторов
};
extern temp_text messages[];

#define MAX_URL_LENGTH 100
#define MAX_PARAM_LENGTH 100
#define MAX_QUOTE_FIELD 30

struct Quote_Settings {
	uint8_t enabled = 1;
	uint8_t period = 2;
	uint8_t update = 1;
	uint8_t server = 0;
	uint8_t lang = 2;
	char url[MAX_URL_LENGTH+1] = "";
	char params[MAX_PARAM_LENGTH+1] = "";
	uint8_t method = 0;
	uint8_t type = 0;
	char quote_field[MAX_QUOTE_FIELD+1] = "";
	char author_field[MAX_QUOTE_FIELD+1] = "";
}; // 271
extern Quote_Settings qs;

struct Quote_Server {
	bool fl_init = false;
	char url[MAX_URL_LENGTH+1] = "";
	char params[MAX_PARAM_LENGTH+1] = "";
	char quote[MAX_QUOTE_FIELD+1] = "";
	char author[MAX_QUOTE_FIELD+1] = "";
	uint8_t method = 0;
	uint8_t type = 0;
};
extern Quote_Server quote;

struct Weather_Settings {
	uint8_t sensors = 0;
	uint16_t term_period = 60;
	uint8_t tiny_term = 0;
	float term_cor = 0.0f;
	uint16_t bar_cor = 0;
	uint16_t term_pool = 120;
	uint8_t weather = 0;
	uint8_t sync_weather_period = 30;
	uint8_t show_weather_period = 2;
	uint8_t weather_code = 1;
	uint8_t temperature = 1;
	uint8_t a_temperature = 1;
	uint8_t humidity = 1;
	uint8_t cloud = 1;
	uint8_t pressure = 1;
	uint8_t wind_speed = 1;
	uint8_t wind_direction = 1;
	uint8_t wind_direction2 = 1;
	uint8_t wind_gusts = 1;
	uint8_t pressure_dir = 1;
	uint8_t forecast = 1;
}; // 32 (228+954+2304+271+32+(4*5)=3809, 4096-3809=287 свободных ячеек)
extern Weather_Settings ws;

struct MQTT_Settings {
	uint8_t enable = 0;
};
extern MQTT_Settings ms;

extern uint16_t sunrise; // время восхода в минутах от начала суток
extern uint16_t sunset; // время заката в минутах от начала суток
extern bool old_bright_boost; // флаг для изменения уровня яркости
extern bool cur_motion; // флаг состояния датчика движения

extern const byte fontSemicolon[][4] PROGMEM;

#ifdef ESP32
#define MAX_ANALOG 4095
// #define LittleFS SPIFFS
#define FORMAT_LITTLEFS_IF_FAILED true
extern esp_chip_info_t chip_info;
#else // ESP8266
#define MAX_ANALOG 1023
#endif

//----------------------------------------------------
#if defined(LOG)
#undef LOG
#endif

#ifdef DEBUG
	//#define LOG                   Serial
	#define LOG(func, ...) Serial.func(__VA_ARGS__)
#else
	#define LOG(func, ...) ;
#endif

#endif