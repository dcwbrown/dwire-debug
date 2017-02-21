# dwire-debug

Simple stand-alone programmer and debugger for DebugWIRE processors connected
directly to an FT232R based usb serial adapter on Linux or Windows.

This project would not have been possible without the careful investigation
and documentation of the DebugWire protocol by RikusW at
http://www.ruemohr.org/docs/debugwire.html.

##### License

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later
version.

##### Cross-platform

Built and tested on Windows 8.1 and Ubuntu 15.04. Requires only standard system
libraries/dlls.

##### Manual

See: [Manual](https://github.com/dcwbrown/dwire-debug/blob/master/Manual.md)


##### Building dwire-debug

Build on linux with a conventional gcc/make setup. The binary produced is
'dwdebug'.

Build on Windows with MinGW-w64 under cygwin. I install 32 bit cygwin and use
cygwin setup to add all the mingw64-i686 packages. The binary produced is
'dwdebug.exe'.

In both cases the binaries use only standard system APIs.

File chooser: If the `load` command is used without a parameter, dwdebug
will display the windows or GTK file chooser. However if built on linux
without GTK build support installed, the `load` command requires a
filename parameter.

##### Stand-alone

I became frustrated that debugging an ATtiny45 required me to install MS Visual
studio and AVR studio, both large packages. A time-consuming download
with a number of dependencies to install turning up along the way, and
Windows only.

I was looking for a simple debugger of the sort used on minicomputers (DEC's
DDT), or CP/M (SID) or MSDOS (DEBUG).

Additionally I hoped to connect directly to the DebugWIRE port, avoiding the
complication of special-pupose hardware like the AVR-ICE or AVR-Dragon (although
one of these is needed initially to program the DWEN fuse which enables the
DebugWIRE port).

dwire-debug satisfies these goals.

As well as reducing complexity (e.g. no special drivers), connecting directly
to the DebugWIRE port is noticeably faster.

##### FT232R hardware

The debugger uses an FT232R USB serial adapter with RX connected directly to
the DebugWIRE port, and TX connected through a diode. I used an £8.26 Foxnovo
FT232R based UART from amazon.co.uk. This board has jumper selectable 3.3V/5V
operation and provides power rails, making it easy to program and debug an
ATtiny out of circuit.

(CP210x based UARTs don't support Windows/Linux APIs for selecting custom baud
rates and I haven't had any success with them. Both the PL2303 based UARTS i
have tried didn't work at all for any purpose - I don't know if a good PL2303
would work.)

##### Wiring the FT232R adapter

Connect the diode's cathode (the end marked with a line around it) to the UART
txd line, and connect the diode's anode to the rxd line and to the DebugeWire
pin (e.g. pin 1 on an ATtiny45).

For an out of circuit connection, set the adapter to whichever voltage matches
the voltage range supported by the AVR device. For an ATtiny45, either voltage
is fine.

For an in-circuit connection, the Vcc power line is not connected, and setting
the adapter to 5v should work for any circuit: The diode protects the circuit
from any damage - it can only pull the DebugWIRE pin low - it cannot feed
overvoltage to the AVR.

![Simple out of circuit hardware](https://github.com/dcwbrown/dwire-debug/blob/master/simple-hardware.jpg)

##### ATtiny configuration

The ATtiny must have DebugWIRE (DWEN) enabled. DWEN is not enabled in chips
as shipped from Atmel, and (not surprisingly) cannot be enabled through
DebugWIRE.

For a chip as supplied by Atmel, ISP programming will be sufficient to enable
debugWIRE. You can use any ISP programming solution, for example:

 - ISP software includes the open source Avrdude or the official Atmel Studio.
 - ISP hardware includes the open source Arduino ISP, or Atmel's Dragon or AVR-ICE.

Once the DWEN fuse has been programmed (set to zero), disconnect the ISP.

Now the FT232 connected to just ground and the device reset pin and controlled
by dwire-debug is sufficient for both programming and debugging.
