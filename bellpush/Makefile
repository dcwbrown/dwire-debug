# Rule input $^, rule output $@

# Bare minimum attiny45 assembler project

target := bellpush

# PATH := /home/dave/projects/avr8/bin:$(PATH)
# Where avr8 is the avr 8-bit toolchain for windows downloaded from:
#   http://www.atmel.com/tools/atmelavrtoolchainforwindows.aspx


EFUSE := 0xff#   0: SELFPRGEN = 1: self programming disabled

#HFUSE := 0xdf#   7: RSTDISBL  = 1: reset not disabled
#             #   6: DWEN      = 1: DebugWIRE not enabled
#             #   5: SPIEN     = 0: serial programming enabled
#             #   4: WDTON     = 1: watchdog timer not enabled
#             #   3: EESAVE    = 1: EEPROM not preserved on chip erase
#             # 2-0: BODLEVEL  = 7: brown-out detect not enabled

HFUSE := 0x9f#   7: RSTDISBL  = 1: reset not disabled
             #   6: DWEN      = 0: DebugWIRE enabled
             #   5: SPIEN     = 0: serial programming enabled
             #   4: WDTON     = 1: watchdog timer not enabled
             #   3: EESAVE    = 1: EEPROM not preserved on chip erase
             # 2-0: BODLEVEL  = 7: brown-out detect not enabled

LFUSE := 0x62#   7: CKDIV8    = 0: clock is divided by 8
             #   6: CKOUT     = 1: clock output is not enabled
             # 5-4: SUTx      = 2: start-up time suitable for slowly rising power
             # 3-0: CKSELx    = 2: internal RS oscillator at 8 MHz.


.PHONY: all
.PHONY: program
.PHONY: clean
.PHONY: checkfuses
.PHONY: setfuses
.PHONY: debug       # Starts up dwdebug (was avrice and avr-gdb)
.PHONY: hvprogram


all: $(target).dump debug

program: $(target).bin
	avrdude -c dragon_isp -p t45 -U flash:w:$(target).bin:r

hvprogram: $(target).bin
	avrdude -c dragon_hvsp -p t45 -U flash:w:$(target).bin:r

%.bin: %.elf
	avr-objcopy -O binary $^ $@

%.dump: %.elf
	avr-objdump -D $^ > $@

$(target).elf: %.elf: %.o
	avr-ld -o $@ *.o -M >$*.map

%.o: %.s
	avr-as -agls -gstabs -mmcu=attiny45 -o $@ $^ >$*.list

clean:
	rm -f *.axf *.map *.o *.bin *.list *.dump *.map *.elf

checkfuses:
	avrdude -c dragon_isp -p t45 -u -U efuse:v:$(EFUSE):m -U hfuse:v:$(HFUSE):m -U lfuse:v:$(LFUSE):m

setfuses:
	avrdude -c dragon_isp -p t45 -u -U efuse:w:$(EFUSE):m -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m

debug:
#	avarice -B 50kHz -g -w -P attiny45 :4242 & sleep 3 ; avr-gdb -tui -ex "layout asm" -ex "display/i $pc" -ex "target remote localhost:4242" $(target).elf
#	avarice -B 50kHz -g -w -P attiny45 :4242 & sleep 3 ; avr-gdb -ex "target remote localhost:4242" -ex "ni" -ex "ni" -ex "layout asm" $(target).elf
	../dwdebug reset, l $(target).elf, reset, r



