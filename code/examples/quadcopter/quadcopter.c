/* Name:	quadcopter.c
 * Project:	USB Interface to android device,
 *			For robotic purposes, No HID Implementation
 * Author:	M. Dayani
 * Created:	02/05/2024
 *
 * Quadcopter Controller
 */

#ifndef F_CPU
#define F_CPU 12000000L
#endif

#define __DELAY_BACKWARD_COMPATIBLE__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "../../thirdparty/usbdrv/usbdrv.h"
#include "../../thirdparty/usbdrv/oddebug.h"

#include "../../mylib/utils.h"
#include "../../mylib/led.h"
#include "../../mylib/vusb_wrapper.h"
#include "../../mylib/controller.h"
#include "../../mylib/mypwm.h"
#include "../../mylib/sensors.h"

/* ============================================ Timer =========================================== */

// With these settings, and 12 MHz clock, 6x250 cycles = 1 ms and
// it takes 120 full interrupts for a 20 ms pulse
ISR(TIMER0_OVF_vect)
{
    TCNT0 = 0x06;
    cycleUpdate();
}

/* ============================================ Main =========================================== */

void setup(void)
{
    // enable 1s watchdog timer
    wdt_enable(WDTO_1S);

    // LED
    initLED();
    // Controller
    initController();

    adcInit();
    usbInit();
    timerInit();

    usbRe_enumerate();

    // Enable interrupts after re-enumeration
    sei();
}

void loop(void)
{
    // keep the watchdog happy
    wdt_reset();
    usbPoll();
    // adcPoll(currChAdc);

    // Update Times
    timeCtrlUpdateAll();

    // Update Cnt Cap
    capUpdateAll();

    // Update PWM state
    pwmUpdateAll();

    countUpdate();
    // testChannels4();
}

int main(void)
{
    setup();
    while (1)
    {
        loop();
    }
    return 0;
}
