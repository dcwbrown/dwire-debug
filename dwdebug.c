//        dwdebug - debugger for DebugWIRE on ATtiny45.

#include "SystemServices.c"
#include "SimpleOutput.c"
#include "SimpleInput.c"

typedef unsigned char u8;


#include "SerialPort.c"
#include "DwPort.c"
#include "Disassemble.c"
#include "DwDebugInput.c"
#include "DeviceCommand.c"
#include "RegistersCommand.c"
#include "StackCommand.c"
#include "UnassembleCommand.c"
#include "Dump.c"
#include "DumpCommand.c"
#include "GoCommand.c"
#include "UserInterface.c"



int Program(int argCount, char **argVector) {UI(); return 0;}
