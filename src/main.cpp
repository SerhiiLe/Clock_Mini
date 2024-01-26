/**
 * @file main.cpp
 * @author Serhii Lebedenko (slebedenko@gmail.com)
 * @brief Clock Mini
 * @version 0.0.1
 * @date 2023-10-25
 * 
 * @copyright Copyright (c) 2023
 */

/*
	Главный файл проекта, в нём инициализация и логика основного цикла.
	"мультизадачность" организована как "кооперативная", через таймеры на millis
*/

#include <Arduino.h>
#include "defines.h"
#include <EncButton.h>
#include <LittleFS.h>

#include "settings.h"
#include "ftp.h"
#include "clock.h"
#include "runningText.h"
#include "wifi_init.h"
#include "ntp.h"
#include "web.h"
#include "dfplayer.h"
// #include "security.h"
#include "rtc.h"
#include "barometer.h"
#include "menu.h"
#include "nvram.h"
#include "beep.h"
#include "textTiny.h"

#if SENSOR_BUTTON == 1
Button btn_sel(PIN_BUTTON_SELECT, INPUT, LOW); // комбинация для сенсорной кнопки
Button btn_set(PIN_BUTTON_SET, INPUT, LOW);
#else
Button btn_sel(PIN_BUTTON_SELECT); // комбинация для обычной кнопки
Button btn_set(PIN_BUTTON_SET);
#endif

timerMinim autoBrightnessTimer(500);	// Таймер отслеживания показаний датчика света при включенном авторегулировании яркости матрицы
timerMinim clockTimer(512);				// Таймер, чтобы разделитель часов и минут мигал примерно каждую секунду
timerMinim scrollTimer(gs.scroll_period);	// Таймер обновления бегущей строки
timerMinim ntpSyncTimer(3600000U * gs.sync_time_period);  // Таймер синхронизации времени с NTP-сервером 3600000U
timerMinim clockDate(1000U * gs.show_date_period); // периодичность вывода даты в секундах
timerMinim textTimer[MAX_RUNNING];		// таймеры бегущих строк
timerMinim alarmTimer(1000);			// для будильника, срабатывает каждую секунду
timerMinim alarmStepTimer(5000);		// шаг увеличения громкости будильника
timerMinim showTermTimer(1000U * gs.show_term_period);	// таймер для показа информации о температуре
timerMinim syncWeatherTimer(60000U * gs.sync_weather_period); // таймер обновления информации о погоде из интернета
// timerMinim telegramTimer(1000U * tb_accelerated);	// период опроса команд из Телеграм
// timerMinim timeoutMp3Timer(3600000U * timeout_mp3); // таймер принудительного сброса mp3

// файловая система подключена
bool fs_isStarted = false;
// время начала работы будильника
time_t alarmStartTime = 0;
// яркость прошлого цикла
int16_t old_brightness = 5000;
// состояние датчика движения
bool cur_motion = false;
// время последней сработки датчика движения
unsigned long last_move = 0;
// флаг отработки действия датчика движения
bool fl_action_move = true;
// разрешение выводить бегущую строку (уведомления, день недели и число)
bool fl_run_allow = true;
// буфер под вывод даты / времени (в юникоде 1 буква = 2 байта)
char timeString[100];
// флаг требования сброса пароля
bool fl_password_reset_req = false;
// Текущая мелодия будильника, которая должна играть
uint8_t active_alarm = 0;
// разрешение увеличения яркости
bool fl_bright_boost = false;
// старое значение fl_bright_boost
bool old_bright_boost = true;
// статус процесса загрузки
uint8_t boot_stage = 1;
// работает меню настройки времени
bool menu_active = false;


void setup() {
	Serial.begin(115200);
	Serial.println(PSTR("Starting..."));
	beep_init();
	display_setup();
	randomSeed(analogRead(PIN_PHOTO_SENSOR)+analogRead(PIN_PHOTO_SENSOR));
	screenIsFree = true;
	// initRString(PSTR("..."),1,8);
	initRString(PSTR("boot"),1,7); //5
	display_tick();
}

// отложенный старт сервисов при запуске системы
bool boot_check() {
	if( ! screenIsFree ) {
		if(scrollTimer.isReady()) display_tick();
		return true;
	}
	switch (boot_stage)	{
		case 1: // попытка подключить диск
			if( LittleFS.begin()) {
				if( LittleFS.exists(F("index.html")) ) {
					fs_isStarted = true; // встроенный диск подключился
					LOG(println, PSTR("LittleFS mounted"));
				} else {
					LOG(println, PSTR("LittleFS is empty"));
					initRString(PSTR("Диск пустой, загрузите файлы!"));
				}
			} else {
				LOG(println, PSTR("ERROR LittleFS mount"));
				initRString(PSTR("Ошибка подключения встроенного диска!!!"));
			}
			break;
		case 2: // проверка наличия NVRAM
			if( ! nvram_init()) {
				LOG(println, PSTR("Couldn't find NVRAM"));
				initRString(PSTR("NVRAM не найден."));
			}
			break;
		case 3: // Загрузка или создание файла конфигурации
			if( ! load_config_main()) {
				LOG(println, PSTR("Create new config file"));
				//  Создаем файл запив в него данные по умолчанию, при любой ошибке чтения
				save_config_main();
				initRString(PSTR("Создан новый файл конфигурации."));
			}
			break;
		case 4: // Загрузка или создание файла со списком будильников
			if( ! load_config_alarms()) {
				LOG(println, PSTR("Create new alarms file"));
				save_config_alarms(); // Создаем файл
				initRString(PSTR("Создан новый файл списка будильников."));
			}
			break;
		case 5: // Загрузка или создание файла со списком бегущих строк
			if( ! load_config_texts()) {
				LOG(println, PSTR("Create new texts file"));
				save_config_texts(); // Создаем файл
				initRString(PSTR("Создан новый файл списка строк."));
			}
			break;
		case 6: // Подключение к модулю RTC и первичная установка времени
			switch(rtc_init()) {
				case 0:
					LOG(println, PSTR("Couldn't find RTC"));
					initRString(PSTR("Модуль часов не работает :("));
					break;
				case 2:
					LOG(println, PSTR("RTC init"));
					initRString(PSTR("Модуль часов инициализирован."));
					break;
				default:
					fl_timeNotSync = false;
					LOG(printf_P, PSTR("current RTC time: %s "), clockCurrentText(timeString));
					LOG(println, dateCurrentTextShort(timeString));
					break;
			}
			break;
		case 7: // Проверка наличия барометра
			if( ! barometer_init()) {
				LOG(println, PSTR("Couldn't find BMP module"));
				initRString(PSTR("Барометр не подключился :("));
			}
			break;
		case 8: // Подключение к WiFi или запуск режима AP и портала подключения
			wifi_setup();
			break;
	
		default: // в конце вывести строку приветствия и завершить процесс загрузки
			boot_stage = 0;
			initRString(gs.str_hello);
			return false;
	}
	boot_stage++;
	return true;
}

// выключение всех активных в данный момент будильников
void alarmsStop() {
	// для начала надо остановить проигрывание мелодии и сбросить таймер активности
	alarmStartTime = 0;
	delay(10);
	// mp3_disableLoop();
	// delay(10);
	mp3_stop();
	// устанавливается флаг, что все активные сейчас будильники отработали
	for(uint8_t i=0; i<MAX_ALARMS; i++)
		if(alarms[i].settings & 1024)
			alarms[i].settings |= 2048;
}

void loop() {
	int16_t i = 0;
	bool fl_doit = false;
	bool fl_save = false;
	tm t;

	if( boot_stage ) {
		if( boot_check() ) return;
	}

	wifi_process();
	if( wifi_isConnected ) {
		// установка времени по ntp.
		if( fl_timeNotSync || fl_needStartTime )
			// первичная установка времени. Если по каким-то причинам опрос не удался, повторять не чаще, чем раз в секунду.
			if( alarmStepTimer.isReady() ) syncTime();
		if(ntpSyncTimer.isReady()) // это плановая синхронизация, не критично, если опрос не прошел
			syncTime();
		// запуск сервисов, которые должны запуститься после запуска сети. (сеть должна подниматься в фоне)
		ftp_process();
		web_process();
		// if(telegramTimer.isReady()) tb_tick();
		if( ! fs_isStarted && ! ftp_isAllow && screenIsFree ) {
			ftp_isAllow = true;
			sprintf_P(timeString, PSTR("FTP для загрузки файлов включён IP: %s"), wifi_currentIP().c_str());
			initRString(timeString);
		}
	}

	if( fl_ntpRequestIsSend ) syncTime();

	beep_process();

	// if( mp3_isInit ) mp3_check();
	btn_set.tick();
	btn_sel.tick();

	if( menu_active ) {
		menu_active = menu_process(
			btn_sel.hold(), btn_sel.hasClicks() ? btn_sel.getClicks(): 0,
			btn_set.hold(), btn_set.step() + (btn_set.hasClicks() ? btn_set.getClicks(): 0));
		return;
	}

	if( btn_sel.hold() ) {
		LOG(println, PSTR("select button is hold"));
		// вход в режим установки даты
		beep_start();
		menu_init();
		menu_active = true;
	}
	if( btn_sel.hasClicks() ) {
		switch(btn_sel.getClicks()) {
			case 1:
				LOG(println, PSTR("1 click (select)"));
				if(alarmStartTime) alarmsStop(); // остановить будильник
				initRString(currentPressureTemp(timeString));
				clockDate.reset();
				break;
			case 2:
				LOG(println, PSTR("2 click (select)"));
				initRString(dateCurrentTextLong(timeString));
				break;
			case 3:
				initRString(PSTR("3 click (sel)"));
				beep_start(7);
				break;
			case 4:
				initRString(PSTR("4 click (sel)"));
				beep_stop();
				break;
			case 5:
				initRString(PSTR("5 click (sel)"));
				beep_start(0,true);
				break;
		}
	}

	if( btn_set.hold() ) {
		LOG(println, PSTR("set button is hold"));
		// зажата вспомогательная кнопка
		if(wifi_isPortal) {
			initRString(wifi_message);
		} else if(!wifi_isConnected) {
			initRString(PSTR("WiFi не найден, для настройки - 1 клик"));
		} else {
			initRString(PSTR("Справка: 1 клик-дата, 2-давление, 3-IP, 4-Яркость, 5-Сброс пароля или WiFi."));
		}
	}
	if( btn_set.hasClicks() )
	switch(btn_set.getClicks()) {
		case 1:
			LOG(println, PSTR("1 click (set)"));
			if(wifi_isPortal) wifi_startConfig(false);
			else
				if(!wifi_isConnected)
					wifi_startConfig(true);
				else {
					initRString(dateCurrentTextLong(timeString));
					clockDate.reset();
				}
			if(alarmStartTime) alarmsStop(); // остановить будильник
			fl_password_reset_req = false;
			break;
		case 2:
			LOG(println, PSTR("2 click (set)"));
			if(fl_password_reset_req) {
				gs.web_password[0] = 0;
				initRString(PSTR("Пароль временно отключен. Зайдите в настройки и задайте новый!"));
				} else
					initRString(currentPressureTemp(timeString));
			break;
		case 3:
			LOG(println, PSTR("3 click (set)"));
			if(fl_password_reset_req) {
				if(!wifi_isPortal) wifi_startConfig(true);
			} else
				initRString("IP: "+wifi_currentIP());
			break;
		case 4:
			LOG(println, PSTR("4 click (set)"));
			char buf[20];
			sprintf_P(buf,PSTR("%i -> %i -> %i"),analogRead(PIN_PHOTO_SENSOR), old_brightness*gs.bright_boost/100, led_brightness);
			initRString(buf);
			break;
		case 5:
			LOG(println, PSTR("5 click (set)"));
			initRString(PSTR("Сброс пароля! Для подтверждения - 2 клика! Или 3 для сброса WiFi. 1 клик - отмена."));
			fl_password_reset_req = true;
			break;
	}

	if(autoBrightnessTimer.isReady()) {
		int16_t cur_brightness = analogRead(PIN_PHOTO_SENSOR);
		int16_t min_brightness = cur_brightness > old_brightness ? old_brightness: cur_brightness; 
		// загрубление датчика освещённости. Чем ярче, тем больше разброс показаний
		if(abs(cur_brightness-old_brightness)>(min_brightness>0?(min_brightness>>4)+1:0) || fl_bright_boost != old_bright_boost) {
			// усиление показаний датчика
			uint16_t val = gs.bright_boost!=100 ? cur_brightness*gs.bright_boost/100: cur_brightness;
			// дополнительная яркость по времени
			uint8_t add_val = fl_bright_boost ? gs.bright_add: 0;
			switch(gs.bright_mode) {
				case 0: // полный автомат от 0 до 15, диапазон на входе 0-1024, кратность 64, 6 bit
					set_brightness(map(val, 0, 1023, add_val, 15));
					break;
				case 1: // автоматический с ограничителем
					set_brightness(map(val, 0, 1023, add_val, gs.bright0));
					break;
				default: // ручной
					set_brightness(constrain((uint16_t)gs.bright0 + (uint16_t)add_val, 0, 15));
			}
			old_brightness = cur_brightness;
			old_bright_boost = fl_bright_boost;
		}
	}

	if(alarmTimer.isReady()) {
		fl_save = false;
		t = getTime();
		// проверка времени работы бегущей строки
		i = t.tm_hour*60+t.tm_min;
		fl_run_allow = gs.run_allow == 0 || (gs.run_allow == 1 && i >= gs.run_begin && i <= gs.run_end);
		fl_bright_boost = gs.boost_mode != 0 && 
			((gs.boost_mode > 0 && gs.boost_mode < 5 && i >= sunrise && i <= sunset) ||
			(gs.boost_mode == 5 && i >= gs.bright_begin && i <= gs.bright_end));
		// перебор всех будильников, чтобы найти активный
		for(i=0; i<MAX_ALARMS; i++)
			if(alarms[i].settings & 512) {
				// активный будильник найден, проверка времени срабатывания
				if(alarms[i].hour == t.tm_hour && alarms[i].minute == t.tm_min) {
					// защита от повторного запуска
					if(!(alarms[i].settings & 1024)) {
						// определение других критериев срабатывания
						fl_doit = false;
						switch ((alarms[i].settings >> 7) & 3) {
							case 0: // разово
							case 1: // каждый день
								fl_doit = true;
								break;
							case 2: // по дням
								if((alarms[i].settings >> t.tm_wday) & 1)
									fl_doit = true;
						}
						if(fl_doit) { // будильник сработал
							if(alarmStartTime == 0) {
								active_alarm = i;
								// mp3_volume(volume_start); // начинать с маленькой громкости
								// mp3_reread(); // перечитать количество треков, почему-то без этого может не запуститься
								// mp3_enableLoop(); // зациклить мелодию
								// delay(10);
								// mp3_play(alarms[i].melody); // запустить мелодию
								// alarmStepTimer.reset();
								alarmStartTime = getTimeU(); // чтобы избежать конфликтов между будильниками на одно время и отсчитывать максимальное время работы
							}
							alarms[i].settings = alarms[i].settings | 1024; // установить флаг активности
						}
					}
				} else if(alarms[i].settings & 2048) { // будильник уже разбудил
					alarms[i].settings &= ~(3072U); // сбросить флаги активности
					if(((alarms[i].settings >> 7) & 3) == 0) { // это разовый будильник, надо отключить и сохранить настройки
						alarms[i].settings &= ~(512U);
						fl_save = true;
					}
				}
			}
		if(fl_save) save_config_alarms();
	}
	// плавное увеличение громкости и ограничение на время работы будильника
	if(alarmStartTime && alarmStepTimer.isReady()) {
		i = alarms[active_alarm].text;
		if(screenIsFree && i >= 0) {
			// вывод текста только на время работы будильника
			initRString(texts[i].text, texts[i].color_mode > 0 ? texts[i].color_mode: texts[i].color);
		}
		// if(!mp3_isPlay()) {
		// 	// мелодия не запустилась, повторить весь цикл сначала. Редко, но случается :(
		// 	mp3_reread();
		// 	mp3_enableLoop();
		// 	delay(10);
		// 	mp3_play(alarms[active_alarm].melody);
		// 	alarmStartTime = getTimeU();
		// } else
		// 	// мелодия играет, увеличить громкость на единицу
		// 	if(cur_Volume<volume_finish) mp3_volume(++cur_Volume);
		if(alarmStartTime + gs.max_alarm_time * 60 < getTimeU()) alarmsStop(); // будильник своё отработал, наверное не разбудил
	}

	// если экран освободился, то выбор, что сейчас надо выводить.
	// проверка разрешения выводить бегущую строку
	if(fl_run_allow && alarmStartTime == 0) {
		fl_save = false;
		// в приоритете бегущая строка
		for(i=0; i<MAX_RUNNING; i++)
			if(screenIsFree) // дополнительная проверка нужна потому, что статус может поменяться в каждом цикле
				if(textTimer[i].isReady() && (texts[i].repeat_mode & 512)) {
					fl_doit = false;
					t = getTime();
					switch ((texts[i].repeat_mode >> 7) & 3) {
						case 0: // всегда, пока включён
							fl_doit = true;
							break;
						case 1: // по дате, в конкретный день месяца
							if(((texts[i].repeat_mode >> 10) & 31) == t.tm_mday)
								fl_doit = true;
							break;
						case 2: // по дням недели
							if((texts[i].repeat_mode >> t.tm_wday) & 1)
								fl_doit = true;
							break;
						default: // до конца дня
							if(((texts[i].repeat_mode >> 10) & 31) == t.tm_mday)
								fl_doit = true;
							else {
								texts[i].repeat_mode &= 511;
								fl_save = true;
							}
					}
					if(fl_doit) initRString(texts[i].text, texts[i].color_mode > 0 ? texts[i].color_mode: texts[i].color);
				}
		if(fl_save) save_config_texts();
		// затем температура и давление
		if(screenIsFree && showTermTimer.isReady()) {
			if(gs.tiny_term) printTinyText(currentPressureTemp(timeString, true));
			else initRString(currentPressureTemp(timeString, false));
		}
		// затем дата
		if(screenIsFree && clockDate.isReady()) {
			if(gs.tiny_date) printTinyText(gs.show_date_short ? dateCurrentTextTiny(timeString): dateCurrentTextTinyFull(timeString), 2);
			else initRString(gs.show_date_short ? dateCurrentTextShort(timeString): dateCurrentTextLong(timeString));
		}
	}
	// если всё уже показано, то вывести время
	if(screenIsFree && clockTimer.isReady()) {
		if(gs.tiny_clock) printTinyText(clockTinyText(timeString), 3, true);
		else initRString(clockCurrentText(timeString), 1, CLOCK_SHIFT);
	}

	if(scrollTimer.isReady()) display_tick();
}
