# Robo Platform, AVR Controller

The AVR microcontroller board, a component of the Robo Platform project. Both hardware schematics and software are available. This project is one component of a bigger project which provides manual control of a robot from a remote desktop.

This project is based on the [Obdev V-USB Library](https://www.obdev.at/products/vusb/index.html).


## Hardware

Initially designed for ATMEL ATmega8 AVR microcontroller using the Cadsoft Eagle (now Autodesk Eagle).
Schematics and PCB layouts are available for both SMD and TH (Through hole).

## Software

Based on the V-USB library, the algorithm handles USB interactions with an Android device (the Robo Platform Android App) and produces the required signals and commands.

The program is written in the C programming language and developed, compiled, and tested in ATMEL AVR Studio 6.1.


