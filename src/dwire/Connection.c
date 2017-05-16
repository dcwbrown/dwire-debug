// Connection.c - common representation of Digispark or USB Serial connections


struct Port {
  char kind;      // 's' for serial port (COMx or ttyUSBx), 'u' for a usbtiny port, 0 for no port.
  int  index;     // Serial device number or usbtiny number
  int  character; // Index to Characteristics[]
  int  baud;
};

struct SPort {  // Serial port
  struct Port port;
  FileHandle  handle;
};

struct UPort {
  struct Port        port;
  struct usb_device *device;
  usb_dev_handle    *handle;
};

struct Port *Ports[32];
int    PortCount   = 0;
int    CurrentPort = -1;


usb_dev_handle *DigisparkPort = 0;
FileHandle      SerialPort    = 0;


void unconnected() {Fail("Unexpected error, device port not connected.");}

void (*DwBreakAndSync)()                  = (void*)unconnected;
int  (*DwReachedBreakpoint)()             = (void*)unconnected;
void (*DwSend)(const u8 *out, int outlen) = (void*)unconnected;
void (*DwFlush)()                         = (void*)unconnected;
int  (*DwReceive)(u8 *in, int inlen)      = (void*)unconnected;
void (*DwSync)()                          = (void*)unconnected;
void (*DwWait)()                          = (void*)unconnected;




// Device specific characteristics

struct Characteristic {
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

struct Characteristic *CurrentCharacteristics() {
  Assert(CurrentPort >= 0);
  return Characteristics + Ports[CurrentPort]->character;
}

int  IoregSize()   {return CurrentCharacteristics()->ioregSize;}
int  SramSize()    {return CurrentCharacteristics()->sramSize;}
int  EepromSize()  {return CurrentCharacteristics()->eepromSize;}
int  FlashSize()   {return CurrentCharacteristics()->flashSize;}   // In bytes
int  PageSize()    {return CurrentCharacteristics()->pageSize;}    // In bytes
int  DWDRreg()     {return CurrentCharacteristics()->DWDR;}
int  DWDRaddr()    {return CurrentCharacteristics()->DWDR + 0x20;} // IO regs come after the 32 regs r0-r31
int  DataLimit()   {return 32 + IoregSize() + SramSize();}
int  BootSect()    {return CurrentCharacteristics()->boot;}
int  BootFlags()   {return CurrentCharacteristics()->bootflags;} // 1 = in ext fuse, 2 = in high fuse
int  EECR()        {return CurrentCharacteristics()->EECR;}
int  EEDR()        {return EECR()+1;}
int  EEARL()       {return EECR()+2;}
int  EEARH()       {return CurrentCharacteristics()->EEARH;}
char *Name()       {return CurrentCharacteristics()->name;}
int  SPMCSR()      {return 0x37;} // SPMCSR is at the same address on all devices

enum {MaxFlashPageSize = 128, MaxFlashSize = 32768, MaxSRamSize = 2048};
