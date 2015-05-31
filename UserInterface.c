/// UserInterface.c

//        User interface
//
//        Reads lines, parses them into commands and parameters, and calls the
//        appropriate handler procedures.





int QuitRequested = 0;

void PCommand()      {PC = ReadNumber(1);}
void Go()            {Wsl("Going, going, ...., gone.");}
void QuitCommand()   {QuitRequested = 1;}
void TraceCommand()  {DwTrace();}
void FailCommand()   {Fail("FailCommand ...");}

void HelpCommand() {
  Wsl("dwdebug commands:\n"
  "p  Set PC\n"
  "q  Quit\n"
  "r  Display registers\n"
  "t  Trace instruction(s)");
}

void EmptyCommand() {Sb(); if (!DwEoln()) {HelpCommand();}}


struct CommandList {char *name; void (*handler)();};

struct CommandList UnconnectedCommands[] = {
  {"q",      QuitCommand},
  {"h",      HelpCommand},
  {"device", DeviceCommand},
  {"",       EmptyCommand}
};

struct CommandList ConnectedCommands[] = {
  {"d",  DumpDataBytesCommand},
  {"dw", DumpDataWordsCommand},
  {"g",  Go},
  {"p",  PCommand},
  {"q",  QuitCommand},
  {"r",  RegistersCommand},
  {"s",  StackCommand},
  {"t",  TraceCommand},
  {"u",  UnassembleCommand},
  {"h",  HelpCommand},

  {"reset",       DwReset},
  {"serialdump",  SerialDump},
  {"help",        HelpCommand},  // testing
  {"device",      DeviceCommand},

  {"",  EmptyCommand}
};


void HandleCommand(const char *cmd, struct CommandList *commands) {
  for (int i=0; i<countof(commands); i++) {
    if (!strcmp(cmd, commands[i].name)) {commands[i].handler(); return;}
  }
  Ws("Unrecognised command: '"); Ws(cmd); Wsl("'.");
}

void ParseAndHandleCommand(struct CommandList *commands) {
  char command[20];

  Sb(); if (NextCh() == ';') {SkipCh(); Sb();}

  if (Eof()) {if (IsUser(Input)) {Wl();}  QuitCommand();}
  else       {Ra(ArrayAddressAndLength(command)); HandleCommand(command, commands);}

  SkipWhile(NotDwEoln); if (Eoln()) {SkipEoln();} else {SkipCh();}
}

void Prompt() {
  u8 buf[4];  // Enough for a 2 word instruction
  Wx(PC, 4); Ws(": ");  // Word address, e.g. as used in pc and bp setting instructions
  DwReadFlash(PC<<1, 4, buf);
  Uaddr = PC + DisassembleInstruction(PC, buf);
  Wt(40);
}

void UnconnectedPrompt() {
  if (BufferTotalContent() == 0  &&  IsUser(Input)) {
    Wt(40); Ws("> "); Flush();
  } else {
    if (Prompted) {Wl();}
  }
  Prompted = 0;
}

void ConnectedPrompt() {
  if (BufferTotalContent() == 0  &&  IsUser(Input)) {
    if (!Prompted) {Prompt();}
    Wt(40); Ws("> "); Flush();
  } else {
    if (Prompted) {Wl();}
  }
  Prompted = 0;
}

void UI() {
  enum {unconnected, connected, running} state = unconnected;

  while (1) {
    if (QuitRequested) return;

    switch (state) {
      case unconnected:
        if (setjmp(FailPoint)) {Sl();}
        UnconnectedPrompt();
        ParseAndHandleCommand(UnconnectedCommands);
      break;

      case connected:
        if (setjmp(FailPoint)) {Sl();}
        ConnectedPrompt();
        ParseAndHandleCommand(ConnectedCommands);
      break;

      case running:
      break;
    }
  }


  //while (1) {
  //
  //  if (QuitRequested) return;
  //
  //  if (BufferTotalContent() == 0  &&  IsUser(Input)) {
  //    if (!Prompted) {Prompt();}
  //    Wt(40); Ws("> "); Flush();
  //  } else {
  //    if (Prompted) {Wl();}
  //  }
  //  Prompted = 0;
  //
  //  Sb(); if (NextCh() == ';') {SkipCh(); Sb();}
  //
  //  if (Eof()) {if (IsUser(Input)) {Wl();} break;}
  //  else       {Ra(ArrayAddressAndLength(command)); HandleCommand(command);}
  //
  //  SkipWhile(NotDwEoln); if (Eoln()) {SkipEoln();} else {SkipCh();}
  //}
}