#
# boards.mk
# Copyright (C) 2018 Marius Greuel. All rights reserved.
#

ifeq ($(BOARD),ArduinoUno)
DEVICE = atmega328p
F_CPU = 16000000
else ifeq ($(BOARD),Digispark)
DEVICE = attiny85
F_CPU = 16500000
FLASH_TOOL = micronucleus
else ifeq ($(BOARD),DigisparkPro)
DEVICE = attiny167
F_CPU = 16000000
FLASH_TOOL = micronucleus
else ifeq ($(BOARD),ProMicro)
DEVICE = atmega32u4
F_CPU = 16000000
else ifeq ($(BOARD),FabISP)
DEVICE = attiny44
F_CPU = 12000000
else ifeq ($(BOARD),USBASP)
DEVICE = atmega8
F_CPU = 12000000
endif
