//        dwdebug - debugger for DebugWIRE on ATtiny45.


#include "SystemServices.c"
#include "SimpleOutput.c"
#include "SimpleInput.c"

#include "GlobalData.c"
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
#include "WriteFlash.c"
#include "OpenFile.c"
#include "LoadFile.c"
#include "UserInterface.c"



int Program(int argCount, char **argVector) {UI(); return 0;}
