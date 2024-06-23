# ROBO Platform | AVR Controller

The AVR microcontroller board is a component of the [Robo Platform project](https://github.com/m-dayani/robo-platform), which provides general-purpose inputs and outputs for a cell phone controller. Both hardware schematics and software are available.

This project is based on the [Obdev V-USB Library](https://www.obdev.at/products/vusb/index.html).


## Hardware

The hardware was initially designed for the [ATMEL ATmega8](https://www.microchip.com/en-us/product/ATMEGA8) AVR microcontroller using the Cadsoft Eagle (now [Autodesk Eagle](https://www.autodesk.com/products/eagle/free-download)).
Schematics and PCB layouts are available for SMD and TH (Through hole) technologies.

## Software

Based on the V-USB library, the algorithm handles USB interactions with an Android device (the [Robo Platform Android App](https://github.com/m-dayani/robo-platform-android)) and produces the required signals and commands.

The program is written in the C programming language and developed, compiled, and tested in [ATMEL AVR Studio 6.1](https://www.microchip.com/en-us/tools-resources/archives/avr-sam-mcus).

## Usage

The instructions to use this project are the same as any other Obdev V-USB project:

- Prepare the development board
- Implement your logic, link the library, and compile
- Upload the .hex file to the microcontroller
- You should also install an appropriate driver to connect the board to a desktop computer. This step can be skipped for connections to Android devices (using an OTG cable, Android auto-detects the device).

You can learn more about it in [this step-by-step tutorial](https://codeandlife.com/2012/01/22/avr-attiny-usb-tutorial-part-1/).

