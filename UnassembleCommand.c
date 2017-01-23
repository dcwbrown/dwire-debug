/// UnassembleCommand.c




void UnassembleCommand() {
  int count = 8; // Default instruction count to display
  Sb(); if (IsDwDebugNumeric(NextCh())) {Uaddr = ReadNumber(1); Wl();}
  Sb(); if (IsDwDebugNumeric(NextCh())) {count = ReadNumber(0);}
  Sb(); if (!DwEoln()) {Wsl("Unrecognised parameters on unassemble command.");}

  int firstByte = Uaddr*2;
  int limitByte = min(firstByte + count*4, FlashSize()); // Allow for up to 2 words per instruction
  int length    = limitByte - firstByte;

  if (length <= 0) {Fail("Nothing to disassemble.");}

  u8 buf[length+2];
  DwReadFlash(firstByte, length, buf);
  buf[length] = 0; buf[length+1] = 0;

  while (1) {
    Uaddr += DisassembleInstruction(Uaddr, &buf[Uaddr*2-firstByte]);
    count--;
    if (count <= 0  ||  Uaddr >= FlashSize()/2) {return;}
    Wl();
  }
}
