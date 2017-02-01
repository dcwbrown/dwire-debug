//        dwdebug - debugger for DebugWIRE on ATtiny45.


#include "system/system.c"
#include "GlobalData.c"
#include "dwire/dwire.c"
#include "commandline/commandline.c"
#include "commands/commands.c"
#include "gdbserver/gdbserver.c"
#include "ui/ui.c"




int main(int argCount, char **argVector) {
  systemstartup(argCount, argVector);
  UI();
  Exit(0);
}
