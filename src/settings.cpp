/*
	Работа с настройками.
	Инициализация по умолчанию, чтение из файла, сохранение в файл
*/

#include <Arduino.h>
#ifndef USE_NVRAM
#include <ArduinoJson.h>
#endif
#include <LittleFS.h>
#include "defines.h"
#include "settings.h"
#include "ntp.h"
#include "nvram.h"
#include "leds_max.h"

Global_Settings gs;

cur_alarm alarms[MAX_ALARMS];
cur_text texts[MAX_RUNNING];

Quote_Settings qs;


void copy_string(char* dst, const char* src, size_t len) {
	if(src != nullptr) {
		strncpy(dst, src, len);
		dst[len] = 0;
	} else
		dst[0] = 0;
}

bool load_config_main() {
#ifdef USE_NVRAM
	Global_Settings ts;
	if(!readBlock(NVRAM_CONFIG_MAIN, (uint8_t*)&ts, sizeof(Global_Settings))) return false;
	memcpy(&gs, &ts, sizeof(Global_Settings));
#else
	File configFile = LittleFS.open(F("/config.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open main config file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	// Fetch values.
	// const char* sensor = doc["sensor"];
	// long time = doc["time"];
	// double latitude = doc["data"][0];
	// double longitude = doc["data"][1];

	copy_string(gs.str_hello, doc[F("str_hello")], LENGTH_HELLO);
	copy_string(gs.str_hostname, doc[F("str_hostname")], LENGTH_HOSTNAME);
	gs.max_alarm_time = doc[F("max_alarm_time")];
	gs.run_allow = doc[F("run_allow")];
	gs.run_begin = doc[F("run_begin")];
	gs.run_end = doc[F("run_end")];
	gs.show_move = doc[F("show_move")];
	gs.delay_move = doc[F("delay_move")];
	gs.tz_shift = doc[F("tz_shift")];
	gs.tz_dst = doc[F("tz_dst")];
	gs.sync_time_period = doc[F("sync_time_period")];
	gs.tz_adjust = doc[F("tz_adjust")];
	gs.tiny_clock = doc[F("tiny_clock")];
	gs.dots_style = doc[F("dots_style")];
	gs.show_date_short = doc[F("date_short")];
	gs.tiny_date = doc[F("tiny_date")];
	gs.show_date_period = doc[F("date_period")];
	gs.show_term_period = doc[F("term_period")];
	gs.tiny_term = doc[F("tiny_term")];
	gs.term_cor = doc[F("term_cor")];
	gs.bar_cor = doc[F("bar_cor")];
	gs.term_pool = doc[F("term_pool")];
	gs.use_internet_weather = doc[F("internet_weather")];
	gs.sync_weather_period = doc[F("sync_weather_period")];
	gs.show_weather_period = doc[F("show_weather_period")];
	gs.latitude = doc[F("latitude")];
	gs.longitude = doc[F("longitude")];
	gs.bright_mode = doc[F("bright_mode")];
	gs.bright0 = doc[F("bright0")];
	gs.bright_boost = doc[F("br_boost")];
	gs.boost_mode = doc[F("boost_mode")];
	gs.bright_add = doc[F("br_add")];
	gs.bright_begin = doc[F("br_begin")];
	gs.bright_end = doc[F("br_end")];
	gs.turn_display = doc[F("turn_display")];
	gs.scroll_period = doc[F("scroll_period")];
	gs.slide_show = doc[F("slide_show")];
	copy_string(gs.web_login, doc[F("web_login")], LENGTH_LOGIN);
	copy_string(gs.web_password, doc[F("web_password")], LENGTH_PASSWORD);

#endif
	clockDate.setInterval(1000U * gs.show_date_period);
	showTermTimer.setInterval(1000U * gs.show_term_period);
	syncWeatherTimer.setInterval(60000U * gs.sync_weather_period);
	messages[MESSAGE_WEATHER].timer.setInterval(1000U * gs.show_weather_period);
	if(gs.bright_mode==2) set_brightness(gs.bright0);
	ntpSyncTimer.setInterval(3600000U * gs.sync_time_period);
	scrollTimer.setInterval(gs.scroll_period);
	return true;
}

void save_config_main() {
#ifdef USE_NVRAM
	if(!writeBlock(NVRAM_CONFIG_MAIN, (uint8_t*)&gs, sizeof(Global_Settings))) {
		LOG(println, PSTR("NVRAM: main config write error!"));
	}
#else
	JsonDocument doc; // временный буфер под объект json

	doc[F("str_hello")] = gs.str_hello;
	doc[F("str_hostname")] = gs.str_hostname;
	doc[F("max_alarm_time")] = gs.max_alarm_time;
	doc[F("run_allow")] = gs.run_allow;
	doc[F("run_begin")] = gs.run_begin;
	doc[F("run_end")] = gs.run_end;
	doc[F("show_move")] = gs.show_move;
	doc[F("delay_move")] = gs.delay_move;
	doc[F("tz_shift")] = gs.tz_shift;
	doc[F("tz_dst")] = gs.tz_dst;
	doc[F("sync_time_period")] = gs.sync_time_period;
	doc[F("tz_adjust")] = gs.tz_adjust;
	doc[F("tiny_clock")] = gs.tiny_clock;
	doc[F("dots_style")] = gs.dots_style;
	doc[F("date_short")] = gs.show_date_short;
	doc[F("tiny_date")] = gs.tiny_date;
	doc[F("date_period")] = gs.show_date_period;
	doc[F("term_period")] = gs.show_term_period;
	doc[F("tiny_term")] = gs.tiny_term;
	doc[F("term_cor")] = gs.term_cor;
	doc[F("bar_cor")] = gs.bar_cor;
	doc[F("term_pool")] = gs.term_pool;
	doc[F("internet_weather")] = gs.use_internet_weather;
	doc[F("sync_weather_period")] = gs.sync_weather_period;
	doc[F("show_weather_period")] = gs.show_weather_period;
	doc[F("latitude")] = gs.latitude;
	doc[F("longitude")] = gs.longitude;
	doc[F("bright_mode")] = gs.bright_mode;
	doc[F("bright0")] = gs.bright0;
	doc[F("br_boost")] = gs.bright_boost;
	doc[F("boost_mode")] = gs.boost_mode;
	doc[F("br_add")] = gs.bright_add;
	doc[F("br_begin")] = gs.bright_begin;
	doc[F("br_end")] = gs.bright_end;
	doc[F("turn_display")] = gs.turn_display;
	doc[F("scroll_period")] = gs.scroll_period;
	doc[F("slide_show")] = gs.slide_show;
	doc[F("web_login")] = gs.web_login;
	doc[F("web_password")] = gs.web_password;

	File configFile = LittleFS.open(F("/config.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(4);

#endif
}

bool load_config_alarms() {
#ifdef USE_NVRAM
	cur_alarm ta[MAX_ALARMS];
	if(!readBlock(NVRAM_CONFIG_ALARMS, (uint8_t*)&ta, sizeof(cur_alarm[MAX_ALARMS]))) return false;
	memcpy((void*)&alarms, (void*)&ta, sizeof(cur_alarm[MAX_ALARMS]));
#else

	File configFile = LittleFS.open(F("/alarms.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for alarms file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();
	
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	for( int i=0; i<min(MAX_ALARMS,(int)doc.size()); i++) {
		alarms[i].settings = doc[i]["s"];
		alarms[i].hour = doc[i]["h"];
		alarms[i].minute = doc[i]["m"];
		alarms[i].melody = doc[i]["me"];
		copy_string(alarms[i].text, doc[i]["t"], LENGTH_TEXT_ALARM);
	}
#endif
	return true;
}

void save_config_alarms(uint8_t chunk) {
#ifdef USE_NVRAM
	if(!writeBlock(NVRAM_CONFIG_ALARMS, (uint8_t*)&alarms, sizeof(cur_alarm[MAX_ALARMS]), chunk, sizeof(cur_alarm))) {
		LOG(println, PSTR("NVRAM: alarms config write error!"));
	}
#else

	JsonDocument doc; // временный буфер под объект json

	for( int i=0; i<MAX_ALARMS; i++) {
		doc[i]["s"] = alarms[i].settings;
		doc[i]["h"] = alarms[i].hour;
		doc[i]["m"] = alarms[i].minute;
		doc[i]["me"] = alarms[i].melody;
		doc[i]["t"] = alarms[i].text;
	}

	File configFile = LittleFS.open(F("/alarms.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(2);
#endif
}

bool load_config_texts() {
#ifdef USE_NVRAM
	cur_text tt[MAX_RUNNING];
	if(!readBlock(NVRAM_CONFIG_TEXTS, (uint8_t*)&tt, sizeof(cur_text[MAX_RUNNING]))) return false;
	memcpy((void*)&texts, (void*)&tt, sizeof(cur_text[MAX_RUNNING]));
	for( int i=0; i<MAX_RUNNING; i++) {
		textTimer[i].setInterval(texts[i].period*1000U);
	}
#else

	File configFile = LittleFS.open(F("/texts.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open config for texts file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();
	
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	for( int i=0; i<min(MAX_RUNNING,(int)doc.size()); i++) {
		// texts[i].text = doc[i]["t"].as<String>();
		copy_string(texts[i].text, doc[i]["t"], LENGTH_TEXT);
		texts[i].period = doc[i]["p"];
		texts[i].repeat_mode = doc[i]["r"];
		// сразу установка таймера
		textTimer[i].setInterval(texts[i].period*1000U);
	}
#endif
	return true;
}

void save_config_texts(uint8_t chunk) {
#ifdef USE_NVRAM
	if(!writeBlock(NVRAM_CONFIG_TEXTS, (uint8_t*)&texts, sizeof(cur_text[MAX_RUNNING]), chunk, sizeof(cur_text))) {
		LOG(println, PSTR("NVRAM: texts config write error!"));
	}
#else

	JsonDocument doc; // временный буфер под объект json

	for( int i=0; i<MAX_RUNNING; i++) {
		doc[i]["t"] = texts[i].text;
		doc[i]["p"] = texts[i].period;
		doc[i]["r"] = texts[i].repeat_mode;
	}

	File configFile = LittleFS.open(F("/texts.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing (texts)"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(4);
#endif
}

bool load_config_quote() {
#ifdef USE_NVRAM
	Quote_Settings ts;
	if(!readBlock(NVRAM_CONFIG_QUOTE, (uint8_t*)&ts, sizeof(Quote_Settings))) return false;
	memcpy(&qs, &ts, sizeof(Quote_Settings));
#else

	File configFile = LittleFS.open(F("/quote.json"), "r");
	if (!configFile) {
		// если файл не найден  
		LOG(println, PSTR("Failed to open quote config file"));
		return false;
	}

	JsonDocument doc; // временный буфер под объект json

	DeserializationError error = deserializeJson(doc, configFile);
	configFile.close();

	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return false;
	}

	qs.enabled = doc[F("enabled")];
	qs.period = doc[F("period")];
	qs.update = doc[F("update")];
	qs.server = doc[F("server")];
	qs.lang = doc[F("lang")];
	copy_string(qs.url, doc[F("url")], MAX_URL_LENGTH);
	copy_string(qs.params, doc[F("params")], MAX_PARAM_LENGTH);
	qs.method = doc[F("method")];
	qs.type = doc[F("type")];
	copy_string(qs.quote_field, doc[F("quote_field")], MAX_QUOTE_FIELD);
	copy_string(qs.author_field, doc[F("author_field")], MAX_QUOTE_FIELD);

#endif
	quoteUpdateTimer.setInterval(900000U * (qs.update+1));
	messages[MESSAGE_QUOTE].timer.setInterval(60000U * qs.period);
	return true;
}

void save_config_quote() {
#ifdef USE_NVRAM
	if(!writeBlock(NVRAM_CONFIG_QUOTE, (uint8_t*)&qs, sizeof(Quote_Settings))) {
		LOG(println, PSTR("NVRAM: quotes config write error!"));
	}
#else

	JsonDocument doc; // временный буфер под объект json

	doc[F("enabled")] = qs.enabled;
	doc[F("period")] = qs.period;
	doc[F("update")] = qs.update;
	doc[F("server")] = qs.server;
	doc[F("lang")] = qs.lang;
	doc[F("url")] = qs.url;
	doc[F("params")] = qs.params;
	doc[F("method")] = qs.method;
	doc[F("type")] = qs.type;
	doc[F("quote_field")] = qs.quote_field;
	doc[F("author_field")] = qs.author_field;

	File configFile = LittleFS.open(F("/quote.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush();
	configFile.close(); // не забыть закрыть файл
	delay(4);

#endif
}

bool load_config_weather() {
	return true;
}

void save_config_weather() {
	;
}