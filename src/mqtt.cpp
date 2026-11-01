#include <Arduino.h>
#include "defines.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#ifdef ESP8266
#include <WiFiClientSecureBearSSL.h>
#endif
#include "mqtt.h"
#include "barometer.h"
#include "ntp.h"
#include "wifi_init.h"

#ifdef ESP8266
BearSSL::WiFiClientSecure WiFi_S;
#else
WiFiClientSecure WiFi_S;
#endif
WiFiClient WiFi_P;

PubSubClient mqtt;

bool mqtt_is_init = false;
String mqtt_ID = "";
String mqtt_LWT = "";
String mqtt_STATE = "";
String mqtt_CMD = "";
String mqtt_REPLY = "";

TimerMinim timerMQTTreconnect(1000ul * 5); // таймер для переподключения
TimerMinim timerMQTTSendStatus(1000ul * 120); // таймер для отсылки обновлений статуса

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
	// разбор команды
	String message = "";
	message.reserve(length+1);
	for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
	
	LOG(printf_P, PSTR("Message in topic [%s]: %s\n"), topic, message.c_str());

	// в данный момент всё, что можно сделать - отравить строку на экран.
	// два варианта, просто текст и json с параметрами. Просто для эксперимента. Смысла в этом нет.
	if (String(topic) == mqtt_CMD) {
		if (message.startsWith(F("{")) && message.endsWith(F("}"))) {
			// сообщение в JSON. Наверное.
			JsonDocument doc;
			DeserializationError error = deserializeJson(doc, message);
			// Test if parsing succeeds.
			if (error) {
				LOG(printf_P, PSTR("deserializeJson() failed: %s\n"), error.c_str());
				mqtt.publish(mqtt_REPLY.c_str(), error.c_str());
				return;
			}
			String key = F("msg");
			if (doc[key].is<String>()) {
				String msg = doc[key].as<String>();
				messages[MESSAGE_WEB].text = msg;
				key = F("cnt");
				if (doc[key].is<int>()) {
					int cnt = doc[key];
					messages[MESSAGE_WEB].count = cnt;
				} else messages[MESSAGE_WEB].count = 2;
				key = F("int");
				if (doc[key].is<int>()) {
					int interval = doc[key];
					messages[MESSAGE_WEB].timer.setInterval(interval * 1000ul);
				} else messages[MESSAGE_WEB].timer.setInterval(20 * 1000ul);
				key = F("tone");
				if (doc[key].is<int>()) {
					int tone = doc[key];
					one_shot_message_tone = tone;
				} else one_shot_message_tone = 2;
			}
		} else {
			// просто отправка текста с параметрами по умалчанию
			messages[MESSAGE_WEB].text = message;
			messages[MESSAGE_WEB].count = 2;
			messages[MESSAGE_WEB].timer.setInterval(20 * 1000ul);
			one_shot_message_tone = 2;
		}
		messages[MESSAGE_WEB].timer.setNext(50);
		
		mqtt.publish(mqtt_REPLY.c_str(), message.c_str());
	}
}

void mqtt_begin() {
	// если уже были подключены, то отключится
	if (mqtt_is_init) {
		mqtt.unsubscribe(mqtt_CMD.c_str());
		mqtt.disconnect();
		mqtt_is_init = false;
	}
	// если выключено или адрес не указан, то включать нечего	
	if (!ms.enable || !strlen(ms.address) || !strlen(ms.topic)) return;
	// вычисление ascii части имени устройства, хотя оно и так должно быть только в ascii
	String hostName = "";
	hostName.reserve(strlen(gs.str_hostname)+1);
	int i = 0;
	while (gs.str_hostname[i]>0) hostName += gs.str_hostname[i++];
	// создание уникального имени
	if (hostName.length()<12) {
		// определения mac адреса для формирования уникального идентификатора устройства
		String mac = wifi_macAddress();
		mac.replace(":", "");
		if (hostName.length()) {
			mqtt_ID = hostName + F("_");
			mqtt_ID += mac.substring(mqtt_ID.length());
		} else
			mqtt_ID = mac;
	} else {
		mqtt_ID = hostName;
	}
	LOG(printf_P, PSTR("MQTT device ID: %s\n"), mqtt_ID.c_str());
	// формирование строк - топиков
	String mqtt_tmp = String(ms.topic) + F("/") + mqtt_ID + F("/");
	mqtt_LWT 	= mqtt_tmp + F("LWT");
	mqtt_STATE 	= mqtt_tmp + F("STATE");
	mqtt_CMD 	= mqtt_tmp + F("CMD");
	mqtt_REPLY 	= mqtt_tmp + F("REPLY");
	// выбор транспорта для подключения
	if (ms.ssl) {
		WiFi_S.setInsecure();
		mqtt.setClient(WiFi_S);
	} else {
		mqtt.setClient(WiFi_P);
	}
	// остальные параметры подключения
	mqtt.setServer(ms.address, ms.port);
	mqtt.setCallback(mqtt_callback);
	mqtt_is_init = true;
}

bool mqtt_connect() {
	static uint8_t errors = 0;
	if (!mqtt_is_init) return false;
	if (mqtt.connected()) return true;
	if (!timerMQTTreconnect.isReady()) return false;

	LOG(print, PSTR("Connecting to MQTT..."));

	// НАСТРОЙКА LWT (Last Will and Testament)
	// Параметры: topic, qos, retain, payload
	int lwt_qos = 1;
	bool lwt_retain = true;

	const char* login = strlen(ms.login) ? ms.login: NULL;
	const char* password = login && strlen(ms.password) ? ms.password: NULL;

	// Подключение с логином, паролем и LWT
	if (mqtt.connect(mqtt_ID.c_str(), login, password, mqtt_LWT.c_str(), lwt_qos, lwt_retain, String(F("Offline")).c_str())) {
		LOG(println, PSTR("connect succesful"));
		
		// Сразу после подключения публикуем статус "online" в тот же топик LWT
		mqtt.publish(mqtt_LWT.c_str(), String(F("Online")).c_str(), true);
		
		mqtt.subscribe(mqtt_CMD.c_str());
	} else {
		LOG(printf_P, PSTR("error, rc=%i\n"), mqtt.state());
		timerMQTTreconnect.setNext(networkErrorPenalty(errors));
		return false;
	}
	return true;
}

void mqtt_send_status() {
	JsonDocument doc;

	char uptime[50];
	doc[F("Uptime")] = getUptime(uptime, false);

	if (getTemperature() > -100) doc[F("Temperature")] = getTemperature();
	if (getPressure() > 0) doc[F("Pressure")] = getPressure()/100;
	if (getHumidity() > 0) doc[F("Humidity")] = getHumidity();

	doc[F("Rssi")] = wifi_rssi();
	doc[F("IP")] = wifi_currentIP();
	doc[F("FreeHeap")] = ESP.getFreeHeap();

	String res;
	serializeJson(doc, res);
	mqtt.publish(mqtt_STATE.c_str(), res.c_str(), ms.retain);
	mqtt.publish(mqtt_REPLY.c_str(), messages[MESSAGE_WEB].count>0 ? messages[MESSAGE_WEB].text.c_str(): "");
}

void mqtt_process() {
	if (mqtt_connect()) {
		mqtt.loop();
		if (timerMQTTSendStatus.isReady()) {
			mqtt_send_status();
		}
	}
}

String mqtt_status_description() {
	int mqtt_state = mqtt.state();
	switch (mqtt_state)	{
		case MQTT_CONNECTION_TIMEOUT:
			return F("the server didn't respond within the keepalive time");
		case MQTT_CONNECTION_LOST:
			return F("the network connection was broken");
		case MQTT_CONNECT_FAILED:
			return F("the network connection failed");
		case MQTT_DISCONNECTED:
			return F("the client is disconnected cleanly");
		case MQTT_CONNECTED:
			return F("the client is connected");
		case MQTT_CONNECT_BAD_CLIENT_ID:
			return F("the server rejected the client identifier");
		case MQTT_CONNECT_UNAVAILABLE:
			return F("the server was unable to accept the connection");
		case MQTT_CONNECT_BAD_CREDENTIALS:
			return F("the username/password were rejected");
		case MQTT_CONNECT_UNAUTHORIZED:
			return F("the client was not authorized to connect");
	
		default:
			return F("unknown error");
	}
}

String mqtt_info() {
	JsonDocument doc;

	doc["connection"] = mqtt_status_description();
	doc["mqtt_ID"] = mqtt_ID;
	doc["mqtt_LWT"] = mqtt_LWT;
	doc["mqtt_STATE"] = mqtt_STATE;
	doc["mqtt_CMD"] = mqtt_CMD;
	doc["mqtt_REPLY"] = mqtt_REPLY;

	String out;
	serializeJson(doc, out);
	return out;
}