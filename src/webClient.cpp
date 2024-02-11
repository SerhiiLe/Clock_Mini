/*
Различные запросы из Internet
Погода: https://open-meteo.com/
Цитаты: https://generator-online.com/

Всё в кучу потому, что запросы в общем-то аналогичные и не хочется размазывать один код по разным файлам. Так делать не хорошо.
*/

// #define USE_HTTPS

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#ifdef USE_HTTPS
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#else
#include <WiFiClient.h>
#endif
#include <ArduinoJson.h>
#include "defines.h"
#include "webClient.h"

#ifdef USE_HTTPS
static BearSSL::WiFiClientSecure WEB_client;
#else
static WiFiClient WEB_client;
#endif
static HTTPClient httpReq;

bool fl_https_notInit = true;

void https_Init() {
	#ifdef USE_HTTPS
	WEB_client.setBufferSizes(2048, 256);
	WEB_client.setInsecure();
	#endif
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
	if (httpReq.begin(WEB_client, req)) {
		int httpCode = httpReq.GET();
		LOG(printf_P, PSTR("http answer code: %i, %s\n"), httpCode, httpReq.errorToString(httpCode).c_str());
		if( httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY )
			status = parseWeather(httpReq.getString().c_str()); //, _httpReq.getSize());
		else status = 2;
		httpReq.end();
	} else status = 3;
	return status;
}

void copy_string2(char* dst, const char* src, size_t len) {
	if(src != nullptr) {
		strncpy(dst, src, len);
		dst[len] = 0;
	} else
		dst[0] = 0;
}

struct quoteData {
	char author[50] = "";
	char quote[256] = "";
} qd;

uint8_t parseQuote(const char* json) {
	LOG(println, json);

	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, json);
	// Test if parsing succeeds.
	if (error) {
		LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
		return 0;
	}

	copy_string2(qd.author, doc[F("data")][F("result")][F("author")], sizeof(qd.author));
	copy_string2(qd.quote, doc[F("data")][F("result")][F("quote")], sizeof(qd.quote));

	return 1;
}

/*
/// HTTP client errors
#define HTTPC_ERROR_CONNECTION_FAILED   (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED       (-4)
#define HTTPC_ERROR_CONNECTION_LOST     (-5)
#define HTTPC_ERROR_NO_STREAM           (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
#define HTTPC_ERROR_ENCODING            (-9)
#define HTTPC_ERROR_STREAM_WRITE        (-10)
#define HTTPC_ERROR_READ_TIMEOUT        (-11)
*/

// https://generator-online.com/api/v1/quotes -d "language=uk"

uint8_t quoteUpdate_off() {
	if (fl_https_notInit) https_Init(); 
	uint8_t status = 1;
	LOG(println, PSTR("send quote request"));
	// BearSSL, не реализует шифрование, требуемое сервером :(
	// if (_httpReq.begin(_SSL_client, F("https://generator-online.com/api/v1/quotes"))) {
	if (httpReq.begin(WEB_client, F("https://joy.od.ua/is_full_info.php"))) {
		httpReq.addHeader("Content-Type", "application/x-www-form-urlencoded");
		int httpCode = httpReq.POST("language=uk");
		LOG(printf_P, PSTR("https answer code: %i, %s\n"), httpCode, httpReq.errorToString(httpCode).c_str());
		if( httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY )
			status = parseWeather(httpReq.getString().c_str());
		else status = 2;
		httpReq.end();
	} else status = 3;
	return status;
}

uint8_t quoteUpdate() {
	if (fl_https_notInit) https_Init(); 
	uint8_t status = 1;
	#ifdef USE_HTTPS
	String req = F("https");
	#else
	String req = F("http");
	#endif
	req += F("://api.forismatic.com/api/1.0/?method=getQuote&format=text&lang=ru");
	LOG(println, req);
	if (httpReq.begin(WEB_client, req)) {
		int httpCode = httpReq.GET();
		LOG(printf_P, PSTR("http answer code: %i, %s\n"), httpCode, httpReq.errorToString(httpCode).c_str());
		if( httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY )
			status = parseWeather(httpReq.getString().c_str());
		else status = 2;
		httpReq.end();
	} else status = 3;
	return status;
}
