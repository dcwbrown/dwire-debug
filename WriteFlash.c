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



// The largest page size is 128 bytes / 64 words

u8 FlashWriteBuffer[14 + 25*(MaxFlashPageSize/2) + 25] = {0};

void AddBytes(u8 **p, const u8 *bytes, int length) {
	Assert(*p >= FlashWriteBuffer);
	Assert(*p+length <= FlashWriteBuffer + sizeof(FlashWriteBuffer));
	memcpy(*p, bytes, length);
	*p += length;
}

void ProgramFlashPage(u16 a, const u8 *buf) {
  const u8 *p = buf;
  u8*       q = FlashWriteBuffer;

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

  DwWrite(FlashWriteBuffer, q-FlashWriteBuffer);
  DwSync();
}




void WriteFlashPage(u16 a, const u8 *buf) {
	// Uses r0, r1, r29, r30, r31

  u8 page[MaxFlashPageSize];
  Assert(PageSize() <= sizeof(page));

  DwReadFlash(a, PageSize(), page);

  if (memcmp(buf, page, PageSize()) == 0) {
  	Ws("Skipping write of unchanged page at $"); Wx(a,4); Wl();
  	return;
  }

  int erase = 0;
  for (int i=0; i<PageSize(); i++) {
  	if (~page[i] & buf[i]) {erase=1; break;}
  }

  if (!erase) {
  	Ws("Skipping erase of page with sufficient 1's at $"); Wx(a,4); Wl();
  } else {
    Ws("Erasing page at $"); Wx(a,4); Wl();
  	EraseFlashPage(a);
  }

  memset(page, 0xff, PageSize());
  if (memcmp(buf, page, PageSize()) == 0) {
    Ws("Skipping write of fully $ff page at $"); Wx(a,4); Wl();
    return;
  }

  Ws("Programming page at $"); Wx(a,4); Wl();
  ProgramFlashPage(a, buf);
}



u8 testbuf[64];

void WriteFlashCommand() { // For testing
  int key = 0;
  Sb(); if (IsDwDebugNumeric(NextCh())) {key = ReadNumber(1);}

	//u8 r29;
	//DwReadRegisters(&r29, 29, 1);

  for (int i=0; i<64; i++) {testbuf[i] = i+key;}

  WriteFlashPage(PageSize(), testbuf);

  DwReset();
  //DwWriteRegisters(&r29, 29, 1);
}



#if 0

void WriteFlashBufferWord(u16 w) {
  // Assumes r27=1
  u8 cmd[] = {
    0xD0, 0x1F, 0x00,              // set PC to bootsection for spm to work
    0xD2, 0xB4, 0x02, 0x23, lo(w), // in r0,DWDR (low byte)
    0xD2, 0xB4, 0x12, 0x23, hi(w), // in r1,DWDR (high byte)
    0xD2, 0xBF, 0xB7, 0x23,        // out SPMCSR,r27 = 01 = SPMEN
    0xD2, 0x95, 0xE8, 0x23,        // spm
    0xD2, 0x96, 0x32, 0x23         // adiw Z,2
  };
  DwBytes(cmd, sizeof cmd);
}
// 24 command bytes per byte of page = 768 bytes, need a large serial input buffer.
// At 125kbaud this would take < 1/10 second. 64 pages would take 6 seconds.
// Instead, how about a tiny loop in high memory
//
//
//            E0 D1    ldi   r29,1
//                     eor   r30,r30
//                     eor   r31,r31
//
// loop       B6 01    in    r0,DWDR
//            B6 11    in    r1,DWDR
//            BF D7    out   SPMCSR,r29
//            95 EB    spm
//            96 32    adiw  Z,2
//                     sbrs  r30,6       Skip jump back to start of loop when 32 words completed.
//                     rjmp  loop
//
//                     break
//
//........



// AtTiny45 Flash details
//
// 4K bytes / 2K words
//
// 32 words per page x 64 pages
//
// PC format (word address):
//
//     5/0, 6/page, 5/word
//
// Programming Z register format (byte address):
//
//    4/0, 6/PCPAGE, 5/PCWORD, 1/0

void WriteFlashPage(u16 page) {  // Parameter is address of start of page to write
  // Erase
  u16 firstPageByte = page & 0x0FC0;
  u8  ZH = hi(firstPageByte);
  u8  ZL = lo(firstPageByte);
  u8  cmd1[] = {
    0x66,                                     // ? context
    0xD0, 0, 26, 0xD1, 0, 32,                 // Set PC=0x1A, BP=0x20 - address registers r26 through r31
    0xC2, 5,     0x20, 3, 1, 5, 0x40, ZL,ZH,  // r26=3, r27=1, r28=5, r29=$40, Z=first byte address of page
    0xD0, 0x1F, 0,                            // Set PC=$1F00, inside the boot section to enable spm
    0x64,                                     // ? Context
    0xD2, 0x01, 0xCF, 0x23,                   // movw r24,r30
    0xD2, 0xBF, 0xA7, 0x23,                   // out SPMCSR,r26 = 03 = PGERS
    0xD2, 0x95, 0xE8, 0x33                    // spm
  };
  wsl("Erasing flash page.");
  DwBytes(cmd1, sizeof(cmd1));

  Expect(0); Expect(0x55); // DwByte(0x83);  Expect(0x55);

  wsl("Erase complete. Reconnected.");


  // Fill page buffer
  u8 cmd44 = 0x44;
  DwBytes(&cmd44, 1);
  u16 f = firstPageByte;
  for (int i=0; i<32; i++) {
    u16 w = FlashBytes[f] | (FlashBytes[f+1] << 8);
    f += 2;
    WriteFlashBufferWord(w);
  }
  wsl("Temporary page buffer write complete.");


  // Write page
  u8 cmd3[] = {
    0xD0, 0x1F, 0x00,          // Set PC to bootsection to enable SPM
    0xD2, 0x01, 0xFC, 0x23,    // movw r30,r24
    0xD2, 0xBF, 0xC7, 0x23,    // out SPMCSR,r28 = 05 = PGWRT
    0xD2, 0x95, 0xE8, 0x33     // spm
  };
  DwBytes(cmd3, sizeof cmd3);

  Expect(0); Expect(0x55);

  // Reset PC.
  u8 cmd4[] = {0x66, 0xD0, 0,0 }; // Set PC=0
  DwBytes(cmd4, sizeof(cmd4));
}



// Resident flash loader - used for performance.
//
// pre-load r29:r28 (Y) with page address
// pre-load SRAM 100..13F with page to burn



void WriteFlashLoader() {
  // The flash loader fits within 1 page of AtTiny45 flash.
  u8 loader[] = {
    /* 0fc0 */  0xC2, 0xB5,  //  fld:      in    r28,DWDR     ; low byte of flash page address
    /* 0fc2 */  0xD2, 0xB5,  //            in    r29,DWDR     ; high byte of flash page address
    /*      */               //
    /*      */               //  ;         Erase flash page
    /*      */               //
    /* 0fc4 */  0xFE, 0x01,  //            movw  r30,r28
    /* 0fc6 */  0xB3, 0xE0,  //            ldi   r27,3        ; Page erase request
    /* 0fc8 */  0xB7, 0xBF,  //            out   SPMCSR,r27
    /* 0fca */  0xE8, 0x95,  //            spm
    /*      */               //
    /*      */               //  ;         Fill temporary page buffer
    /*      */               //
    /* 0fcc */  0xEE, 0x27,  //            eor   r30,r30
    /* 0fce */  0xFF, 0x27,  //            eor   r31,r31
    /* 0fd0 */  0xB1, 0xE0,  //            ldi   r27,1
    /*      */               //
    /* 0fd2 */  0x02, 0xB4,  //  fld2:     in    r0,DWDR      ; low byte of next flash word
    /* 0fd4 */  0x12, 0xB4,  //            in    r1,DWDR      ; hight byte of next flash word
    /* 0fd6 */  0xB7, 0xBF,  //            out   SPMCSR,r27   ; r27=1: store program memory
    /* 0fd8 */  0xE8, 0x95,  //            spm
    /* 0fda */  0x32, 0x96,  //            adiw  r30,2
    /* 0fdc */  0xE6, 0xFF,  //            sbrs  r30,6        ; Skip jump back to start of loop when 32 words completed.
    /* 0fde */  0xF9, 0xCF,  //            rjmp  fld2
    /*      */               //
    /*      */               //  ;         Write temporary page to flash
    /*      */               //
    /* 0fe0 */  0xFE, 0x01,  //            movw  r30,r28
    /* 0fe2 */  0xB5, 0xE0,  //            ldi   r27,5        ; 5: page write
    /* 0fe4 */  0xB7, 0xBF,  //            out   SPMCSR,r27
    /* 0fe6 */  0xE8, 0x95,  //            spm
    /*      */               //
    /*      */               //  ;         Return to DebugWire client
    /*      */               //
    /* 0fe8 */  0x98, 0x95,  //            break


    // 0xc2, 0xb5, 0xd2, 0xb5, 0xfe, 0x01, 0xb3, 0xe0, 0xb7, 0xbf, 0xe8, 0x95, 0xee, 0x27, 0xff, 0x27,
    // 0xb1, 0xe0, 0x02, 0xb4, 0x12, 0xb4, 0xb7, 0xbf, 0xe8, 0x95, 0x32, 0x96, 0xe6, 0xff, 0xf9, 0xcf,
    // 0xfe, 0x01, 0xb5, 0xe0, 0xb7, 0xbf, 0xe8, 0x95, 0x98, 0x95,
                                                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  wsl("WriteFlashLoader starting.");

  // First see if the loader is already present.
  LoadFlashBytes(0xfc0, 0x1000);
  int present = 1;
  for (int i=0; i<64; i++) {
    if (FlashBytes[0xfc0+i] != loader[i]) {
      present = 0;
      FlashBytes[0xfc0+i] = loader[i];
    }
  }
  if (present) {
    wsl("Skipping programming resident flash loader as it is already programmed.");
  }else {
    WriteFlashPage(0xfc0);
    // Reset PC. Somehow this restores normal operation following the
    // flash programming. Without it we lose access to the processor.
    u8 cmd1[] = {0x66, 0xD0, 0,0 }; // Set PC=0
    DwBytes(cmd1, sizeof(cmd1));
  }
}



void WriteFlashPageWithLoader(u16 page) {  // Parameter is address of start of page to write
  u16 firstPageByte = page & 0x0FC0;
  u16 startpc = 0xfc0/2;
  u8 cmd[] = {
    0xD0, hi(startpc), lo(startpc), 0x64,
    0xD1, 0x01, 0xFF,                    // Set BP at end of flash
    0xD0, hi(startpc), lo(startpc), 0x30 // Set PC to start of boot flash loader code and go
  };
  ws("Writing flash page $"); wx(page,3); wsl(" with resident loader.");

  DebugDwBytes(cmd, sizeof cmd);
  wsl("PC set to start of loader and code started.");



  // Expect(0); Expect(0x55);

  // Debugging - get PC and display it.
  ws("Debugging 2. PC="); wx(GetPC(),4); wl();


  DebugDwBytes((u8*)&firstPageByte, 2);

  // Resident code erases the page now

  ws("First page byte address $"); wx(firstPageByte,3); wsl(" sent.");

  Expect(0x55);


  // Debugging - get PC and display it.
  ws("Debugging 2. PC="); wx(GetPC(),4); wl();


   wsl("Sending page content.");
   DebugDwBytes(&FlashBytes[firstPageByte], 64);
   wsl("Page content send completed.");


   ws("Debugging - response: ");
   while (1) {
     wx(ReadPort(),2); wl();
   }

   Expect(0); Expect(0x55);
   DwByte(0x83);  Expect(0x55);
   // Reset PC. Somehow this restores normal operation following the
   // flash programming. Without it we lose access to the processor.
   u8 cmd1[] = {0x66, 0xD0, 0,0 }; // Set PC=0
   DebugDwBytes(cmd1, sizeof(cmd1));
}



void WriteFlash(u8 *writeBytes, u16 length) {

  if (length >= 0xfc0) {wsl("Binary too large to write to flash, not programmed."); return;}


  // Check whether the binary already matches flash content

  LoadFlashBytes(0, (length+0x3f) & 0xfc0); // Round up to whole page boundary
  int matches = 1;
  for (int i=0; i<length; i++) {
    if (FlashBytes[i] != writeBytes[i]) {
      matches = 0;
      break;
    }
  }

  if (matches) {wsl("Skipping programming as flash already contains loaded binary."); return;}

//  WriteFlashLoader();

  int offset = 0;
  while (offset < length) {
    int limit = min(length, offset + 64);

    matches = 1;
    for (int i=offset; i<limit; i++) {
      if (FlashBytes[i] != writeBytes[i]) {
        FlashBytes[i] = writeBytes[i];
        matches = 0;
      }
    }

    if (matches) {
      ws("Skipping flash page at $"); wx(offset,3); wl();
    } else {
      ws("Programming flash page at $"); wx(offset,3); wl();
      //WriteFlashPageWithLoader(offset);
      WriteFlashPage(offset);
    }


    // return; // Debugging.

    offset += 64;
  }
}
#endif