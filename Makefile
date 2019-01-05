###############################################################################
# Makefile for the project zx-tape
###############################################################################

## General Flags
PROJECT = zx-tape
MCU = atmega16
TARGET = zx-tape
CC = avr-gcc
CPP = avr-g++

BUILDDIR = debug
## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

DEBUG = -ggdb
#DEBUG = -gdwarf-2

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall $(DEBUG) -std=gnu99 -DF_CPU=16000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF $(BUILDDIR)/dep/$(@F).d

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,$(DEBUG)

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -lm -Wl,-Map=$(BUILDDIR)/zx-tape.map
#-Wl,-u,vfprintf -lprintf_min

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = main.o

## Objects explicitly added by the user
LINKONLYOBJECTS =

## Build
all: $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).lss size

## Compile
$(OBJECTS): main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $< -o $(BUILDDIR)/$@

##Link
$(TARGET).elf: $(OBJECTS)
	 $(CC) $(LDFLAGS) $(BUILDDIR)/$(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(BUILDDIR)/$(TARGET).elf

%.hex: $(BUILDDIR)/$(TARGET).elf
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $(BUILDDIR)/$@

%.eep: $(BUILDDIR)/$(TARGET).elf
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $(BUILDDIR)/$@ || exit 0

%.lss: $(BUILDDIR)/$(TARGET).elf
	avr-objdump -h -S $< > $(BUILDDIR)/$@

size: $(BUILDDIR)/${TARGET}.elf
	@echo
	@avr-size -C --mcu=${MCU} $(BUILDDIR)/${TARGET}.elf

## Clean target
.PHONY: clean
clean:
	-rm -rf $(BUILDDIR)/$(OBJECTS) $(BUILDDIR)/zx-tape.elf $(BUILDDIR)/debug/dep/* $(BUILDDIR)/zx-tape.hex $(BUILDDIR)/zx-tape.eep $(BUILDDIR)/zx-tape.lss $(BUILDDIR)/zx-tape.map


## Other dependencies
-include $(shell mkdir $(BUILDDIR)/dep) $(wildcard $(BUILDDIR)/dep/*)

