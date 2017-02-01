### DwDebug source tree

directory | contains
--- | ---
system      | Provides a cross platform interface for i/o across Windows and Linux
dwire       | Implements the debugWire protocol
commandline | Prompting and command parsing
commands    | Implementation of each dwdebug command
gdbserver   | Prototype implementation of GDB server (GDB target remote command) courtesy of @kadamski
ui          | The main user interface loop
