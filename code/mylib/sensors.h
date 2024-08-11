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

#define LEN_ADC_BUFF 16

#define SENSORS_ADC_AVAILABLE 1
#define SENSORS_EXT_AVAILABLE 0
#define ROBO_CTRL_AVAILABLE 1

#define ADC_N_CHANNELS 1
#define ADC_PRESCALER 128
#define ADC_SRC_FREQ_MHZ 12
#define ADC_RESOLUTION_BITS 10


extern uchar currChAdc;
extern uchar adcStartedFlag;

// ACD results buffer (8 2-byte channels)
extern uchar adcBuffer[LEN_ADC_BUFF];

/* ====================================== Sensor Functions ===================================== */

/* --------------------------------------- ADC Functions --------------------------------------- */

uchar isAdcStarted(void);

void adcStart(void);

void adcStop(void);

void adcInit(void);

void adcPoll(uchar channel);

/* ------------------------------------- External Sensors -------------------------------------- */

/* ---------------------------------------- Management ----------------------------------------- */

void setSensorsInfo(void);
