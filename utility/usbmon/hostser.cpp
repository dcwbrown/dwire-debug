#include "command.hpp"
#include "rpc.hpp"
#include "hostser.hpp"

#include <asm/termios.h>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <fcntl.h>
#include <stdarg.h>
#include <stropts.h> // ioctl
#include <sys/file.h> // flock
#include <unistd.h>

LuXactSer::LuXactSer(const char* serialnumber) {
  label.AddSegment(sgNumber - 1);
  label.Segment(sgSerial, serialnumber);

  I2cAddr = 0x25;
  I2cSCLo = TIOCM_DTR;
  I2cSDAo = TIOCM_RTS;
  I2cSDAi = TIOCM_CTS;
}

LuXactSer::~LuXactSer() {
  Close();
}

bool LuXactSer::Open() {
  Close();

  struct Dir {
    Dir() { dir = opendir("/dev"); }
    ~Dir() { closedir(dir); }
    operator DIR*() { return dir; }
    DIR* dir;
  } dir;

  dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (strncmp("ttyUSB", entry->d_name, 6))
      continue;

    // open file and acquire exclusive lock
    fd = open((std::string("/dev/") + entry->d_name).c_str(), O_RDWR|O_NOCTTY);
    if (fd < 0)
      continue;

    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
      close(fd); fd = -1;
      continue;
    }

    // initialize i2c
    Gio();
    I2cInit();

    // get dwlink version
    {
      char command = dlcVersion;
      uint8_t version = 0;
      if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
          || I2cRead(dlrReturn, (char*)&version, sizeof(version)) != sizeof(version)
          || version != dlVersion) {
        if (version != dlVersion)
            std::cerr << "incorrect dwlink version\n";
        close(fd); fd = -1;
        continue;
      }
    }

    // get serial number
    union {
      char buffer[];
      uint16_t value;
      operator uint16_t() { return value; }
    } serial; {
      char command = dlcGetSerial;
      if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
          || I2cRead(dlrData0, serial.buffer, sizeof(serial)) != sizeof(serial)) {
        close(fd); fd = -1;
        continue;
      }
    }

    // check serial number match
    std::string sSerial{label.Segment(sgSerial)};
    if (sSerial.length() != 0 && (char16_t)strtol(sSerial.c_str(), nullptr, 0) != serial) {
      close(fd); fd = -1;
      continue;
    }

    // save device name and serial number
    label.Segment(sgDevice, entry->d_name);
    if (sSerial.length() == 0)
      label.Segment(sgSerial, std::to_string(serial));
    break;
  }
  if (!entry)
    return false;

  // clear input buffer
  recv_size = 0;
  recv_skip = 0;

  Label();
  return true;
}

bool LuXactSer::IsOpen() const {
  return fd >= 0;
}

bool LuXactSer::Close() {
  if (!IsOpen()) return false;
  close(fd);
  fd = -1;

  Label();
  return true;
}

void LuXactSer::Send(char _data) {
  char data[3] = { 0, _data, 0 };
  int xfer = write(fd, data, sizeof(data));
  if (xfer > 0)
    recv_skip += xfer;
}

void LuXactSer::Send(struct Rpc& rpc, uint8_t index) {
}

//#define DEBUG_Recv
void LuXactSer::Recv() {
  for (;;) {

#ifdef DEBUG_Recv
    {
      std::cerr << "recv_size: " << (int)recv_size << ", recv_skip: " << (int)recv_skip << ", read:";
      for (uint16_t i = 0; i < recv_size; ++i) std::cerr << ' ' << (int)recv_buff[i];
      std::cerr << "\n";}
#endif

    uint8_t data[4097];
    if (recv_skip) {
      int xfer = read(fd, data, recv_skip < sizeof(data) ? recv_skip : sizeof(data));
      if (xfer < 0) return;
      recv_skip -= xfer;
      if (recv_skip) return;
    }

    memcpy(data, recv_buff, recv_size);
    int xfer = read(fd, data + recv_size, sizeof(data) - 1 - recv_size);
    if (xfer <= 0) return;

#ifdef DEBUG_Recv
    if(xfer){
      std::cerr << "recv_size: " << (int)recv_size << ", xfer: " << xfer << ", read:";
      for (uint16_t i = 0; i < xfer; ++i) std::cerr << ' ' << (int)data[recv_size + i];
      std::cerr << "\n";}
#endif

    xfer += recv_size;

    // this make strlen work on the last packet if there's no zero
    data[xfer] = '\0';

    uint8_t* p = data;
    while (p - data < xfer) {
      if (!p[0]) {
        ++p;
        continue;
      }
      uint16_t size = strlen((char*)p);
      uint8_t rpc[256];
      if (size > sizeof(rpc)) {
        // too long
        p += size + 1;
#ifdef DEBUG_Recv
        std::cerr << "too long\n";
#endif
        continue;
      }
      if (p - data + size >= xfer) {
        // partial packet (no terminating zero)
#ifdef DEBUG_Recv
        std::cerr << "partial packet\n";
#endif
        break;
      }
      uint8_t* t = p; p += size + 1;
      size = RpcSend::COBSdecode(rpc, t, size);
#ifdef DEBUG_Recv
    {
      std::cerr << "rpc: " << (int)size;
      for (uint16_t i = 0; i < size; ++i) std::cerr << ' ' << (int)rpc[i];
      std::cerr << "\n";}
#endif
      if (!rpc[size - 1]) {
        if (sink)
          sink(rpc);
      }
      else if (rpc[size - 1] < RpcNumber)
        ((Rpc*)rpc)->VtAct(rpc[size - 1]);
    }
    recv_size = xfer - (p - data) < sizeof(recv_buff) ? xfer - (p - data) : 0;
    memcpy(recv_buff, p, recv_size);
  }
}

void LuXactSer::Break(uint8_t length) {
  Ioctl(TCFLSH, TCIOFLUSH);
  Ioctl(TIOCSBRK);
  usleep(length * 1000);
  Ioctl(TIOCCBRK);
}

bool LuXactSer::Reset() {
  return false;
}

bool LuXactSer::ResetDw() {
  // break and ignore all received bytes
  union {
    char buffer[];
    uint32_t value;
    operator uint32_t() { return value; }
  } baudrate; {
    char command = dlcBreakSync;
    if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
        || I2cRead(dlrBaudrate0, baudrate.buffer, sizeof(baudrate)) != sizeof(baudrate)
        || !baudrate)
      return false;
  }

  union {
    char buffer[];
    uint8_t value;
    operator uint8_t() { return value; }
  } flagUartSoft; {
    char command = dlcFlagUartSoft;
    if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
        || I2cRead(dlrData0, flagUartSoft.buffer, sizeof(flagUartSoft)) != sizeof(flagUartSoft))
      return false;
  }

  label.Segment(sgBaudrate, std::to_string(baudrate) + " Bd");
  std::cerr << label.Display() << (flagUartSoft ? " UartSoft" : " dw") << '\n';

  // set baudrate
  Baudrate(baudrate);

  // Disable Dw if it was enabled
  if (!flagUartSoft) {
    // this resets the target and turns off dw
    // which stays off until a power cycle
    char command = 6;
    int xfer = write(fd, &command, sizeof(command));
    recv_skip = sizeof(command);
  }

  Label();
  return true;
}

bool LuXactSer::ResetPower() {
  char command = dlcPowerOff;
  if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)) return false;
  usleep(100 * 1000);
  command = dlcPowerOn;
  if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)) return false;
  usleep(100 * 1000);
  return true;
}

/* power supply

V+5--------------o
                 |    _______
               | <E   | PNP |
      10K     B|/     |BC327|
PBx>---zz--o---|\     --|||--
           |   | \C     CBE
           |     |
           --||--o---->V+
            10n

V+ on at boot when PB0 tristated.
V+ off when PB0 set high to turn off Q.
*/

bool LuXactSer::SetSerial(const char* _serial) {
  uint16_t serial = strtol(_serial, nullptr, 0);
  char command[4];
  command[0] = dlcSetSerial;
  command[1] = 0;
  command[2] = serial;
  command[3] = serial >> 8;
  if (I2cWrite(dlrCommand, command, sizeof(command)) != sizeof(command)) {
    std::cerr << "error changing serial number" << std::endl;
    return false;
  }
  return true;
}

void LuXactSer::Baudrate(uint32_t value) {
  //https://linux.die.net/man/3/termios
  //http://man7.org/linux/man-pages/man3/termios.3.html
  //http://man7.org/linux/man-pages/man4/tty_ioctl.4.html

  termios2 config;
  Ioctl(TCGETS2, &config);

  config.c_cflag = CS8 | BOTHER | CLOCAL;
  config.c_iflag = IGNPAR | IGNBRK;
  config.c_oflag = 0;
  config.c_lflag = 0;
  config.c_ispeed = value;
  config.c_ospeed = value;
  config.c_cc[VMIN] = 0;  // minimum # of characters
  config.c_cc[VTIME] = 0; // read timeout * .1 sec
  Ioctl(TCSETS2, &config);

  // flush input and output
  Ioctl(TCFLSH, TCIOFLUSH);

  // clear input buffer
  recv_size = 0;
  recv_skip = 0;
}

void LuXactSer::Ioctl(unsigned long request, ...) {

  va_list va;
  va_start(va, request);
  void* argp = va_arg(va, void*);
  va_end(va);

  for (uint8_t i = 0; i < 5; ++i) {
    if (ioctl(fd, request, argp) >= 0) {
      if (i > 0)
        std::cerr << "ioctl: " << (int)i << '\n';
      return;
    }
    usleep(100);
  }
  SysAbort(__PRETTY_FUNCTION__);
}

uint32_t LuXactSer::Gio() {
  Ioctl(TIOCMGET, &gio);
  return gio;
}

void LuXactSer::Gio(uint32_t value, uint32_t mask) {
  uint32_t last = gio;
  gio = (gio &~ mask) | (value & mask);
  if (gio != last)
    Ioctl(TIOCMSET, &gio);
}

//http://www.robot-electronics.co.uk/i2c-tutorial

uint8_t LuXactSer::I2cWrite(uint8_t regn, const char* data, uint8_t size) {
  // wait for bus
  for (uint8_t i = 0; i < 100; ++i) {
    if (I2cSDA()) break;
    usleep(1000);
  }
  if (!I2cSDA()) return -1;
  I2cStart();
  const char* p = data;
  if (I2cSend(I2cAddr << 1) || I2cSend(regn))
    goto error;
  while (p - data < size)
    if (I2cSend(*p++))
      break;
 error:
  I2cStop();
  return p != data ? p - data : -1;
}

uint8_t LuXactSer::I2cRead(uint8_t regn, char* data, uint8_t size) {
  // wait for bus
  for (uint8_t i = 0; i < 100; ++i) {
    if (I2cSDA()) break;
    usleep(1000);
  }
  if (!I2cSDA()) return -1;
  I2cStart();
  char* p = data;
  if (I2cSend(I2cAddr << 1) || I2cSend(regn))
    goto error;
  I2CRestart();
  if (I2cSend((I2cAddr << 1) | 1))
    goto error;
  while (p - data < size) {
    *p++ = I2cRecv();
    if (p - data < size)
      I2cAck(); else I2cNak();
  }
 error:
  I2cStop();
  return p != data ? p - data : -1;
}

void LuXactSer::I2cSCL(bool v) {
  Gio(!v ? -1 : 0, I2cSCLo);
  usleep(4);
}

void LuXactSer::I2cSDA(bool v) {
  Gio(!v ? -1 : 0, I2cSDAo);
  usleep(4);
}

bool LuXactSer::I2cSDA() {
  return (Gio() & I2cSDAi) == 0;
}

void LuXactSer::I2cInit() {
  I2cSDA(1); I2cSCL(1); }

void LuXactSer::I2cStart() {
  I2cSDA(0); I2cSCL(0); }

void LuXactSer::I2CRestart() {
  I2cSDA(1); I2cSCL(1); I2cSDA(0); I2cSCL(0); }

void LuXactSer::I2cStop() {
  I2cSCL(0); I2cSDA(0); I2cSCL(1); I2cSDA(1); }

void LuXactSer::I2cAck() {
  I2cSDA(0); I2cSCL(1); I2cSCL(0); I2cSDA(1); }

void LuXactSer::I2cNak() {
  I2cSDA(1); I2cSCL(1); I2cSCL(0); }

bool LuXactSer::I2cSend(uint8_t data) {
  for (uint8_t i = 0; i < 8; ++i) {
    I2cSDA(data & 0x80); data <<= 1;
    I2cSCL(1); I2cSCL(0);
  }
  I2cSDA(1); I2cSCL(1);
  bool ack = I2cSDA();
  I2cSCL(0);
  return ack;
}

uint8_t LuXactSer::I2cRecv() {

  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    I2cSCL(1);
    data = (data << 1) | I2cSDA();
    I2cSCL(0);
  }
  return data;
}

void LuXactSer::I2cReset() {
  I2cSDA(1);
  for (uint8_t i = 0; i < 16; ++i) {
    I2cSCL(0); I2cSCL(1);
  }
  I2cStop();
}

void LuXactSer::I2cDebug() {
  auto i2cWrite = [&](uint8_t regn, uint8_t data)->void {
    I2cStart();
    std::cerr << (I2cSend((0x25<<1)) ? "nak " : "ack ");
    std::cerr << (I2cSend(regn) ? "nak " : "ack ");
    std::cerr << (I2cSend(data) ? "nak\n" : "ack\n");
    I2cStop();
  };
  auto i2cRead = [&](uint8_t regn)->uint8_t {
    I2cStart();
    std::cerr << (I2cSend((0x25<<1)) ? "nak " : "ack ");
    std::cerr << (I2cSend(regn) ? "nak " : "ack ");
    I2CRestart();
    std::cerr << (I2cSend((0x25<<1) | 1) ? "nak " : "ack ");
    uint8_t data = I2cRecv();
    std::cerr << std::hex << (int)data << std::dec << '\n';
    I2cNak();
    I2cStop();
    return data;
  };

  for (uint8_t i = 0;;++i) {
    std::cerr << "c: SCL 0, C: SCL 1, d: SDA 0, D: SDA 1, 0: zero, 1: one\n"
      "setup, f: write, g: read\n"
      "read,  h: -1, i: 0, j: 1, k: 2 \n"
      "write, r: -1, s: 0, t: 1, u: 2 \n"
      "2: idle, 3: start, 4: stop, 5: cycle clock, 6: cycle clock * 8, 8: reset\n";
    std::cerr << "sda: " << (int)I2cSDA() << '\n';
    switch (SysConRead()) {
    case 'c':I2cSCL(0);break;
    case 'C':I2cSCL(1);break;
    case 'd':I2cSDA(0);break;
    case 'D':I2cSDA(1);break;
    case '0':I2cSDA(0);I2cSCL(1);I2cSCL(0);break;
    case '1':I2cSDA(1);I2cSCL(1);I2cSCL(0);break;
    case '2':I2cInit();break;
    case '3':I2cStart();break;
    case '4':I2cStop();break;
    case '5':I2cSCL(0);I2cSCL(1);break;
    case '6':for(uint8_t i=0;i<8;++i){I2cSCL(0);I2cSCL(1);}break;
    case '8':I2cReset();break;

    case 'f':I2cStart();I2cSend((0x25<<1));break;
    case 'g':I2cStart();I2cSend((0x25<<1));I2cSend(0);I2cStop();
      I2cStart();I2cSend((0x25<<1)|1);break;
    case 'e':I2cStart();I2cSend((0x25<<1));I2cSend(0);I2cStop();
      I2cStart();

      {uint8_t data=(0x25<<1)|1;
  for (uint8_t i = 0; i < 8; ++i) {
    I2cSDA(data & 0x80); data <<= 1;
    I2cSCL(1); I2cSCL(0);
  }
  I2cSDA(1);
      }
      break;

    case 'h':i2cRead(-1);break;
    case 'i':i2cRead(0);break;
    case 'j':i2cRead(1);break;
    case 'k':i2cRead(2);break;

    case 'r':i2cWrite(-1,i);break;
    case 's':i2cWrite(0,i);break;
    case 't':i2cWrite(1,i);break;
    case 'u':i2cWrite(2,i);break;
    }
  }
}

int count = 0;
uint8_t buf1[1024];
int count2 = 0;
uint8_t buf2[1024];

void LuXactSer::Debug() {

  for(;;) {
    {
      char command = dlcGioSet;
      char data = 2;
      std::cerr << "data: " << (int)data << '\n';
      if (I2cWrite(dlrData0, &data, sizeof(data)) != sizeof(data)
          || I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
          )
        std::cerr << "bad\n";
    }
    {
      char command = dlcGioGet;
      char data = 4;
      if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
          || I2cRead(dlrData0, &data, sizeof(data)) != sizeof(data))
        std::cerr << "bad\n";
      std::cerr << "data: " << (int)data << '\n';
    }
    usleep(1000*1000);
    {
      char command = dlcGioSet;
      char data = 4;
      std::cerr << "data: " << (int)data << '\n';
      if (I2cWrite(dlrData0, &data, sizeof(data)) != sizeof(data)
          || I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
          )
        std::cerr << "bad\n";
    }
    {
      char command = dlcGioGet;
      char data = 4;
      if (I2cWrite(dlrCommand, &command, sizeof(command)) != sizeof(command)
          || I2cRead(dlrData0, &data, sizeof(data)) != sizeof(data))
        std::cerr << "bad\n";
      std::cerr << "data: " << (int)data << '\n';
    }
    usleep(1000*1000);
  }

  //I2cDebug();


  auto sink = [&](uint8_t value)->void {
    buf1[count++] = value;
  };
  auto sink2 = [&](uint8_t value)->void {
    buf2[count2++] = value;
  };

#if 0
  std::cerr << std::hex;

  for (uint32_t i = 0; i < 256; ++i)
    sink(i);
  std::cerr << '\n';
#endif

  if(0){
static uint8_t buf0[] = {
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16
, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64
, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80
, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96
, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,112
,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128
,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144
,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160
,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176
,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192
,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208
,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224
,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240
,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
  };
}

  if(0){
static uint8_t buf0[] = {
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16
, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64
, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80
, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96
, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,112
,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128
,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144
,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160
,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176
,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192
,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208
,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224
,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240
   ,241,242,243,244,245,246,247,248,249,250,251,252,0
  };

  std::cerr << "i: " << sizeof(buf0);
  for (uint32_t i = 0; i < sizeof(buf0); ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf0[i]);
  }
  std::cerr << '\n';

  count = 0; RpcSend::COBSencode(sink, buf0, sizeof(buf0));
  memset(buf1, 0, sizeof(buf1));
  RpcSend::COBSencode(buf1, buf0, sizeof(buf0));

  std::cerr << "e: " << count;
  for (uint32_t i = 0; i < count; ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf1[i]);
  }
  std::cerr << '\n';

  count2 = 0; //RpcSend::COBSdecode(sink2, buf1, count);
  memset(buf2, 0, sizeof(buf2));
  count2 = RpcSend::COBSdecode(buf2, buf1, count);

  std::cerr << "d: " << count2;
  if (sizeof(buf0) != count2) std::cerr << "***";
  for (uint32_t i = 0; i < count2; ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf2[i]);
    if (buf2[i] != buf0[i]) std::cerr << "***";
  }
  std::cerr << '\n';
}
  if(1){
static uint8_t buf0[] = {
  1,  2,  3
};

  std::cerr << "i: " << sizeof(buf0);
  for (uint32_t i = 0; i < sizeof(buf0); ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf0[i]);
  }
  std::cerr << '\n';

  memset(buf2, 0, sizeof(buf2));
  count2 = RpcSend::COBSdecode(buf2, buf0, sizeof(buf0));

  std::cerr << "d: " << count2;
  for (uint32_t i = 0; i < count2; ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf2[i]);
  }
  std::cerr << '\n';

}
  if(1){
static uint8_t buf0[] = {
  23,24,0
  };

  std::cerr << "i: " << sizeof(buf0);
  for (uint32_t i = 0; i < sizeof(buf0); ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf0[i]);
  }
  std::cerr << '\n';

  count = 0; RpcSend::COBSencode(sink, buf0, sizeof(buf0));
  memset(buf1, 0, sizeof(buf1));
  RpcSend::COBSencode(buf1, buf0, sizeof(buf0));

  std::cerr << "e: " << count;
  for (uint32_t i = 0; i < count; ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf1[i]);
  }
  std::cerr << '\n';

  count2 = 0; //RpcSend::COBSdecode(sink2, buf1, count);
  memset(buf2, 0, sizeof(buf2));
  count2 = RpcSend::COBSdecode(buf2, buf1, count);

  std::cerr << "d: " << count2;
  if (sizeof(buf0) != count2) std::cerr << "***";
  for (uint32_t i = 0; i < count2; ++i) {
    if ((i & 0xF) == 0)
      fprintf(stderr, "\n");
    fprintf(stderr, ",%3d", buf2[i]);
    if (buf2[i] != buf0[i]) std::cerr << "***";
  }
  std::cerr << '\n';
}
}
