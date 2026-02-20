DEVKITPRO := /opt/devkitpro
DEVKITARM := $(DEVKITPRO)/devkitARM

CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
OBJCOPY := $(DEVKITARM)/bin/arm-none-eabi-objcopy

CFLAGS := -mthumb -mthumb-interwork -mcpu=arm7tdmi -O2 -Wall
CFLAGS += -I$(DEVKITPRO)/libtonc/include
CFLAGS += -Iinclude

LDFLAGS := -mthumb -mthumb-interwork -specs=gba.specs
LDFLAGS += -L$(DEVKITPRO)/libtonc/lib -ltonc

CFILES := source/main.c source/Background.c source/tube.c source/tube_top.c source/tube_top_seal.c source/tube_bottom_seal.c source/bird.c

OBJFILES := $(CFILES:.c=.o)

all: game.gba

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

game.elf: $(OBJFILES)
	$(CC) $(OBJFILES) $(LDFLAGS) -o $@

game.gba: game.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f $(OBJFILES) *.elf *.gba

