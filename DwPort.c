/// DebugPort.c

#define ByteArrayLiteral(...) (u8[]){__VA_ARGS__}, sizeof((u8[]){__VA_ARGS__})







/// DebugWire port access

void DwExpect(const u8 *bytes, int len) {
  u8 actual[len];
  SerialRead(actual, len);
  for (int i=0; i<len; i++) {
    if (actual[i] != bytes[i]) {
      Ws("WriteDebug, byte "); Wd(i+1,1); Ws(" of "); Wd(len,1);
      Ws(": Read "); Wx(actual[i],2); Ws(" expected "); Wx(bytes[i],2); Wl(); Fail("");
    }
  }
}


void DwWrite(const u8 *bytes, int len) {
  SerialWrite(bytes, len);
  DwExpect(bytes, len);
}


u8 hi(int w) {return (w>>8)&0xff;}
u8 lo(int w) {return (w   )&0xff;}


void DwWriteWord(int word) {
  DwWrite(ByteArrayLiteral(hi(word), lo(word)));
}


int DwReadByte() {u8 byte   = 0;   SerialRead(&byte, 1); return byte;}
int DwReadWord() {u8 buf[2] = {0}; SerialRead(buf, 2);   return (buf[0] << 8) | buf[1];}




int IoregSize  = 0;
int SramSize   = 0;
int DataLimit  = 0;     // Max addressable data = 32 (registers) + IoregSize + SramSize.
int EepromSize = 0;
int FlashSize  = 0;     // In bytes
int DWDR       = 0x42;  // Data address of DebugWIRE port.


void DwSync() { // Used after reset/go/break
  Assert(DwReadByte() == 0);
  u8 byte = DwReadByte();
  while (byte == 0xFF) {byte = DwReadByte();}
  Assert(byte == 0x55);
}

void DwBreak() {SerialBreak(400); DwSync();}


void SetSizes(int signature) {
  switch (signature) {
    case 0x9108: IoregSize = 64;  SramSize = 128;  EepromSize = 128;  FlashSize = 2048;  Wsl("ATtiny25");  break;
    case 0x9206: IoregSize = 64;  SramSize = 256;  EepromSize = 256;  FlashSize = 4096;  Wsl("ATtiny45");  break;
    case 0x930B: IoregSize = 64;  SramSize = 512;  EepromSize = 512;  FlashSize = 8192;  Wsl("ATtiny85");  break;
    default:     Ws("Unrecognised device signature: "); Wx(signature,4); Fail("");
  }
  DataLimit = 32 + IoregSize + SramSize;
}

void DwReadRegisters(u8 *registers, int first, int count) {
  //wsl("Load Registers.");
  // Load Registers (destroys PC and BP)
  DwWrite(ByteArrayLiteral(
    0x66,                 // Access registers/memory mode
    0xD0, 0, first,       // Set PC = first
    0xD1, 0, first+count, // Set BP = limit
    0xC2, 1, 0x20         // Start register read
  ));
  SerialRead(registers, count);
}


void DwWriteRegisters(u8 *registers, int first, int count) {
  //wsl("Load Registers.");
  // Load Registers (destroys PC and BP)
  DwWrite(ByteArrayLiteral(
    0x66,                 // Access registers/memory mode
    0xD0, 0, first,       // Set PC = first
    0xD1, 0, first+count, // Set BP = limit
    0xC2, 5, 0x20         // Start register write
  ));
  DwWrite(registers, count);
}


void DwReadAddr(int addr, int len, u8 *buf) {
  if (addr <= DWDR  &&  addr+len > DWDR) {
    // Split the read accross DWDR as reading DWDR hangs (since it's in use)
    if (addr < DWDR) {DwReadAddr(addr, DWDR-addr, buf);}
    buf[DWDR-addr] = 0;
    if (addr+len+1 > DWDR) {DwReadAddr(DWDR+1, len - (DWDR+1-addr), buf + (DWDR+1-addr));}
    return;
  }

  DwWrite(ByteArrayLiteral(
    0xD0, 0,0x1e, 0xD1, 0,0x20,             // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
    0xC2, 5, 0x20, lo(addr), hi(addr),      // Write SRAM address of first IO register to Z
    0xD0, 0,0, 0xD1, hi(len*2), lo(len*2),  // Set PC=0, BP=2*length
    0xC2, 0, 0x20                           // Start the read
  ));
  SerialRead(buf, len);
}


void DwWriteAddr(int addr, int len, u8 *buf) {
  // Avoid reading address 0x22 - it is the DebugWire port and will hang if read.
  DwWrite(ByteArrayLiteral(
    0xD0, 0,0x1e, 0xD1, 0,0x20,                 // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
    0xC2, 5, 0x20, lo(addr), hi(addr),          // Write SRAM address of first IO register to Z
    0xD0, 0,1, 0xD1, hi(len*2+1), lo(len*2+1),  // Set PC=0, BP=2*length
    0xC2, 4, 0x20                               // Start the write
  ));
  DwWrite(buf, len);
}


void DwReadFlash(int addr, int len, u8 *buf) {
  DwWrite(ByteArrayLiteral(
    0xD0, 0,0x1e, 0xD1, 0,0x20,            // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
    0xC2, 5, 0x20, lo(addr),hi(addr),      // Write addr to Z
    0xD0, 0,0, 0xD1, hi(2*len),lo(2*len),  // Set PC=0, BP=2*len
    0xC2, 2, 0x20                          // Read length bytes from flash starting at first
  ));
  SerialRead(buf, len);
}


int PC = 0;
u8 Registers[32] = {0};  // Note: r30 & r31 are read on connection, the rest only on demand


void DwReconnect() {
  DwWrite(ByteArrayLiteral(0xF0)); PC = DwReadWord()-1;
  DwReadRegisters(&Registers[30], 30, 2);  // Save r30 & r31
}

void DwConnect() {
  DwReconnect();
  DwWrite(ByteArrayLiteral(0xF3)); SetSizes(DwReadWord());
}

void DwReset() {
  DwBreak();
  DwWrite(ByteArrayLiteral(7)); DwSync();
  DwReconnect();
}

void DwGo() { // Restore saved registers and restart execution

}

int BP = -1;

void DwTrace() { // Execute one instruction
  DwWrite(ByteArrayLiteral(
    0x66,                          // Register or memory access mode
    0xD0, 0, 30,                   // Set up to set registers starting from r30
    0xD1, 0, 32,                   // Register limit is 32 (i.e. stop at r31)
    0xC2, 5, 0x20,                 // Select reigster write mode and start
    Registers[30], Registers[31],  // Saved value of r30 and r31
    0x60, 0xD0, hi(PC), lo(PC),    // Address to restart execution at
    0x31                           // Continue execution
  ));

  //SerialDump();

  DwSync(); DwReconnect();
}


//  // Load IO space up to DWDR (0x22) (Destroys PC, BP and r30/r31)
//  //wsl("Load first IO space.");
//  u8 cmd2[] = {
//    0xD0, 0,0x1e, 0xD1, 0,0x20,  // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
//    0xC2, 5, 0x20, 0x20,0,       // Write SRAM address of first IO register to Z
//    0xD0, 0,0, 0xD1, 0,0x44,     // Set PC=0, BP=2*length
//    0xC2, 0, 0x20                // Read length bytes from memory
//  };
//  DwWrite(cmd2, sizeof cmd2);
//  SerialReadBytes(&DataMemory[0x20], 0x22);
//  DataMemory[Dwdr] = 0; // Meaningless to read DWDR as it represents the debugwire interface
//
//
//  // Load remaining IO space and data memeory (0x43 through 0x15F as SRAM offsets)
//  //wsl("Load Remaining IO space.");
//  u16 len = 0x60-0x43;
//  u8 cmd3[] = {
//    0xD0, 0,0x1e, 0xD1, 0,0x20,           // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
//    0xC2, 5, 0x20, 0x43,0,                // Write frst data memoery address to read to Z
//    0xD0, 0,0, 0xD1, hi(2*len),lo(2*len), // Set PC=0, BP=2*length
//    0xC2, 0, 0x20                         // Read length bytes from memory
//  };
//  DwWrite(cmd3, sizeof cmd3);
//  SerialReadBytes(&DataMemory[0x43], len);
//  //wsl("Load data memory complete.");
//}





#if 0

u16 GetPC()  {DwByte(0xF0); return DwReadWord();}
u16 GetBP()  {DwByte(0xF1); return DwReadWord();}
u16 GetIR()  {DwByte(0xF2); return DwReadWord();}
u16 GetSR()  {DwByte(0xF3); return DwReadWord();}

u16 PC = 0;
u16 BP = 0;
u16 IR = 0;
u16 SR = 0;



void ShowControlRegisters() {
  ws("Control registers: PC "); wx(PC,4);
  ws(", BP ");       wx(BP,4);
  ws(", IR ");       wx(IR,4);
  ws(", SR ");       wx(SR,4);
  wl();
}

#ifdef windows
  void __chkstk_ms(void) {}
#endif






// Data memory
//
// The data array tracks the AtTiny45 data memory:
//    Data[$00-$01F]: 32 bytes of r0-r31
//    Data[$20-$05F]: 64 bytes of I/O registers
//    Data[$60-$15F]: 256 bytes of static RAM


u8 DataMemory[32+64+256] = {};  // 32 bytes r0-r31, 64 bytes of IO registers, 256 bytes of RAM.
int RamLoaded = 0;

enum { // IO register address in data memory
  DdrB  = 0x37,
  PortB = 0x38,
  Dwdr  = 0x42, // (Inaccesible)
  SpL   = 0x5D,
  SpH   = 0x5E,
  Sreg  = 0x5F
};



void LoadRAM() {
  u8 cmd[] = {
    0xD0, 0,0x1e, 0xD1, 0,0x20,  // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
    0xC2, 5, 0x20, 0x60,0,       // Set Z to address of start of SRAM in data memory
    0xD0, 0,0, 0xD1, 2,0,        // Set PC=0, BP=2*length
    0xC2, 0, 0x20                // Read 256 bytes from memory
  };
  DwWrite(cmd, sizeof cmd);
  SerialReadBytes(&DataMemory[0x60], 256);
  RamLoaded = 1;
}


void StoreRegisters() {
  u8 cmd[] = {
    0x66,                   // Access registers/memory mode
    0xD0, 0,0, 0xD1, 0,32,  // Set PC=0, BP=32 (i.e. address all registers)
    0xC2, 5, 0x20           // Start register write
  };
  DwWrite(cmd, sizeof(cmd));
  DwWrite(DataMemory, 32);
}


//void StoreRAM() {
//  u8 cmd[] = {
//    0xD0, 0,0x1e, 0xD1, 0,0x20,  // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
//    0xC2, 5, 0x20, 0x60,0,       // Set Z to address of start of SRAM in data memory
//    0xD0, 0,0, 0xD1, 2,0,        // Set PC=0, BP=2*length
//    0xC2, 4, 0x20                // Write 256 bytes to memory
//  };
//  DwWrite(cmd, sizeof cmd);
//  SerialWriteBytes(&DataMemory[0x60], 256);
//}



void ReadFlash(const u8 *buf, u16 first, u16 length) {
  u8 cmd[] = {
    0xD0, 0,0x1e, 0xD1, 0,0x20,                  // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
    0xC2, 5, 0x20, lo(first),hi(first),          // Write first to Z
    0xD0, 0,0, 0xD1, hi(2*length),lo(2*length),  // Set PC=0, BP=2*length
    0xC2, 2, 0x20                                // Read length bytes from flash starting at first
  };
  DwWrite(cmd, sizeof cmd);
  SerialReadBytes(buf, length);
}

enum {FlashSize=4096};
u8 FlashBytes[FlashSize] = {};

void LoadAllFlash() {
  int chunksize = 256;
  for (int i=0; i<sizeof(FlashBytes)/chunksize; i++) {
    ReadFlash(&FlashBytes[i*chunksize], i*chunksize, chunksize);
  }
}

void LoadFlashBytes(u16 first, u16 limit) {
  u16 length = limit-first;
  u8 cmd[] = {
    0xD0, 0,0x1e, 0xD1, 0,0x20,                  // Set PC=0x001E, BP=0x0020 (i.e. address register Z)
    0xC2, 5, 0x20, lo(first),hi(first),          // Write first to Z
    0xD0, 0,0, 0xD1, hi(2*length),lo(2*length),  // Set PC=0, BP=2*length
    0xC2, 2, 0x20                                // Read length bytes from flash starting at first
  };
  DwWrite(cmd, sizeof cmd);
  SerialReadBytes(&FlashBytes[first], length);
}

u16 FlashWord(u16 addr) {
  return FlashBytes[2*addr] + (FlashBytes[2*addr + 1] << 8);
}


#endif


//void ReadFlash(u16 first, u16 limit) {
//  SetRegisterPair(0x1e, first);  // Write start address to register pair R30/R31 (aka Z)
//  // Read Flash starting at Z
//  SetPC(0); SetBP(2*(limit-first)); SetMode(2); DwGo();
//  for (int i=first; i<limit; i++) {wDumpByte(i, SerialRead());}
//  wDumpEnd();
//  RestoreControlRegisters();
//}


//void SetSRAMByte(u16 addr, u8 value) {
//  SetRegisterPair(0x1e, addr);
//  SetPC(1); SetBP(3); SetMode(4); DwGo();
//  DwByte(value);
//  RestoreControlRegisters();
//}


/// DebugWire protocol




/*

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

C2      11 00 00 10   Set read/write mode (folowed by SRAM 0, Regs 1, Flash 2)

w:  word operation (low byte only if 0)
cc: control regs: 0: PC, 1: BP, 2: IR, 3: Sig.
Cx/Dx   11 0w xx cc   Set control reg
Ex/Fx   11 1w xx cc   Read control reg


Modes:

0 (SRAM) repeating instructions:

ld  r16,Z+       or     in r16,DWDR
out DWDR,r16     or     st Z+,r16

1 (Regs) repeating instructions
out DWDR,r0      or     in r0,DWDR
out DWDR,r1      or     in r1,DWDR
out DWDR,r2      or     in r2,DWDR
...                     ....

2 (Flash) repeating instructions

lpm r?,Z+        or    ?unused
out SWDR,r?      or    ?unused



-------------------------------------------------------------------------



40/60   0   0   0   0   0   GO                         Set GO context  (No bp?)
41/61   0   0   0   0   1   Run to cursor              Set run to cursor context (Run to hardware BP?)
43/63   0   0   0   1   1   Step out                   Set step out context (Run to return instruction?)
44/64   0   0   1   0   0   Write flash page           Set up for single step using loaded instruction
46/66   0   0   1   1   0   Use virtual instructions   Set up for read/write using repeating simulated instructions
59/79   1   1   0   0   1   Step in/autostep           Set step-in / autostep context or when resuming a sw bp (Execute a single instruction?)
5A/7A   1   1   0   1   0   Single step                Set single step context
        ^   ^   ^   ^   ^
        |   |   |   '---'------ 00 no break, 01 Break at BP, 10 ?, 11 ? break at return?
        |   |   '-------------- PC represents Flash (0) or virtual space (1)
        '---'------------------




20      0 0 0   go start reading/writing SRAM/Flash based on low byte of PC
21      0 0 1   single step read/write a single register
23      0 1 1   single step an instruction loaded with D2
30      1 0 0   go normal execution
31      1 0 1   single step (Rikusw says PC increments twice?)
32      1 1 0   go using loaded instruction
33      1 1 1   single step using slow loaded instruction (specifically spm)
                      will generate break and 0x55 output when complete.
        ^ ^ ^
        | | '---- Single step - stop after 1 instruction
        | '------ Execute (at least initially) instruction loaded with D2.
        '-------- Use Flash (1) or virtual memory selected by C2 (0)


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





*/