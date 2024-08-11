#pragma once

#ifndef ARDUINO
#include <avr/io.h>
#include <avr/interrupt.h>
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

// A 50 Hz PWM pulse (20000 us)
// For a real servo or ESC:
// cnt_offset = 1415, max_cnt = 2815, max_cnt_cycle = 117
// For LED test (dimmer):
// cnt_offset = 0, max_cnt = 200, max_cnt_cycle = 200 (or 180)
extern unsigned short max_cnt_cycle;
extern unsigned short cnt_cycle;

extern unsigned short max_cnt;
extern unsigned short cnt_offset;
extern double max_time;
extern unsigned short max_time_int;

// 4 Channels for PWM Generation (to ESC)
// A: PB0, B: PB1, C: PB2, D: PB3
// Dynamic counters (changed in every loop)
extern unsigned long cnt_a, cnt_b, cnt_c, cnt_d;
// Max count determining duty cycle (changed with commands)
extern unsigned int cap_a, cap_b, cap_c, cap_d;
// Change max count based on the time of command
extern unsigned long t_a, t_b, t_c, t_d;

extern char state_a, state_b, state_c, state_d;
extern int n_cycle;

/* ==================================== Signal Gen (timer) ======================================== */

#ifdef __cplusplus
extern "C" {
#endif

void capUpdate(unsigned int *cap, const unsigned long t);

void capUpdateAll(void);

void timeCtrlUpdate(unsigned long *t, const uchar nb_p, const uchar nb_n, const uchar nsh_p, const uchar nsh_n);

void timeCtrlUpdateAll(void);

void pwmUpdate(const unsigned long cnt, const unsigned int cap, const uchar p_val);

void pwmUpdateAll(void);

void countUpdate(void);

void cycleUpdate(void);

#ifndef ARDUINO
void timerInit(void);
#endif


void set_timer(void);

char update_state(const uchar mask, const uchar n_bit);

void update_states(void);

void update_cnt(unsigned long* cnt, const char state);

#ifdef __cplusplus
}
#endif

