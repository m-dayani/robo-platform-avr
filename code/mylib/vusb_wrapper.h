#pragma once

#ifndef ARDUINO
#include "../thirdparty/usbdrv/usbdrv.h"
#include "../thirdparty/usbdrv/oddebug.h"
#else
#include <Arduino.h>
#endif

#ifndef ARDUINO
#ifndef uchar
#define uchar unsigned char
#endif
#else
typedef unsigned char uchar;
#endif

#define LEN_USB_BUFF_IN 64
#define LEN_USB_BUFF_OUT 64

#define DEFAULT_TEST_IN_MESSAGE "in-code-9372"
#define DEFAULT_TEST_OUT_MESSAGE                                        \
    {                                                                   \
        'o', 'u', 't', '-', 'c', 'o', 'd', 'e', '-', '6', '3', '3', '4' \
    }
#define DEFAULT_TEST_OUT_MSG_LEN 13
#define DEFAULT_TEST_IN_MSG_LEN 12

enum UsbCommand
{
    CMD_BROADCAST,
    CMD_UPDATE_OUTPUT,
    CMD_GET_SENSOR_INFO,
    CMD_GET_CMD_RES,
    CMD_ADC_START,
    CMD_ADC_READ,
    CMD_ADC_STOP,
    CMD_RUN_TEST,
    CMD_TEST_LTC,
    CMD_TEST_TP,
    CMD_STR_MSG
} command_flag;

// USB input (read) buffer
extern uchar inputBuffer[LEN_USB_BUFF_IN];
// USB output (write) buffer
extern uchar outputBuffer[LEN_USB_BUFF_OUT];
// USB temp buffer (writing long messages)
extern uchar tempBuffer[LEN_USB_BUFF_OUT];
// Testing Note3 claim.
extern uchar stateBuffPos;
extern uchar lenDataCmd;
extern int tpCount;

#ifdef __cplusplus
extern "C"
{
#endif

    void clearBuffer(uchar *buff, uchar offset, uchar len);

    void insertBuffer(uchar *lBuff, uchar lBuffLen, uchar *rBuff, uchar rBuffLen, uchar offset);

    void readBuffer(uchar *lBuff, uchar lBuffLen, uchar *rBuff, uchar rBuffLen, uchar offset);

    uchar decodeData(uchar *input, uchar lenInput, uchar *output, uchar lenOutput, uchar *state);

    void encodeData(uchar *input, uchar lenInput, uchar *output, uchar lenOutput, uchar state);

    uchar cmdCompare(char *lCmd, uchar *rCmd, uchar len);

    void setResponseOK(uchar state);

    uchar cmp_code(char *code1, char lenCode1, uchar *code2, char lenCode2);

    void setLatencyCode(uchar state);

    uchar processTpBuff(uchar *buff, uchar len);

    void cleanStatesCmdRes(void);

#ifdef __cplusplus
}
#endif

/* ======================================== USB Section ======================================== */

/* ----------------------------------- HID Report Descriptors ---------------------------------- */

/* ------------------------------------- Helper functions -------------------------------------- */

void usbRe_enumerate(void);

void runTestSequence(void);

uchar processDataCommand(uchar lenCmd, uchar state);

void processCommand(void);

// Interrupt part of USB library.
#ifdef USB_CFG_HAVE_INTRIN_ENDPOINT
void usbInterrupt(void);
#endif

/* --------------------------------------- Setup function -------------------------------------- */

#ifndef ARDUINO
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]);
#endif

/* ------------------------------------ Read/Write functions ----------------------------------- */

#if USB_CFG_IMPLEMENT_FN_WRITE
// Regular 8Bytes write.
// For at most 8 Bytes of data, writes are much simpler
// void receive8ByteDt(uchar *data, uchar len);

// Use this for large buffer writes (> 8 Bytes).
// void receiveBuffer(uchar *data, uchar len);

char receiveBuffer1(uchar *data, uchar len);

/*
    For long data in, if usbFunctionSetup returns USB_NO_MSG, this function
    is automatically called.
    In this case, outputBuffer is updated for both test/non-test commands
*/
#ifndef ARDUINO
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len);
#endif
#endif

#if USB_CFG_IMPLEMENT_FN_READ
/*
 * usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information, see the documentation in usbdrv/usbdrv.h.
 */
#ifndef ARDUINO
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len);
#endif
#endif
