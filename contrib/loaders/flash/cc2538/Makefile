BIN2C = ../../../../src/helper/bin2char.sh

CROSS_COMPILE ?= arm-none-eabi-
GCC     = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

FLAGS  = -mthumb -Os -ffunction-sections -fdata-sections -g -gdwarf-3
FLAGS += -gstrict-dwarf -Wall -fno-strict-aliasing --asm
CFLAGS = -c -I. -mcpu=cortex-m3

OBJS := flashloader.o main.o startup.o flash.o

all: cc2538_algo.inc

%.o: %.c
	$(GCC) $(FLAGS) $(CFLAGS) -o $@ $<

%.o: %.s
	$(GCC) $(FLAGS) $(CFLAGS) -o $@ $<

cc2538_algo.out: $(OBJS)
	$(GCC) $(FLAGS) -o $@ $^ -Wl,-T"cc2538.lds"

%.bin: %.out
	$(OBJCOPY) -Obinary $< $@

%.inc: %.bin
	$(BIN2C) < $< > $@ || (rm -f $@ ; false)

clean:
	rm -f *.inc *.bin *.out *.map *.o *.d

.PRECIOUS: %.bin

.PHONY: all clean
