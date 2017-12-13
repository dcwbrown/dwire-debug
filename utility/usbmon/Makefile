#!/usr/bin/make

#ARCH=RPi
ifeq ($(ARCH),RPi)
#sudo apt-get install libudev-dev
#mv libusb.h /zaloh/make/RPtools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot/usr/include/libusb-1.0
#mv libusb-1.0.a libudev.so /zaloh/make/RPtools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot/lib
RASPBERRYPI_TOOLS=/zaloh/make/RPtools/arm-bcm2708
RASPBERRYPI_TOOLS_BIN=$(RASPBERRYPI_TOOLS)/arm-rpi-4.9.3-linux-gnueabihf/bin
CXX=$(RASPBERRYPI_TOOLS_BIN)/arm-linux-gnueabihf-g++
CC=$(RASPBERRYPI_TOOLS_BIN)/arm-linux-gnueabihf-gcc
LIBS= -lusb-1.0 -lpthread -ludev
else
LIBS= -lusb-1.0
endif

# intermediate files
MKDIR=mkdir -p
INTDIR = build

# -g for debug -Os for optimize
CPPFLAGS = -std=c++11 -Os

sources = $(wildcard *.cpp)
objects = $(addprefix $(INTDIR)/,$(addsuffix .o,$(sources)))
targets = $(notdir $(PWD))

ifneq ($(wildcard ../core/rpc.hpp),)
CPPFLAGS += -DCORE_RPC_HPP
sources += $(abspath ../core/rpc.cpp)
endif

all: $(targets)

$(INTDIR)/%.cpp.o: %.cpp
	@$(MKDIR) $(dir $@) ; echo $@
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MT $@ -o $(addsuffix .d,$@)

$(targets): $(objects)
	@echo $@
	@$(CXX) $^ $(LIBS) -o $@

%.cpp.lst: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -O0 -g -fverbose-asm -Wa,-adhln $< > $@

clean:
	rm -rf $(INTDIR)

# include dependencies
-include $(wildcard $(INTDIR)/*.d)
