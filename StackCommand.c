/// StackCommand.c




void StackCommand () {
  u8 sp[2];
  Sb();
  if (IsDwDebugNumeric(NextCh())) {
    // Update stack pointer
    int newSp = ReadNumber(1);
    sp[0] = newSp & 0xff;
    sp[1] = (newSp >> 8) & 0xff;
    DwWriteAddr(0x5D, 2, sp);

  } else if (DwEoln()) {
    // Display stack content

    DwReadAddr(0x5D, 2, sp);

    int addr = min(DataLimit(), ((sp[1] << 8) | sp[0]) + 1);
    int len  = min(DataLimit(), addr+16) - addr;

    Ws("SP = "); Wx(addr-1,4);

    if (len) {
      u8 buf[16];
      DwReadAddr(addr, len, buf);
      Ws(".  "); Wx(addr,4); Ws(": ");
      for (int i=0; i<len; i++) {Wx(buf[i],2); Wc(' ');}
    }
    Wl();

  } else {Wsl("Unrecognised parameters on stack command.");}
}
