/*
Различные запросы из Internet
Погода: https://open-meteo.com/
Цитаты: https://generator-online.com/

Всё в кучу потому, что запросы в общем-то аналогичные и не хочется размазывать один код по разным файлам. Так делать не хорошо.
*/

// #define USE_HTTPS

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
// #ifdef USE_HTTPS
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
// #else
#include <WiFiClient.h>
// #endif
#include <ArduinoJson.h>
#include "defines.h"
#include "webClient.h"
#include "settings.h"

BearSSL::WiFiClientSecure WEB_S;
WiFiClient WEB_P;
HTTPClient httpReq;

Quote_Server quote;

bool fl_https_notInit = true;

void https_Init() {
	WEB_S.setBufferSizes(1536, 256);
	WEB_S.setInsecure();
	fl_https_notInit = false;
}

struct weatherData {
	int utc_offset_seconds;
	float temperature;
	float apparent_temperature;
	uint8_t humidity;
	uint8_t weather_code;
	uint8_t cloud_cover;
	float pressure;
	float wind_speed;
	float wind_gusts;
	uint16_t wind_direction;
} wd;

uint8_t parseWeather(const char* json) {
	LOG(println, json);

	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, json);
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return 0;
	}

	const char current[] = "current";
	wd.utc_offset_seconds = doc[F("utc_offset_seconds")];
	wd.temperature = doc[current][F("temperature_2m")];
	wd.apparent_temperature = doc[current][F("apparent_temperature")];
	wd.humidity = doc[current][F("relative_humidity_2m")];
	wd.weather_code = doc[current][F("weather_code")];
	wd.cloud_cover = doc[current][F("cloud_cover")];
	wd.pressure = doc[current][F("surface_pressure")];
	wd.wind_direction = doc[current][F("wind_direction_10m")];
	wd.wind_speed = doc[current][F("wind_speed_10m")];
	wd.wind_gusts = doc[current][F("wind_gusts_10m")];

	char txt[512];
	sprintf_P(txt, PSTR("Погода: %+0.1f\xc2\xb0\x43 по ощущениям %+0.1f\xc2\xb0\x43 влажность %u%% облачность %u%% ветер %1.0fм/сек порывы %1.0fм/сек."),
		wd.temperature, wd.apparent_temperature, wd.humidity, wd.cloud_cover, wd.wind_speed, wd.wind_gusts);
	messages[MESSAGE_WEATHER].text = String(txt);
	messages[MESSAGE_WEATHER].count = 100;

	return 1;
}

// https://api.open-meteo.com/v1/forecast?latitude=46.4857&longitude=30.7438&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,cloud_cover,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&wind_speed_unit=ms&timeformat=unixtime&timezone=auto&past_days=1&forecast_days=1

/*
Статусы ответа:
0 - ошибка обработки запроса
1 - успешно
2 - адрес не работает
3 - ошибка сети
*/

uint8_t weatherUpdate() {
	if (fl_https_notInit) https_Init();
	// WiFiClient WEB_client;
	// HTTPClient httpReq;

	char LatLong[50];
	sprintf_P(LatLong, PSTR("latitude=%.4f&longitude=%.4f"), gs.latitude, gs.longitude);
	uint8_t status = 1;
	#ifdef USE_HTTPS
	String req = F("https");
	#else
	String req = F("http");
	#endif
	req += F("://api.open-meteo.com/v1/forecast?");
	req += LatLong;
	req += F("&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,cloud_cover,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&wind_speed_unit=ms&timeformat=unixtime&timezone=auto&past_days=0&forecast_days=1");
	LOG(println, req);
	if (httpReq.begin(WEB_P, req)) {
		int httpCode = httpReq.GET();
		LOG(printf_P, PSTR("http answer code: %i, %s\n"), httpCode, httpReq.errorToString(httpCode).c_str());
		if( httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY )
			status = parseWeather(httpReq.getString().c_str()); //, _httpReq.getSize());
		else status = 2;
		httpReq.end();
	} else status = 3;
	return status;
}

/*
Эти переходы между различными кодировками немного задолбали. На языках более высокого уровня обычно этого даже не замечаешь, а здесь функции длинной в километр.
*/

// Конвертер utf16 вида \uABCD в текст utf8
String decodeUTF16(String unicodeStr) {
	// String out = "";
	int len = unicodeStr.length();
	char out[len];
	char* cursor = out;
	char iChar;
	char* error; // указатель на символ который не является шестнадцатеричным числом.
	char unicode[6]; // буфер в котором будем создавать число по формату функции strtol 0xABCD
	for (int i = 0; i < len; i++) {
		iChar = unicodeStr[i];
		if(iChar == '\\') { // если найден esc символ, то приступаем
			iChar = unicodeStr[++i];
			if(iChar == 'u') { // о, да это же похоже на utf16
				unicode[0] = '0';
				unicode[1] = 'x';
				for (int j = 0; j < 4; j++){
					iChar = unicodeStr[++i];
					unicode[j + 2] = iChar;
				}
				long uFirst = strtol(unicode, &error, 16); // первый промежуточный вариант

				uint32_t codepoint = 0x0;
				// utf16 может быть 16 бит и 32 бита (utf8 может иметь 8, 16, 24, 32 бита)
				if( uFirst <= 0xD7FF ) { // это похоже на 16 битный вариант utf16
					codepoint = uFirst;
				} else if (uFirst <= 0xDBFF) { // это похоже на 32 битный вариант utf16
					// надо повторить предыдущий шаг, чтобы получить ещё 16 бит.
					unicode[0] = '0';
					unicode[1] = 'x';
					for (int j = 0; j < 4; j++){
						iChar = unicodeStr[++i];
						unicode[j + 2] = iChar;
					}
					long uSecond = strtol(unicode, &error, 16); // второй промежуточный вариант
					codepoint = (((uFirst - 0xD800) << 10) | (uSecond - 0xDC00)) + 0x10000;
				}
		        //-------(2) Codepoint to UTF-8 -------
				if( codepoint <= 0x007F ) {
					*cursor++ = (char)codepoint;
        		} else if( codepoint <= 0x07FF ) {
					*cursor++ = ((codepoint >> 6) & 0x1F) | 0xC0;
					*cursor++ = (codepoint & 0x3F) | 0x80;
				} else if( codepoint <= 0xFFFF ) {
					*cursor++ = ((codepoint >> 12) & 0x0F) | 0xE0;
					*cursor++ = ((codepoint >> 6) & 0x3F) | 0x80;
					*cursor++ = ((codepoint) & 0x3F) | 0x80;
				} else if (codepoint <= 0x10FFFF) {
					*cursor++ = ((codepoint >> 18) & 0x07) | 0xF0;
					*cursor++ = ((codepoint >> 12) & 0x3F) | 0x80;
					*cursor++ = ((codepoint >> 6) & 0x3F) | 0x80;
					*cursor++ = ((codepoint) & 0x3F) | 0x80;
				}
			// Кроме непосредственно utf16 могут быть другие символы, которые должны быть экранированы в json
			} else if(iChar == '\\' || iChar == '\"' || iChar == '\'') {
				*cursor++ = iChar;
			} else if(iChar == 'n') {
				*cursor++ = '\n';
			} else if(iChar == 'r') {
				*cursor++ = '\r';
			} else if(iChar == 't') {
				*cursor++ = ' '; // '\t';
			} else if(iChar == 'b') {
				; // *cursor++ = '\b';
			} else if(iChar == 'f') {
				; // *cursor++ += '\f';
			}
		} else {
			*cursor++ = iChar;
		}
	}
	*cursor = 0;
	return String(out);
}

String digJSON(String& str, const char* search, bool json=true) {
	if( strlen(search) == 0 ) return String("");
	int s1, s2, s3;
	s1 = str.indexOf(search);
	if( s1 > 0 ) {
		s2 = str.indexOf(json ? ":\"": ">", s1);
		if( s2 > 0 ) {
			s2 += json ? 2: 1;
			s3 = str.indexOf(json ? "\"": "</", s2);
			if( s3 > 0 ) {
				return decodeUTF16(str.substring(s2, s3));
			}
		}
	}
	return String("");
}

// обрезание лишних 
void myTrim(String& str) {
	str.replace("\n", "");
	str.replace("\r", "");
	str.trim();
}

// выделение текста из json/XML
void parseQuote(String txt, bool type=true) {
	String s = digJSON(txt, quote.quote, type);
	if( s.length() > 0 ) myTrim(s);
	messages[MESSAGE_QUOTE].text = s;
	s = digJSON(txt, quote.author, type);
	if( s.length() > 0 ) {
		myTrim(s);
		messages[MESSAGE_QUOTE].text += " (" + s + ")";
	}
	if( messages[MESSAGE_QUOTE].text.length() == 0 )
		LOG(printf_P, PSTR("Error parse JSON/XML.\nSource:\n%s\n"), txt.c_str());
	else
		LOG(printf_P, PSTR("Quote: %s\n"), messages[MESSAGE_QUOTE].text.c_str());
}

void quoteGet() {
	unsigned long start_time = millis();
	if (fl_https_notInit) https_Init();
	bool fl_isSecure = strstr(quote.url, "https://") != NULL;

	String req = quote.url; // F("api.forismatic.com/api/1.0/?method=getQuote&format=text&lang=ru");
	String params = quote.params;
	if( params.length() > 0 ) {
		switch (qs.lang) {
			case 1:
				params += F("en");
				break;
			case 2:
				params += F("ru");
				break;
			case 3:
				params += F("uk");
				break;
		}
		if(quote.method == Q_GET) {
			req += "?";
			req += params;
		}
	}

	LOG(println, req);
	if (httpReq.begin(fl_isSecure? WEB_S: WEB_P, req)) {
		int httpCode;
		if(quote.method) {
			httpReq.addHeader("Content-Type", "application/x-www-form-urlencoded");
			httpCode = httpReq.POST(params);
		} else
			httpCode = httpReq.GET();

		LOG(printf_P, PSTR("http answer code: %i, %s\n"), httpCode, httpReq.errorToString(httpCode).c_str());
		if( httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY ) {
			if(quote.type)
				parseQuote(httpReq.getString(), quote.type == Q_JSON);
			else
				messages[MESSAGE_QUOTE].text = httpReq.getString();
			messages[MESSAGE_QUOTE].count = 100;
			messages[MESSAGE_QUOTE].timer.setInterval(60000U * qs.period);
		}
		httpReq.end();
	}
	LOG(printf_P, PSTR("request time is: %lu msec\n"), millis()-start_time);
}

// Заполнение структуры с параметрами сервера.
void quotePrepare(bool force) {
	if( !quote.fl_init || force ) {
		switch (qs.server) {
			case 1: // ultragenerator.com
				if(random(0,1)) {
					strncpy_P(quote.url, PSTR("https://ultragenerator.com/citaty/handler.php"), MAX_URL_LENGTH);
					quote.type = Q_XML;
					strncpy_P(quote.quote, PSTR("quote"), MAX_QUOTE_FIELD);
					strncpy_P(quote.author, PSTR("author"), MAX_QUOTE_FIELD);
				} else {
					strncpy_P(quote.url, PSTR("https://ultragenerator.com/facts/handler.php"), MAX_URL_LENGTH);
					quote.type = Q_JSON;
					strncpy_P(quote.quote, PSTR("text"), MAX_QUOTE_FIELD);
					quote.author[0] = 0;
				}
				quote.params[0] = 0; // у этого сервиса нет параметров. И вообще не факт, что это самостоятельный сервис
				quote.method = Q_GET;
				break;
			case 2: // own server
				strncpy(quote.url, qs.url, MAX_URL_LENGTH);
				strncpy(quote.params, qs.params, MAX_PARAM_LENGTH);
				quote.method = qs.method;
				quote.type = qs.type;
				strncpy(quote.quote, qs.quote_field, MAX_QUOTE_FIELD);
				strncpy(quote.author, qs.author_field, MAX_QUOTE_FIELD);
				break;
			default: // forismatic.com
				strncpy_P(quote.url, PSTR("http://api.forismatic.com/api/1.0/"), MAX_URL_LENGTH);
				strncpy_P(quote.params, PSTR("method=getQuote&format=text&lang="), MAX_PARAM_LENGTH);
				quote.method = Q_GET;
				quote.type = Q_TEXT;
				quote.quote[0] = 0;
				quote.author[0] = 0;
				break;
		}
		quote.url[MAX_URL_LENGTH-1] = 0;
		quote.params[MAX_PARAM_LENGTH-1] = 0;
		quote.quote[MAX_QUOTE_FIELD-1] = 0;
		quote.author[MAX_QUOTE_FIELD-1] = 0;
		quote.fl_init = true;
	}
}

void quoteUpdate() {
	quotePrepare(qs.server == 1);
	quoteGet();
}
