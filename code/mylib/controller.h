#pragma once

#ifndef ARDUINO
#include <avr/io.h>
#else
#include <Arduino.h>
#endif

// typedef unsigned char uchar;

#define OUT_CTRL_DDR DDRB
#define OUT_CTRL_PORT PORTB

#define DEFAULT_OUT_CTRL_INIT_VAL 0x00

extern unsigned char ctrl_state;

void initController(void);

void updateState(unsigned char state);

void controlCar(void);

void testChannels4(void);
