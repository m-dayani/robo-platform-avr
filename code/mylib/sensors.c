#include "sensors.h"
#include "vusb_wrapper.h"

uchar currChAdc = 0;
uchar adcStartedFlag = 0;

uchar adcBuffer[LEN_ADC_BUFF];

/* ====================================== Sensor Functions ===================================== */

/* --------------------------------------- ADC Functions --------------------------------------- */

uchar isAdcStarted(void)
{
	return adcStartedFlag;
}

void adcStart(void)
{
	adcStartedFlag = 1;
	ADCSRA |= (1 << ADSC);
}

void adcStop(void)
{
	adcStartedFlag = 0;
	ADCSRA &= ~(1 << ADSC);
}

void adcInit(void)
{
	// channel: ADC0, vRef = AVCC (5v), right adjust (for 10 bit res)
	ADMUX |= (1 << REFS0); //| (1 << ADLAR)
	// Pre-scaler: 128
	ADCSRA |= (SENSORS_ADC_AVAILABLE << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //| (1 << ADATE) | (1 << ADIE)
	// no free running mode, no high speed mode
	// SFIOR |= ;
}

void adcPoll(uchar channel)
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

void setSensorsInfo(void)
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
