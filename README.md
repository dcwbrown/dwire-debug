# dwire-debug

Simple stand-alone debugger for ATtiny 45 and other DebugWIRE chips connected
directly to an FT232R or similar on Linux or Windows.

###### License

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later
version.

###### Cross-platform

Built and tested on Windows 8.1 and Ubuntu 15.04. Requires only standard system
libraries/dlls.

Build on Windows with MinGW-w64. I install 32 bit cygwin and use cygwin setup
to add all the mingw64-i686 packages.

###### Stand-alone

I became frustrated that debugging an ATtiny45 required me to install MS Visual
studio and AVR studio, both large packages. It's a lot to download,
time-consuming to install, there are a number of dependencies to install, and
it is Windows only.

I wanted a simple debugger like we used to use on minicomputers (DEC's DDT), or
CP/M (SID) or MSDOS (DEBUG).

###### FT232R hardware

The debugger does not work with special hardware, instead it uses a standard USB
UART and a schottky diode. I used an £8.26 Foxnovo FT232R based UART from
amazon.co.uk. This board has jumper selectable 3.3V/5V operation and provides
power rails, making it easy to program and debug an ATtiny out of circuit, but
any UART that works with your OS and at the same voltage as your circuit should
work.

Connect the diode's cathode (the end marked with a line) to the UART txd line,
and connect the diode's anode to the rxd line and to the DebugeWire pin (pin 1
on an ATtiny45).

###### ATtiny configuration

The ATtiny must have DebugWIRE (DWEN) enabled. DWEN is not enabled in chips
as shipped from Atmel, and (not surprisingly) cannot be enabled through
DebugWIRE. Therefore the chips must have the DWEN fuse programmed (set to 0)
using a conventional Atmel programmer - I use an AVR Dragon. Once this is
done dwire-debug is sufficient for both programming and debugging.

