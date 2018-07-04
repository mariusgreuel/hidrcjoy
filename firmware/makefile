#
# makefile
# Copyright (C) 2018 Marius Greuel. All rights reserved.
#

#BOARD ?= Digispark
#BOARD ?= DigisparkPro
BOARD ?= FabISP
CFLAGS = -I. -DDEBUG_LEVEL=0
SOURCES = main.cpp  usbdrv/usbdrv.c usbdrv/usbdrvasm.S

vpath %.c . usbdrv
vpath %.S . usbdrv

include avr.mk
include boards.mk
