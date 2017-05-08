// NonVolatile.c


enum {SPMEN=1, PGERS=3, PGWRT=5, RWWSRE=0x11, BLBSET = 0x09, SIGRD = 0x21};


u8 ReadSPMCSR() {
  u8 spmcsr;
  DwSend(Bytes(0x64));        // Set up for single step mode
  DwIn(30, SPMCSR());         // in r30,SPMCSR
  DwGetRegs(30, &spmcsr, 1);  // spmcsr := r30
  //Ws(" SPMCSR $"); Wx(spmcsr,2); Wsl(".");
  return spmcsr;
}


void RenableRWW() {
  if (BootSect()) {
    DwSetPC(BootSect());  // Set PC that allows access to all of flash
    DwSetReg(29, RWWSRE); // r29 := RWWSRE
    DwOut(SPMCSR(), 29);  // out SPMCSR,r29
    DwInst(0x95E8);       // spm
  }
}


//void ReadConfigBits(u8 pmc, u8 index, u8 *dest) {
//  u16 outr0 = 0xB800 | ((DWDRreg() & 0x30) << 5) | (DWDRreg() & 0xF);
//  DwSend(Bytes(
//    0x66,                                  // Register or memory access mode
//    0xD0, 0x10, 29,                        // Set up to set registers starting from r29
//    0xD1, 0x10, 32,                        // Register limit is 32 (i.e. stop at r31)
//    0xC2, 0x05, 0x20,                      // Select reigster write mode and start
//    pmc, index, 0,                         // Set r29 to BLBSET LPM flags, and Z to desired fuse index
//    0xD0, hi(BootSect()), lo(BootSect()),  // Set PC inside the boot section to enable spm
//    0x64,                                  // Single step mode
//    0xD2, 0xBF, 0xD7, 0x23,                // OUT SPMCSR,r29
//    0xD2, 0x95, 0xC8, 0x23,                // LPM: Reads fuse or lock bits to R0
//    0xD2, hi(outr0), lo(outr0), 0x23       // OUT DRDW,r0
//  ));
//  DwReceive(dest, 1);
//}

void ReadConfigBits(u8 index, u8 *dest) {
  DwGetRegs(0, R, 1);                     // Cache R0
  DwSetRegs(29, Bytes(BLBSET, index, 0)); // r29 := BLBSET flags, Z := index
  DwSend(Bytes(0x64));                    // Set up for single step mode
  DwOut(SPMCSR(), 29);                    // OUT SPMCSR,r29
  DwInst(0x95C8);                         // LPM: Reads fuse or lock bits to R0
  DwGetRegs(0, dest, 1);                  // *dest := r0
  DwSetRegs(0, R, 1);                     // Restore R0.
}


void DumpConfig() {
  u8 cb;
  u8 hfuse;
  u8 efuse;

  // Show Known device info
  Ws(Name()); Wsl(".");
  Ws("IO regs:   ");     Wx(32,2); Ws(".."); Wx(IoregSize()+31,2); Ws("   ("); Wd(IoregSize(),1); Wsl(" bytes).");
  Ws("SRAM:     ");      Wx(IoregSize()+32,3); Ws(".."); Wx(31 + IoregSize() + SramSize(), 3); Ws("  ("); Wd(SramSize(),1); Wsl(" bytes).");
  Ws("Flash:   0000.."); Wx(FlashSize()-1, 4); Ws(" ("); Wd(FlashSize(),1); Wsl(" bytes).");
  Ws("EEPROM:   000.."); Wx(EepromSize()-1, 3); Ws("  ("); Wd(EepromSize(),1); Wsl(" bytes).");

  // Dump uninterpreted fuse and lock bit values
  ReadConfigBits(0, &cb);    Ws("Fuses: low "); Wx(cb,2);     Flush();
  ReadConfigBits(3, &hfuse); Ws(", high ");           Wx(hfuse,2);  Flush();
  ReadConfigBits(2, &efuse); Ws(", extended ");       Wx(efuse,2);  Flush();
  ReadConfigBits(1, &cb);    Ws(", lock bits ");      Wx(cb,2);     Wsl(".");

  // Interpret boot sector information
  if (BootFlags()) {
    int bootsize = 0;
    int bootvec  = 0;
    switch (BootFlags()) {
      case 1:  bootsize = 128 << (3 - ((efuse/2) & 3));  bootvec = efuse & 1; break; // atmega88 & 168
      case 2:  bootsize = 256 << (3 - ((hfuse/2) & 3));  bootvec = hfuse & 1; break; // atmega328
      default: Fail("Invalid BootFlags global data setting.");
    }

    bootsize *= 2;  // Bootsize is in words but we report in bytes.

    Ws("  Boot space: ");
    Wx(FlashSize()-bootsize, 4); Ws(".."); Wx(FlashSize()-1, 4);
    Ws(" ("); Wd(bootsize,1); Wsl(" bytes).");
    Ws("  Vectors:   ");
    if (bootvec) Wsl("0."); else {Wx(FlashSize()-bootsize, 4); Wsl(".");}
  }
}


// Smallest boot size is 128 words on atmega 88 and 168.
// Smallest boot size on atmega328 is 256 words.

void DwReadFlash(int addr, int len, u8 *buf) {
  int limit = addr + len;
  if (limit > FlashSize()) {Fail("Attempt to read beyond end of flash.");}
  while (addr < limit) {
    int length = min(limit-addr, 64);      // Read no more than 64 bytes at a time so PC remains in valid address space.
    DwSetZ(addr);                          // Z := First address to read
    DwSetPC(BootSect());                   // Set PC that allows access to all of flash
    DwSetBP(BootSect()+2*length);          // Set BP to load length bytes
    DwSend(Bytes(0x66, 0xC2, 0x02, 0x20)); // Set flash read mode and start reading
    DwReceive(buf, length);
    addr += length;
    buf  += length;
  }
}


void EraseFlashPage(u16 a) { // a = byte address of first word of page
  Assert((a & (PageSize()-1)) == 0);
  DwSetRegs(29, Bytes(PGERS, lo(a), hi(a))); // r29 := op (erase page), Z = first byte address of page
  DwSetPC(BootSect());                       // Set PC that allows access to all of flash
  DwSend(Bytes(0x64));                       // Set up for single step mode
  DwOut(SPMCSR(), 29);                       // out SPMCSR,r29 (select page erase)
  DwSend(Bytes(0xD2, 0x95, 0xE8, 0x33));     // spm (do page erase)
  DwSync();
}



void LoadPageBuffer(u16 a, const u8 *buf) {
  DwSetRegs(29, Bytes(SPMEN, lo(a), hi(a))); // r29 := op (write next page buffer word), Z = first byte address of page
  DwSend(Bytes(0x64));                       // Set up for single step mode
  const u8 *limit = buf + PageSize();
  while (buf < limit) {
    DwSetRegs(0, buf, 2); buf += 2;  // r0 := low byte, r1 := high byte
    DwSetPC(BootSect());             // Set PC that allows access to all of flash
    DwOut(SPMCSR(), 29);             // out SPMCSR,r29 (write next page buffer word)
    DwInst(0x95E8);                  // spm
    DwInst(0x9632);                  // adiw Z,2
  }
}



void ProgramPage(u16 a) {
  DwSetRegs(29, Bytes(PGWRT, lo(a), hi(a))); // r29 = op (page write), Z = first byte address of page
  DwSetPC(BootSect());                       // Set PC that allows access to all of flash
  DwSend(Bytes(0x64));                       // Set up for single step mode
  DwOut(SPMCSR(), 29);                       // out SPMCSR,r29 (PGWRT)
  DwInst(0x95E8);                            // spm
  while ((ReadSPMCSR() & 0x1F) != 0) {Wc('.'); Flush();} // Wait while programming busy
}




void ShowPageStatus(u16 a, char *msg) {
  Ws("$"); Wx(a,4); Ws(" - $"); Wx(a+PageSize()-1,4);
  Wc(' '); Ws(msg); Ws(".                "); Wr();
}




void WriteFlashPage(u16 a, const u8 *buf) {
  // Uses r0, r1, r29, r30, r31

  u8 page[MaxFlashPageSize];
  Assert(PageSize() <= sizeof(page));

  RenableRWW();

  DwReadFlash(a, PageSize(), page);

  if (memcmp(buf, page, PageSize()) == 0) {
    ShowPageStatus(a, "unchanged");
    return;
  }

  int erase = 0;
  for (int i=0; i<PageSize(); i++) {
    if (~page[i] & buf[i]) {erase=1; break;}
  }

  if (erase) {
    ShowPageStatus(a, "erasing");
    EraseFlashPage(a);
  }

  memset(page, 0xff, PageSize());
  if (memcmp(buf, page, PageSize()) == 0) {
    return;
  }

  ShowPageStatus(a, "loading page buffer");
  LoadPageBuffer(a, buf);

  ShowPageStatus(a, "programming");
  ProgramPage(a);

  RenableRWW();
}




u8 pageBuffer[MaxFlashPageSize] = {0};

void WriteFlash(u16 addr, const u8 *buf, int length) {

  Assert(addr + length <= FlashSize());
  Assert(length >= 0);
  if (length == 0) return;

  DwGetRegs(0, R, 2); // Cache R0 and R1

  int pageOffsetMask = PageSize()-1;
  int pageBaseMask   = ~ pageOffsetMask;

  if (addr & pageOffsetMask) {

    // buf starts in the middle of a page

    int partBase   = addr & pageBaseMask;
    int partOffset = addr & pageOffsetMask;
    int partLength = min(PageSize()-partOffset, length);

    DwReadFlash(partBase, PageSize(), pageBuffer);
    memcpy(pageBuffer+partOffset, buf, partLength);
    WriteFlashPage(partBase, pageBuffer);

    addr   += partLength;
    buf    += partLength;
    length -= partLength;
  }

  Assert(length == 0  ||  ((addr & pageOffsetMask) == 0));

  // Write whole pages

  while (length >= PageSize()) {
    WriteFlashPage(addr, buf);
    addr   += PageSize();
    buf    += PageSize();
    length -= PageSize();
  }

  // Write any remaining partial page

  if (length) {
    Assert(length > 0);
    Assert(length < PageSize());
    Assert((addr & pageOffsetMask) == 0);
    DwReadFlash(addr, PageSize(), pageBuffer);
    memcpy(pageBuffer, buf, length);
    WriteFlashPage(addr, pageBuffer);
  }

  Ws("                                       \r");

  // Restore cached registers R0 and R1
  DwSetRegs(0, R, 2);
}



void DumpFlashBytesCommand() {
  int length = 128; // Default byte count to display
  ParseDumpParameters("dump flash bytes", FlashSize(), &FBaddr, &length);

  u8 buf[length];
  DwReadFlash(FBaddr, length, buf);

  DumpBytes(FBaddr, length, buf);
  FBaddr += length;
}




void DumpFlashWordsCommand() {
  int length = 128; // Default byte count to display
  ParseDumpParameters("dump flash words", FlashSize(), &FWaddr, &length);

  u8 buf[length];
  DwReadFlash(FWaddr, length, buf);

  DumpWords(FWaddr, length, buf);
  FWaddr += length;
}




void WriteFlashBytesCommand() {
  u8 buf[16];
  int addr;
  int len;
  ParseWriteParameters("Flash write", &addr, FlashSize(), buf, &len);
  if (len) {WriteFlash(addr, buf, len);}
}






void DwReadEEPROM(int addr, int len, u8 *buf)
{
  u8 quint[5];
  int limit = addr + len;
  if (limit > EepromSize()) {Fail("Attempt to read beyond end of EEPROM.");}

  DwGetRegs(0, R, 5); // Cache r0..r4

  // Preload registers: r29 = EECR read flag, r31:r30 = EEPROM address
  DwSetRegs(29, Bytes(1, lo(addr), hi(addr))); // r29 := 1, r31:r30 := address

  // Read all requested bytes
  while (addr < limit) {
    DwSend(Bytes(0x64));               // Set up for single step mode
    for (int i=0; i<5; i++) {
      DwOut(EEARL(), 30);              // out  EEARL,r30
      if (EEARH()) DwOut(EEARH(), 31); // out  EEARH,r31
      DwOut(EECR(), 29);               // out  EECR,r29
      DwInst(0x9631);                  // adiw Z,1
      DwIn(i, EEDR());                 // in   ri,EEDR
    }
    DwGetRegs(0, quint, 5);

    int i = 0;
    while (i < 5  &&  addr < limit) {
      *buf = quint[i];
      buf++;
      addr++;
      i++;
    }
  }

  DwSetRegs(0, R, 5); // Restore r0..r4
}


void DwWriteEEPROM(int addr, int len, u8 *buf)
{
  int limit = addr + len;
  if (limit > EepromSize()) {Fail("Attempt to write beyond end of EEPROM.");}

  DwGetRegs(0, R, 1); // Cache r0

  // Preload registers:
  //   r28     - EEMWE flag (4)
  //   r29     - EEWE flag (2)
  //   r31:r30 - Initial EEPROM address

  DwSetRegs(28, Bytes(4, 2, lo(addr), hi(addr))); // r28 := 4, r29 := 2, r31:r30 := address
  DwSend(Bytes(0x64));                            // Set up for single step mode

  // Write all requested bytes
  while (addr < limit) {
    DwOut(EEARL(), 30);              // out  EEARL,r30
    if (EEARH()) DwOut(EEARH(), 31); // out  EEARH,r31
    DwSetRegs(0, buf, 1);            // r0 := next byte
    DwOut(EEDR(), 0);                // out  EEDR,r0
    DwInst(0x9631);                  // adiw Z,1
    DwOut(EECR(), 28);               // out  EECR,r28
    DwOut(EECR(), 29);               // out  EECR,r29

    DwFlush();
    buf++;
    addr++;
    delay(5); // Allow eeprom write to complete.
  }

  DwSetRegs(0, R, 1); // Restore r0
}




void DumpEEPROMBytesCommand() {
  int length = 32; // Default byte count to display
  ParseDumpParameters("dump EEPROM bytes", EepromSize(), &EBaddr, &length);

  u8 buf[length];
  DwReadEEPROM(EBaddr, length, buf);

  DumpBytes(EBaddr, length, buf);
  EBaddr += length;
}




void DumpEEPROMWordsCommand() {
  int length = 32; // Default byte count to display
  ParseDumpParameters("dump EEPROM words", EepromSize(), &EWaddr, &length);

  u8 buf[length];
  DwReadEEPROM(EWaddr, length, buf);

  DumpWords(EWaddr, length, buf);
  EWaddr += length;
}




void WriteEEPROMBytesCommand() {
  u8 buf[16];
  int addr;
  int len;
  ParseWriteParameters("EEPROM write", &addr, EepromSize(), buf, &len);
  if (len) {DwWriteEEPROM(addr, len, buf);}
}
