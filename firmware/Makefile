#
# Makefile
# Copyright (C) 2018 Marius Greuel
# SPDX-License-Identifier: GPL-3.0-or-later
#

MCU = atmega32u4
F_CPU = 16000000

TARGET = hidrcjoy
SOURCES = src/hidrcjoy.cpp

CPPFLAGS += -DBOARD_ARDUINO_LEONARDO=1
CPPFLAGS += -Iinclude

AVRDUDE_FLAGS ?= -c avr109 -p $(MCU) -P usb:2341:0036 -D 

include avr8.mk
