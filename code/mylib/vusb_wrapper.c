#ifndef ARDUINO
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#endif

#ifndef F_CPU
#define F_CPU 12000000L
#endif
#ifndef ARDUINO
#include <util/delay.h>
#endif

#include "vusb_wrapper.h"
#include "sensors.h"
#include "utils.h"
#include "led.h"
#include "controller.h"

// USB input (read) buffer
uchar inputBuffer[LEN_USB_BUFF_IN];
// USB output (write) buffer
uchar outputBuffer[LEN_USB_BUFF_OUT];
// Testing Note3 claim.
uchar stateBuffPos = 0;

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

void usbRe_enumerate(void)
{
    usbDeviceDisconnect(); // enforce re-enumeration
    for (int i = 0; i < 250; i++)
    {                // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();
}

void runTestSequence(void)
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

void processCommand(void)
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


uchar cmp_code(char* code1, char lenCode1, uchar* code2, char lenCode2) {
  if (lenCode1 != lenCode2) {
    return 0;
  }
  for (int i = 0; i < lenCode1; i++) {
    if (code1[i] != code2[i]) {
      return 0;
    }
  }
  return 1;
}



// Interrupt part of USB library.
#ifdef USB_CFG_HAVE_INTRIN_ENDPOINT
void usbInterrupt(void)
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

#ifndef ARDUINO
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
#endif

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


//    For long data in, if usbFunctionSetup returns USB_NO_MSG, this function
//    is automatically called.
//    In this case, outputBuffer is updated for both test/non-test commands

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
// * usbFunctionRead() is called when the host requests a chunk of data from
// * the device. For more information, see the documentation in usbdrv/usbdrv.h.
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

/*
void usbRe_enumerate(void)
{
    usbDeviceDisconnect(); // enforce re-enumeration
    for (int i = 0; i < 250; i++)
    {                // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();
}

void runTestSequence(void)
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

void processCommand(void)
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
void usbInterrupt(void)
{
    if (usbInterruptIsReady())
    {
        // uchar msg[2] = {'b', 'i'};
        // buildReport(msg, 0, 2); // later, use the ADC values instead
        usbSetInterrupt((void *)inputBuffer, sizeof(inputBuffer));
    }
}
#endif

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


//    For long data in, if usbFunctionSetup returns USB_NO_MSG, this function
//    is automatically called.
//    In this case, outputBuffer is updated for both test/non-test commands

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
//
// * usbFunctionRead() is called when the host requests a chunk of data from
// * the device. For more information, see the documentation in usbdrv/usbdrv.h.

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
*/
