/* Name:	android_usb_mega8.c
 * Project:	USB Interface to android device,
 *			For robotic purposes, No HID Implementation
 * Author:	M. Dayani
 * Created:	12/24/2019
 *
 * Car Continuous Controller (Sticky Directional Controls)
 *
 * Note1:	Working with LibUsb in different platforms (Linux/win/android)
 *			We can try 4 methods with this:
 *				1. Just control transfer
 *				2. Using read/write functions
 *				3. Using USB interrupt or poll-based
 *				4. HID USB devices.
 * Note2:	It seems that for large messages sent to device from host,
 *			for every 8 bytes we start from the beginning of the buffer while
 *			it has more than 8 bytes. I assume this might be related to
 *			the internal mechanism of data handling of either of V-USB or Android Sys.
 *			We can easily overcome this by defining a global variable to
 *			keep track of the current position of buffer!
 * Note3:	For car manual control, I attached a driver and it has
 *			and enable with is assigned to PORTB2 and is always on.
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

#include "../../mylib/utils.h"
#include "../../mylib/led.h"
#include "../../mylib/vusb_wrapper.h"
#include "../../mylib/controller.h"
#include "../../mylib/mypwm.h"
#include "../../mylib/sensors.h"

void mainInit(void)
{
    // LED
    initLED();
    // Controller
    initController();
}

/* ============================================ Main =========================================== */

int main(void)
{
    // enable 1s watchdog timer
    wdt_enable(WDTO_1S);

    mainInit();
    adcInit();
    usbInit();

    usbRe_enumerate();

    // Enable interrupts after re-enumeration
    sei();

    while (1)
    {
        // keep the watchdog happy
        wdt_reset();
        usbPoll();
        adcPoll(currChAdc);
		controlCar();
    }

    return 0;
}
