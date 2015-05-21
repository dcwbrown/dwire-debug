/// UserInterface.c

//        User interface
//
//        Reads lines, parses them into commands and parameters, and calls the
//        appropriate handler procedures.


#define countof(array) (sizeof(array)/(sizeof(array)[0]))
#define AdrAndLim(buffer) buffer,sizeof(buffer)




int ReadNumber() {  // Recognise Leading '$' or trailing 'h' as hex, or '#' or 'd' for decimal.
  int  decimal = 0;
  int  hex     = 0;
  char c       = 0;

  Sb(); c = NextCh();

  if      (c == '$') {decimal = -1; SkipCh();}
  else if (c == '#') {hex = -1;     SkipCh();}
  else if (c < '0'  ||  c > '9') {Fail("Expected digit");}

  while (1) {
    c = NextCh();
    if      (c == 'h') {decimal = -1;}
    else if (c == 'd') {hex = -1;}
    else if (c  >= '0'  &&  c <= '9') {decimal = decimal*10 + c-'0'; hex = hex * 16 + c - '0';}
    else if (c  >= 'a'  &&  c <= 'f') {decimal = -1; hex = hex * 16 + c - 'a' + 10;}
    else if (c  >= 'A'  &&  c <= 'F') {decimal = -1; hex = hex * 16 + c - 'F' + 10;}
    else {if (decimal > 0) {return decimal;} if (hex > 0) {return hex;} Fail("Hex digits in decimal number");}
    SkipCh();
  }
}





int QuitRequested = 0;

void SetPC()     {PC = ReadNumber();}
void Go()        {Wsl("Going, going, ...., gone.");}
void Quit()      {QuitRequested = 1;}
void None()      {/* empty command */}

void Registers() {
  u8 registers[32] = {0};
  DwReadRegisters(registers);

  for (int row=0; row<4; row++) {
    for (int column=0; column<8; column++) {
      if (column == 2  &&  row < 2) {Wc(' ');}
      Ws("r"); Wd(row+column*4,1); Wc(' ');
      Wx(registers[row+column*4],2); Ws("   ");
    }
    Wl();
  }

  u8 io[] = {0};  // SPL, SPH, SREG
  DwReadAddr(0x5D, io, sizeof(io));


  Ws("SREG   ");

  for (int i=0; i<8; i++) {
    Wc((io[3] & (1<<(7-i))) ? "ITHSVNZC"[i] : "ithsvnzc"[i]); Wc(' ');
  }

  // if (io[3] & 0x80) {Ws("I ");} else {Ws("i ");}
  // if (io[3] & 0x40) {Ws("T ");} else {Ws("t ");}
  // if (io[3] & 0x20) {Ws("H ");} else {Ws("h ");}
  // if (io[3] & 0x10) {Ws("S ");} else {Ws("s ");}
  // if (io[3] & 0x08) {Ws("V ");} else {Ws("v ");}
  // if (io[3] & 0x04) {Ws("N ");} else {Ws("n ");}
  // if (io[3] & 0x02) {Ws("Z ");} else {Ws("z ");}
  // if (io[3] & 0x01) {Ws("C ");} else {Ws("c ");}
  Ws("  ");
  Ws("P ");  Wx(PC,4);                                 Ws("   ");
  Ws("S ");  Wx(io[1],2); Wx(io[0],2);                 Ws("   ");
  Ws("X ");  Wx(registers[27],2); Wx(registers[26],2); Ws("   ");
  Ws("Y ");  Wx(registers[29],2); Wx(registers[28],2); Ws("   ");
  Ws("Z ");  Wx(registers[31],2); Wx(registers[30],2); Ws("   ");
  Wl();


  //Ws("DDRB:    "); WriteDDRB();
  //Ws("PORTB:   "); WritePORTB();
}

struct {char *name; void (*handler)();} commands[] = {
  {"g", Go},
  {"p", SetPC},
  {"q", Quit},
  {"r", Registers},
  {"",  None}
};


int Compare(const char *a, const char *b) {
  while (*a  &&  *b  &&  *a == *b) {a++; b++;}
  return *a - *b;
}


void HandleCommand(const char *cmd) {
  for (int i=0; i<countof(commands); i++) {
    if (Compare(cmd, commands[i].name) == 0) {commands[i].handler(); return;}
  }
  Ws("Unrecognised command: '"); Ws(cmd); Wsl("'.");
}

void UI() {
  char command[10];

  if (setjmp(FailPoint)) {  // Failures will restart the UI loop rather than exiting DwDebug.
    Sl();
  }
  while (1) {

    if (QuitRequested) return;

    if (IsUser(Input)) {Wx(PC,4); Ws("> "); Flush();}

    Sb();
    if (Eof()) {if (IsUser(Input)) {Wl();} QuitRequested = 1;}
    else       {Ra(AdrAndLim(command)); HandleCommand(command);}

    Sl();
  }
}