//        dwdebug - debugger for DebugWIRE on ATtiny45.

#include "SystemServices.c"
#include "SimpleOutput.c"
#include "SimpleInput.c"

typedef unsigned char u8;


#include "SerialPort.c"
#include "DwPort.c"
#include "Disassemble.c"
#include "DwDebugInput.c"
#include "RegistersCommand.c"
#include "UserInterface.c"

int Program(int argCount, char **argVector) {

  #ifdef windows
    MakeSerialPort("COM6", 1000000/128);
  #else
    MakeSerialPort("/dev/ttyUSB0", 1000000/128);
  #endif

  DwBreak();
  DwConnect();

  UI();
  return 0;
}
