#ifndef SUART
#define SUART

#include <avr/pgmspace.h>

void xmit(char);
void xmitstr(const char * PROGMEM);
void xmitval(int16_t, int8_t, int8_t);
void xmitf(const char * PROGMEM, ...);
uint8_t rcvr();
void rcvrstr(char *, uint8_t);
uint8_t pickval(char **, uint16_t *, uint8_t);

#endif	/* SUART */
