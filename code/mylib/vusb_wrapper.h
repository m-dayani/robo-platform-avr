#pragma once

#include "../thirdparty/usbdrv/usbdrv.h"
#include "../thirdparty/usbdrv/oddebug.h"

#define LEN_USB_BUFF_IN 64
#define LEN_USB_BUFF_OUT 64

#define DEFAULT_TEST_IN_MESSAGE "in-code-9372"
#define DEFAULT_TEST_OUT_MESSAGE                                        \
    {                                                                   \
        'o', 'u', 't', '-', 'c', 'o', 'd', 'e', '-', '6', '3', '3', '4' \
    }
#define DEFAULT_TEST_OUT_MSG_LEN 13
#define DEFAULT_OUT_CTRL_INIT_VAL 0x00

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

// USB input (read) buffer
extern uchar inputBuffer[LEN_USB_BUFF_IN];
// USB output (write) buffer
extern uchar outputBuffer[LEN_USB_BUFF_OUT];
// Testing Note3 claim.
extern uchar stateBuffPos;

void clearBuffer(uchar *buff, uchar offset, uchar len);

void insertBuffer(uchar *lBuff, uchar lBuffLen, uchar *rBuff, uchar rBuffLen, uchar offset);

void readBuffer(uchar *lBuff, uchar lBuffLen, uchar *rBuff, uchar rBuffLen, uchar offset);

uchar decodeData(uchar *input, uchar lenInput, uchar *output, uchar lenOutput, uchar *state);

void encodeData(uchar *input, uchar lenInput, uchar *output, uchar lenOutput, uchar state);

uchar cmdCompare(char *lCmd, uchar *rCmd, uchar len);

void setResponseOK(uchar state);

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

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]);

/* ------------------------------------ Read/Write functions ----------------------------------- */

#if USB_CFG_IMPLEMENT_FN_WRITE
// Regular 8Bytes write.
// For at most 8 Bytes of data, writes are much simpler
void receive8ByteDt(uchar *data, uchar len);

// Use this for large buffer writes (> 8 Bytes).
void receiveBuffer(uchar *data, uchar len);

/*
    For long data in, if usbFunctionSetup returns USB_NO_MSG, this function
    is automatically called.
    In this case, outputBuffer is updated for both test/non-test commands
*/
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len);
#endif

#if USB_CFG_IMPLEMENT_FN_READ
/*
 * usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information, see the documentation in usbdrv/usbdrv.h.
 */
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len);
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

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]);

/* ------------------------------------ Read/Write functions ----------------------------------- */

#if USB_CFG_IMPLEMENT_FN_WRITE
// Regular 8Bytes write.
// For at most 8 Bytes of data, writes are much simpler
void receive8ByteDt(uchar *data, uchar len);

// Use this for large buffer writes (> 8 Bytes).
void receiveBuffer(uchar *data, uchar len);

/*
    For long data in, if usbFunctionSetup returns USB_NO_MSG, this function
    is automatically called.
    In this case, outputBuffer is updated for both test/non-test commands
*/
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len);
#endif

#if USB_CFG_IMPLEMENT_FN_READ
/*
 * usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information, see the documentation in usbdrv/usbdrv.h.
 */
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len);
#endif
