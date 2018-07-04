# R/C to PC Joystick

hidrcjoy is my ATtiny based solution to convert a R/C transmitter PPM signal to a USB based HID joystick.

## Background

Many tranmitters for radio controlled models use PPM signal coding to transmit the position of the control sticks to the receiver. Often, the transmitter has a connector, from which this PPM signal can be obtained. The hidrcjoy firmware is capable of decoding the PPM signal and converting the signals to an USB joystick. The R/C transmitter can then be used as an USB joystick on a PC, for instance to fly a model airplane on a PC based 3D simulation.

## Features

- Decodes a standard PPM signal from a remote control transmitter
- Blinking LED with two different frequencies to indicate signal quality
- Windows application to adjust PPM timing parameters, channel mapping, and channel polarity
- Works with $2 ATtiny boards

## Hardware

I tried three different boards, mostly for educational purposes. All three boards are inexpensive boards without native USB support, so USB is provided by a software USB stack called V-USB.

If you have an ISP programmer, go for the USBtinyISP/FabISP board, otherwise the Digispark board with the built-in bootloader works good enough.

### Digispark (ATtiny85)

My first try was a Digispark clone board with an ATtiny85. The ATtiny85 does not have a input capture pin, so I used a free running timer in order to measure the PPM signal. I used the USI overflow interrupt instead of the regular pin change interrupt, as I did not want to disturb the timing required by the V-USB implementation. This works reasonably well, however, there is a little noise on the channel readings, probably due to some delay introduced by the V-USB interrupt.

Connect the PPM signal to pin PB2, and the R/C transmitter ground to the board ground pin. The LED is the built-in LED on port PB1.

### FabISP (ATtiny44)

The second board I tried was a FabISP board based on an ATtiny44, which is variant of the popular USBtinyISP programmer. The ATtiny44 has an input capture, but unfortunatly, the ICP1 pin is tied up by the USB connection. As a workaround, I configured the input capture to use the analog comparator. That way, we can compare the PPM signal voltage to the bandgap voltage (1.1V) and use any analog input pin. Using the input capture provide much more stable channel readings as compared to the pin change interrupt solution.

Connect the PPM signal to pin PA6/ADC6/MOSI (pin 4 of the ISP connector), and the R/C transmitter ground to the board ground pin. The LED is the connected to port PA5/MISO (pin 1 of the ISP connector).

### DigisparkPro (ATtiny167)

The third board I tried was a Digispark Pro clone based on an ATtiny167. The ATtiny167 finally has a usable input capture, which does not require any workarounds.

Connect the PPM signal to pin PA4, and the R/C transmitter ground to the board ground pin. The LED is the built-in LED on port PB1.

## Building the software

I put the precompiled binaries into the releases folder. To build the binaries yourself, see below:

### Firmware

To build the firmware, you need the AVR8 toolchain 3.5.4.1709, GNU make, and avrdude or micronucleus in your path. Depending on the board, type
make BOARD=Digispark
make BOARD=DigisparkPro
make BOARD=FabISP

### Windows Software

To build the PC software, you need Visual Studio 2017. Just open the solution and hit build.

## License

hidrcjoy is released under the GNU GPLv2.

Thanks to Objective Development for the firmware-only USB driver V-USB.
