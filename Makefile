#PATH := /bin

# Uncomment the following line to remove the Linux dependency on GTK
#NOFILEDIALOG:=1

TARGET := dwdebug

ifdef WINDIR
BINARY := $(TARGET).exe
else
BINARY := $(TARGET)
endif

.PHONY: all
.PHONY: clean
.PHONY: run
.PHONY: install

#all: clean run
all: run

run: $(BINARY)
	./$(BINARY) f0,q

$(BINARY): *.c Makefile
ifdef WINDIR
ifdef NOFILEDIALOG
	i686-w64-mingw32-gcc -std=gnu99 -Wall -o $(BINARY) -Dwindows -DNOFILEDIALOG $(TARGET).c -lKernel32
else
	i686-w64-mingw32-gcc -std=gnu99 -Wall -o $(BINARY) -Dwindows $(TARGET).c -lKernel32 -lComdlg32
	#i686-w64-mingw32-gcc -g -oo -std=gnu99 -Wall -o $(BINARY) -Dwindows $(TARGET).c -lKernel32 -lComdlg32
endif
else
ifdef NOFILEDIALOG
	gcc -std=gnu99 -g -fno-pie -rdynamic -fPIC -Wall -o $(BINARY) -DNOFILEDIALOG $(TARGET).c `pkg-config --cflags`
else
	gcc -std=gnu99 -g -fno-pie -rdynamic -fPIC -Wall -o $(BINARY) $(TARGET).c `pkg-config --cflags --libs gtk+-3.0`
endif
endif
	ls -lap $(BINARY)

clean:
	-rm -f *.o *.map *.list $(BINARY)

install:
	cp -p $(BINARY) /usr/local/bin
