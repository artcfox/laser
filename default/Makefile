###############################################################################
# Makefile for Laser Puzzle
###############################################################################

## General Flags
PROJECT = Laser
GAME= laser
MCU = atmega644
TARGET = $(GAME).elf
CC = avr-gcc
INFO=../gameinfo.properties
UZEBIN_DIR=../../../bin

## Used for programming the HEX file directly
PROGRAMMER = -c avrispmkII -P usb
AVRDUDE = avrdude $(PROGRAMMER) -p $(MCU)

## Kernel settings
KERNEL_DIR = ../../../kernel
KERNEL_OPTIONS  = -DVIDEO_MODE=3 -DINTRO_LOGO=1 -DSCROLLING=0 -DSOUND_MIXER=1 -DSOUND_CHANNEL_5_ENABLE=1
KERNEL_OPTIONS += -DMAX_SPRITES=21 -DRAM_TILES_COUNT=33 -DSCREEN_TILES_V=28
KERNEL_OPTIONS += -DOVERLAY_LINES=0 -DTRANSLUCENT_COLOR=0x1C

# Only necessary if scrolling is enabled
#KERNEL_OPTIONS += -DVRAM_TILES_V=32

## In order to save quite a bit of flash space, it is possible to include only
## the waves that your patches.inc file uses, by including a custom sounds.inc
## file rather than the one included in the kernel by default. The trickery
## below is required to escape spaces in mixer path (due to custom sounds.inc).
## Be aware that if you remove waves from the wavetable, the indicies of the
## waves you specify in your patches.inc file may need to be changed to match!
## To use a custom sounds.inc file, copy the one from kernel/data/sounds.inc
## into your game's data directory, and uncomment the following lines:
# EMPTY :=
# SPACE := $(EMPTY) $(EMPTY)
# SPACE_ESC := \\$(SPACE)
# MIX_PATH := $(realpath ../data/sounds.inc)
# MIX_PATH_ESC := $(subst $(SPACE),$(SPACE_ESC),$(MIX_PATH))
# KERNEL_OPTIONS += -DMIXER_WAVES=\"$(MIX_PATH_ESC)\"

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -Wextra -Winline -gdwarf-2 -std=gnu99 -DF_CPU=28636360UL -Os -fsigned-char -ffunction-sections -mstrict-X -maccumulate-args
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d
CFLAGS += $(KERNEL_OPTIONS)


## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=$(GAME).map 
LDFLAGS += -Wl,-gc-sections

## The next line is only for video mode 3 with scrolling. Adjust the .data value to be 0x800100+VRAM_TILES_H*VRAM_TILES_V
#LDFLAGS += -Wl,--section-start,.noinit=0x800100 -Wl,--section-start,.data=0x800500

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = uzeboxVideoEngineCore.o uzeboxCore.o uzeboxSoundEngine.o uzeboxSoundEngineCore.o uzeboxVideoEngine.o $(GAME).o

## Objects explicitly added by the user
LINKONLYOBJECTS =

## Include Directories
INCLUDES = -I"$(KERNEL_DIR)"

## Build
all: $(TARGET) $(GAME).hex $(GAME).eep $(GAME).lss $(GAME).uze

## Compile Kernel files
uzeboxVideoEngineCore.o: $(KERNEL_DIR)/uzeboxVideoEngineCore.s
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $<

uzeboxSoundEngineCore.o: $(KERNEL_DIR)/uzeboxSoundEngineCore.s
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $<

uzeboxCore.o: $(KERNEL_DIR)/uzeboxCore.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uzeboxSoundEngine.o: $(KERNEL_DIR)/uzeboxSoundEngine.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uzeboxVideoEngine.o: $(KERNEL_DIR)/uzeboxVideoEngine.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

## Compile game sources
$(GAME).o: ../$(GAME).c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@
	avr-size -A --format=avr --mcu=$(MCU) $^

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

%.uze: $(TARGET)
	-$(UZEBIN_DIR)/packrom $(GAME).hex $@ $(INFO)

## Clean target
.PHONY: clean flash read_flash
clean:
	-rm -rf $(OBJECTS) $(GAME).eep $(GAME).elf $(GAME).hex $(GAME).lss $(GAME).map $(GAME).o $(GAME).uze dep/*

flash: all
	$(AVRDUDE) -U flash:w:$(GAME).hex:i

read_flash:
	$(AVRDUDE) -U flash:r:$(GAME).bin:r

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)
