#PATH := /bin

# Uncomment the following line to remove the Linux dependency on GTK
#NOFILEDIALOG:=1

TARGET := dwdebug

ifdef WINDIR
BINARY := $(TARGET).exe
else
BINARY := $(TARGET)

#autodetect FILEDIALOG options for Linux based on existance of gtk+-3.0
ifndef NOFILEDIALOG
       FILEDIALOG := $(shell pkg-config --exists gtk+-3.0 && echo \`pkg-config --cflags --libs gtk+-3.0\`)
endif
ifeq ($(strip $(FILEDIALOG)),)
       FILEDIALOG = -DNOFILEDIALOG
endif

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
	i686-w64-mingw32-gcc -std=gnu99 -Wall -o $(BINARY) -Dwindows -DNOFILEDIALOG $(TARGET).c -lKernel32 -lWinmm
else
	i686-w64-mingw32-gcc -std=gnu99 -Wall -o $(BINARY) -Dwindows $(TARGET).c -lKernel32 -lWinmm -lComdlg32
	#i686-w64-mingw32-gcc -g -oo -std=gnu99 -Wall -o $(BINARY) -Dwindows $(TARGET).c -lKernel32 -lComdlg32
endif
else
	gcc -std=gnu99 -g -fno-pie -rdynamic -Wall -o $(BINARY) $(TARGET).c $(FILEDIALOG)
endif
	ls -lap $(BINARY)

clean:
	-rm -f *.o *.map *.list $(BINARY)

install:
	cp -p $(BINARY) /usr/local/bin
