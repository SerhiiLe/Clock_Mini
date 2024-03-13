/*
	подключение FTP сервера, вынесено сюда, чтобы не захламлять основной код
*/
/*
	Все реализации FTP, которые я нашел, не полноценные. Только активный режим, только один поток, не полный набор команд.
	Можно подключаться из командной строки, можно, как я, через "Midnight Commander", в описаниях обычно FileZilla.
	Файловая система LittleFS для esp32 пока сильно сырая и адекватно не работает. А SPIFFS вообще не совсем файловая система.
	На практике с esp8266 более-менее работает только https://github.com/charno/FTPClientServer.git
	С esp32 с SPIFFS и LittleFS ничего нормально не работает. Другое дело, если используется внешний SD-card reader и там FAT,
	тогда лучший вариант https://github.com/peterus/ESP-FTP-Server-Lib он наиболее полно поддерживает протокол.
	Другие варианты:
	https://github.com/xreef/SimpleFTPServer - проблема работы с сетью, похоже не совместим новыми версиями платформы esp32, но автору это не интересно, он копает другие платформы
	https://github.com/nailbuster/esp8266FTPServer - похоже, что ноги у всех ftp растут отсюда. Давно заброшен.
	https://github.com/dplasa/FTPClientServer - форк того, что выше, добавлен ещё клиент. Работает плохо.
	А вот тут лежат разные сервисы на IDF, вполне возможно тут FTP рабочий https://github.com/nkolban/esp32-snippets/tree/master/cpp_utils
	А тут что-то на эту тему, тоже на IDF https://github.com/mattihirvonen/esp32multiserver

	PS. Проблема оказалась в SPIFFS. С LittleFS и https://github.com/charno/FTPClientServer всё работает.
	PPS. Ещё точнее, проблема с хранением времени создания файла в SPIFFS.
*/
// #define ESP8266

#include <FTPServer.h>

FTPServer ftpSrv(LittleFS);
bool ftp_isStarted = false;
bool ftp_isAllow = false;

void ftp_process () {
	if( ftp_isStarted ) {
		if(ftp_isAllow) ftpSrv.handleFTP();
		else {
			ftpSrv.stop();
			ftp_isStarted = false;
		}
	}
	else {
		if(ftp_isAllow) {
			ftpSrv.begin(String(gs.web_login), String(gs.web_password));
			ftp_isStarted = true;
		}
	}
}