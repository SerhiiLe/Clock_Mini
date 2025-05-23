/*
	встроенный web сервер для настройки часов
	(для начальной настройки ip и wifi используется wifi_init)
*/

#include <Arduino.h>
#ifdef ESP32
#include <WebServer.h>
#include "mHTTPUpdateServer.h"
#include <ESPmDNS.h>
#include <rom/rtc.h>
#include <esp_system.h>
#else // ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#endif
#include <LittleFS.h>
// #include <sys/time.h>
#include "defines.h"
#include "web.h"
#include "settings.h"
#include "runningText.h"
#include "textTiny.h"
#include "ntp.h"
#include "rtc.h"
#include "beep.h"
#include "clock.h"
#include "wifi_init.h"
#include "barometer.h"
#include "webClient.h"
#include "forecaster.h"

#define HPP(txt, ...) HTTP.client().printf_P(PSTR(txt), __VA_ARGS__)
const char PROGMEM txt_save[] = "save";

#ifdef ESP32
WebServer HTTP(80);
HTTPUpdateServer httpUpdater;
#endif
#ifdef ESP8266
ESP8266WebServer HTTP(80);
ESP8266HTTPUpdateServer httpUpdater;
#endif
bool web_isStarted = false;

void save_settings();
void save_alarm();
void off_alarm();
void save_text();
void off_text();
void save_quote();
void show_quote();
void save_weather();
void show_sensors();
void show_weather();
void show();
void sysinfo();
void play();
void maintence();
void set_clock();
void onoff();
void logout();
void show_status();
// #ifdef USE_NVRAM
void make_config();
void make_alarms();
void make_texts();
void make_quote();
void make_weather();
// #endif

bool fileSend(String path);
bool need_save = false;
bool fl_mdns = false;

// отключение веб сервера для активации режима настройки wifi
void web_disable() {
	HTTP.stop();
	web_isStarted = false;
	LOG(println, PSTR("HTTP server stoped"));

	#ifdef ESP32
	MDNS.disableWorkstation();
	#else // ESP8266
	MDNS.close();
	#endif
	fl_mdns = false;
	LOG(println, PSTR("MDNS responder stoped"));
}

// отправка простого текста
void text_send(String s, uint16_t r = 200) {
	HTTP.send(r, F("text/plain"), s);
}
// отправка сообщение "не найдено"
void not_found() {
	text_send(F("Not Found"), 404);
}

// диспетчер вызовов веб сервера
void web_process() {
	if( web_isStarted ) {
		HTTP.handleClient();
		#ifdef ESP8266
		if(fl_mdns) MDNS.update();
		#endif
	} else {
		HTTP.begin();
		// Обработка HTTP-запросов
		HTTP.on(F("/save_settings"), save_settings);
		HTTP.on(F("/save_alarm"), save_alarm);
		HTTP.on(F("/off_alarm"), off_alarm);
		HTTP.on(F("/save_text"), save_text);
		HTTP.on(F("/off_text"), off_text);
		HTTP.on(F("/save_quote"), save_quote);
		HTTP.on(F("/show_quote"), show_quote);
		HTTP.on(F("/save_weather"), save_weather);
		HTTP.on(F("/show_sensors"), show_sensors);
		HTTP.on(F("/show_weather"), show_weather);
		HTTP.on(F("/show"), show);
		HTTP.on(F("/sysinfo"), sysinfo);
		HTTP.on(F("/play"), play);
		HTTP.on(F("/clear"), maintence);
		HTTP.on(F("/clock"), set_clock);
		HTTP.on(F("/onoff"), onoff);
		HTTP.on(F("/logout"), logout);
		HTTP.on(F("/status"), show_status);
		if(USE_NVRAM && nvram_enable) {
			HTTP.on(F("/config.json"), make_config);
			HTTP.on(F("/alarms.json"), make_alarms);
			HTTP.on(F("/texts.json"), make_texts);
			HTTP.on(F("/quote.json"), make_quote);
			HTTP.on(F("/weather.json"), make_weather);
		}
		HTTP.on(F("/who"), [](){
			text_send(String(gs.str_hostname));
		});
		HTTP.onNotFound([](){
			if(!fileSend(HTTP.uri()))
				not_found();
			});
		web_isStarted = true;
  		httpUpdater.setup(&HTTP, String(gs.web_login), String(gs.web_password));
		LOG(println, PSTR("HTTP server started"));

		#ifdef ESP32
		if(MDNS.begin(gs.str_hostname)) {
		#else // ESP8266
		if(MDNS.begin(gs.str_hostname, WiFi.localIP())) {
		#endif
			MDNS.addService("http", "tcp", 80);
			fl_mdns = true;
			LOG(println, PSTR("MDNS responder started"));
		}
	}
}

// страничка выхода, будет предлагать ввести пароль, пока он не перестанет совпадать с реальным
void logout() {
	if(strlen(gs.web_login) > 0 && strlen(gs.web_password) > 0)
		if(HTTP.authenticate(gs.web_login, gs.web_password))
			HTTP.requestAuthentication(DIGEST_AUTH);
	if(!fileSend(F("/logged-out.html")))
		not_found();
}

// список файлов, для которых авторизация не нужна, остальные под паролем
bool auth_need(String s) {
	if(s == F("/index.html")) return false;
	if(s == F("/about.html")) return false;
	if(s == F("/send.html")) return false;
	if(s == F("/logged-out.html")) return false;
	if(s.endsWith(F(".js"))) return false;
	if(s.endsWith(F(".css"))) return false;
	if(s.endsWith(F(".ico"))) return false;
	if(s.endsWith(F(".png"))) return false;
	return true;
}

// авторизация. много комментариев из документации, чтобы по новой не искать
bool is_no_auth() {
	// allows you to set the realm of authentication Default:"Login Required"
	// const char* www_realm = "Custom Auth Realm";
	// the Content of the HTML response in case of Unauthorized Access Default:empty
	// String authFailResponse = "Authentication Failed";
	if(strlen(gs.web_login) > 0 && strlen(gs.web_password) > 0 )
		if(!HTTP.authenticate(gs.web_login, gs.web_password)) {
		    //Basic Auth Method with Custom realm and Failure Response
			//return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
			//Digest Auth Method with realm="Login Required" and empty Failure Response
			//return server.requestAuthentication(DIGEST_AUTH);
			//Digest Auth Method with Custom realm and empty Failure Response
			//return server.requestAuthentication(DIGEST_AUTH, www_realm);
			//Digest Auth Method with Custom realm and Failure Response
			HTTP.requestAuthentication(DIGEST_AUTH);
			return true;
		}
	return false;
}

// отправка файла
bool fileSend(String path) {
	// если путь пустой - исправить на индексную страничку
	if( path.endsWith("/") ) path += F("index.html");
	// проверка необходимости авторизации
	if(auth_need(path))
		if(is_no_auth()) return false;
	// определение типа файла
	const char *ct = nullptr;
	if(path.endsWith(F(".html"))) ct = PSTR("text/html");
	else if(path.endsWith(F(".css"))) ct = PSTR("text/css");
	else if(path.endsWith(F(".js"))) ct = PSTR("application/javascript");
	else if(path.endsWith(F(".json"))) ct = PSTR("application/json");
	else if(path.endsWith(F(".png"))) ct = PSTR("image/png");
	else if(path.endsWith(F(".jpg"))) ct = PSTR("image/jpeg");
	else if(path.endsWith(F(".gif"))) ct = PSTR("image/gif");
	else if(path.endsWith(F(".ico"))) ct = PSTR("image/x-icon");
	else ct = PSTR("text/plain");
	// открытие файла на чтение
	if(!fs_isStarted) {
		// файловая система не загружена, переход на страничку обновления
		HTTP.client().printf_P(PSTR("HTTP/1.1 200\r\nContent-Type: %s\r\nContent-Length: 80\r\nConnection: close\r\n\r\n<html><body><h1><a href='/update'>File system not exist!</a></h1></body></html>"),ct);
		return true;
	}
	if(LittleFS.exists(path)) {
		File file = LittleFS.open(path, "r");
		// файл существует и открыт, выделение буфера передачи и отсылка заголовка
		char buf[1476];
		size_t sent = 0;
		int siz = file.size();
		HTTP.client().printf_P(PSTR("HTTP/1.1 200\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"),ct,siz);
		// отсылка файла порциями, по размеру буфера или остаток
		while(siz > 0) {
			size_t len = std::min((int)(sizeof(buf) - 1), siz);
			file.read((uint8_t *)buf, len);
			HTTP.client().write((const char*)buf, len);
			siz -= len;
			sent+=len;
		}
		file.close();  
	} else return false; // файла нет, ошибка
	return true;
}

// декодирование времени, заданного в поле input->time
uint16_t decode_time(String s) {
	// выделение часов и минут из строки вида 00:00
	size_t pos = s.indexOf(":");
	uint8_t h = constrain(s.toInt(), 0, 23);
	uint8_t m = constrain(s.substring(pos+1).toInt(), 0, 59);
	return h*60 + m;
}

// печать байта в виде шестнадцатеричного числа
void print_byte(char *buf, byte c, size_t& pos) {
	if((c & 0xf) > 9)
		buf[pos+1] = (c & 0xf) - 10 + 'A';
	else
		buf[pos+1] = (c & 0xf) + '0';
	c = (c>>4) & 0xf;
	if(c > 9)
		buf[pos]=c - 10 + 'A';
	else
		buf[pos] = c+'0';
	pos += 2;
}

// кодирование строки для json
const char* jsonEncode(char* buf, const char *str, size_t max_length) {
	// size_t len = strlen(str);
	size_t p = 0, i = 0;
	byte c;

	while( str[i] != '\0' && p < max_length-8) {
		// Выделение символа UTF-8 и перевод его в UTF-16 для вывода в JSON
		// 0xxxxxxx - 7 бит 1 байт, 110xxxxx - 10 бит 2 байта, 1110xxxx - 16 бит 3 байта, 11110xxx - 21 бит 4 байта
		c = (byte)str[i++];
		if( c > 127  ) {
			buf[p++] = '\\';
			buf[p++] = 'u';
			// utf8 -> utf16
			if( c >> 5 == 6 ) {
		        uint16_t cc = ((uint16_t)(str[i-1] & 0x1F) << 6);
				cc |= (uint16_t)(str[i++] & 0x3F);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
			} else if( c >> 4 == 14 ) {
				uint16_t cc = ((uint16_t)(str[i-1] & 0x0F) << 12);
				cc |= ((uint16_t)(str[i++] & 0x3F) << 6);
				cc |= (uint16_t)(str[i++] & 0x3F);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
			} else if( c >> 3 == 30 ) {
				uint32_t CP = ((uint32_t)(str[i-1] & 0x07) << 18);
				CP |= ((uint32_t)(str[i++] & 0x3F) << 12);
				CP |= ((uint32_t)(str[i++] & 0x3F) << 6);
				CP |= (uint32_t)(str[i++] & 0x3F);
				CP -= 0x10000;
				uint16_t cc = 0xD800 + (uint16_t)((CP >> 10) & 0x3FF);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
				cc = 0xDC00 + (uint16_t)(CP & 0x3FF);
				print_byte(buf, (byte)(cc>>8), p);
				print_byte(buf, (byte)(cc&0xff), p);
			}
		} else {
			buf[p++] = c;
		}
	}

	buf[p++] = '\0';
	return buf;
}

/****** шаблоны простых операций для выделения переменных из web ******/

// определение выбран checkbox или нет
bool set_simple_checkbox(const __FlashStringHelper * name, uint8_t &var) {
	if( HTTP.hasArg(name) ) {
		if( var == 0 ) {
			var = 1;
			need_save = true;
			return true;
		}
	} else {
		if( var > 0 ) {
			var = 0;
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение простых целых чисел
template <typename T>
bool set_simple_int(const __FlashStringHelper * name, T &var, long from, long to) {
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name).toInt() != (long)var ) {
			var = constrain(HTTP.arg(name).toInt(), from, to);
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение дробных чисел
bool set_simple_float(const __FlashStringHelper * name, float &var, float from, float to, float prec=8.0f) {
	if( HTTP.hasArg(name) ) {
		if( round(HTTP.arg(name).toFloat()*pow(10.0f,prec)) != round(var*pow(10.0f,prec)) ) {
			var = constrain(HTTP.arg(name).toFloat(), from, to);
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение простых строк (для String)
bool set_simple_string(const __FlashStringHelper * name, String &var) {
	if( HTTP.hasArg(name) ) {
		if( HTTP.arg(name) != var ) {
			var = HTTP.arg(name);
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение простых строк (для char[])
bool set_simple_string(const __FlashStringHelper * name, char * var, size_t len) {
	if( HTTP.hasArg(name) ) {
		if( strcmp(HTTP.arg(name).c_str(), var) != 0 ) {
			strncpy_P(var, HTTP.arg(name).c_str(), len);
			var[len] = 0;
			need_save = true;
			return true;
		}
	}
	return false;
}
// определение времени
bool set_simple_time(const __FlashStringHelper * name, uint16_t &var) {
	if( HTTP.hasArg(name) ) {
		if( decode_time(HTTP.arg(name)) != var ) {
			var = decode_time(HTTP.arg(name));
			need_save = true;
			return true;
		}
	}
	return false;
}

/****** обработка разных запросов ******/

// сохранение настроек
void save_settings() {
	if(is_no_auth()) return;
	need_save = false;

	if( set_simple_int(F("language"), gs.language, 0, 2) ) {
		if( ws.weather ) weatherUpdate();
		if( qs.enabled ) quoteUpdate();
	};
	set_simple_string(F("str_hello"), gs.str_hello, LENGTH_HELLO);
	if(set_simple_string(F("str_hostname"), gs.str_hostname, LENGTH_HOSTNAME))
		#ifdef ESP32
		if(fl_mdns)	MDNS.setInstanceName(gs.str_hostname);
		#else // ESP8266
		if(fl_mdns)	MDNS.setHostname(gs.str_hostname);
		#endif
	set_simple_int(F("max_alarm_time"), gs.max_alarm_time, 1, 30);
	set_simple_int(F("run_allow"), gs.run_allow, 0, 2);
	set_simple_time(F("run_begin"), gs.run_begin);
	set_simple_time(F("run_end"), gs.run_end);
	set_simple_checkbox(F("dsp_off"), gs.dsp_off);
	set_simple_checkbox(F("show_move"), gs.show_move);
	set_simple_int(F("delay_move"), gs.delay_move, 0, 10);
	bool sync_time = false;
	if( set_simple_int(F("tz_shift"), gs.tz_shift, -12, 12) )
		sync_time = true;
	if( set_simple_checkbox(F("tz_dst"), gs.tz_dst) )
		sync_time = true;
	if( set_simple_int(F("sync_time_period"), gs.sync_time_period, 1, 255) )
		ntpSyncTimer.setInterval(3600000U * gs.sync_time_period);
	set_simple_checkbox(F("tz_adjust"), gs.tz_adjust);
	set_simple_int(F("tiny_clock"), gs.tiny_clock, 0, 9);
	set_simple_int(F("dots_style"), gs.dots_style, 0, 11);
	set_simple_checkbox(F("t12h"), gs.t12h);
	set_simple_int(F("date_short"), gs.show_date_short, 0, 3);
	set_simple_checkbox(F("tiny_date"), gs.tiny_date);
	if( set_simple_int(F("date_period"), gs.show_date_period, 20, 1439) )
		clockDate.setInterval(1000U * gs.show_date_period);
	if( set_simple_float(F("latitude"), gs.latitude, -180.0f, 180.0f) )
		sync_time = true;
	if( set_simple_float(F("longitude"), gs.longitude, -180.0f, 180.0f) )
		sync_time = true;
	bool need_bright = false;
	if( set_simple_int(F("bright_mode"), gs.bright_mode, 0, 2) )
		need_bright = true;
	if( set_simple_int(F("bright0"), gs.bright0, 1, 255) )
		need_bright = true;
	set_simple_int(F("br_boost"), gs.bright_boost, 1, 1000);
	if( set_simple_int(F("boost_mode"), gs.boost_mode, 0, 5) )
		sync_time = true;
	if( set_simple_int(F("br_add"), gs.bright_add, 1, 255) )
		need_bright = true;
	set_simple_time(F("br_begin"), gs.bright_begin);
	set_simple_time(F("br_end"), gs.bright_end);
	set_simple_int(F("turn_display"), gs.turn_display, 0, 3);
	if( set_simple_int(F("scroll_period"), gs.scroll_period, 0, 50) )
		scrollTimer.setInterval(60 - gs.scroll_period);
	set_simple_int(F("slide_show"), gs.slide_show, 1, 10);
	set_simple_int(F("minim_show"), gs.minim_show, 0, 20);
	bool need_web_restart = false;
	if( set_simple_string(F("web_login"), gs.web_login, LENGTH_LOGIN) )
		need_web_restart = true;
	if( set_simple_string(F("web_password"), gs.web_password, LENGTH_PASSWORD) )
		need_web_restart = true;

	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_main();
	// initRString(PSTR("Настройки сохранены"));
	printTinyText(txt_save,9);
	if( sync_time ) syncTime();
	if( need_bright ) old_bright_boost = !old_bright_boost;
	if(need_web_restart) httpUpdater.setup(&HTTP, String(gs.web_login), String(gs.web_password));
}

// перезагрузка часов, сброс ком-порта, отключение сети и диска, чтобы ничего не мешало перезагрузке
void reboot_clock() {
	Serial.flush();
	#ifdef ESP32
	WiFi.getSleep();
	#else // ESP8266
	WiFi.forceSleepBegin(); //disable AP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
	#endif
	LittleFS.end();
	delay(1000);
	ESP.restart();
}

void reset_settings(int t) {
	switch (t) {
		case NVRAM_CONFIG_MAIN: { // главная конфигурация
			LOG(println, PSTR("reset settings"));
			if(USE_NVRAM && nvram_enable) {
				Global_Settings ts;
				memcpy(&gs, &ts, sizeof(Global_Settings));
				save_config_main();
			} else {
				if(LittleFS.exists(F("/config.json"))) LittleFS.remove(F("/config.json"));
			}
			} break;
		case NVRAM_CONFIG_ALARMS: { // настройки будильников
			LOG(println, PSTR("reset alarms"));
			if(USE_NVRAM && nvram_enable) {
				cur_alarm ta[MAX_ALARMS];
				memcpy((void*)&alarms, (void*)&ta, sizeof(cur_alarm[MAX_ALARMS]));
				save_config_alarms();
			} else {
				if(LittleFS.exists(F("/alarms.json"))) LittleFS.remove(F("/alarms.json"));
			}
			} break;
		case NVRAM_CONFIG_TEXTS: { // настройки бегущих строк
			LOG(println, PSTR("reset texts"));
			if(USE_NVRAM && nvram_enable) {
				cur_text tt[MAX_RUNNING];
				memcpy((void*)&texts, (void*)&tt, sizeof(cur_text[MAX_RUNNING]));
				save_config_texts();
			} else {
				if(LittleFS.exists(F("/texts.json"))) LittleFS.remove(F("/texts.json"));
			}
			} break;
		case NVRAM_CONFIG_QUOTE: { // настройки цитат
			LOG(println, PSTR("reset quotes"));
			if(USE_NVRAM && nvram_enable) {
				Quote_Settings tq;
				memcpy(&qs, &tq, sizeof(Quote_Settings));
				save_config_quote();
			} else {
				if(LittleFS.exists(F("/quote.json"))) LittleFS.remove(F("/quote.json"));
			}
			} break;
		case NVRAM_CONFIG_WEATHER: { // настройки сервера погоды
			LOG(println, PSTR("reset weather"));
			if(USE_NVRAM && nvram_enable) {
				Weather_Settings tw;
				memcpy(&ws, &tw, sizeof(Weather_Settings));
				save_config_weather();
			} else {
				if(LittleFS.exists(F("/weather.json"))) LittleFS.remove(F("/weather.json"));
			}
			} break;
		case NVRAM_CONFIG_MQTT: { // настройки MQTT
			LOG(println, PSTR("reset MQTT"));
			if(USE_NVRAM && nvram_enable) {
				MQTT_Settings tm;
				memcpy(&ms, &tm, sizeof(MQTT_Settings));
				save_config_mqtt();
			} else {
				if(LittleFS.exists(F("/mqtt.json"))) LittleFS.remove(F("/mqtt.json"));
			}
			} break;
		default: // такого раздела нет, просто перезагрузится
			break;
	}
}

void maintence() {
	if(is_no_auth()) return;
	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303); 
	String name = F("target");
	if( HTTP.hasArg(name) ) {
		int t = constrain(HTTP.arg(name).toInt(), 0, 255);
		initRString(PSTR("Сброс"));
		if(t == 96) {
			// сброс всех настроек (кроме wifi)
			for(uint8_t i=0; i<=5; i++)
				reset_settings(i);
		} else if(t > 0) {
			// сброс конкретного раздела настроек
			reset_settings(t-1);
		}
		reboot_clock();
	}
}

// сохранение настроек будильника
void save_alarm() {
	if(is_no_auth()) return;
	need_save = false;
	uint8_t target = 0;
	uint16_t settings = 512;
	String name = F("target");
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		name = F("time");
		if( HTTP.hasArg(name) ) {
			// выделение часов и минут из строки вида 00:00
			size_t pos = HTTP.arg(name).indexOf(":");
			uint8_t h = constrain(HTTP.arg(name).toInt(), 0, 23);
			uint8_t m = constrain(HTTP.arg(name).substring(pos+1).toInt(), 0, 59);
			if( h != alarms[target].hour || m != alarms[target].minute ) {
				alarms[target].hour = h;
				alarms[target].minute = m;
				need_save = true;
			}
		}
		name = F("rmode");
		if( HTTP.hasArg(name) ) settings |= constrain(HTTP.arg(name).toInt(), 0, 3) << 7;
		if( HTTP.hasArg(F("mo")) ) settings |= 2;
		if( HTTP.hasArg(F("tu")) ) settings |= 4;
		if( HTTP.hasArg(F("we")) ) settings |= 8;
		if( HTTP.hasArg(F("th")) ) settings |= 16;
		if( HTTP.hasArg(F("fr")) ) settings |= 32;
		if( HTTP.hasArg(F("sa")) ) settings |= 64;
		if( HTTP.hasArg(F("su")) ) settings |= 1;
		if( settings != alarms[target].settings ) {
			alarms[target].settings = settings;
			need_save = true;
		}
		set_simple_int(F("melody"), alarms[target].melody, 0, 255);
		set_simple_string(F("text"), alarms[target].text, LENGTH_TEXT_ALARM);
	}
	HTTP.sendHeader(F("Location"),F("/alarms.html"));
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_alarms(target);
	beep_stop();
	initRString(PSTR("Будильник установлен"));
}

// отключение будильника
void off_alarm() {
	if(is_no_auth()) return;
	uint8_t target = 0;
	String name = "t";
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		if( alarms[target].settings & 512 ) {
			alarms[target].settings &= ~(512U);
			save_config_alarms(target);
			text_send(F("1"));
			initRString(PSTR("Будильник отключён"));
		}
	} else
		text_send(F("0"));
}

// сохранение настроек бегущей строки. Строки настраиваются по одной.
void save_text() {
	if(is_no_auth()) return;
	need_save = false;
	uint8_t target = 0;
	uint16_t settings = 512;
	String name = F("target");
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		set_simple_string(F("text"), texts[target].text, LENGTH_TEXT);
		if( set_simple_int(F("period"), texts[target].period, 30, 3600) )
			textTimer[target].setInterval(texts[target].period*1000U);
		name = F("rmode");
		if( HTTP.hasArg(name) ) settings |= constrain(HTTP.arg(name).toInt(), 0, 3) << 7;
		if( HTTP.hasArg("mo") ) settings |= 2;
		if( HTTP.hasArg("tu") ) settings |= 4;
		if( HTTP.hasArg("we") ) settings |= 8;
		if( HTTP.hasArg("th") ) settings |= 16;
		if( HTTP.hasArg("fr") ) settings |= 32;
		if( HTTP.hasArg("sa") ) settings |= 64;
		if( HTTP.hasArg("su") ) settings |= 1;
		if((settings >> 7 & 3) == 3) { // если режим "до конца дня", то записать текущий день
			tm t = getTime();
			settings |= t.tm_mday << 10;
		} else { // иначе то, что передано формой
			name = "sel_day";
			if( HTTP.hasArg(name) ) settings |= constrain(HTTP.arg(name).toInt(), 1, 31) << 10;
		}
		if( settings != texts[target].repeat_mode ) {
			texts[target].repeat_mode = settings;
			need_save = true;
		}
	}
	HTTP.sendHeader(F("Location"),F("/running.html"));
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_texts(target);
	initRString(PSTR("Текст установлен"));
}

// отключение бегущей строки
void off_text() {
	if(is_no_auth()) return;
	uint8_t target = 0;
	String name = "t";
	if( HTTP.hasArg(name) ) {
		target = HTTP.arg(name).toInt();
		if( texts[target].repeat_mode & 512 ) {
			texts[target].repeat_mode &= ~(512U);
			save_config_texts(target);
			text_send(F("1"));
			initRString(PSTR("Текст отключён"));
		}
	} else
		text_send(F("0"));
}

// обслуживает страничку плейера.
void play() {
	if(is_no_auth()) return;
	uint8_t p = 0;
	uint16_t c = 1;
	String name = "p";
	if( HTTP.hasArg(name) ) p = HTTP.arg(name).toInt();
	name = "c";
	if( HTTP.hasArg(name) ) c = constrain(HTTP.arg(name).toInt(), 0, melody_count);
	switch (p)	{
		case 1: // играть
			beep_start(c, true);
			break;
		case 0: // остановить
			beep_stop();
			break;
	}
	char buff[10];
	sprintf_P(buff,PSTR("%u"), fl_beep_active);
	text_send(buff);
}

// Установка времени. Для крайних случаев, когда интернет отсутствует
void set_clock() {
	if(is_no_auth()) return;
	uint8_t type=0;
	String name = "t";
	if(HTTP.hasArg(name)) {
		struct tm tm;
		type = HTTP.arg(name).toInt();
		if(type==0 || type==1) {
			name = F("time");
			if(HTTP.hasArg(name)) {
				size_t pos = HTTP.arg(name).indexOf(":");
				tm.tm_hour = constrain(HTTP.arg(name).toInt(), 0, 23);
				tm.tm_min = constrain(HTTP.arg(name).substring(pos+1).toInt(), 0, 59);
				name = F("date");
				if(HTTP.hasArg(name)) {
					size_t pos = HTTP.arg(name).indexOf("-");
					tm.tm_year = constrain(HTTP.arg(name).toInt()-1900, 0, 65000);
					tm.tm_mon = constrain(HTTP.arg(name).substring(pos+1).toInt()-1, 0, 11);
					size_t pos2 = HTTP.arg(name).substring(pos+1).indexOf("-");
					tm.tm_mday = constrain(HTTP.arg(name).substring(pos+pos2+2).toInt(), 1, 31);
					name = F("sec");
					if(HTTP.hasArg(name)) {
						tm.tm_sec = constrain(HTTP.arg(name).toInt()+1, 0, 60);
						tm.tm_isdst = gs.tz_dst;
						time_t t = mktime(&tm);
						LOG(printf_P,"web time: %llu\n",t);
						// set the system time
						timeval tv = { t, 0 };
						settimeofday(&tv, nullptr);
						rtc_saveTIME(t);
					}
				}
			}
		} else {
			syncTime();
		}
		HTTP.sendHeader(F("Location"),F("/maintenance.html"));
		HTTP.send(303);
		delay(1);
	} else {
		tm t = getTime();
		HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
		HTTP.client().printf_P(PSTR("\"time\":\"%u\","), t.tm_hour*60+t.tm_min);
		HTTP.client().printf_P(PSTR("\"date\":\"%u-%02u-%02u\"}"), t.tm_year +1900, t.tm_mon +1, t.tm_mday);
		#ifdef ESP8266
		HTTP.client().stop();
		#endif
	}
}

// Включение/выключение различных режимов
void onoff() {
	if(is_no_auth()) return;
	int8_t a=0;
	bool cond=false;
	String name = "a";
	if(HTTP.hasArg(name)) a = HTTP.arg(name).toInt();
	name = "t";
	if(HTTP.hasArg(name)) {
		// if(HTTP.arg(name) == F("demo")) {
		// 	// включает/выключает демо режим
		// 	if(a) fl_demo = !fl_demo;
		// 	cond = fl_demo;
		// } else 
		if(HTTP.arg(name) == F("ftp")) {
			// включает/выключает ftp сервер, чтобы не кушал ресурсов просто так
			if(a) ftp_isAllow = !ftp_isAllow;
			cond = ftp_isAllow;
		}
	}
	text_send(cond?F("1"):F("0"));
}

#ifdef ESP32

/**
* @brief The structure represents information about the chip
*/
/*
typedef struct {
esp_chip_model_t model; //!< chip model, one of esp_chip_model_t
uint32_t features; //!< bit mask of CHIP_FEATURE_x feature flags
uint8_t cores; //!< number of CPU cores
uint8_t revision; //!< chip revision number
} esp_chip_info_t;

Sample code:
#include <esp_system.h>
esp_chip_info_t chip_info[sizeof(esp_cesp_chip_infohip_info_t)]; // reserve memory for chip information struct
esp_chip_info(chip_info); // get the ESP32 chip information
esp_chip_model_t chip_Model = chip_info->model; // ESP32 chip model
uint8_t chip_Cores = chip_info->cores; // ESP32 nr of cores
uint8_t chip_Revision = chip_info->revision; // ESP32 revision nr
const char * espIdfVersion = esp_get_idf_version(); // get ESP Development Framework version
*/

const char* print_full_platform_info(char* buf) {
	int p = 0;
	const char *cpu;
	switch (chip_info.model) {
		case 1: // ESP32
			cpu = "ESP32";
			break;
		case 2: // ESP32-S2
			cpu = "ESP32-S2";
			break;
		case 9: // ESP32-S3
			cpu = "ESP32-S4";
			break;
		case 5: // ESP32-C3
			cpu = "ESP32-C3";
			break;
		case 6: // ESP32-H2
			cpu = "ESP32-H2";
			break;
		default:
			cpu = "unknown";
	}
	p = sprintf(buf, "Chip:%s_r%u/", cpu, chip_info.revision);
	p += sprintf(buf+p, "Cores:%u/%s", chip_info.cores, ESP.getSdkVersion());
	return buf;
}

// декодирование информации о причине перезагрузки ядра
const char* print_reset_reason(char *buf) {
	int p = 0;
	uint8_t old_reason = 127;
	const char *res;
	for(int i=0; i<chip_info.cores; i++) {
		uint8_t reason = rtc_get_reset_reason(i);
		if( old_reason != reason ) {
			old_reason = reason;
			if( p ) p += sprintf(buf+p, ", ");
			switch ( reason ) {
				case 1 : res = "PowerON"; break;                /**<1, Vbat power on reset*/
				case 3 : res = "SW_RESET"; break;               /**<3, Software reset digital core*/
				case 4 : res = "OWDT_RESET"; break;             /**<4, Legacy watch dog reset digital core*/
				case 5 : res = "DeepSleep"; break;              /**<5, Deep Sleep reset digital core*/
				case 6 : res = "SDIO_RESET"; break;             /**<6, Reset by SLC module, reset digital core*/
				case 7 : res = "TG0WDT_SYS_RESET"; break;       /**<7, Timer Group0 Watch dog reset digital core*/
				case 8 : res = "TG1WDT_SYS_RESET"; break;       /**<8, Timer Group1 Watch dog reset digital core*/
				case 9 : res = "RTCWDT_SYS_RESET"; break;       /**<9, RTC Watch dog Reset digital core*/
				case 10 : res = "INTRUSION_RESET"; break;       /**<10, Intrusion tested to reset CPU*/
				case 11 : res = "TGWDT_CPU_RESET"; break;       /**<11, Time Group reset CPU*/
				case 12 : res = "SW_CPU_RESET"; break;          /**<12, Software reset CPU*/
				case 13 : res = "RTCWDT_CPU_RESET"; break;      /**<13, RTC Watch dog Reset CPU*/
				case 14 : res = "EXT_CPU_RESET"; break;         /**<14, for APP CPU, reseted by PRO CPU*/
				case 15 : res = "RTCWDT_BROWN_OUT"; break;	    /**<15, Reset when the vdd voltage is not stable*/
				case 16 : res = "RTCWDT_RTC_RESET"; break;      /**<16, RTC Watch dog reset digital core and rtc module*/
				default : res = "NO_MEAN";
			}
			p += sprintf(buf+p, "%s", res);
		}
	}
	return buf;
}
#endif

// Информация о состоянии железки
void sysinfo() {
	if(is_no_auth()) return;
	char buf[100];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
	HPP("\"Uptime\":\"%s\",", getUptime(buf));
	HPP("\"Time\":\"%s\",", clockCurrentText(buf));
	HPP("\"Date\":\"%s\",", dateCurrentTextLong(buf));
	HPP("\"Sunrise\":\"%u:%02u\",", sunrise / 60, sunrise % 60);
	HPP("\"Sunset\":\"%u:%02u\",", sunset / 60, sunset % 60);
	HPP("\"Illumination\":%i,", analogRead(PIN_PHOTO_SENSOR));
	HPP("\"LedBrightness\":%i,", led_brightness);
	HPP("\"Barometer\":%u,", fl_barometerIsInit);
	HPP("\"Rssi\":%i,", wifi_rssi());
	HPP("\"IP\":\"%s\",", wifi_currentIP().c_str());
	HPP("\"FreeHeap\":%i,", ESP.getFreeHeap());
	#ifdef ESP32
	HPP("\"MaxFreeBlockSize\":%i,", ESP.getMaxAllocHeap());
	HPP("\"HeapFragmentation\":%i,", 100-ESP.getMaxAllocHeap()*100/ESP.getFreeHeap());
	HPP("\"ResetReason\":\"%s\",", print_reset_reason(buf));
	HPP("\"FullVersion\":\"%s\",", print_full_platform_info(buf));
	#else // ESP8266
	HPP("\"MaxFreeBlockSize\":%i,", ESP.getMaxFreeBlockSize());
	HPP("\"HeapFragmentation\":%i,", ESP.getHeapFragmentation());
	HPP("\"ResetReason\":\"%s\",", ESP.getResetReason().c_str());
	HPP("\"FullVersion\":\"%s\",", ESP.getFullVersion().c_str());
	#endif
	HPP("\"CpuFreqMHz\":%i,", ESP.getCpuFreqMHz());
	HPP("\"RTC\":%u,", rtc_enable);
	HPP("\"TimeDrift\":%i,", rtc_enable ? getTimeU() - (gs.tz_shift+gs.tz_dst)*3600 - getRTCTimeU(): 0);
	HPP("\"NVRAM\":%u,", nvram_enable ? eeprom_chip: 0);
	HPP("\"BuildTime\":\"%s %s\"}", F(__DATE__), F(__TIME__));
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}

// #ifdef USE_NVRAM
void make_config() {
	if(is_no_auth()) return;
	char buf[LENGTH_HELLO*3];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
	HPP("\"str_hello\":\"%s\",", jsonEncode(buf, gs.str_hello, sizeof(buf)));
	HPP("\"str_hostname\":\"%s\",", jsonEncode(buf, gs.str_hostname, sizeof(buf)));
    HPP("\"max_alarm_time\":%u,", gs.max_alarm_time);
    HPP("\"run_allow\":%u,", gs.run_allow);
    HPP("\"run_begin\":%u,", gs.run_begin);
    HPP("\"run_end\":%u,", gs.run_end);
	HPP("\"dsp_off\":%u,", gs.dsp_off);
    HPP("\"show_move\":%u,", gs.show_move);
    HPP("\"delay_move\":%u,", gs.delay_move);
    HPP("\"tz_shift\":%i,", gs.tz_shift);
    HPP("\"tz_dst\":%u,", gs.tz_dst);
    HPP("\"sync_time_period\":%u,", gs.sync_time_period);
    HPP("\"tz_adjust\":%u,", gs.tz_adjust);
	HPP("\"tiny_clock\":%u,", gs.tiny_clock);
	HPP("\"dots_style\":%u,", gs.dots_style);
	HPP("\"t12h\":%u,", gs.t12h);
    HPP("\"date_short\":%u,", gs.show_date_short);
    HPP("\"tiny_date\":%u,", gs.tiny_date);
    HPP("\"date_period\":%u,", gs.show_date_period);
    HPP("\"latitude\":%1.8f,", gs.latitude);
    HPP("\"longitude\":%1.8f,", gs.longitude);
    HPP("\"bright_mode\":%u,", gs.bright_mode);
    HPP("\"bright0\":%u,", gs.bright0);
    HPP("\"br_boost\":%u,", gs.bright_boost);
    HPP("\"boost_mode\":%u,", gs.boost_mode);
    HPP("\"br_add\":%u,", gs.bright_add);
    HPP("\"br_begin\":%u,", gs.bright_begin);
    HPP("\"br_end\":%u,", gs.bright_end);
    HPP("\"turn_display\":%u,", gs.turn_display);
    HPP("\"scroll_period\":%u,", gs.scroll_period);
    HPP("\"slide_show\":%u,", gs.slide_show);
	HPP("\"minim_show\":%u,", gs.minim_show);
	HPP("\"language\":%u,", gs.language);
    HPP("\"web_login\":\"%s\",", jsonEncode(buf, gs.web_login, sizeof(buf)));
    HPP("\"web_password\":\"%s\"}", jsonEncode(buf, gs.web_password, sizeof(buf)));
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}
// #endif

// #ifdef USE_NVRAM
void make_alarms() {
	if(is_no_auth()) return;
	char buf[LENGTH_TEXT_ALARM*3];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n["));
	for(uint8_t i=0; i<MAX_ALARMS; i++) {
		HPP("{\"s\":%u,\"h\":%u,\"m\":%u,\"me\":%u,\"t\":\"%s\"}%s", alarms[i].settings, alarms[i].hour, alarms[i].minute, alarms[i].melody, jsonEncode(buf, alarms[i].text, sizeof(buf)), i<MAX_ALARMS-1 ? ",":""); 
	}
	HTTP.client().print("]");
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}
// #endif

// #ifdef USE_NVRAM
void make_texts() {
	if(is_no_auth()) return;
	char buf[LENGTH_TEXT*3];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n["));
	for(uint8_t i=0; i<MAX_RUNNING; i++) {
		HPP("{\"p\":%u,\"r\":%u,\"t\":\"%s\"}%s", texts[i].period, texts[i].repeat_mode, jsonEncode(buf, texts[i].text, sizeof(buf)), i<MAX_RUNNING-1 ? ",":""); 
	}
	HTTP.client().print("]");
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}
// #endif

// сохранение настроек цитат
void save_quote() {
	if(is_no_auth()) return;
	need_save = false;

	if( set_simple_checkbox(F("enabled"), qs.enabled) ) {
		// если цитаты отключили, то сбросить текущую цитату 
		if( qs.enabled == 0 ) messages[MESSAGE_QUOTE].count = 0;
	}
	if( set_simple_int(F("period"), qs.period, 0, 255) )
		messages[MESSAGE_QUOTE].timer.setInterval(60000U * (qs.period+1));
	if( set_simple_int(F("update"), qs.update, 0, 3) )
		quoteUpdateTimer.setInterval(900000U * (qs.update+1));
	set_simple_int(F("server"), qs.server, 0, 2);
	set_simple_int(F("lang"), qs.lang, 0, 3);
	set_simple_string(F("url"), qs.url, MAX_URL_LENGTH);
	set_simple_string(F("params"), qs.params, MAX_PARAM_LENGTH);
	set_simple_int(F("method"), qs.method, 0, 1);
	set_simple_int(F("type"), qs.type, 0, 2);
	set_simple_string(F("quote_field"), qs.quote_field, MAX_QUOTE_FIELD);
	set_simple_string(F("author_field"), qs.author_field, MAX_QUOTE_FIELD);

	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) {
		save_config_quote();
		quote.fl_init = false;
	}
	// initRString(PSTR("Настройки сохранены"));
	printTinyText(txt_save,9);
}

void show_quote() {
	if(is_no_auth()) return;
	text_send(messages[MESSAGE_QUOTE].text);
}

// #ifdef USE_NVRAM
void make_quote() {
	if(is_no_auth()) return;
	char buf[MAX_URL_LENGTH*3];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
    HPP("\"enabled\":%u,", qs.enabled);
    HPP("\"period\":%u,", qs.period);
    HPP("\"update\":%u,", qs.update);
    HPP("\"server\":%u,", qs.server);
    HPP("\"lang\":%u,", qs.lang);
	HPP("\"url\":\"%s\",", jsonEncode(buf, qs.url, sizeof(buf)));
	HPP("\"params\":\"%s\",", jsonEncode(buf, qs.params, sizeof(buf)));
    HPP("\"method\":%u,", qs.method);
    HPP("\"type\":%u,", qs.type);
    HPP("\"quote_field\":\"%s\",", jsonEncode(buf, qs.quote_field, sizeof(buf)));
    HPP("\"author_field\":\"%s\"}", jsonEncode(buf, qs.author_field, sizeof(buf)));
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}
// #endif

const char help[] PROGMEM = R"""(
(h)elp - this help
(m)sg="text" - show this "text" on the matrix
(c)nt=5 - show 5 times (2 by default, max 100)
(i)nt=60 - show with interval 60 seconds (30 by default, max 600)
)""";

void show() {
	bool fl_web = false;
	bool fl_msg = false;
	bool fl_cnt = false;
	bool fl_int = false;
	bool cond = false;
	int num_args = HTTP.args();
	if(num_args==0) {
		HTTP.send_P(200, PSTR("text/plain"), help);
		LOG(println, F("show: send help"));
		return;
	}
	for(int i=0; i<num_args; i++) {
		String arg_name = HTTP.argName(i);
		if(arg_name.startsWith("h")) {
			HTTP.send_P(200, PSTR("text/plain"), help);
			LOG(println, F("show: send help"));
		}
		if(arg_name.startsWith("m")) {
			messages[MESSAGE_WEB].text = HTTP.arg(i);
			if(messages[MESSAGE_WEB].text.length() > 1) fl_msg = true;
		}
		if(arg_name.startsWith("c")) {
			messages[MESSAGE_WEB].count = constrain(HTTP.arg(i).toInt(), 1, 100);
			fl_cnt = true;
		}
		if(arg_name.startsWith("i")) {
			messages[MESSAGE_WEB].timer.setInterval(constrain(HTTP.arg(i).toInt()*1000, 15000, 600000));
			fl_int = true;
		}
		if(arg_name.startsWith("w")) {
			fl_web = true;
		}
	}
	if( fl_msg ) {
		if( ! fl_cnt ) messages[MESSAGE_WEB].count = 2;
		if( ! fl_int ) messages[MESSAGE_WEB].timer.setInterval(30000);
		cond = true;
	} else
		messages[MESSAGE_WEB].count = 0;
	if( fl_web ) {
		HTTP.sendHeader(F("Location"),"/maintenance.html");
		HTTP.send(303);
		delay(1);
	} else
		text_send(cond?F("1"):F("0"));
}

void save_weather() {
	if(is_no_auth()) return;
	need_save = false;
	bool need_weather = false;

	set_simple_checkbox(F("sensors"), ws.sensors);
	set_simple_int(F("term_period"), ws.term_period, 20, 60000);
	set_simple_checkbox(F("tiny_term"), ws.tiny_term);
	set_simple_float(F("term_cor"), ws.term_cor, -100.0f, 100.0f, 1.0f);
	set_simple_int(F("bar_cor"), ws.bar_cor, -1000, 1000);
	set_simple_int(F("term_pool"), ws.term_pool, 30, 600);
	need_weather = set_simple_checkbox(F("weather"), ws.weather);
	if( set_simple_int(F("sync_weather_period"), ws.sync_weather_period, 15, 1439) )
		syncWeatherTimer.setInterval(60000U * ws.sync_weather_period);
	if( set_simple_int(F("show_weather_period"), ws.show_weather_period, 1, 1439) )
		messages[MESSAGE_WEATHER].timer.setInterval(60000U * ws.show_weather_period);
	set_simple_checkbox(F("weather_code"), ws.weather_code);
	set_simple_checkbox(F("temperature"), ws.temperature);
	set_simple_checkbox(F("a_temperature"), ws.a_temperature);
	set_simple_checkbox(F("humidity"), ws.humidity);
	set_simple_checkbox(F("cloud"), ws.cloud);
	set_simple_checkbox(F("pressure"), ws.pressure);
	set_simple_checkbox(F("wind_speed"), ws.wind_speed);
	set_simple_checkbox(F("wind_direction"), ws.wind_direction);
	set_simple_checkbox(F("wind_direction2"), ws.wind_direction2);
	set_simple_checkbox(F("wind_gusts"), ws.wind_gusts);
	set_simple_checkbox(F("pressure_dir"), ws.pressure_dir);
	set_simple_checkbox(F("forecast"), ws.forecast);
	if( set_simple_int(F("altitude"), ws.altitude, -1000, 12000) )
		forecaster_setH(ws.altitude);

	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) {
		save_config_weather();
		if( ws.weather ) {
			if( need_weather ) syncWeatherTimer.setNext(50);
			char txt[512];
			messages[MESSAGE_WEATHER].text = String(generate_weather_string(txt));
		} else {
			messages[MESSAGE_WEATHER].count = 0;
		}
	}
	// initRString(PSTR("Настройки сохранены"));
	printTinyText(txt_save,9);
}

void show_sensors() {
	if(is_no_auth()) return;
	char txt[100];
	currentPressureTemp(txt);
	text_send(String(txt));
}

void show_weather() {
	if(is_no_auth()) return;
	char txt[512];
	text_send(String(generate_weather_string(txt)));
}

// #ifdef USE_NVRAM
void make_weather() {
	if(is_no_auth()) return;
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
	HPP("\"sensors\":%u,", ws.sensors);
	HPP("\"term_period\":%u,", ws.term_period);
	HPP("\"tiny_term\":%u,", ws.tiny_term);
	HPP("\"term_cor\":%1.1f,", ws.term_cor);
	HPP("\"bar_cor\":%i,", ws.bar_cor);
	HPP("\"term_pool\":%u,", ws.term_pool);
	HPP("\"weather\":%u,", ws.weather);
	HPP("\"sync_weather_period\":%u,", ws.sync_weather_period);
	HPP("\"show_weather_period\":%u,", ws.show_weather_period);
	HPP("\"weather_code\":%u,", ws.weather_code);
	HPP("\"temperature\":%u,", ws.temperature);
	HPP("\"a_temperature\":%u,", ws.a_temperature);
	HPP("\"humidity\":%u,", ws.humidity);
	HPP("\"cloud\":%u,", ws.cloud);
	HPP("\"pressure\":%u,", ws.pressure);
	HPP("\"wind_speed\":%u,", ws.wind_speed);
	HPP("\"wind_direction\":%u,", ws.wind_direction);
	HPP("\"wind_direction2\":%u,", ws.wind_direction2);
	HPP("\"wind_gusts\":%u,", ws.wind_gusts);
	HPP("\"pressure_dir\":%u,", ws.pressure_dir);
	HPP("\"forecast\":%u,", ws.forecast);
	HPP("\"altitude\":%i}", ws.altitude);
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}
// #endif

void show_status() {
	// char buf[100];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
	HPP("\"hostname\":\"%s\",", gs.str_hostname);
	HPP("\"is_auth\":%i,", HTTP.authenticate(gs.web_login, gs.web_password) && strlen(gs.web_password) > 0 ? 1 : 0);
	HPP("\"use_rtc\":%i,", USE_RTC);
	HPP("\"use_nvram\":%i,", USE_NVRAM);
	HPP("\"use_bmp\":%i}", USE_BMP);
	#ifdef ESP8266
	HTTP.client().stop();
	#endif
}