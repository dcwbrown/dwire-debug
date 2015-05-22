/// RegistersCommand.c




void DisplayAllRegisters() {

  DwReadRegisters(Registers, 0, 30);  // r30 and r31 were already loaded at connection time

  for (int row=0; row<4; row++) {
    for (int column=0; column<8; column++) {
      if (column == 2  &&  row < 2) {Wc(' ');}
      Ws("r"); Wd(row+column*4,1); Wc(' ');
      Wx(Registers[row+column*4],2); Ws("   ");
    }
    Wl();
  }

  u8 io[3] = {0};  // SPL, SPH, SREG
  DwReadAddr(0x5D, sizeof(io), io);

  Ws("SREG ");

  for (int i=0; i<8; i++) {
    Wc((io[2] & (1<<(7-i))) ? "ITHSVNZC"[i] : "ithsvnzc"[i]); Wc(' ');
  }

  Ws("   ");
  Ws("PC ");  Wx(PC,4);                                 Ws("  ");
  Ws("SP ");  Wx(io[1],2); Wx(io[0],2);                 Ws("   ");
  Ws("X ");   Wx(Registers[27],2); Wx(Registers[26],2); Ws("   ");
  Ws("Y ");   Wx(Registers[29],2); Wx(Registers[28],2); Ws("   ");
  Ws("Z ");   Wx(Registers[31],2); Wx(Registers[30],2); Ws("   ");
  Wl();
}




void RegistersCommand() {
  Sb();
  if (IsNumeric(NextCh())) {
    int reg = ReadNumber(); Sb();
    Assert(reg >=0  &&  reg <= 31);
    if (IsDwDebugNumeric(NextCh())) {
      u8 newvalue = ReadNumber();
      Assert(newvalue >= 0  &&  newvalue <= 0xff);
      if (reg<30) {DwWriteRegisters(&newvalue, reg, 1);} else {Registers[reg] = newvalue;}
    } else {
      u8 value = 0;
      if (reg<30) {DwReadRegisters(&value, reg, 1);} else {value = Registers[reg];}
      Wc('R'); Wd(reg,1); Ws(" = "); Wx(value,2); Wl();
    }
  } else {
    DisplayAllRegisters();
  }
}
