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
	i686-w64-mingw32-gcc -std=gnu99 -Wall -o $(BINARY) -Dwindows $(TARGET).c -lKernel32 -lComdlg32
else
	gcc -std=gnu99 -g -fno-pie -rdynamic -Wall -o $(BINARY) $(TARGET).c `pkg-config --cflags --libs gtk+-3.0`
endif
	ls -lap $(BINARY)

clean:
	-rm -f *.o *.map *.list $(BINARY)
