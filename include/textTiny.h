#ifndef textTiny_h
#define textTiny_h

void drawSlide();
void printTinyText(const char *txt, int16_t posX = 0, bool instant=false, bool clear=false);
void printTinyText_P(const char *txt, int16_t posX = 0, bool instant=false, bool clear=false);

#endif