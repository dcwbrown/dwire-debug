// WriteFlash.c


enum {SPMEN=1, PGERS=3, PGWRT=5};


void EraseFlashPage(u16 a) { // a = byte address of first word of page
  // Uses r29, r30, r31
  Assert((a & (PageSize()-1)) == 0);
  DwWrite(ByteArrayLiteral(
    0x66, 0xD0, 0, 29, 0xD1, 0, 32,      // Set PC=29, BP=32 - address registers r29 through r31
    0xC2, 5, 0x20, PGERS, lo(a), hi(a),  // r29=op, Z=first byte address of page
    0xD0, 0x1F, 0,                       // Set PC=$1F00, inside the boot section to enable spm
    0x64, 0xD2, 0xBF, 0xD7, 0x23,        // out SPMCSR,r29 (3=PGERS, 5=PGWRT)
    0xD2, 0x95, 0xE8, 0x33               // spm
  ));
  DwSync();
}




u8 FlashWriteCommandBuffer[14 + 25*(MaxFlashPageSize/2) + 25] = {0};

void AddBytes(u8 **p, const u8 *bytes, int length) {
  Assert(*p >= FlashWriteCommandBuffer);
  Assert(*p+length <= FlashWriteCommandBuffer + sizeof(FlashWriteCommandBuffer));
  memcpy(*p, bytes, length);
  *p += length;
}

void ProgramFlashPage(u16 a, const u8 *buf) {
  const u8 *p = buf;
  u8*       q = FlashWriteCommandBuffer;

  AddBytes(&q, ByteArrayLiteral(
    0x66, 0xD0, 0, 29, 0xD1, 0, 32,      // Set PC=29, BP=32 - address registers r29 through r31
    0xC2, 5, 0x20, SPMEN, lo(a), hi(a),  // r29 = op (write next page buffer word), Z = first byte address of page
    0x64));

  for (int i=0; i<PageSize()/2; i++) {
    AddBytes(&q, ByteArrayLiteral(
      0xD0, 0x1F, 0x00,                  // Set PC to bootsection for spm to work
      0xD2, 0xB4, 0x02, 0x23, *(p++),    // in r0,DWDR (low byte)
      0xD2, 0xB4, 0x12, 0x23, *(p++),    // in r1,DWDR (high byte)
      0xD2, 0xBF, 0xD7, 0x23,            // out SPMCSR,r29 (write next page buffer word)
      0xD2, 0x95, 0xE8, 0x23,            // spm
      0xD2, 0x96, 0x32, 0x23));          // adiw Z,2
  }

  AddBytes(&q, ByteArrayLiteral(
    0x66, 0xD0, 0x1F, 0x00,              // Set PC to bootsection for spm to work
    0xD0, 0, 29, 0xD1, 0, 32,            // Set PC=29, BP=32 - address registers r29 through r31
    0xC2, 5, 0x20, PGWRT, lo(a), hi(a),  // r29 = op (page write), Z = first byte address of page
    0x64, 0xD2, 0xBF, 0xD7, 0x23,        // out SPMCSR,r29 (3=PGERS, 5=PGWRT)
    0xD2, 0x95, 0xE8, 0x33));            // spm

  DwWrite(FlashWriteCommandBuffer, q-FlashWriteCommandBuffer);
  DwSync();
}




void WriteFlashPage(u16 a, const u8 *buf) {
  // Uses r0, r1, r29, r30, r31

  u8 page[MaxFlashPageSize];
  Assert(PageSize() <= sizeof(page));

  DwReadFlash(a, PageSize(), page);

  if (memcmp(buf, page, PageSize()) == 0) {
    Ws("Unchanged   $"); Wx(a,4); Ws(" - $"); Wx(a+PageSize()-1,4); Wl(); // Wc('\r'); Flush();
    return;
  }

  int erase = 0;
  for (int i=0; i<PageSize(); i++) {
    if (~page[i] & buf[i]) {erase=1; break;}
  }

  if (erase) {
    Ws("Erasing     $"); Wx(a,4); Ws(" - $"); Wx(a+PageSize()-1,4); Wl(); // Wc('\r'); Flush();
    EraseFlashPage(a);
  }

  memset(page, 0xff, PageSize());
  if (memcmp(buf, page, PageSize()) == 0) {
    //Ws("Skipping write of fully $ff page at $"); Wx(a,4); Wl();
    return;
  }

  Ws("Programming $"); Wx(a,4); Ws(" - $"); Wx(a+PageSize()-1,4); Wl(); // Wc('\r'); Flush();
  //Ws("Programming page at $"); Wx(a,4); Wl();
  ProgramFlashPage(a, buf);
}




u8 pageBuffer[MaxFlashPageSize] = {0};

void WriteFlash(u16 addr, const u8 *buf, int length) {

  Assert(addr + length <= FlashSize());

  u8 r0and1[2];
  u8 r29;

  DwReadRegisters(r0and1, 0, 2);
  DwReadRegisters(&r29,  29, 1);

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

  //Ws("                           \r");

  DwWriteRegisters(r0and1, 0, 2);
  DwWriteRegisters(&r29,  29, 1);
}
