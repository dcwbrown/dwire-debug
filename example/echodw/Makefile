
#MCU=attiny13
#F_CPU = 9600000
MCU=attiny84
F_CPU = 8000000
#MCU=attiny85
#F_CPU = 1000000

CFLAGS =
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Wl,--gc-sections
CFLAGS += -fdata-sections -ffunction-sections
COMPILE = avr-gcc -Wall -Os --std=c++11 -save-temps=obj -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(MCU)

BASENAME = $(notdir $(PWD))
OBJECTS = $(addsuffix .o, $(basename $(wildcard *.cpp)))

all: $(BASENAME).elf

dwload:
	dwdebug l $(BASENAME).elf ,qr

clean:
	rm -f *.elf *.map *.o *.i *.ii *.s *.lss

.cpp.o:
	$(COMPILE) -c $< -o $@

%.i: %.cpp
	$(COMPILE) -E -c $< -o $@

$(BASENAME).elf: $(OBJECTS)
	$(COMPILE) -Wl,-Map,$(BASENAME).map -o $(BASENAME).elf $(OBJECTS)

disasm:	$(BASENAME).elf
	avr-objdump -d $(BASENAME).elf > $(BASENAME).lss
