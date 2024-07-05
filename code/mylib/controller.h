#pragma once

#include <avr/io.h>

//typedef unsigned char uchar;

#define OUT_CTRL_DDR DDRB
#define OUT_CTRL_PORT PORTB

extern unsigned char ctrl_state;

void initController(void);

void updateState(unsigned char state);

void controlCar();