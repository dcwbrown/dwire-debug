/// RegistersCommand.c




void DisplayAllRegisters() {

  u8 buf[32];
  DwReadRegisters(buf, 0, 30);
  buf[30] = R30;
  buf[31] = R31;

  for (int row=0; row<4; row++) {
    for (int column=0; column<8; column++) {
      if (column == 2  &&  row < 2) {Wc(' ');}
      Ws("r"); Wd(row+column*4,1); Wc(' ');
      Wx(buf[row+column*4],2); Ws("   ");
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
  Ws("PC ");  Wx(PC,4);                     Ws("  ");
  Ws("SP ");  Wx(io[1],2); Wx(io[0],2);     Ws("   ");
  Ws("X ");   Wx(buf[27],2); Wx(buf[26],2); Ws("   ");
  Ws("Y ");   Wx(buf[29],2); Wx(buf[28],2); Ws("   ");
  Ws("Z ");   Wx(buf[31],2); Wx(buf[30],2); Ws("   ");
  Wl();
}




void RegistersCommand() {
  Sb();
  if (IsDwDebugNumeric(NextCh())) {

    // Command addresses an individual register
    int reg = ReadNumber(0); Sb();
    Assert(reg >=0  &&  reg <= 31);

    if (IsDwDebugNumeric(NextCh())) {

      // Change register value
      u8 newvalue = ReadNumber(1);
      Assert(newvalue >= 0  &&  newvalue <= 0xff);
      if      (reg <  30) {DwWriteRegisters(&newvalue, reg, 1);}
      else if (reg == 30) {R30 = newvalue;}
      else                {R31 = newvalue;}

    } else {

      // Display register value
      u8 value = 0;
      if      (reg <  30) {DwReadRegisters(&value, reg, 1);}
      else if (reg == 30) {value = R30;}
      else                {value = R31;}
      Wc('R'); Wd(reg,1); Ws(" = "); Wx(value,2); Wl();

    }

  } else {
    if (!DwEoln()) {Wsl("Unrecognised parameters on registers command.");}
    else           {DisplayAllRegisters();}
  }
}
