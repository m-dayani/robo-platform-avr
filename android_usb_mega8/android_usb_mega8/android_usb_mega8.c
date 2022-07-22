
/* Name:	android_usb_mega8.c
 * Project:	USB Interface to android device, 
 *			For robotic purposes, No HID Implementation
 * Author:	M. Dayani
 * Created:	12/24/2019
 *
 * Note1:	Working with LibUsb in different platforms (Linux/win/android)
 *			We can try 4 methods with this:
 *				1. Just control transfer
 *				2. Using read/write functions
 *				3. Using USB interrupt or poll-based
 *				4. HID USB devices.
 * Note2:	Never use strcmp on a received pointer like *data to
 *			decode a command as strcmp only works for null-terminated strings!
 * Note3:	It seems that for large messages sent to device from host,
 *			for every 8 bytes we start from the beginning of the buffer although
 *			it has more than 8 bytes. I assume this might be related to
 *			the internal mechanism of data handling of either of V-USB or Android Sys.
 *			We can easily overcome this by defining a global variable to
 *			keep track of the current position of buffer!
 * Note4:	For car manual control, I attached a driver and it has
 *			and enable with is assigned to PORTB2 and is alwayse on.
 */

#define F_CPU 12000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "oddebug.h"

#define LED_BIT 0
#define LED_DDR DDRB
#define LED_PORT PORTB

#define OUT_CTRL_DDR DDRC
#define OUT_CTRL_PORT PORTC

enum USB_COMMANDS {
	BROADCAST,
	REPORT_SENSOR,	//send long msg to device
	UPDATE_STATE,	//get message (state updates) from device
	RUN_TEST		//run predefined test sequence:
					//test: read, write, sensor availability, output modification
} ucmd;

#define SENSORS_ADC_AVAILABLE 0
#define SENSORS_EXT_AVAILABLE 0

#define SENSOR_BUFFER_LEN 64
#define STATE_BUFFER_LEN 64

#define DEFAULT_TEST_IN_MESSAGE "Hello"
#define DEFAULT_TEST_OUT_MESSAGE { 'w', 'o', 'r', 'l', 'd', '\0' }
#define DEFAULT_OUT_CTRL_INIT_VAL 0x00


static uchar ledStat = 0;
static uchar testFlag = 0;

/* ------------------------------------ USB Data Structures ------------------------------------- */

static uchar sensorBuffer[SENSOR_BUFFER_LEN];	//used for sending sensor data
//The only buffer for both in/out messaging
static uchar stateBuffer[STATE_BUFFER_LEN];
static uchar stateBuffPos = 0; //Testing Note3 claim.


/* =================================== Function Declarations =================================== */

uchar is_all_sensors_available();
void updateStates();
void updateMotor1();

/* ===================================== Helper functions ====================================== */

void mainInit()
{
	/*for(int i=0; i<sizeof(reportBuffer); i++) // clear report initially
		reportBuffer[i] = 0;*/
	
	LED_DDR |= 1 << LED_BIT;
	OUT_CTRL_DDR = 0xff;
	//Enable motor driver: Enable wire attached to PB2
	DDRB |= 0x04;
	PORTB |= 0x04;
	//OUT_CTRL_PORT = DEFAULT_OUT_CTRL_INIT_VAL;
	stateBuffer[0] = DEFAULT_OUT_CTRL_INIT_VAL;
	updateStates();
}

static void setLED(uchar newValue)
{
	ledStat = newValue;
	if(ledStat) {
		LED_PORT |= 1 << LED_BIT;    // LED on
	}
	else {
		LED_PORT &= ~(1 << LED_BIT);      // LED off
	}
}

void toggleLED(void)
{
	if (ledStat) {
		setLED(0);
	}
	else {
		setLED(1);
	}
}

void setTestFlag(uchar state)
{
	testFlag = state;
}

uchar getTestFlag()
{
	return testFlag;
}

/* ======================================== USB Section ======================================== */


/* ----------------------------------- HID Report Descriptors ---------------------------------- */



/* ------------------------------------- Helper functions -------------------------------------- */

/*void buildReport(int val)
{
	usbMessage[0] = (uchar) val;
	usbMessage[1] = (uchar) val >> 8;
}

void buildReport(uchar lsVal, uchar msVal)
{
	sensorBuffer[0] = lsVal;
	sensorBuffer[1] = msVal;
}*/

void buildReport(uchar *arr, uchar offset, uchar len)
{
	uchar i;
	for (i = offset; i < len; i++) {
		sensorBuffer[i] = arr[i];
	}
}

uchar processCmd(char *cmd, uchar* rcmd, uchar len)
{
	for (uchar i = 0; i < len; i++) {
		if (cmd[i] != rcmd[i])
			return 0;
	}
	return 1;
}

void setTestResponse(uchar *out_msg, uchar len)
{
	if (len+1 > SENSOR_BUFFER_LEN) {
		return;
	}
	uchar i = 0;
	for (; i < len; i++) {
		sensorBuffer[i] = out_msg[i];
	}
	sensorBuffer[i] = 0;
}

void runTestSequence(uchar *data, uchar len)
{
	//For test purposes.
	if (processCmd(DEFAULT_TEST_IN_MESSAGE, data, len)) {
		uchar out_msg[] = DEFAULT_TEST_OUT_MESSAGE;
		//check for sensors availability
		if (is_all_sensors_available()) {
			//send response, sensorBuffer is sent when host queries.
			out_msg[0] = 'y';
			setTestResponse(out_msg, 5);
		}
		else {
			out_msg[0] = 'n';
			setTestResponse(out_msg, 5);
		}
		//test output updates
		toggleLED();
	}
	setTestFlag(0);
}

void usbRe_enumerate()
{
	usbDeviceDisconnect(); // enforce re-enumeration
	for(int i = 0; i < 250; i++) { // wait 500 ms
		wdt_reset(); // keep the watchdog happy
		_delay_ms(2);
	}
	usbDeviceConnect();
}

//Interrupt part of usb library.
#ifdef USB_CFG_HAVE_INTRIN_ENDPOINT
void usbInterrupt()
{
	if(usbInterruptIsReady()) {
		uchar bili[2] = {'b', 'i'};
		buildReport(bili, 0, 2); // later, use the adc values instead
		usbSetInterrupt((void *)sensorBuffer, sizeof(sensorBuffer));
	}
}
#endif

/*void fillBufferWithRQ(uchar *buffer, usbRequest_t *rq, unsigned int offset)
{
	if (offset+4 > USB_BUFFER_LEN) {
		//error
		return;
	}
	buffer[offset] = rq->wValue.bytes[0];
	buffer[offset+1] = rq->wValue.bytes[1];
	buffer[offset+2] = rq->wIndex.bytes[0];
	buffer[offset+3] = rq->wIndex.bytes[1];
}*/

/* --------------------------------------- Setup function -------------------------------------- */

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) 
{
	usbRequest_t *rq = (void *) data;
	
	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR) {
		switch (rq->bRequest) {
		case BROADCAST:
			//Do nothing for now
		case REPORT_SENSOR:
			usbMsgPtr = sensorBuffer;
			return sizeof(sensorBuffer);	
		case UPDATE_STATE:
			/*dataLength = (uchar)rq->wLength.word;
			dataReceived = 0;*/
			return USB_NO_MSG; // usbFunctionWrite will be called now
		case RUN_TEST:
			setTestFlag(1);
			return USB_NO_MSG; // usbFunctionWrite will be called now
		default:
			toggleLED();
			return 0; // do nothing for now
		}
	}
	return 0;
}

/* ------------------------------------ Read/Write functions ----------------------------------- */

#if USB_CFG_IMPLEMENT_FN_WRITE

//Regular 8Bytes write.
//For at most 8 Bytes of data, writes are much simpler
void receive8ByteDt(uchar *data, uchar len)
{
	for(int i = 0; i < len; i++) {
		stateBuffer[i] = data[i];
	}
}

//Use this for large buffer writes (> 8 Bytes).
void receiveBuffer(uchar *data, uchar len)
{
	//Put this check in every function that writes to buffer
	//This is intended for test. Better methods can be used to prevent data loss.
	if (stateBuffPos >= STATE_BUFFER_LEN || len+stateBuffPos > STATE_BUFFER_LEN) {
		stateBuffPos = 0;
	}
	int i = stateBuffPos;
	for(; i < len+stateBuffPos; i++) {
		stateBuffer[i] = data[i-stateBuffPos];
	}
	stateBuffPos = i;
}

/*
	For long data in, if usbFunctionSetup returns USB_NO_MSG, this function
	is automatically called.
	In this case, stateBuffer is updated for both test/non-test commands
*/
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) 
{
	if (len > STATE_BUFFER_LEN) {
		return 0;
	}
	
	//for len > 8 Bytes use receiveBuffer instead
	receive8ByteDt(data, len);
	//receiveBuffer(data, len);
	
	//if we have a test sequence:
	if (getTestFlag()) {
		runTestSequence(stateBuffer, len);
	}
	else {
		updateStates();
	}
	
	return 1; // 1 if we received it all, 0 if not
}
#endif

#if USB_CFG_IMPLEMENT_FN_READ
/* 
 * usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information, see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionRead(uchar *data, uchar len)
{
	if (len > SENSOR_BUFFER_LEN || len <= 0) {
		return 0;
	}
	
	for(uint8_t x = 0; x < len; x++) {
		*data = sensorBuffer[x];
		data++;
	}

	return len;
}
#endif

/* ====================================== Sensor Functions ===================================== */

/* --------------------------------------- ADC Functions --------------------------------------- */
/*
static void adcInit(void)
{
	//channel: ADC0, vRef = AVCC (5v), right adjust (for 10 bit res)
	ADMUX |= (1 << REFS0); //| (1 << ADLAR)
	//Pre-scaler: 125
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | 
		(1 << ADPS0); //| (1 << ADATE) | (1 << ADIE)
	//no free running mode, no high speed mode
	//SFIOR |= ;
}

static void adcPoll(void)
{
    if ( !(ADCSRA & (1 << ADSC))) { 		//adcPending &&
       // adcPending = 0;
		
		uchar lval = ADCL;
		uchar hval = ADCH;
		buildReport(lval, hval);
		
		ADCSRA |= (1 << ADSC);  // start next conversion *
    }
}

void adcStart() 
{
	ADCSRA |= (1 << ADSC);
}
*/

uchar is_adc_available() {
	return SENSORS_ADC_AVAILABLE;
}

/* ------------------------------------- External Sensors -------------------------------------- */

uchar is_ext_sensors_available() {
	return SENSORS_EXT_AVAILABLE;
}

/* ---------------------------------------- Management ----------------------------------------- */

uchar is_all_sensors_available() {
	return is_adc_available() && is_ext_sensors_available();
}

/* ============================= Basic Controllers / State Modifiers =========================== */

/*
	currently, the LS byte is the on/off control
	state byte.
*/
void updateStates() 
{
	//setLED(stateBuffer[0]);
	OUT_CTRL_PORT = stateBuffer[0];
	updateMotor1();
}

void updateMotor1()
{
	if (stateBuffer[0] & 0x01 || stateBuffer[0] & 0x02) {
		//enable
		PORTB |= 0x04;
	}
	else {
		//disable
		PORTB &= ~0x04;
	}
}

/* ============================================ Main =========================================== */

int main(void)
{
	wdt_enable(WDTO_1S); // enable 1s watchdog timer

	mainInit();
	usbInit();
	//adcInit();
	
	usbRe_enumerate();
	
	//adcStart();
	
	sei(); // Enable interrupts after re-enumeration
	
	while(1) {
		wdt_reset(); // keep the watchdog happy
		usbPoll();
		//adcPoll();
		
		//usbInterrupt();
		/*for(int i = 0; i < 250; i++) { // wait 500 ms
			wdt_reset(); // keep the watchdog happy
			_delay_ms(2);
		}
		toggleLED();
		stateBuffer[0] = 0x02;
		updateStates();
		for(int i = 0; i < 250; i++) { // wait 500 ms
			wdt_reset(); // keep the watchdog happy
			_delay_ms(2);
		}
		toggleLED();
		stateBuffer[0] = 0x01;
		updateStates();*/
	}
	
	return 0;
}
