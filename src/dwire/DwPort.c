/// DebugPort.c

// Exposed APIs:
//
//   DwSend  - send bytes; (Bytes are buffered and will be sent on next
//                          DwFlush, DwReceive or DwCapture call.)
//   DwFlush - Flush buffer to device


char CurrentPortKind() {Assert(CurrentPort >= 0); return Ports[CurrentPort]->kind;}

void DwBreakAndSync(void) {
  if (CurrentPortKind() == 's')
    SerialBreakAndSync((struct SPort*)Ports[CurrentPort]);
  else
    DigisparkBreakAndSync((struct UPort*)Ports[CurrentPort]);
}

int  DwReachedBreakpoint(void) {
  Assert(CurrentPortKind() == 'u');
  return DigisparkReachedBreakpoint((struct UPort*)Ports[CurrentPort]);
}

void DwSend(const u8 *out, int outlen) {
  if (CurrentPortKind() == 's')
    SerialSend((struct SPort*)Ports[CurrentPort], out, outlen);
  else
    DigisparkSend((struct UPort*)Ports[CurrentPort], out, outlen);
}

void DwFlush(void) {
  if (CurrentPortKind() == 's')
    SerialFlush((struct SPort*)Ports[CurrentPort]);
  else
    DigisparkFlush((struct UPort*)Ports[CurrentPort]);
}

int  DwReceive(u8 *in, int inlen) {
  if (CurrentPortKind() == 's')
    return SerialReceive((struct SPort*)Ports[CurrentPort], in, inlen);
  else
    return DigisparkReceive((struct UPort*)Ports[CurrentPort], in, inlen);
}

void DwSync(void) {
  if (CurrentPortKind() == 's')
    SerialSync((struct SPort*)Ports[CurrentPort]);
  else
    DigisparkSync((struct UPort*)Ports[CurrentPort]);
}

void DwWait(void) {
  if (CurrentPortKind() == 's')
    SerialWait((struct SPort*)Ports[CurrentPort]);
  else
    DigisparkWait((struct UPort*)Ports[CurrentPort]);
}





int DwReadByte(void) {u8 byte = 0; DwReceive(&byte, 1); return byte;}
int DwReadWord(void) {u8 buf[2] = {0}; DwReceive(buf, 2); return (buf[0] << 8) | buf[1];}


u8 hi(int w) {return (w>>8)&0xff;}
u8 lo(int w) {return (w   )&0xff;}


void DwSetPC(u16 pc) {DwSend(Bytes(0xD0, hi(pc)|AddrFlag(), lo(pc)));}
void DwSetBP(u16 bp) {DwSend(Bytes(0xD1, hi(bp)|AddrFlag(), lo(bp)));}

void DwInst(u16 inst) {DwSend(Bytes(0x64, 0xD2, hi(inst), lo(inst), 0x23));}

void DwIn(u8 reg, u16 ioreg)  {DwInst(0xB000 | ((ioreg << 5) & 0x600) | ((reg << 4) & 0x01F0) | (ioreg & 0x000F));}
void DwOut(u16 ioreg, u8 reg) {DwInst(0xB800 | ((ioreg << 5) & 0x600) | ((reg << 4) & 0x01F0) | (ioreg & 0x000F));}



// Register access
//


void DwGetRegs(int first, u8 *regs, int count) {
  if (count == 1) {
    DwOut(DWDRreg(), first);
  } else {
    DwSetPC(first);
    DwSetBP(first+count);
    DwSend(Bytes(0x66, 0xC2, 1, 0x20)); // Start register read
  }
  DwReceive(regs, count);
}

void DwSetReg(int reg, u8 val) {DwIn(reg, DWDRreg()); DwSend(Bytes(val));}

void DwSetRegs(int first, const u8 *regs, int count) {
  if (count <= 3) {
    DwSend(Bytes(0x64)); // Set single step loaded instruction mode
    while (count > 0) {DwSetReg(first, *regs); first++; regs++; count--;}
  } else {
    DwSetPC(first);
    DwSetBP(first+count);
    DwSend(Bytes(0x66, 0xC2, 0x05, 0x20)); // Start register write
    DwSend(regs, count);
  }
}

void DwSetZ(u16 z) {DwSetRegs(30, (u8*)&z, 2);}



// Data area access
//


void DwUnsafeReadAddr(int addr, int len, u8 *buf) {
  // Do not read addresses 30, 31 or DWDR as these interfere with the read process
  DwSetZ(addr);
  DwSetPC(0);
  DwSetBP(2*len);
  DwSend(Bytes(0x66, 0xC2, 0x00, 0x20)); // Start data area read
  DwReceive(buf, len);
}

void DwReadAddr(int addr, int len, u8 *buf) {
  // Read range before r28
  int len1 = min(len, 28-addr);
  if (len1 > 0) {DwUnsafeReadAddr(addr, len1, buf); addr+=len1; len-=len1; buf+=len1;}

  // Some registers are cached - use the cached values.
  while (addr >= 28  &&  addr <= 31  &&  len > 0) {
    *buf = R[addr];  addr++;  len--;  buf++;
  }

  // Read range from 32 to DWDR
  int len2 = min(len, DWDRaddr()-addr);
  if (len2 > 0) {DwUnsafeReadAddr(addr, len2, buf); addr+=len2; len-=len2; buf+=len2;}

  // Provide dummy 0 value for DWDR
  if (addr == DWDRaddr()  &&  len > 0) {buf[0] = 0; addr++; len--; buf++;}

  // Read anything beyond DWDR no more than 128 bytes at a time
  while (len > 128) {DwUnsafeReadAddr(addr, 128, buf); addr+=128; len -=128; buf+=128;}
  if (len > 0) {DwUnsafeReadAddr(addr, len, buf);}
}


void DwWriteAddr(int addr, int len, const u8 *buf) {
  DwSetZ(addr);
  DwSetBP(3);
  DwSend(Bytes(0x66, 0xC2, 0x04)); // Set data area write mode
  int limit = addr + len;
  while (addr < limit) {
    if (addr < 28  ||  (addr > 31  &&  addr != DWDRaddr())) {
      DwSetPC(1);
      DwSend(Bytes(0x20, *buf)); // Write one byte to data area and increment Z
    } else {
      if (addr >= 28  &&  addr <= 31) {R[addr] = *buf;}
      DwSetZ(addr+1);
    }
    addr++;
    buf++;
  }
}



// Start / stop
//


void DwReconnect(void) {
  DwSend(Bytes(0xF0));  // Request current PC
  PC = (2 * (DwReadWord() - 1) % FlashSize());
  DwGetRegs(28, R+28, 4); // Cache r28 through r31
}

void DwReset(void) {
  DwSend(Bytes(0x07));  // dWIRE reset
  DwSync();
  DwReconnect();
}

void DwDisable(void) {
  DwSend(Bytes(0x06));
  DwFlush();
}


void DwTrace(void) { // Execute one instruction
  DwSetRegs(28, R+28, 4);       // Restore cached registers
  DwSetPC(PC/2);             // Trace start address
  DwSend(Bytes(0x60, 0x31)); // Single step
  DwSync();
  DwReconnect();
}


void DwGo(void) { // Begin executing.
  DwSetRegs(28, R+28, 4);  // Restore cached registers
  DwSetPC(PC/2);        // Execution start address
  if (BP < 0) {         // Prepare to start execution with no breakpoint set
    DwSend(Bytes(TimerEnable ? 0x40 : 0x60)); // Set execution context
  } else {              // Prpare to start execution with breakpoint set
    DwSetBP(BP/2);      // Breakpoint address
    DwSend(Bytes(TimerEnable ? 0x41 : 0x61)); // Set execution context including breakpoint enable
  }
  DwSend(Bytes(0x30)); // Continue execution (go)
  DwWait();

  // Note: We return to UI with the target processor running. The UI go command handles
  // getting control back, either on the debice hitting the hardware breakpoint, or
  // on the user pressing return to trigger a break.
}


int GetDeviceType(void) {
  DwSend(Bytes(0xF3));  // Request signature
  int signature = DwReadWord();
  int i=0; while (    Characteristics[i].signature
                  &&  Characteristics[i].signature != signature) {i++;}
  return Characteristics[i].signature ? i : -1;
}


void DescribePort(int i) {
  Assert(i < PortCount);
  if (Ports[i]->character < 0) {
    Ws("Unknown device ");
  } else {
    Ws(Characteristics[Ports[i]->character].name);
  }
  Ws(" on ");
  if (Ports[i]->kind == 's') {
    #if windows
      Ws("COM");
    #elif defined(__APPLE__)
      Ws("/dev/tty.usbserial");
    #else
      Ws("/dev/ttyUSB");
    #endif
  } else {
    Ws("UsbTiny");
  }
  Wd(Ports[i]->index,1); Ws(" at ");
  Wd(Ports[i]->baud,1);
  Wsl(" baud.");
}


void ConnectPort(int i, int baud) {
  if (Ports[i]->kind == 's') {
    ConnectSerialPort((struct SPort*)Ports[i], baud);
  } else {
    ConnectUsbtinyPort((struct UPort*)Ports[i]);
  }
  if (Ports[i]->baud > 0) {
    CurrentPort = i;
    Ports[i]->character = GetDeviceType();
  }
}


void DwFindPort(char kind, int index, int baud) {
  int i = 0;
  //Ws("Debug. DwFindPort("); Wc(kind); Ws(", "); Wd(index,1); Ws(", "); Wd(baud,1); Wsl(")");
  while (i<PortCount) {
    if (Ports[i]->kind) {
      if (    ((kind == 0)  || (kind  == Ports[i]->kind))
          &&  ((index == -1) || (index == Ports[i]->index))) {
        ConnectPort(i, baud);
        if (Ports[i]->baud > 0) break;
      }
    }
    i++;
  }
  if (i < PortCount  &&  Ports[i]->baud > 0) {
    CurrentPort = i;
    ResetDumpStates();
    DwReconnect();
  } else {
    CurrentPort = -1;
  }
}


void DwListDevices(void) {
  if (PortCount <= 0) {
    Fail("No devices available.");
  } else {
    int i;
    for (i=0; i<PortCount; i++) {
      if (Ports[i]->baud >= 0) {ConnectPort(i, 0);}
    }
    for (i=0; i<PortCount; i++) {
      if (Ports[i]->baud > 0) {DescribePort(i);}
    }
  }
  CurrentPort = -1;
}


void ConnectFirstPort(void) {
  DwFindPort(0,-1,0);
  if (CurrentPort >= 0) {
    Assert(Ports[CurrentPort]->character >= 0);
    Ws("Connected to ");
    DescribePort(CurrentPort);
  }
}



/*

/// DebugWire protocol notes

See RikusW's excellent work at http://www.ruemohr.org/docs/debugwire.html.


DebugWire command byte interpretation:

06      00 xx x1 10   Disable dW (Enable ISP programming)
07      00 xx x1 11   Reset

20      00 10 00 00   go start reading/writing SRAM/Flash based on low byte of IR
21      00 10 00 01   go read/write a single register
23      00 10 00 11   execute IR (single word instruction loaded with D2)

30      00 11 00 00   go normal execution
31      00 11 00 01   single step (Rikusw says PC increments twice?)
32      00 11 00 10   go using loaded instruction
33      00 11 00 11   single step using slow loaded instruction (specifically spm)
                      will generate break and 0x55 output when complete.

t: disable timers
40/60   01 t0 00 00   Set GO context  (No bp?)
41/61   01 t0 00 01   Set run to cursor context (Run to hardware BP?)
43/63   01 t0 00 11   Set step out context (Run to return instruction?)
44/64   01 t0 01 00   Set up for single step using loaded instruction
46/66   01 t0 01 10   Set up for read/write using repeating simulated instructions
59/79   01 t1 10 01   Set step-in / autostep context or when resuming a sw bp (Execute a single instruction?)
5A/7A   01 t1 10 10   Set single step context



83      10 d0 xx dd   Clock div

C2      11 00 00 10   Set read/write mode (folowed by 0/4 SRAM, 1/5 regs, 2 flash)

w:  word operation (low byte only if 0)
cc: control regs: 0: PC, 1: BP, 2: IR, 3: Sig.
Cx/Dx   11 0w xx cc   Set control reg  (Cx for byte register, Dx for word register)
Ex/Fx   11 1w xx cc   Read control reg (Ex for byte register, Fx for word register)


Modes:

SRAM repeating instructions:
C2 00                   C2 04
ld  r16,Z+       or     in r16,DWDR
out DWDR,r16     or     st Z+,r16

Regs repeating instructions
C2 01            or     C2 05
out DWDR,r0      or     in r0,DWDR
out DWDR,r1      or     in r1,DWDR
out DWDR,r2      or     in r2,DWDR
...                     ....

Flash repeating instructions
C2 03
lpm r?,Z+        or    ?unused
out SWDR,r?      or    ?unused



-------------------------------------------------------------------------



40/60   0 1  x  0 0  0  0 0   GO                         Set GO context  (No bp?)
41/61   0 1  x  0 0  0  0 1   Run to cursor              Set run to cursor context (Run to hardware BP?)
43/63   0 1  x  0 0  0  1 1   Step out                   Set step out context (Run to return instruction?)
44/64   0 1  x  0 0  1  0 0   Write flash page           Set up for single step using loaded instruction
46/66   0 1  x  0 0  1  1 0   Use virtual instructions   Set up for read/write using repeating simulated instructions
59/79   0 1  x  1 1  0  0 1   Step in/autostep           Set step-in / autostep context or when resuming a sw bp (Execute a single instruction?)
5A/7A   0 1  x  1 1  0  1 0   Single step                Set single step context
             |  | |  |  | |
             |  | |  |  '-'------ 00 no break
             |  | |  |  '-'------ 01 break when PC = BP, or single step resuming a sw bp
             |  | |  |  '-'------ 10 Used for executing from virtual space OR single step
             |  | |  |  '-'------ 11 break at return?
             |  | |  '----------- Instructions will load from flash (0) or virtual space (1)
             |  '-'-------------- 00 Not single step
             |  '-'-------------- 01 ?
             |  '-'-------------- 10 ?
             |  '-'-------------- 11 Single step or maybe, use IR instead of (PC) for first instruction
             '------------------- Run with timers disabled


20      0 0 1 0 0 0 0 0    go start reading/writing reg/SRAM/Flash based on IR and low byte of PC
21      0 0 1 0 0 0 0 1    single step read/write a single register
22      0 0 1 0 0 0 1 0    MAYBE go starting with instruction in IR followed by virtual instrucion?
23      0 0 1 0 0 0 1 1    single step an instruction in IR (loaded with D2)
30      0 0 1 1 0 0 0 0    go normal execution
31      0 0 1 1 0 0 0 1    single step (Rikusw says PC increments twice?)
32      0 0 1 1 0 0 1 0    go using loaded instruction
33      0 0 1 1 0 0 1 1    single step using slow loaded instruction (specifically spm)
              |     | |    will generate break and 0x55 output when complete.
              |     | |
              |     | '--- Single step - stop after 1 instruction
              |-----'----- 00 Execute from virtual space
              |-----'----- 01 Execute from loaded IR
              |-----'----- 10 Execute from flash
              |-----'----- 11 Execute from loaded IR and generate break on completion (specifically fro SPM)


Resume execution:              60/61/79/7A 30
Resume from SW BP:             79 32
Step out:                      63 30
Execute instruction (via D2):  ?? 23
Read/write registers/SRAM:     66 20
Write single register:         66 21



Resuming execution

D0 00 00 xx -- set PC, xx = 40/60 - 41/61 - 59/79 - 5A/7A
D1 00 01 -- set breakpoint (single step in this case)
D0 00 00 30 -- set PC and GO







Writing a Flash Page

66
D0 00 1A D1 00 20 C2 05 20 03 01 05 40 00 00 -- Set X, Y, Z
D0 1F 00                                     -- Set PC to 0x1F00, inside the boot section to enable spm--

64
D2  01 CF  23        -- movw r24,r30
D2  BF A7  23        -- out SPMCSR,r26 = 03 = PGERS
D2  95 E8  33        -- spm

<00 55> 83 <55>

44 - before the first one
And then repeat the following until the page is full.

D0  1F 00            -- set PC to bootsection for spm to work
D2  B6 01  23 ll     -- in r0,DWDR (ll)
D2  B6 11  23 hh     -- in r1,DWDR (hh)
D2  BF B7  23        -- out SPMCSR,r27 = 01 = SPMEN
D2  95 E8  23        -- spm
D2  96 32  23        -- adiw Z,2


D0 1F 00
D2 01 FC 23 movw r30,r24
D2 BF C7 23 out SPMCSR,r28 = 05 = PGWRT
D2 95 E8 33 spm
<00 55>

D0 1F 00
D2 E1 C1 23 ldi r28,0x11
D2 BF C7 23 out SPMCSR,r28 = 11 = RWWSRE
D2 95 E8 33 spm
<00 55> 83 <55>

Reading Eeprom

66 D0 00 1C D1 00 20 C2 05 20 --01 01 00 00-- --Set YZ--
64 D2 BD F2 23 D2 BD E1 23 D2 BB CF 23 D2 B4 00 23 D2 BE 01 23 xx

66 D0 00 1C D1 00 20 C2 05 20 --01 01 00 00-- --Set YZ--
64
D2 BD F2 23 out EEARH,r31
D2 BD E1 23 out EEARL,r30
D2 BB CF 23 out EECR,r28 = 01 = EERE
D2 B4 00 23 in r0,EEDR
D2 BE 01 23 out DWDR,r0
xx -- Byte from target


Writing Eeprom

66 D0 00 1A D1 00 20 C2 05 20 --04 02 01 01 10 00-- --Set XYZ--
64 D2 BD F2 23 D2 BD E1 23 D2 B6 01 23 xx D2 BC 00 23 D2 BB AF 23 D2 BB BF 23

64
D2 BD F2 23 out EEARH,r31 = 00
D2 BD E1 23 out EEARL,r30 = 10
D2 B6 01 23 xx in r0,DWDR = xx - byte to target
D2 BC 00 23 out EEDR,r0
D2 BB AF 23 out EECR,r26 = 04 = EEMWE
D2 BB BF 23 out EECR,r27 = 02 = EEWE

*/
