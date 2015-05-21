#PATH := /bin

TARGET := dwdebug

ifdef WINDIR
BINARY := $(TARGET).exe
else
BINARY := $(TARGET)
endif

.PHONY: all
.PHONY: clean
.PHONY: run

#all: clean run
all: run

run: $(BINARY)
	echo "r" | ./$(BINARY)

$(BINARY): *.c Makefile
ifdef WINDIR
	i686-w64-mingw32-gcc -nostartfiles -std=gnu99 -Wall -Wl,-eEntryPoint -o $(BINARY) -Dwindows $(TARGET).c -lKernel32
else
	gcc -nostartfiles -std=gnu99 -g -fno-pie -Wall -Wl,-eEntryPoint -o $(BINARY) $(TARGET).c
endif
	ls -lap $(BINARY)

clean:
	-rm -f *.o *.map *.list $(BINARY)
