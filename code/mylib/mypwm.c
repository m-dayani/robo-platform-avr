#include "mypwm.h"
#include "controller.h"

unsigned short max_cnt_cycle = 117;
unsigned short cnt_cycle = 0;

unsigned short max_cnt = 2815;
unsigned short cnt_offset = 1415;
double max_time = 10000.0;
unsigned short max_time_int = 10000;

unsigned long cnt_a = 0, cnt_b = 0, cnt_c = 0, cnt_d = 0;
unsigned int cap_a = 0, cap_b = 0, cap_c = 0, cap_d = 0;
unsigned long t_a = 0, t_b = 0, t_c = 0, t_d = 0;

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
    if (cnt_cycle >= max_cnt_cycle)
    {
        cnt_cycle = 0;
        cnt_a = 0;
        cnt_b = 0;
        cnt_c = 0;
        cnt_d = 0;
    }
}

/* ============================================ Timer =========================================== */

void timerInit(void)
{
    TIMSK = (1 << TOIE0); /* Enable Timer0 overflow interrupts */

    TCNT0 = 0x06;        /* Load TCNT0, count for 10ms*/
    TCCR0 = (1 << CS01); /* Start timer0 with /8 prescaler*/
}
