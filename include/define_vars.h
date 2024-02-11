#ifndef define_vars_h
#define define_vars_h

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
extern timerMinim showWeatherTimer;

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
    uint16_t show_date_period = 30; // периодичность вывода даты в секундах
	uint16_t show_term_period = 60; // периодичность вывода температуры и давления в секундах
	uint8_t tiny_term = 0; // выводить температуру крошечными цифрами
	float term_cor = 0.0f; // корректировка показаний температуры
	int16_t bar_cor = 0; // корректировка показаний барометра (из-за высоты)
	uint16_t term_pool = 120; // минимальное время между опросами температуры
	uint8_t use_internet_weather = 0; // использовать данные о погоде и часовом поясе из интернета https://open-meteo.com/
	uint16_t sync_weather_period = 30; // периодичность синхронизации данных о погоде, в минутах
    uint16_t show_weather_period = 180; // периодичность вывода детальной информации о погоде
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
}; // 244 байт
extern Global_Settings gs;

struct cur_alarm {
	uint16_t settings = 0;	// настройки (побитовое поле)
	uint8_t hour = 0;	// часы
	uint8_t minute = 0;	// минуты
	uint8_t melody = 0;	// номер мелодии
	// int8_t text = -1;	// номер текста, который выводится при срабатывании
	char text[LENGTH_TEXT_ALARM+1] = "";	// сам текст
}; // 954 байт на 9 записей
extern cur_alarm alarms[];

struct cur_text {
	char text[LENGTH_TEXT+1] = "";	// текст который надо отобразить
	// String text = "";	// текст который надо отобразить
	// uint8_t color_mode = 0; // режим цвета, как везде (0 )
	// uint32_t color = 0xFFFFFF; // по умолчанию - белый
	uint16_t period = 60; // период повтора в секундах
	uint16_t repeat_mode = 0; // режим повтора (0 пока активно, 1 до конца дня, 2 день недели, 3 день месяца)
}; // 2304 байт на 10 записей
extern cur_text texts[];

extern uint8_t sec_enable;
extern uint8_t sec_curFile;

extern uint16 sunrise; // время восхода в минутах от начала суток
extern uint16 sunset; // время заката в минутах от начала суток
extern bool old_bright_boost; // флаг для изменения уровня яркости

extern const byte fontSemicolon[][4] PROGMEM;

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