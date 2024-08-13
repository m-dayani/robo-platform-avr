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
// #include "../../mylib/controller.h"
// #include "../../mylib/mypwm.h"
// #include "../../mylib/sensors.h"

/* ============================================ Main =========================================== */

void setup(void)
{
    // enable 1s watchdog timer
    wdt_enable(WDTO_1S);

    // LED
    initLED();

    usbInit();
    // timerInit();

    usbRe_enumerate();

    // Enable interrupts after re-enumeration
    sei();
}

void loop(void)
{
    // keep the watchdog happy
    wdt_reset();
    usbPoll();
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
