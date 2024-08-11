#pragma once

#ifndef ARDUINO
#include <avr/io.h>
#else
#include <Arduino.h>
#endif

#ifndef ARDUINO
#ifndef uchar
#define uchar unsigned char
#endif
#else
typedef unsigned char uchar;
#endif

//#ifndef ARDUINO
#define LED_BIT 2
#define LED_DDR DDRB
#define LED_PORT PORTB
//#endif

//static uchar ledStat;

#ifdef __cplusplus
extern "C" {
#endif

void initLED(void);

void setLED(uchar newValue);

void toggleLED(void);

void toggle_led(void);

#ifdef __cplusplus
}
#endif

