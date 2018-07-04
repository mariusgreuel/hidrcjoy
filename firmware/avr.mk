#
# avr.mk
# Copyright (C) 2018 Marius Greuel. All rights reserved.
#

CC = avr-gcc
CXX = avr-g++
AS = avr-as
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm

CFLAGS += -mmcu=$(DEVICE) -DF_CPU=$(F_CPU)
CFLAGS += -Wall -MMD -MP
CFLAGS += -Os
CXXFLAGS += -std=c++11 -fno-exceptions
ASFLAGS += -x assembler-with-cpp
LDFLAGS += -Llibs

FLASH_TOOL ?= avrdude
AVRDUDE_FLAGS ?= -c usbtiny -D
AVRDUDE_FLAGS += -p $(DEVICE)
MICRONUCLEUS_FLAGS += --run

OBJDIR ?= objs
TARGET ?= main
ELFFILE ?= $(OBJDIR)/$(TARGET).elf
HEXFILE ?= $(OBJDIR)/$(TARGET).hex
LSTFILE ?= $(OBJDIR)/$(TARGET).lst

OBJECTS = $(addprefix $(OBJDIR)/,$(addsuffix .o, $(basename $(notdir $(SOURCES)))))

ifeq ($(OS),Windows_NT)
    RM = del
    MKDIR = mkdir
    RMDIR = rmdir /s /q
    ospath = $(subst /,\,$1)
else
    RM = rm
    MKDIR = mkdir -p
    RMDIR = rm -r -f
    ospath = $1
endif

all: $(HEXFILE) size
	@echo Done: $(call ospath,$(abspath $(HEXFILE)))

$(ELFFILE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(ELFFILE) $(OBJECTS)

$(HEXFILE): $(ELFFILE)
	avr-objcopy -j .text -j .data -O ihex $(ELFFILE) $(HEXFILE)

hex: $(HEXFILE)

size: $(HEXFILE)
	$(SIZE) --mcu=$(DEVICE) $(HEXFILE)

flash: $(HEXFILE) size
ifeq ($(FLASH_TOOL),avrdude)
	$(FLASH_TOOL) $(AVRDUDE_FLAGS) -U flash:w:$(HEXFILE):i
else ifeq ($(FLASH_TOOL),micronucleus)
	$(FLASH_TOOL) $(MICRONUCLEUS_FLAGS) $(HEXFILE)
else
	@echo Error: FLASH_TOOL is invalid or not supported
endif

fuse:
ifeq ($(FLASH_TOOL),avrdude)
	echo $(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:w:$(FUSE_L):m -U hfuse:w:$(FUSE_H):m -U efuse:w:$(FUSE_E):m
else
	@echo Error: FLASH_TOOL is invalid or feature is not supported
endif

list: $(ELFFILE)
	$(OBJDUMP) -d -S $(ELFFILE) >$(LSTFILE)

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	$(MKDIR) $(call ospath,$(OBJDIR))

$(OBJDIR)/%.o: %.c
	@echo $(call ospath,$<)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@echo $(call ospath,$<)
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	@echo $(call ospath,$<)
	$(CC) $(CFLAGS) $(ASFLAGS) -c $< -o $@
	
clean:
	-$(RMDIR) $(call ospath,$(OBJDIR)) 2>nul

.PHONY: hex size flash fuse list clean

-include $(OBJECTS:.o=.d)
