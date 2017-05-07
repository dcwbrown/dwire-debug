// GlobalData.c


// LittleWire connection

usb_dev_handle *Port = 0;




// Current device state

int PC  = 0;  // PC as a flash address, twice the value used over the dwire interface
int BP  =-1;  // BP as a flash address, twice the value used over the dwire interface
u8  R[32];    // r28-r31 cached at break, others loaded on demand


int TimerEnable = 0;


// Device specific characteristics

struct {
  char *name;
  int   signature;
  int   ioregSize;
  int   sramSize;
  int   eepromSize;
  int   flashSize;  // In bytes
  int   DWDR;       // DebugWIRE data register, aka MONDR - Monitor data register
  int   pageSize;   // In bytes
  int   boot;       // Lowest PC value giving boot section access
  int   bootflags;  // Where to find the boot sectore control flags, if any
  int   EECR;       // EEPROM control register index. EEDR and EEARL always follow directly.
  int   EEARH;      // EEPROM address high (doesn't exist on all devices)
} Characteristics[] = {
//  name             sig   io  sram eeprom flash  dwdr   pg  boot    bf eecr  eearh
  {"ATtiny13",    0x9007,  64,   64,   64,  1024, 0x2E,  32, 0x0000, 0, 0x1C, 0x00},
  {"ATtiny2313",  0x910a,  64,  128,  128,  2048, 0x1f,  32, 0x0000, 0, 0x1C, 0x00},

  {"ATtiny25",    0x9108,  64,  128,  128,  2048, 0x22,  32, 0x0000, 0, 0x1C, 0x1F},
  {"ATtiny24",    0x910B,  64,  128,  128,  2048, 0x27,  32, 0x0000, 0, 0x1C, 0x1F},

  {"ATmega48A",   0x9205, 224,  512,  256,  4096, 0x31,  64, 0x0000, 0, 0x1F, 0x22},
  {"ATtiny45",    0x9206,  64,  256,  256,  4096, 0x22,  64, 0x0000, 0, 0x1C, 0x1F},
  {"ATtiny44",    0x9207,  64,  256,  256,  4096, 0x27,  64, 0x0000, 0, 0x1C, 0x1F},
  {"ATmega48PA",  0x920A, 224,  512,  256,  4096, 0x31,  64, 0x0000, 0, 0x1F, 0x22},
  {"ATtiny441",   0x9215, 224,  256,  256,  4096, 0x27,  16, 0x0000, 0, 0x1C, 0x1F},

  {"ATmega88A",   0x930A, 224, 1024,  512,  8192, 0x31,  64, 0x0F80, 1, 0x1F, 0x22},
  {"ATtiny85",    0x930B,  64,  512,  512,  8192, 0x22,  64, 0x0000, 0, 0x1C, 0x1F},
  {"ATtiny84",    0x930C,  64,  512,  512,  8192, 0x27,  64, 0x0000, 0, 0x1C, 0x1F},
  {"ATmega88PA",  0x930F, 224, 1024,  512,  8192, 0x31,  64, 0x0F80, 1, 0x1F, 0x22},
  {"ATtiny841",   0x9315, 224,  512,  512,  8192, 0x27,  16, 0x0000, 0, 0x1C, 0x1F},
  {"ATmega8U2",   0x9389, 224,  512,  512,  8192, 0x31,  64, 0x0000, 0, 0x1F, 0x22},

  {"ATmega168A",  0x9406, 224, 1024,  512, 16384, 0x31, 128, 0x1F80, 1, 0x1F, 0x22},
  {"ATmega168PA", 0x940B, 224, 1024,  512, 16384, 0x31, 128, 0x1F80, 1, 0x1F, 0x22},
  {"ATmega16U2",  0x9489, 224,  512,  512, 16384, 0x31, 128, 0x0000, 0, 0x1F, 0x22},

  {"ATmega328P",  0x950F, 224, 2048, 1024, 32768, 0x31, 128, 0x3F00, 2, 0x1F, 0x22},
  {"ATmega328",   0x9514, 224, 2048, 1024, 32768, 0x31, 128, 0x3F00, 2, 0x1F, 0x22},
  {"ATmega32U2",  0x958A, 224, 1024, 1024, 32768, 0x31, 128, 0x0000, 0, 0x1F, 0x22},
  {0,                  0,   0,    0,    0,     0,    0,   0,      0, 0,    0}
};

int DeviceType = -1;

void CheckDevice() {if (DeviceType<0) {Fail("Device not recognised.");}}

int  IoregSize()   {CheckDevice(); return Characteristics[DeviceType].ioregSize;}
int  SramSize()    {CheckDevice(); return Characteristics[DeviceType].sramSize;}
int  EepromSize()  {CheckDevice(); return Characteristics[DeviceType].eepromSize;}
int  FlashSize()   {CheckDevice(); return Characteristics[DeviceType].flashSize;}   // In bytes
int  PageSize()    {CheckDevice(); return Characteristics[DeviceType].pageSize;}    // In bytes
int  DWDRreg()     {CheckDevice(); return Characteristics[DeviceType].DWDR;}
int  DWDRaddr()    {CheckDevice(); return Characteristics[DeviceType].DWDR + 0x20;} // IO regs come after the 32 regs r0-r31
int  DataLimit()   {CheckDevice(); return 32 + IoregSize() + SramSize();}
int  BootSect()    {CheckDevice(); return Characteristics[DeviceType].boot;}
int  BootFlags()   {CheckDevice(); return Characteristics[DeviceType].bootflags;} // 1 = in ext fuse, 2 = in high fuse
int  EECR()        {CheckDevice(); return Characteristics[DeviceType].EECR;}
int  EEDR()        {return EECR()+1;}
int  EEARL()       {return EECR()+2;}
int  EEARH()       {CheckDevice(); return Characteristics[DeviceType].EEARH;}
char *Name()       {CheckDevice(); return Characteristics[DeviceType].name;}
int  SPMCSR()      {return 0x37;} // SPMCSR is at the same address on all devices

enum {MaxFlashPageSize = 128, MaxFlashSize = 32768, MaxSRamSize = 2048};


// Current loaded file

FileHandle CurrentFile          = 0;
char       CurrentFilename[500] = "";


// Representation of symbols and line numbers loaded from ELF files

int   HasLineNumbers           = 0;
int   LineNumber[MaxFlashSize] = {0};
char *FileName[MaxFlashSize]   = {0};
char *CodeSymbol[MaxFlashSize] = {0};
char *SramSymbol[MaxSRamSize]  = {0};


// Current dump instruction states

int DBaddr = 0;  // Data area by byte
int DWaddr = 0;  // Data area by word
int EBaddr = 0;  // EEPROM by byte
int EWaddr = 0;  // EEPROM by word
int FBaddr = 0;  // Flash by byte
int FWaddr = 0;  // Flash by word
int Uaddr  = 0;  // Disassembly (Unassemble command)


// UI state

int QuitRequested = 0;
