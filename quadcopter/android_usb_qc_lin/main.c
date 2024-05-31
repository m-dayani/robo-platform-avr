/* Name:	android_usb_mega8.c
 * Project:	USB Interface to android device,
 *			For robotic purposes, No HID Implementation
 * Author:	M. Dayani
 * Created:	02/05/2024
 *
 * Quadcopter Controller (Compiled on Linux)
 */

#define __DELAY_BACKWARD_COMPATIBLE__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "oddebug.h"

#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })

#define min(a, b) \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })

#define LED_BIT 7
#define LED_DDR DDRB
#define LED_PORT PORTB

#define OUT_CTRL_DDR DDRB
#define OUT_CTRL_PORT PORTB

#define LEN_USB_BUFF_IN 64
#define LEN_USB_BUFF_OUT 64
#define LEN_ADC_BUFF 16

#define DEFAULT_TEST_IN_MESSAGE "in-code-9372"
#define DEFAULT_TEST_OUT_MESSAGE                                        \
	{                                                                   \
		'o', 'u', 't', '-', 'c', 'o', 'd', 'e', '-', '6', '3', '3', '4' \
	}
#define DEFAULT_TEST_OUT_MSG_LEN 13
#define DEFAULT_OUT_CTRL_INIT_VAL 0x00

#define SENSORS_ADC_AVAILABLE 1
#define SENSORS_EXT_AVAILABLE 0
#define ROBO_CTRL_AVAILABLE 1

#define ADC_N_CHANNELS 1
#define ADC_PRESCALER 128
#define ADC_SRC_FREQ_MHZ 12
#define ADC_RESOLUTION_BITS 10

enum UsbCommand
{
	CMD_BROADCAST,
	CMD_UPDATE_OUTPUT,
	CMD_GET_SENSOR_INFO,
	CMD_GET_CMD_RES,
	CMD_ADC_START,
	CMD_ADC_READ,
	CMD_ADC_STOP,
	CMD_RUN_TEST
} command_flag;

static uchar ledStat = 0;

static uchar currChAdc = 0;
static uchar adcStartedFlag = 0;

// ACD results buffer (8 2-byte channels)
static uchar adcBuffer[LEN_ADC_BUFF];

// USB input (read) buffer
static uchar inputBuffer[LEN_USB_BUFF_IN];
// USB output (write) buffer
static uchar outputBuffer[LEN_USB_BUFF_OUT];
// Testing Note3 claim.
static uchar stateBuffPos = 0;

uchar ctrl_state = 0x00;

// A 50 Hz PWM pulse (20000 us)
// For a real servo or ESC:
// cnt_offset = 1415, max_cnt = 2815, max_cnt_cycle = 117
// For LED test (dimmer):
// cnt_offset = 0, max_cnt = 200, max_cnt_cycle = 200 (or 180)
const unsigned short max_cnt_cycle = 117;
unsigned short cnt_cycle = 0;

const unsigned short max_cnt = 2815;
const unsigned short cnt_offset = 1415;
const double max_time = 10000.0;
const unsigned short max_time_int = 10000;

// 4 Channels for PWM Generation (to ESC)
// A: PB0, B: PB1, C: PB2, D: PB3
// Dynamic counters (changed in every loop)
unsigned long cnt_a = 0, cnt_b = 0, cnt_c = 0, cnt_d = 0;
// Max count determining duty cycle (changed with commands)
unsigned int cap_a = 0, cap_b = 0, cap_c = 0, cap_d = 0;
// Change max count based on the time of command
unsigned long t_a = 0, t_b = 0, t_c = 0, t_d = 0;

/* =================================== Function Declarations =================================== */

/* ===================================== Config. & Helpers ===================================== */

void mainInit()
{
	// LED
	LED_DDR |= 1 << LED_BIT;
	// set control port as output
	OUT_CTRL_DDR = 0xFF;
	// Enable motor driver: Enable wire attached to PB2
	OUT_CTRL_PORT = 0x00;
}

static void setLED(uchar newValue)
{
	ledStat = newValue;
	if (ledStat)
	{
		LED_PORT |= 1 << LED_BIT; // LED on
	}
	else
	{
		LED_PORT &= ~(1 << LED_BIT); // LED off
	}
}

void toggleLED(void)
{
	if (ledStat)
	{
		setLED(0);
	}
	else
	{
		setLED(1);
	}
}

void clearBuffer(uchar *buff, uchar offset, uchar len)
{
	for (uchar i = offset, totalLen = offset + len; i < totalLen; i++)
	{
		buff[i] = 0;
	}
}

void insertBuffer(uchar *lBuff, uchar lBuffLen, uchar *rBuff, uchar rBuffLen, uchar offset)
{
	for (uchar i = offset, totalBuffLen = offset + rBuffLen; i < lBuffLen && i < totalBuffLen; i++)
	{
		lBuff[i] = rBuff[i - offset];
	}
}

void readBuffer(uchar *lBuff, uchar lBuffLen, uchar *rBuff, uchar rBuffLen, uchar offset)
{
	for (uchar i = offset, totalBuffLen = offset + rBuffLen; i < lBuffLen && i < totalBuffLen; i++)
	{
		lBuff[i - offset] = rBuff[i];
	}
}

uchar decodeData(uchar *input, uchar lenInput, uchar *output, uchar lenOutput, uchar *state)
{
	if (lenInput < 2)
	{
		return 0;
	}

	uchar lenData = input[0];
	*state = input[1];

	lenOutput = min(lenData, lenOutput);
	readBuffer(output, lenOutput, input, lenInput, 2);

	return lenOutput;
}

void encodeData(uchar *input, uchar lenInput, uchar *output, uchar lenOutput, uchar state)
{
	if (lenOutput < 2)
	{
		return;
	}

	output[0] = lenInput;
	output[1] = state; // command or data

	insertBuffer(output, lenOutput, input, lenInput, 2);
}

uchar cmdCompare(char *lCmd, uchar *rCmd, uchar len)
{
	uchar lCmdLen = sizeof(lCmd);

	for (uchar i = 0; i < len && i < lCmdLen; i++)
	{
		if (lCmd[i] != rCmd[i])
			return 0;
	}

	return 1;
}

void setResponseOK(uchar state)
{
	clearBuffer(inputBuffer, 0, LEN_USB_BUFF_IN);
	uchar buff[] = {state};
	encodeData(buff, 1, inputBuffer, LEN_USB_BUFF_IN, 0);
}

/* ====================================== Sensor Functions ===================================== */

/* --------------------------------------- ADC Functions --------------------------------------- */

static uchar isAdcStarted()
{
	return adcStartedFlag;
}

static void adcStart()
{
	adcStartedFlag = 1;
	ADCSRA |= (1 << ADSC);
}

static void adcStop()
{
	adcStartedFlag = 0;
	ADCSRA &= ~(1 << ADSC);
}

static void adcInit(void)
{
	// channel: ADC0, vRef = AVCC (5v), right adjust (for 10 bit res)
	ADMUX |= (1 << REFS0); //| (1 << ADLAR)
	// Pre-scaler: 128
	ADCSRA |= (SENSORS_ADC_AVAILABLE << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //| (1 << ADATE) | (1 << ADIE)
																							// no free running mode, no high speed mode
																							// SFIOR |= ;
}

static void adcPoll(uchar channel)
{
	// conversion complete?
	if (isAdcStarted() && !(ADCSRA & (1 << ADSC)))
	{

		uchar adcValues[2];
		adcValues[0] = ADCL;
		adcValues[1] = ADCH;

		insertBuffer(adcBuffer, LEN_ADC_BUFF, adcValues, 2, 2 * channel);

		// for faster response, fill input buffer here
		// WARNING: with this, we cannot send any other commands except usb read commands
		// encodeData(adcBuffer, 2 * ADC_N_CHANNELS, inputBuffer, LEN_USB_BUFF_IN, 0);

		// start next conversion *
		ADCSRA |= (1 << ADSC);
	}
}

/* ------------------------------------- External Sensors -------------------------------------- */

/* ---------------------------------------- Management ----------------------------------------- */

void setSensorsInfo()
{

	uchar lenInfo = 5;
	uchar infoBuffer[lenInfo];

	// first byte is status
	infoBuffer[0] = (SENSORS_ADC_AVAILABLE) | (SENSORS_EXT_AVAILABLE < 1) | (ROBO_CTRL_AVAILABLE << 2) | (isAdcStarted() << 3);
	// number of adc channels
	infoBuffer[1] = ADC_N_CHANNELS;
	// src frequency (MHz)
	infoBuffer[2] = ADC_SRC_FREQ_MHZ;
	// pre-scaler factor
	infoBuffer[3] = ADC_PRESCALER;
	// channel resolution (bits)
	infoBuffer[4] = ADC_RESOLUTION_BITS;

	// set the return buffer
	encodeData(infoBuffer, lenInfo, inputBuffer, LEN_USB_BUFF_IN, 0);
}

/* ============================= Basic Controllers / State Modifiers =========================== */

void updateState(uchar state)
{
	// OUT_CTRL_PORT = state;
	ctrl_state = state;
}

/* ======================================== USB Section ======================================== */

/* ----------------------------------- HID Report Descriptors ---------------------------------- */

/* ------------------------------------- Helper functions -------------------------------------- */

void usbRe_enumerate()
{
	usbDeviceDisconnect(); // enforce re-enumeration
	for (int i = 0; i < 250; i++)
	{				 // wait 500 ms
		wdt_reset(); // keep the watchdog happy
		_delay_ms(2);
	}
	usbDeviceConnect();
}

void runTestSequence()
{
	// send response, inputBuffer is sent when host queries.
	uchar out_msg[] = DEFAULT_TEST_OUT_MESSAGE;
	encodeData(out_msg, DEFAULT_TEST_OUT_MSG_LEN, inputBuffer, LEN_USB_BUFF_IN, 0);

	// test output updates
	toggleLED();
}

uchar processDataCommand(uchar lenCmd, uchar state)
{
	if (state)
	{
		return 0;
	}

	// currently, command flag is not used in decisions
	if (cmdCompare(DEFAULT_TEST_IN_MESSAGE, outputBuffer, lenCmd))
	{
		// if we have a test sequence:
		runTestSequence();
	}

	return 1;
}

void processCommand()
{
	switch (command_flag)
	{

	case CMD_ADC_START:
		adcStart();
		setResponseOK(isAdcStarted());
		break;

	case CMD_ADC_READ:
		encodeData(adcBuffer, 2 * ADC_N_CHANNELS, inputBuffer, LEN_USB_BUFF_IN, 0);
		break;

	case CMD_ADC_STOP:
		adcStop();
		setResponseOK(!isAdcStarted());
		break;

	case CMD_GET_SENSOR_INFO:
		setSensorsInfo();
		break;

	default:
		break;
	}
}

// Interrupt part of USB library.
#ifdef USB_CFG_HAVE_INTRIN_ENDPOINT
void usbInterrupt()
{
	if (usbInterruptIsReady())
	{
		// uchar msg[2] = {'b', 'i'};
		// buildReport(msg, 0, 2); // later, use the ADC values instead
		usbSetInterrupt((void *)inputBuffer, sizeof(inputBuffer));
	}
}
#endif

/* --------------------------------------- Setup function -------------------------------------- */

USB_PUBLIC uchar usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *)data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR)
	{

		command_flag = rq->bRequest;

		switch (rq->bRequest)
		{

		case CMD_UPDATE_OUTPUT:
			updateState(rq->wValue.bytes[0]);
			return 0;

		case CMD_GET_CMD_RES:
			usbMsgPtr = (int)inputBuffer;
			return sizeof(inputBuffer);

		case CMD_ADC_START:
		case CMD_ADC_READ:
		case CMD_ADC_STOP:
		case CMD_GET_SENSOR_INFO:
			processCommand();
			return 0;

		case CMD_RUN_TEST:
			// usbFunctionWrite will be called now
			return USB_NO_MSG;

		case CMD_BROADCAST:
		default:
			// toggleLED();
			return 0;
		}
	}
	return 0;
}

/* ------------------------------------ Read/Write functions ----------------------------------- */

#if USB_CFG_IMPLEMENT_FN_WRITE
// Regular 8Bytes write.
// For at most 8 Bytes of data, writes are much simpler
void receive8ByteDt(uchar *data, uchar len)
{
	for (int i = 0; i < len; i++)
	{
		outputBuffer[i] = data[i];
	}
}

// Use this for large buffer writes (> 8 Bytes).
void receiveBuffer(uchar *data, uchar len)
{
	// Put this check in every function that writes to buffer
	// This is intended for test. Better methods can be used to prevent data loss.
	if (stateBuffPos >= LEN_USB_BUFF_OUT || len + stateBuffPos > LEN_USB_BUFF_OUT)
	{
		stateBuffPos = 0;
	}
	int i = stateBuffPos;
	for (; i < len + stateBuffPos; i++)
	{
		outputBuffer[i] = data[i - stateBuffPos];
	}
	stateBuffPos = i;
}

/*
	For long data in, if usbFunctionSetup returns USB_NO_MSG, this function
	is automatically called.
	In this case, outputBuffer is updated for both test/non-test commands
*/
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len)
{
	// for len > 8 Bytes use receiveBuffer instead
	// receive8ByteDt(data, len);
	// receiveBuffer(data, len);

	uchar state = 0;
	uchar lenCmd = decodeData(data, len, outputBuffer, LEN_USB_BUFF_OUT, &state);

	// process command
	if (!processDataCommand(lenCmd, state))
	{
		return 0;
	}
	// 1 if we received it all, 0 if not
	return 1;
}
#endif

#if USB_CFG_IMPLEMENT_FN_READ
/*
 * usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information, see the documentation in usbdrv/usbdrv.h.
 */
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len)
{
	uchar i;
	for (i = 0; i < len && i < LEN_USB_BUFF_IN; i++)
	{
		*data = inputBuffer[i];
		data++;
	}

	return i;
}
#endif

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

void countUpdate()
{
	cnt_a = cnt_cycle * 250 + TCNT0;
	cnt_b = cnt_a;
	cnt_c = cnt_a;
	cnt_d = cnt_a;
}

void cycleUpdate()
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

// With these settings, and 12 MHz clock, 6x250 cycles = 1 ms and
// it takes 120 full interrupts for a 20 ms pulse
ISR(TIMER0_OVF_vect)
{
	TCNT0 = 0x06;
	cycleUpdate();
}

void timerInit(void)
{
	TIMSK = (1 << TOIE0); /* Enable Timer0 overflow interrupts */

	TCNT0 = 0x06;		 /* Load TCNT0, count for 10ms*/
	TCCR0 = (1 << CS01); /* Start timer0 with /8 prescaler*/
}

/* ============================================ Main =========================================== */

int main(void)
{
	// enable 1s watchdog timer
	wdt_enable(WDTO_1S);

	mainInit();
	adcInit();
	usbInit();
	timerInit();

	// Enable interrupts after re-enumeration
	sei();

	while (1)
	{
		// keep the watchdog happy
		wdt_reset();
		usbPoll();
		adcPoll(currChAdc);

		// Update Times
		timeCtrlUpdateAll();

		// Update Cnt Cap
		capUpdateAll();

		// Update PWM state
		pwmUpdateAll();

		countUpdate();
	}

	return 0;
}
