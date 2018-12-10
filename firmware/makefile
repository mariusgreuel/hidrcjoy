#
# makefile
# Copyright (C) 2018 Marius Greuel. All rights reserved.
#

ifndef MY_TOOLCHAIN_AVR8
    $(error error : environment variable MY_TOOLCHAIN_AVR8 is not defined. MY_TOOLCHAIN_AVR8 must point to the tools-avr8 repository)
endif

#BOARD ?= Digispark
#BOARD ?= DigisparkPro
#BOARD ?= FabISP
BOARD ?= ProMicro

include $(MY_TOOLCHAIN_AVR8)/build/boards.mk

TARGET = hidrcjoy
SOURCES = hidrcjoy.cpp Descriptors.c
CPPFLAGS += -DBOARD_$(BOARD) -DUSB_$(USB)

ifeq ($(USB),V_USB)
    SOURCES += usbdrv/usbdrv.c usbdrv/usbdrvasm.S
    CPPFLAGS += -I. -DDEBUG_LEVEL=0
else ifeq ($(USB),LUFA)
    ARCH = AVR8
    F_USB = $(F_CPU)

ifndef MY_LUFA_PATH
    $(error error : environment variable MY_LUFA_PATH is not defined. MY_LUFA_PATH must point to the LUFA folder 'lufa-LUFA-170418')
endif

    LUFA_PATH = $(MY_LUFA_PATH)/LUFA
    DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
    include $(DMBS_LUFA_PATH)/lufa-sources.mk
    include $(DMBS_LUFA_PATH)/lufa-gcc.mk

    SOURCES += $(LUFA_SRC_USB)
    CPPFLAGS += $(LUFA_CXX_INCLUDES) $(LUFA_CXX_DEFINES) $(LUFA_CXX_FLAGS)
    CPPFLAGS += -D USB_DEVICE_ONLY -D USE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
    CPPFLAGS += -D USE_FLASH_DESCRIPTORS -D FIXED_CONTROL_ENDPOINT_SIZE=8 -D FIXED_NUM_CONFIGURATIONS=1
    CPPFLAGS += -D NO_INTERNAL_SERIAL -D NO_DEVICE_REMOTE_WAKEUP -D NO_DEVICE_SELF_POWER
endif

include $(MY_TOOLCHAIN_AVR8)/build/avr8.mk
