// GlobalData.c

char UsbSerialPortName[256]  = {0}; // e.g. 'COM6' or '/dev/ttyUSB0'
FileHandle SerialPort = 0;


int PC  = 0;
u8  R30 = 0;
u8  R31 = 0;

struct {
  int   signature;
  int   ioregSize;
  int   sramSize;
  int   eepromSize;
  int   flashSize;
  int   DWDR;       // DebugWIRE data register, aka MONDR - Monitor data register
  char *name;
} Characteristics[] = {
//    sig   io  sram eeprom flash  dwdr
  {0x9108,  64,  128,  128,  2048, 0x42, "ATtiny25"},
  {0x910B,  64,  128,  128,  2048, 0x47, "ATtiny24"},

  {0x9205, 224,  512,  256,  4096, 0x51, "ATmega48A"},
  {0x9206,  64,  256,  256,  4096, 0x42, "ATtiny45"},
  {0x9207,  64,  256,  256,  4096, 0x47, "ATtiny44"},
  {0x920A, 224,  512,  256,  4096, 0x51, "ATmega48PA"},
  {0x9215, 224,  256,  256,  4096, 0x47, "ATtiny441"},

  {0x930A, 224, 1024,  512,  8192, 0x51, "ATmega88A"},
  {0x930B,  64,  512,  512,  8192, 0x42, "ATtiny85"},
  {0x930C,  64,  512,  512,  8192, 0x47, "ATtiny84"},
  {0x930F, 224, 1024,  512,  8192, 0x51, "ATmega88PA"},
  {0x9315, 224,  512,  512,  8192, 0x47, "ATtiny841"},
  {0x9389, 224,  512,  512,  8192, 0x51, "ATmega8U2"},

  {0x9406, 224, 1024,  512, 16384, 0x51, "ATmega168A"},
  {0x940B, 224, 1024,  512, 16384, 0x51, "ATmega168PA"},
  {0x9489, 224,  512,  512, 16384, 0x51, "ATmega16U2"},

  {0x950F, 224, 2048, 1024, 32768, 0x51, "ATmega328P"},
  {0x9514, 224, 2048, 1024, 32768, 0x51, "ATmega328"},
  {0x958A, 224, 1024, 1024, 32768, 0x51, "ATmega32U2"},
  {0,      0,   0,    0,    0,     0,    0}
};


int DeviceType = -1;

// Current dump instruction states

int DBaddr = 0;
int DWaddr = 0;
int EBaddr = 0;
int EWaddr = 0;
int FBaddr = 0;
int FWaddr = 0;
int Uaddr  = 0;

//int Prompted = 0; // Set to 1 by dump commands. TODO - Use OutputPosition instead.

int QuitRequested = 0;

FileHandle CurrentFile          = 0;
char       CurrentFileName[500] = "";

