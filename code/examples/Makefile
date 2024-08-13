# Name: Makefile
# Project: PowerSwitch
# Author: Christian Starkjohann
# Creation Date: 2004-12-29
# Tabsize: 4
# Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
# This Revision: $Id: Makefile 277 2007-03-20 10:53:33Z cs $

DEVICE = atmega32
F_CPU = 12000000
AVRDUDE = avrdude -c usbasp -P usb -p $(DEVICE)
PROJ_SRC = ..
LIB_USB = $(PROJ_SRC)/thirdparty/usbdrv
MY_LIB = $(PROJ_SRC)/mylib
EXAMPLES_SRC = $(PROJ_SRC)/examples
SRC_FILE = comm_test
EXE_NAME = $(EXAMPLES_SRC)/$(SRC_FILE)/$(SRC_FILE)
# Choose your favorite programmer and interface above.

COMPILE = avr-gcc -Wall -DF_CPU=$(F_CPU) -Os -I$(LIB_USB) -I. -I$(MY_LIB) -mmcu=$(DEVICE) #-DDEBUG_LEVEL=2
# NEVER compile the final product with debugging! Any debug output will
# distort timing so that the specs can't be met.

OBJECTS = $(LIB_USB)/usbdrv.o $(LIB_USB)/usbdrvasm.o $(LIB_USB)/oddebug.o $(EXE_NAME).o
LIB_OBJECTS = $(MY_LIB)/controller.o $(MY_LIB)/led.o $(MY_LIB)/mypwm.o $(MY_LIB)/sensors.o $(MY_LIB)/vusb_wrapper.o

# symbolic targets:
all:	$(EXE_NAME).hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:$(EXE_NAME).hex:i


# Fuse low byte:
# 0xef = 1 1 1 0   1 1 1 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> crystal @ 12 MHz)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)
#
# Fuse high byte:
# 0xdb = 1 1 0 1   1 0 1 1
#        ^ ^ ^ ^   \-+-/ ^
#        | | | |     |   +---- RSTDISBL (disable external reset -> enabled)
#        | | | |     +-------- BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        +-------------------- DWEN (debug wire enable)
fuse_tiny2313:	# only needed for attiny2313
	$(AVRDUDE) -U hfuse:w:0xdb:m -U lfuse:w:0xef:m


clean:
	rm -f $(EXE_NAME).hex $(EXE_NAME).lst $(EXE_NAME).obj $(EXE_NAME).cof $(EXE_NAME).list $(EXE_NAME).map $(EXE_NAME).eep.hex $(EXE_NAME).bin $(EXE_NAME)*.o $(EXE_NAME).s $(LIB_USB)/*.o $(LIB_USB)/oddebug.s $(LIB_USB)/usbdrv.s $(MY_LIB)/*.o

# file targets:
$(EXE_NAME).bin:	$(OBJECTS) $(LIB_OBJECTS)
	$(COMPILE) -o $(EXE_NAME).bin $(OBJECTS) $(LIB_OBJECTS)

$(EXE_NAME).hex:	$(EXE_NAME).bin
	rm -f $(EXE_NAME).hex $(EXE_NAME).eep.hex
	avr-objcopy -j .text -j .data -O ihex $(EXE_NAME).bin $(EXE_NAME).hex
	./checksize $(EXE_NAME).bin
# do the checksize script as our last action to allow successful compilation
# on Windows with WinAVR where the Unix commands will fail.

disasm:	$(EXE_NAME).bin
	avr-objdump -d $(EXE_NAME).bin

cpp:
	$(COMPILE) -E $(EXE_NAME).c
