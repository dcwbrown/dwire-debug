
MCU=attiny45
F_CPU = 16000000

CPPFLAGS+=-DI2CSOFT_ARGS='(0x25,10,(B,0,),(B,1,),PCINT0_vect)'
INCLUDE_LIBS += core

OBJDIR = build
CFLAGS =
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Wl,--gc-sections
CFLAGS += -fdata-sections -ffunction-sections
COMPILE = avr-gcc -Wall -Os --std=c++11 -save-temps=obj -DF_CPU=$(F_CPU) $(CFLAGS) $(CPPFLAGS) -mmcu=$(MCU)

BASENAME = $(notdir $(PWD))
OBJECTS = $(addsuffix .o, $(patsubst ..%,$(OBJDIR)%, \
../$(BASENAME)/$(BASENAME).cpp $(wildcard ../core/*.cpp)))

all: $(OBJDIR)/$(BASENAME).elf

dwload:
	dwdebug l $(BASENAME).elf ,qr

clean:
	rm -rf *.elf *.map $(OBJDIR)

$(OBJDIR)/%.cpp.o: ../%.cpp
	mkdir -p $(dir $@)
	$(COMPILE) -c $< -o $@

%.i: %.cpp
	$(COMPILE) -E -c $< -o $@

$(OBJDIR)/$(BASENAME).elf: $(OBJECTS)
	$(COMPILE) -Wl,-Map,$(BASENAME).map -o $(BASENAME).elf $(OBJECTS)

disasm:	$(BASENAME).elf
	avr-objdump -d $(BASENAME).elf > $(BASENAME).lss
