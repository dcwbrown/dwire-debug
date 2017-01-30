// GlobalData.c


// FT232 connection

char UsbSerialPortName[256]  = {0}; // e.g. 'COM6' or '/dev/ttyUSB0'
FileHandle SerialPort = 0;


// Current device state

int PC  = 0;  // PC as a flash address, twice the value used over the dwire interface
int BP  =-1;  // BP as a flash address, twice the value used over the dwire interface
u8  R30 = 0;
u8  R31 = 0;


// Device specific characteristics

struct {
  int   signature;
  int   ioregSize;
  int   sramSize;
  int   eepromSize;
  int   flashSize;  // In bytes
  int   DWDR;       // DebugWIRE data register, aka MONDR - Monitor data register
  int   pageSize;   // In bytes
  char *name;
} Characteristics[] = {
//    sig   io  sram eeprom flash  dwdr   pg   name
  {0x9007,  64,   64,   64,  1024, 0x2E,  32, "ATtiny13"},

  {0x9108,  64,  128,  128,  2048, 0x22,  32, "ATtiny25"},
  {0x910B,  64,  128,  128,  2048, 0x27,  32, "ATtiny24"},

  {0x9205, 224,  512,  256,  4096, 0x31,  64, "ATmega48A"},
  {0x9206,  64,  256,  256,  4096, 0x22,  64, "ATtiny45"},
  {0x9207,  64,  256,  256,  4096, 0x27,  64, "ATtiny44"},
  {0x920A, 224,  512,  256,  4096, 0x31,  64, "ATmega48PA"},
  {0x9215, 224,  256,  256,  4096, 0x27,  16, "ATtiny441"},

  {0x930A, 224, 1024,  512,  8192, 0x31,  64, "ATmega88A"},
  {0x930B,  64,  512,  512,  8192, 0x22,  64, "ATtiny85"},
  {0x930C,  64,  512,  512,  8192, 0x27,  64, "ATtiny84"},
  {0x930F, 224, 1024,  512,  8192, 0x31,  64, "ATmega88PA"},
  {0x9315, 224,  512,  512,  8192, 0x27,  16, "ATtiny841"},
  {0x9389, 224,  512,  512,  8192, 0x31,  64, "ATmega8U2"},

  {0x9406, 224, 1024,  512, 16384, 0x31, 128, "ATmega168A"},
  {0x940B, 224, 1024,  512, 16384, 0x31, 128, "ATmega168PA"},
  {0x9489, 224,  512,  512, 16384, 0x31, 128, "ATmega16U2"},

  {0x950F, 224, 2048, 1024, 32768, 0x31, 128, "ATmega328P"},
  {0x9514, 224, 2048, 1024, 32768, 0x31, 128, "ATmega328"},
  {0x958A, 224, 1024, 1024, 32768, 0x31, 128, "ATmega32U2"},
  {0,      0,   0,    0,    0,     0,    0}
};

enum {MaxFlashPageSize = 128, MaxFlashSize = 32768, MaxSRamSize = 2048};

int DeviceType = -1;


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

int DBaddr = 0;
int DWaddr = 0;
int EBaddr = 0;
int EWaddr = 0;
int FBaddr = 0;
int FWaddr = 0;
int Uaddr  = 0;


// UI state

int QuitRequested = 0;
