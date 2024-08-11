#include "mypwm.h"
#include "controller.h"

unsigned short max_cnt_cycle = 120;
unsigned short cnt_cycle = 0;

unsigned short max_cnt = 2000;
unsigned short cnt_offset = 1000;
double max_time = 20000.0;
unsigned short max_time_int = 20000;

unsigned long cnt_a = 0, cnt_b = 0, cnt_c = 0, cnt_d = 0;
unsigned int cap_a = 0, cap_b = 0, cap_c = 0, cap_d = 0;
unsigned long t_a = 0, t_b = 0, t_c = 0, t_d = 0;

char state_a = 0, state_b = 0, state_c = 0, state_d = 0;
int n_cycle = 0;

/* ==================================== Signal Gen (timer) ======================================== */

void capUpdate(unsigned int *cap, const unsigned long t)
{
    *cap = (int)((((double)t) / max_time) * (max_cnt - cnt_offset) + cnt_offset);

    if (*cap > max_cnt)
    {
        *cap = max_cnt;
    }
    if (*cap < cnt_offset)
    {
        *cap = cnt_offset;
    }
}

void capUpdateAll(void)
{
    capUpdate(&cap_a, t_a);
    capUpdate(&cap_b, t_b);
    capUpdate(&cap_c, t_c);
    capUpdate(&cap_d, t_d);
}

void timeCtrlUpdate(unsigned long *t, const uchar nb_p, const uchar nb_n, const uchar nsh_p, const uchar nsh_n)
{
    // todo: remove some global variables like ctrl_state
    uchar pcond = (ctrl_state & nb_p) >> nsh_p;
    uchar ncond = (ctrl_state & nb_n) >> nsh_n;

    if (pcond && *t < max_time_int)
    {
        *t += 1;
    }
    if (ncond && *t > 0)
    {
        *t -= 1;
    }
}

void timeCtrlUpdateAll(void)
{
    timeCtrlUpdate(&t_a, 0x01, 0x02, 0, 1);
    timeCtrlUpdate(&t_b, 0x04, 0x08, 2, 3);
    timeCtrlUpdate(&t_c, 0x10, 0x20, 4, 5);
    timeCtrlUpdate(&t_d, 0x40, 0x80, 6, 7);
}

void pwmUpdate(const unsigned long cnt, const unsigned int cap, const uchar p_val)
{
    if (cnt < cap)
    {
        OUT_CTRL_PORT |= p_val;
    }
    else
    {
        OUT_CTRL_PORT &= ~p_val;
    }
}

void pwmUpdateAll(void)
{
    pwmUpdate(cnt_a, cap_a, 0x01);
    pwmUpdate(cnt_b, cap_b, 0x02);
    pwmUpdate(cnt_c, cap_c, 0x04);
    pwmUpdate(cnt_d, cap_d, 0x08);
}

void countUpdate(void)
{
    cnt_a = cnt_cycle * 250 + TCNT0;
    cnt_b = cnt_a;
    cnt_c = cnt_a;
    cnt_d = cnt_a;
}

void cycleUpdate(void)
{
    // Update global counter
    cnt_cycle += 1;

    // Reset counters when a cycle is complete
    if (cnt_cycle > max_cnt_cycle)
    {
        cnt_cycle = 0;
        cnt_a = 0;
        cnt_b = 0;
        cnt_c = 0;
        cnt_d = 0;
    }
}

/* ============================================ Timer =========================================== */

#ifndef ARDUINO
void timerInit(void)
{
    TIMSK = (1 << TOIE0); /* Enable Timer0 overflow interrupts */

    TCNT0 = 0x06;        /* Load TCNT0, count for 10ms*/
    TCCR0 = (1 << CS01); /* Start timer0 with /8 prescaler*/
}
#endif

#ifdef ARDUINO
void set_timer(void)
{

    // cli();//stop interrupts
    noInterrupts();

    // set timer0 interrupt at 2kHz
    TCCR0A = 0; // set entire TCCR0A register to 0
    TCCR0B = 0; // same for TCCR0B
    TCNT0 = 0;  // initialize counter value to 0
    // set compare match register for 2khz increments
    OCR0A = 124; // = (16*10^6) / (2000*64) - 1 (must be <256)
    // turn on CTC mode
    TCCR0A |= (1 << WGM01);
    // Set CS01 and CS00 bits for 64 prescaler
    TCCR0B |= (1 << CS01) | (1 << CS00);
    // enable timer compare interrupt
    TIMSK0 |= (1 << OCIE0A);

    // sei();//allow interrupts
    interrupts();
}
#endif

char update_state(const uchar mask, const uchar n_bit)
{
    uchar pcond = (ctrl_state & mask) >> n_bit;
    uchar ncond = (ctrl_state & (mask << 1)) >> (n_bit + 1);
    if (pcond)
        return 1;
    else if (ncond)
        return -1;
    else
        return 0;
}

void update_states(void)
{
    state_a = update_state(0x01, 0);
    state_b = update_state(0x04, 2);
    state_c = update_state(0x10, 4);
    state_d = update_state(0x40, 6);
}

void update_cnt(unsigned long *cnt, const char state)
{
    *cnt += state;
    if (*cnt > max_cnt)
        *cnt = max_cnt;
    if (*cnt < 0)
        *cnt = 0;
}
