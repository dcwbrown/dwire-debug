/// UserInterface.c

//        User interface
//
//        Reads lines, parses them into commands and parameters, and calls the
//        appropriate handler procedures.





void HelpCommand();

void PCommand()            {PC = ReadInstructionAddress("PC");}
void BPCommand()           {BP = ReadInstructionAddress("BP");}
void BCCommand()           {BP = -1;}
void QuitRunningCommand()  {if (CurrentPort >= 0) DwGo();      Exit(0);}
void DisableCommand()      {if (CurrentPort >= 0) DwDisable(); Exit(0);}
void QuitStoppedCommand()  {Exit(0);}
void FailCommand()         {Fail("FailCommand ...");}
void EmptyCommand()        {Sb(); if (!DwEoln()) {HelpCommand();}}
void VerboseCommand()      {Verbose = 1;}
void TimerEnableCommand()  {TimerEnable = 1;}
void TimerDisableCommand() {TimerEnable = 0;}


void QuitUnconnectedCommand() {
  if (CurrentPort >= 0) {
    Wl ();
    Wsl("A device is connected. Please use one of these quit commands:");
    Wl ();
    Wsl("  qr - Quit leaving device running. Executes a go command before exiting.");
    Wsl("  qs - Quit leaving the device stopped.");
    Wsl("  qi - Quit in In-System Programming mode. Use this if you have SCK, MISO and MOSI connected");
    Wsl("       and want to use SPI programming software such as AVRDUDE.");
    Fail("");
  }
  Exit(0);
}

void DisassemblyPrompt() {
  u8 buf[4];  // Enough for a 2 word instruction
  DwReadFlash(PC, 4, buf);
  Uaddr = PC + DisassembleInstruction(PC, buf);
  Wt(HasLineNumbers ? 60 : 40);
}


void TraceCommand() {
  int count = 1;
  Sb(); if (IsDwDebugNumeric(NextCh())) {count = ReadNumber(0);}
  if (count < 1) Fail("Trace count should be at least 1");
  DwTrace();
  while (count > 1) {DisassemblyPrompt(); Wl(); DwTrace(); count--;}
}


struct {char *name; char *help; int requiresConnection; void (*handler)();} Commands[] = {
  {"b",           "Set breakpoint",                                1, BPCommand},
  {"bc",          "Clear breakpoint",                              1, BCCommand},
  {"d",           "Dump data bytes",                               1, DumpDataBytesCommand},
  {"dw",          "Dump data words",                               1, DumpDataWordsCommand},
  {"wd",          "Write data bytes",                              1, WriteDataBytesCommand},
  {"e",           "Dump EEPROM bytes",                             1, DumpEEPROMBytesCommand},
  {"ew",          "Dump EEPROM words",                             1, DumpEEPROMWordsCommand},
  {"we",          "Write EEPROM bytes",                            1, WriteEEPROMBytesCommand},
  {"f",           "Dump flash bytes",                              1, DumpFlashBytesCommand},
  {"fw",          "Dump flash words",                              1, DumpFlashWordsCommand},
  {"wf",          "Write flash bytes",                             1, WriteFlashBytesCommand},
  {"l",           "Load file",                                     1, LoadFileCommand},
  {"ls",          "List available devices",                        0, DwListDevices},
  {"g",           "Go",                                            1, GoCommand},
  {"p",           "PC set / query",                                1, PCommand},
  {"q",           "Quit (available only when no device connected)",0, QuitUnconnectedCommand},
  {"qr",          "Quit with device running",                      0, QuitRunningCommand},
  {"qs",          "Quit with device stopped",                      0, QuitStoppedCommand},
  {"qi",          "Quit in ISP mode until next power cycle",       0, DisableCommand},
  {"r",           "Display registers",                             1, RegistersCommand},
  {"s",           "Stack",                                         1, StackCommand},
  {"t",           "Trace",                                         1, TraceCommand},
  {"te",          "Timer enable",                                  1, TimerEnableCommand},
  {"td",          "Timer disable",                                 1, TimerDisableCommand},
  {"u",           "Unassemble",                                    1, UnassembleCommand},
  {"h",           "Help",                                          0, HelpCommand},
  {"reset",       "Reset processor",                               1, DwReset},
  {"device",      "Connect to named device",                       0, DeviceCommand},
  {"config",      "Display fuses and lock bits",                   1, DumpConfig},
  {"help",        "Help",                                          0, HelpCommand},
  {"verbose",     "Set verbose mode",                              0, VerboseCommand},
  {"gdbserver",   "Start server for GDB",                          1, GdbserverCommand},
  {"",            0,                                               0, EmptyCommand},
};


int IsInteractive = 0;


void HelpCommand() {
  for (int i=0; Commands[i].help; i++) {
    Ws("  "); Ws(Commands[i].name); Wt(12); Ws("- "); Ws(Commands[i].help); Wsl(".");
  }
}


void HandleCommand(const char *cmd) {
  for (int i=0; i<countof(Commands); i++) {
    if (!strcmp(cmd, Commands[i].name)) {
      if (CurrentPort < 0  &&  Commands[i].requiresConnection) {
        ConnectFirstPort();
      }
      Commands[i].handler();
      return;
    }
  }
  Ws("Unrecognised command: '"); Ws(cmd); Wsl("'.");
}


void ParseAndHandleCommand() {
  char command[20];

  Sb(); if (IsCommandSeparator(NextCh())) {SkipCh(); Sb();}

  if (Eof()) {if (IsInteractive) {Wl();}  QuitRunningCommand();}
  else       {Ra(ArrayAddressAndLength(command)); HandleCommand(command);}

  SkipWhile(NotDwEoln); SkipWhile(IsDwEoln);
}


void Prompt() {
  if (BufferTotalContent() == 0  &&  IsInteractive) {
    if (OutputPosition == 0) {
      if (CurrentPort >= 0) {DisassemblyPrompt();} else {Ws("Unconnected.");}
    }
    Wt(HasLineNumbers ? 60 : 40); Ws("> "); Wflush();
    HorizontalPosition = 0;  // Let SimpleOutput know user returned to column 1
  } else {
    if (OutputPosition) {Wl();}
  }
}


void UI() {
  PreloadInput(GetCommandParameters());
  FindUsbtinys();
  FindSerials();
  while (1) {
    if (BufferTotalContent() == 0) {IsInteractive = Interactive(Input);}
    if (IsInteractive) {
      if (setjmp(FailPoint)) {SkipWhile(NotEoln); SkipEoln();}
    }
    Prompt();
    ParseAndHandleCommand();
  }
}