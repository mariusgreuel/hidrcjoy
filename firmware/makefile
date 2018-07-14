#
# makefile
# Copyright (C) 2018 Marius Greuel. All rights reserved.
#

PROJECT_ROOT=../..

#BOARD ?= Digispark
#BOARD ?= DigisparkPro
#BOARD ?= FabISP
BOARD ?= ProMicro

include $(PROJECT_ROOT)/avr-tools/build/boards.mk

TARGET = hidrcjoy
SOURCES = hidrcjoy.cpp
CPPFLAGS += -DBOARD_$(BOARD) -DUSB_$(USB)

ifeq ($(USB),V_USB)
    SOURCES += usbdrv/usbdrv.c usbdrv/usbdrvasm.S
    CPPFLAGS += -I. -DDEBUG_LEVEL=0
else ifeq ($(USB),LUFA)
    ARCH = AVR8
    F_USB = $(F_CPU)

    LUFA_PATH = $(PROJECT_ROOT)/lufa-LUFA-170418/LUFA
    DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
    include $(DMBS_LUFA_PATH)/lufa-sources.mk
    include $(DMBS_LUFA_PATH)/lufa-gcc.mk

    SOURCES += Descriptors.c $(LUFA_SRC_USB)
    CPPFLAGS += $(LUFA_CXX_INCLUDES) $(LUFA_CXX_DEFINES) $(LUFA_CXX_FLAGS)
    CPPFLAGS += -D USB_DEVICE_ONLY -D USE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
    CPPFLAGS += -D USE_FLASH_DESCRIPTORS -D FIXED_CONTROL_ENDPOINT_SIZE=8 -D FIXED_NUM_CONFIGURATIONS=1
    CPPFLAGS += -D NO_INTERNAL_SERIAL -D NO_DEVICE_REMOTE_WAKEUP -D NO_DEVICE_SELF_POWER
endif

include $(PROJECT_ROOT)/avr-tools/build/avr8.mk
