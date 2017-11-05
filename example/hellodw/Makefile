
DEVICE  = attiny84
F_CPU   = 8000000

CFLAGS =
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Wl,--gc-sections
CFLAGS += -fdata-sections -ffunction-sections
COMPILE = avr-gcc -Wall -Os --std=gnu99 -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

BASENAME = hellodw
OBJECTS = $(BASENAME).o

all: $(BASENAME).elf

dwload:
	dwdebug l $(BASENAME).elf ,qr

clean:
	rm -f *.elf *.map *.o *.i *.lss

.c.o:
	$(COMPILE) -c $< -o $@

%.i: %.c
	$(COMPILE) -E -c $< -o $@

$(BASENAME).elf: $(BASENAME).o
	$(COMPILE) -Wl,-Map,$(BASENAME).map -o $(BASENAME).elf $(BASENAME).o

disasm:	$(BASENAME).elf
	avr-objdump -d $(BASENAME).elf > $(BASENAME).lss
