#pragma once

#include <avr/io.h>

#ifndef uchar
#define uchar unsigned char
#endif

#define LED_BIT 2
#define LED_DDR DDRB
#define LED_PORT PORTB

//static uchar ledStat;

void initLED(void);

void setLED(uchar newValue);

void toggleLED(void);
