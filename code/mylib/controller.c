#include "controller.h"

typedef unsigned char uchar;

uchar ctrl_state = 0x00;

void initController(void)
{
    // set control port as output
    OUT_CTRL_DDR = 0xFF;
    // Enable motor driver: Enable wire attached to PB2
    OUT_CTRL_PORT = 0x00;
}

void updateState(uchar state)
{
    // OUT_CTRL_PORT = state;
    ctrl_state = state;
}

void controlCar(void)
{
    // OUT_CTRL_PORT = state;
    if (ctrl_state == 0x01)
    {
        // enable
        OUT_CTRL_PORT |= 0x01;
    }
    else if (ctrl_state == 0x02)
    {
        // disable
        OUT_CTRL_PORT &= ~(0x03);
    }
    else if (ctrl_state == 0x04)
    {
        // forward
        OUT_CTRL_PORT |= 0x02;
    }
    else
    {
        // backward
        OUT_CTRL_PORT &= ~(0x02);
    }
}

void testChannels4(void)
{
    // OUT_CTRL_PORT = state;
    if (ctrl_state == 0x01)
    {
        // enable
        OUT_CTRL_PORT |= 0x01;
    }
    else if (ctrl_state == 0x02)
    {
        // disable
        OUT_CTRL_PORT &= ~(0x01);
    }
    else if (ctrl_state == 0x04)
    {
        // forward
        OUT_CTRL_PORT |= 0x02;
    }
    else if (ctrl_state == 0x08)
    {
        // backward
        OUT_CTRL_PORT &= ~(0x02);
    }
    else if (ctrl_state == 0x10)
    {
        // enable
        OUT_CTRL_PORT |= 0x04;
    }
    else if (ctrl_state == 0x20)
    {
        // disable
        OUT_CTRL_PORT &= ~(0x04);
    }
    else if (ctrl_state == 0x40)
    {
        // forward
        OUT_CTRL_PORT |= 0x08;
    }
    else if (ctrl_state == 0x80)
    {
        // backward
        OUT_CTRL_PORT &= ~(0x08);
    }
    else
    {
        // nothing
        OUT_CTRL_PORT &= 0x00;
    }
}