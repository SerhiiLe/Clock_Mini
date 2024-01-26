/*
	Работа с настройками.
	Инициализация по умолчанию, чтение из файла, сохранение в файл
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "defines.h"
#include "settings_init.h"
#include "settings.h"
#include "ntp.h"
#include "nvram.h"

byte char_to_byte(char n) {
	if(n>='0' && n<='9') return (byte)n - 48;
	if(n>='A' && n<='F') return (byte)n - 55;
	if(n>='a' && n<='f') return (byte)n - 87;
	return 48;
}

uint32_t text_to_color(const char *s) {
	uint32_t c = 0;
	int8_t i = 0;
	byte t = 0;
	char a[7];
	for(i=0; i<(int8_t)strlen(s); i++ ) {
		if( c==6 ) break;
		if( isxdigit(s[i]) ) a[c++] = s[i];
	}
	if(c<3) for(i=c; i>3; i++) a[i] = 'f';
	a[6] = '\0';
	c = 0;
	if(strlen(a)<6) {
		for(i=0; i<3; i++) {
			t = char_to_byte(a[2-i]);
			c |= (t << (i*8)) | (t << (i*8+4)); 
		}
	} else {
		for(i=0; i<6; i++) {
			t = char_to_byte(a[5-i]);
			c |= t << (i*4);
		}
	}
	return c;
}

String color_to_text(uint32_t c) {
	char a[] = "#ffffff";
	byte t = 0;
	for(int8_t i=1; i<=6; i++) {
		t = (byte)((c >> ((6-i)*4)) & 0xF);
		t += t<10? 48: 87;
		a[i] = (char)t;
	}
	return String(a);
}

void copy_string(char* dst, const char* src, size_t len) {
	if(src != nullptr) {
		strncpy(dst, src, len-1);
		dst[len-1] = 0;
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

	// LOG(printf_P, PSTR("размер объекта config: %i\n"), doc.memoryUsage());
#endif
	clockDate.setInterval(1000U * gs.show_date_period);
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

	// LOG(printf_P, PSTR("размер объекта config: %i\n"), doc.memoryUsage());
#endif
}

bool load_config_alarms() {
	cur_alarm ta[MAX_ALARMS];
	// if(!readBlock(NVRAM_CONFIG_ALARMS, (uint8_t*)&ta, sizeof(ta))) return false;
	// memcpy((void*)&alarms, (void*)&ta, sizeof(alarms));
	readBlock(NVRAM_CONFIG_ALARMS, (uint8_t*)&ta, sizeof(cur_alarm[MAX_ALARMS]));


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
		alarms[i].text = doc[i]["t"];
	}

	// LOG(printf_P, PSTR("размер объекта alarms: %i\n"), doc.memoryUsage());
	return true;
}

void save_config_alarms() {
	writeBlock(NVRAM_CONFIG_ALARMS, (uint8_t*)&alarms, sizeof(cur_alarm[MAX_ALARMS]));

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

	// LOG(printf_P, PSTR("размер объекта alarms: %i\n"), doc.memoryUsage());
}

bool load_config_texts() {
	// if(!fs_isStarted) return false;

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
		texts[i].text = doc[i]["t"].as<String>();
		texts[i].color_mode = doc[i]["cm"];
		texts[i].color = text_to_color(doc[i]["c"]);
		texts[i].period = doc[i]["p"];
		texts[i].repeat_mode = doc[i]["r"];
		// сразу установка таймера
		textTimer[i].setInterval(texts[i].period*1000U);
	}

	// LOG(printf_P, PSTR("размер объекта texts: %i\n"), doc.memoryUsage());
	return true;
}

void save_config_texts() {
	// if(!fs_isStarted) return;

	JsonDocument doc; // временный буфер под объект json

	for( int i=0; i<MAX_RUNNING; i++) {
		doc[i]["t"] = texts[i].text;
		doc[i]["cm"] = texts[i].color_mode;
		doc[i]["c"] = color_to_text(texts[i].color);
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

	// LOG(printf_P, PSTR("размер объекта texts: %i\n"), doc.memoryUsage());
}

bool load_config_security() {
	if(!fs_isStarted) return false;

	File configFile = LittleFS.open(F("/security.json"), "r");
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

	sec_enable = doc[F("sec_enable")];
	sec_curFile = doc[F("sec_curFile")];

	// LOG(printf_P, PSTR("размер объекта security: %i\n"), doc.memoryUsage());
	return true;
}

void save_config_security() {
	if(!fs_isStarted) return;

	JsonDocument doc; // временный буфер под объект json

	doc[F("sec_enable")] = sec_enable;
	doc[F("sec_curFile")] = sec_curFile;
	doc[F("logs_count")] = SEC_LOG_COUNT;

	File configFile = LittleFS.open(F("/security.json"), "w"); // открытие файла на запись
	if (!configFile) {
		LOG(println, PSTR("Failed to open config file for writing (texts)"));
		return;
	}
	serializeJson(doc, configFile); // Записываем строку json в файл
	configFile.flush(); // подождать, пока данные запишутся. Хотя close должен делать это сам, но без иногда перезагружается.
	configFile.close(); // не забыть закрыть файл
	delay(2);

	// LOG(printf_P, PSTR("размер объекта security: %i\n"), doc.memoryUsage());
}

// чтение последних cnt строк лога
String read_log_file(int16_t cnt) {
	if(!fs_isStarted) return String(F("no fs"));

	// всего надо отдать cnt последних строк.
	// Если файл только начал писаться, то надо показать последние записи предыдущего файла
	// сначала считывается предыдущий файл
	int16_t cur = 0;
	int16_t aCnt = 0;
	char aStr[cnt][SEC_LOG_MAX];
	uint8_t prevFile = sec_curFile > 0 ? sec_curFile-1: SEC_LOG_COUNT-1; // вычисление предыдущего лог-файла
	char fileName[32];
	sprintf_P(fileName, SEC_LOG_FILE, prevFile);
	File logFile = LittleFS.open(fileName, "r");
	if(logFile) {
		while(logFile.available()) {
			strncpy(aStr[cur], logFile.readStringUntil('\n').c_str(), SEC_LOG_MAX); // \r\n
			cur = (cur+1) % cnt;
			aCnt++;
		}
	}
	logFile.close();
	// теперь считывается текущий файл
	sprintf_P(fileName, SEC_LOG_FILE, sec_curFile);
	logFile = LittleFS.open(fileName, "r");
	if(logFile) {
		while(logFile.available()) {
			strncpy(aStr[cur], logFile.readStringUntil('\n').c_str(), SEC_LOG_MAX);
			cur = (cur+1) % cnt;
			aCnt++;
		}
	}
	logFile.close();
	// теперь надо склеить массив в одну строку и отдать назад
	String str = "";
	char *ptr;
	for(int16_t i = min(aCnt,cnt); i > 0; i--) {
		cur = cur > 0 ? cur-1: cnt-1;
		ptr = aStr[cur] + strlen(aStr[cur]) - 1;
		strcpy(ptr, "%0A"); // замена последнего символа на url-код склейки строк
		str += aStr[cur];
	}
	return str;
}

void save_log_file(uint8_t mt) {
	if(!fs_isStarted) return;

	char fileName[32];
	sprintf_P(fileName, SEC_LOG_FILE, sec_curFile);
	File logFile = LittleFS.open(fileName, "a");
	if (!logFile) {
		// не получилось открыть файл на дополнение
		LOG(println, PSTR("Failed to open log file"));
		return;
	}
	// проверка, не превышен ли лимит размера файла, если да, то открыть второй файл.
	size_t size = logFile.size();
	if (size > SEC_LOG_SIZE) {
		LOG(println, PSTR("Log file size is too large, switch file"));
		logFile.close();
		sec_curFile = (sec_curFile+1) % SEC_LOG_COUNT;
		save_config_security();
		sprintf_P(fileName, SEC_LOG_FILE, sec_curFile);
		logFile = LittleFS.open(fileName, "w");
		if (!logFile) {
			// ошибка создания файла
			LOG(println, PSTR("Failed to open new log file"));
			return;
		}
	}
	// составление строки которая будет занесена в файл
	char str[SEC_LOG_MAX];
	tm t = getTime();
	const char *lm = nullptr;
	switch (mt)	{
		case SEC_TEXT_DISABLE: lm = PSTR("Stop."); break;
		case SEC_TEXT_ENABLE: lm = PSTR("Start."); break;
		case SEC_TEXT_MOVE: lm = PSTR("Move!"); break;
		case SEC_TEXT_BRIGHTNESS: lm = PSTR("Brightness!"); break;
		case SEC_TEXT_BOOT: lm = PSTR("Boot clock"); break;
		case SEC_TEXT_POWERED: lm = PSTR("Power is ON"); break;
		case SEC_TEXT_POWEROFF: lm = PSTR("Power is OFF"); break;
	}
	snprintf_P(str, SEC_LOG_MAX, PSTR("%04u-%02u-%02u %02u:%02u:%02u : %s"), t.tm_year +1900, t.tm_mon +1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, lm);

	LOG(println, str);
	logFile.println(str);

	logFile.flush();
	logFile.close();
	delay(2);
}