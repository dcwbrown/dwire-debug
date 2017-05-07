/// RegistersCommand.c




void DisplayAllRegisters() {

  // Read first 28 registers (r28 - r31 are already cached)
  DwGetRegs(0, R, 28);

  for (int row=0; row<4; row++) {
    for (int column=0; column<8; column++) {
      if (column == 2  &&  row < 2) {Wc(' ');}
      Ws("r"); Wd(row+column*4,1); Wc(' ');
      Wx(R[row+column*4],2); Ws("   ");
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
  Ws("PC ");  Wx(PC,4);                 Ws("  ");
  Ws("SP ");  Wx(io[1],2); Wx(io[0],2); Ws("   ");
  Ws("X ");   Wx(R[27],2); Wx(R[26],2); Ws("   ");
  Ws("Y ");   Wx(R[29],2); Wx(R[28],2); Ws("   ");
  Ws("Z ");   Wx(R[31],2); Wx(R[30],2); Ws("   ");
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
      if (reg <  28) {DwSetRegs(reg, &newvalue, 1);}
      else           {R[reg] = newvalue;}

    } else {

      // Display register value
      u8 value = 0;
      if (reg <  28) {DwGetRegs(reg, &value, 1);}
      else           {value = R[reg];}
      Wc('R'); Wd(reg,1); Ws(" = "); Wx(value,2); Wl();

    }

  } else {
    if (!DwEoln()) {Wsl("Unrecognised parameters on registers command.");}
    else           {DisplayAllRegisters();}
  }
}
