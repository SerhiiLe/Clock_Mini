#ifndef webClient_h
#define webClient_h

uint8_t weatherUpdate();
void quoteUpdate();
void quotePrepare(bool force=false);

#define Q_TEXT 0
#define Q_JSON 1
#define Q_XML 2

#define Q_GET 0
#define Q_POST 1

#endif