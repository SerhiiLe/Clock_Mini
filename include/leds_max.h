#ifndef leds_max_h
#define leds_max_h

void set_brightness(uint8_t);
uint32_t maximizeBrightness(uint32_t color, uint8_t limit=15);
uint32_t getPixColorXY(int8_t x, int8_t y);
void drawPixelXY(int8_t x, int8_t y, uint8_t color=1);
void fillAll();
void clearALL();
void showALL(bool clear = false);
void display_setup();
void display_tick(bool clear = true);

extern bool screenIsFree;
extern bool itsTinyText;

#endif
