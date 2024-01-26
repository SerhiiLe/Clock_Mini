/*
	встроенный web сервер для настройки часов
	(для начальной настройки ip и wifi используется wifi_init)
*/

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <time.h>
#include "defines.h"
#include "web.h"
#include "settings.h"
#include "runningText.h"
#include "ntp.h"
#include "rtc.h"
#include "dfplayer.h"
#include "clock.h"
#include "wifi_init.h"
#include "barometer.h"

#define HPP(txt, ...) HTTP.client().printf_P(PSTR(txt), __VA_ARGS__)

ESP8266WebServer HTTP(80);
ESP8266HTTPUpdateServer httpUpdater;
bool web_isStarted = false;

void save_settings();
void save_alarm();
void off_alarm();
void save_text();
void off_text();
void sysinfo();
void play();
void maintence();
void set_clock();
void onoff();
void logout();
#ifdef USE_NVRAM
void make_config();
#endif

bool fileSend(String path);
bool need_save = false;
bool fl_mdns = false;

// отключение веб сервера для активации режима настройки wifi
void web_disable() {
	HTTP.stop();
	web_isStarted = false;
	LOG(println, PSTR("HTTP server stoped"));

	MDNS.close();
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
		if(fl_mdns) MDNS.update();
	} else {
		HTTP.begin();
		// Обработка HTTP-запросов
		HTTP.on(F("/save_settings"), save_settings);
		HTTP.on(F("/save_alarm"), save_alarm);
		HTTP.on(F("/off_alarm"), off_alarm);
		HTTP.on(F("/save_text"), save_text);
		HTTP.on(F("/off_text"), off_text);
		HTTP.on(F("/sysinfo"), sysinfo);
		HTTP.on(F("/play"), play);
		HTTP.on(F("/clear"), maintence);
		HTTP.on(F("/clock"), set_clock);
		HTTP.on(F("/onoff"), onoff);
		HTTP.on(F("/logout"), logout);
		#ifdef USE_NVRAM
		HTTP.on(F("/config.json"), make_config);
		#endif
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

		if(MDNS.begin(gs.str_hostname, WiFi.localIP())) {
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
			strncpy_P(var, HTTP.arg(name).c_str(), len-1);
			var[len-1] = 0;
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
// определение цвета
bool set_simple_color(const __FlashStringHelper * name, uint32_t &var) {
	if( HTTP.hasArg(name) ) {
		if( text_to_color(HTTP.arg(name).c_str()) != var ) {
			var = text_to_color(HTTP.arg(name).c_str());
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

	set_simple_string(F("str_hello"), gs.str_hello, LENGTH_HELLO);
	set_simple_string(F("str_hostname"), gs.str_hostname, LENGTH_HOSTNAME);
	set_simple_int(F("max_alarm_time"), gs.max_alarm_time, 1, 30);
	set_simple_int(F("run_allow"), gs.run_allow, 0, 2);
	set_simple_time(F("run_begin"), gs.run_begin);
	set_simple_time(F("run_end"), gs.run_end);
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
	set_simple_checkbox(F("tiny_clock"), gs.tiny_clock);
	set_simple_checkbox(F("date_short"), gs.show_date_short);
	set_simple_checkbox(F("tiny_date"), gs.tiny_date);
	if( set_simple_int(F("date_period"), gs.show_date_period, 20, 1439) )
		clockDate.setInterval(1000U * gs.show_date_period);
	if( set_simple_int(F("term_period"), gs.show_term_period, 20, 1439) )
		showTermTimer.setInterval(1000U * gs.show_term_period);
	set_simple_checkbox(F("tiny_term"), gs.tiny_term);
	set_simple_float(F("term_cor"), gs.term_cor, -100.0f, 100.0f, 1.0f);
	set_simple_int(F("bar_cor"), gs.bar_cor, -1000, 1000);
	set_simple_int(F("term_pool"), gs.term_pool, 30, 600);
	set_simple_checkbox(F("internet_weather"), gs.use_internet_weather);
	if( set_simple_int(F("sync_weather_period"), gs.sync_weather_period, 15, 1439) )
		syncWeatherTimer.setInterval(60000U * gs.sync_weather_period);
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
	if( set_simple_int(F("scroll_period"), gs.scroll_period, 20, 1440) )
		scrollTimer.setInterval(gs.scroll_period);
	set_simple_int(F("slide_show"), gs.slide_show, 1, 15);
	bool need_web_restart = false;
	if( set_simple_string(F("web_login"), gs.web_login, LENGTH_LOGIN) )
		need_web_restart = true;
	if( set_simple_string(F("web_password"), gs.web_password, LENGTH_PASSWORD) )
		need_web_restart = true;

	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_main();
	initRString(PSTR("Настройки сохранены"));
	if( sync_time ) syncTime();
	if( need_bright ) old_bright_boost = !old_bright_boost;
	if(need_web_restart) httpUpdater.setup(&HTTP, String(gs.web_login), String(gs.web_password));
}

// перезагрузка часов, сброс ком-порта, отключение сети и диска, чтобы ничего не мешало перезагрузке
void reboot_clock() {
	Serial.flush();
	WiFi.forceSleepBegin(); //disable AP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
	LittleFS.end();
	delay(1000);
	ESP.restart();
}

void maintence() {
	if(is_no_auth()) return;
	HTTP.sendHeader(F("Location"),"/");
	HTTP.send(303); 
	initRString(PSTR("Сброс"));
	if( HTTP.hasArg("t") ) {
		if( HTTP.arg("t") == "t" && LittleFS.exists(F("/texts.json")) ) {
			LOG(println, PSTR("reset texts"));
			LittleFS.remove(F("/texts.json"));
			reboot_clock();
		}
		if( HTTP.arg("t") == "a" && LittleFS.exists(F("/alarms.json")) ) {
			LOG(println, PSTR("reset alarms"));
			LittleFS.remove(F("/alarms.json"));
			reboot_clock();
		}
		if( HTTP.arg("t") == "c" 
			#ifndef USE_NVRAM
			&& LittleFS.exists(F("/config.json"))
			#endif
			 ) {
			LOG(println, PSTR("reset settings"));
			#ifdef USE_NVRAM
			Global_Settings ts;
			memcpy((void*)&gs, (void*)&ts, sizeof(gs));
			save_config_main();
			#else
			LittleFS.remove(F("/config.json"));
			#endif
			reboot_clock();
		}
		if( HTTP.arg("t") == "l" ) {
			LOG(println, PSTR("erase logs"));
			char fileName[32];
			for(int8_t i=0; i<SEC_LOG_COUNT; i++) {
				sprintf_P(fileName, SEC_LOG_FILE, i);
				if( LittleFS.exists(fileName) ) LittleFS.remove(fileName);
			}
			reboot_clock();
		}
		if( HTTP.arg("t") == "r" ) {
			reboot_clock();
		}
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
		set_simple_int(F("melody"), alarms[target].melody, 1, mp3_all);
		set_simple_int(F("txt"), alarms[target].text, -1, MAX_RUNNING-1);
	}
	HTTP.sendHeader(F("Location"),F("/alarms.html"));
	HTTP.send(303);
	delay(1);
	if( need_save ) save_config_alarms();
	mp3_stop();
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
			save_config_alarms();
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
		set_simple_string(F("text"), texts[target].text);
		if( set_simple_int(F("period"), texts[target].period, 30, 3600) )
			textTimer[target].setInterval(texts[target].period*1000U);
		set_simple_int(F("color_mode"), texts[target].color_mode, 0, 3);
		set_simple_color(F("color"), texts[target].color);
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
	if( need_save ) save_config_texts();
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
			save_config_texts();
			text_send(F("1"));
			initRString(PSTR("Текст отключён"));
		}
	} else
		text_send(F("0"));
}

// костыль для настройки режима повтора. Работает 50/50
void repeat_mode(uint8_t r) {
	static uint8_t old_r = 0;
	if(r==0) {
		if(old_r==1) mp3_disableLoop();
		if(old_r==2) mp3_disableLoopAll();
		// mp3_stop();
	}
	if(r==1) {
		if(old_r==2) mp3_disableLoopAll();
		mp3_enableLoop();
	}
	if(r==2) {
		if(old_r==1) mp3_disableLoop();
		mp3_enableLoopAll();
	}
	if(r==3) mp3_randomAll();
	old_r = r;
}

// обслуживает страничку плейера.
void play() {
	if(is_no_auth()) return;
	uint8_t p = 0;
	uint8_t r = 0;
	uint8_t v = 15;
	uint16_t c = 1;
	int t = 0;
	String name = "p";
	if( HTTP.hasArg(name) ) p = HTTP.arg(name).toInt();
	name = "c";
	if( HTTP.hasArg(name) ) c = constrain(HTTP.arg(name).toInt(), 1, mp3_all);
	name = "r";
	if( HTTP.hasArg(name) ) r = constrain(HTTP.arg(name).toInt(), 0, 3);
	name = "v";
	if( HTTP.hasArg(name) ) v = constrain(HTTP.arg(name).toInt(), 1, 30);
	switch (p)	{
		case 1: // предыдущий трек
			t = mp3_current - 1;
			if(t<1) t=mp3_all;
			mp3_play(t);
			break;
		case 2: // следующий трек
			t = mp3_current + 1;
			if(t>mp3_all) t=1;
			mp3_play(t);
			break;
		case 3: // играть
			repeat_mode(r);
			delay(10);
			mp3_play(c);
			break;
		case 4: // пауза
			mp3_pause();
			break;
		case 5: // режим
			repeat_mode(r);
			break;
		case 6: // остановить
			mp3_stop();
			break;
		case 7: // тише
			t = cur_Volume - 1;
			if(t<0) t=0;
			mp3_volume(t);
			break;
		case 8: // громче
			t = cur_Volume + 1;
			if(t>30) t=30;
			mp3_volume(t);
			break;
		case 9: // громкость
			mp3_volume(v);
			break;
		default:
			// if(!mp3_isInit) mp3_init();
			// else mp3_reread();
			// if(mp3_isInit) mp3_update();
			mp3_reread();
			mp3_update();
			break;
	}
	char buff[20];
	sprintf_P(buff,PSTR("%i:%i:%i:%i"),mp3_current,mp3_all,cur_Volume,mp3_isPlay());
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
		HTTP.client().stop();
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
		}// else
		// if(HTTP.arg(name) == F("security")) {
		// 	// включает/выключает режим "охраны"
		// 	if(a) sec_enable = !(bool)sec_enable;
		// 	cond = sec_enable;
		// 	if(a) {
		// 		save_log_file(cond?SEC_TEXT_ENABLE:SEC_TEXT_DISABLE);
		// 		save_config_security();
		// 	}
		// }
	}
	text_send(cond?F("1"):F("0"));
}

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
	HPP("\"Temperature\":%1.1f,", getTemperature());
	HPP("\"Pressure\":%u,", getPressure()/100);
	HPP("\"Rssi\":%i,", wifi_rssi());
	HPP("\"FreeHeap\":%i,", ESP.getFreeHeap());
	HPP("\"MaxFreeBlockSize\":%i,", ESP.getMaxFreeBlockSize());
	HPP("\"HeapFragmentation\":%i,", ESP.getHeapFragmentation());
	HPP("\"CpuFreqMHz\":%i,", ESP.getCpuFreqMHz());
	HPP("\"ResetReason\":\"%s\",", ESP.getResetReason().c_str());
	HPP("\"TimeDrift\":%i,", getTimeU()-getRTCTimeU());
	HPP("\"NVRAM\":%i,", nvram_enable);
	HPP("\"FullVersion\":\"%s\",", ESP.getFullVersion().c_str());
	HPP("\"BuildTime\":\"%s %s\"}", F(__DATE__), F(__TIME__));
	HTTP.client().stop();
}

#ifdef USE_NVRAM
void make_config() {
	if(is_no_auth()) return;
	// char buf[100];
	HTTP.client().print(PSTR("HTTP/1.1 200\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n{"));
	HPP("\"str_hello\":\"%s\",", gs.str_hello);
	HPP("\"str_hostname\":\"%s\",", gs.str_hostname);
    HPP("\"max_alarm_time\":%u,", gs.max_alarm_time);
    HPP("\"run_allow\":%u,", gs.run_allow);
    HPP("\"run_begin\":%u,", gs.run_begin);
    HPP("\"run_end\":%u,", gs.run_end);
    HPP("\"show_move\":%u,", gs.show_move);
    HPP("\"delay_move\":%u,", gs.delay_move);
    HPP("\"tz_shift\":%i,", gs.tz_shift);
    HPP("\"tz_dst\":%u,", gs.tz_dst);
    HPP("\"sync_time_period\":%u,", gs.sync_time_period);
    HPP("\"tz_adjust\":%u,", gs.tz_adjust);
	HPP("\"tiny_clock\":%u,", gs.tiny_clock);
    HPP("\"date_short\":%u,", gs.show_date_short);
    HPP("\"tiny_date\":%u,", gs.tiny_date);
    HPP("\"date_period\":%u,", gs.show_date_period);
	HPP("\"term_period\":%u,", gs.show_term_period);
    HPP("\"tiny_term\":%u,", gs.tiny_term);
	HPP("\"term_cor\":%1.1f,", gs.term_cor);
	HPP("\"bar_cor\":%i,", gs.bar_cor);
	HPP("\"term_pool\":%u,", gs.term_pool);
	HPP("\"internet_weather\":%u,", gs.use_internet_weather);
	HPP("\"sync_weather_period\":%u,", gs.sync_weather_period);
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
    HPP("\"web_login\":\"%s\",", gs.web_login);
    HPP("\"web_password\":\"%s\"}", gs.web_password);
	HTTP.client().stop();
}
#endif