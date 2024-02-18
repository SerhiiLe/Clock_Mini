#ifndef settings_h
#define settings_h

bool load_config_main();
void save_config_main();
bool load_config_alarms();
void save_config_alarms(uint8_t chunk=255);
bool load_config_texts();
void save_config_texts(uint8_t chunk=255);
bool load_config_quote();
void save_config_quote();
void copy_string(char* dst, const char* src, size_t len);

#define NVRAM_CONFIG_MAIN 0		// номер блока с главным конфигом
#define NVRAM_CONFIG_ALARMS 1	// номер блока с настройками будильников
#define NVRAM_CONFIG_TEXTS 2	// номер блока с настройками бегущих строк
#define NVRAM_CONFIG_QUOTE 3	// номер блока с настройками цитат
#define NVRAM_CONFIG_WEATHER 4	// номер блока с настройками сервера погоды

#endif