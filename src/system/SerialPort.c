// SerialPort.c


#if defined(__APPLE__)
#include <sys/ioctl.h>
#define TCFLSH  0x540B
#define BOTHER 0010000
#define IOSSIOSPEED _IOW('T', 2, speed_t)
#endif


//// Serial port access.


#if windows
  char portfilename[] = "//./COMnnn";

  void MakeSerialPort(char *portname, int baudrate, FileHandle *SerialPort) {
    strncpy(portfilename+4, portname, sizeof(portfilename)-5);
    portfilename[sizeof(portfilename)-1] = 0;
    //Wc('<'); Flush();
    *SerialPort = CreateFile(portfilename, GENERIC_WRITE | GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
    //Wc('>'); Flush();
    if (*SerialPort==INVALID_HANDLE_VALUE) {
      DWORD winError = GetLastError();
      Ws("Couldn't open ");
      Ws(portname);
      Ws(": ");
      WWinError(winError);
      Fail("");
    }

    DCB dcb = {sizeof dcb, 0};
    dcb.fBinary  = TRUE;
    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    if (!SetCommState(*SerialPort, &dcb)) {
      // This is probably not an FT232
      CloseHandle(*SerialPort); *SerialPort = 0; return;
    }

    WinOK(SetCommTimeouts(*SerialPort, &(COMMTIMEOUTS){300,300,1,300,1}));
  }
#elif defined(__APPLE__)
  void MakeSerialPort(char *portname, int baudrate, FileHandle *SerialPort) {
    char serialFullPath[64] = "/dev/";
    strcat (serialFullPath, portname);
    if ((*SerialPort = open(serialFullPath, O_RDWR|O_NONBLOCK|O_NDELAY)) < 0) {
      Ws("Couldn't open serial port "); Ws(portname); Fail(".");
    }
    struct termios config = {0};
    if (tcgetattr(*SerialPort, &config) == -1) {Close(*SerialPort); *SerialPort = 0; return;}
    config.c_cflag     = CS8 | CLOCAL;
    config.c_iflag     = IGNPAR | IGNBRK;
    config.c_oflag     = 0;
    config.c_lflag     = 0;
    //config.c_ispeed    = baudrate;
    //config.c_ospeed    = baudrate;
    config.c_cc[VMIN]  = 0;           // Return as soon as one byte is available
    config.c_cc[VTIME] = 5;           // 0.5 seconds timeout per byte
    speed_t speed = baudrate; // Set 14400 baud
    
    if (tcsetattr(*SerialPort, TCSANOW, &config) == -1) {Close(*SerialPort); *SerialPort = 0; return;}
    if (ioctl(*SerialPort, IOSSIOSPEED, &speed) == -1) {
        printf("Error calling ioctl(..., IOSSIOSPEED, ...) %s - %s(%d).\n",
               portname, strerror(errno), errno);
    }
    
    usleep(10000); // Allow 10ms for USB to settle.
    ioctl(*SerialPort, TCFLSH, TCIOFLUSH);
  }
#else
  void MakeSerialPort(char *portname, int baudrate, FileHandle *SerialPort) {
    if ((*SerialPort = open(portname, O_RDWR/*|O_NONBLOCK|O_NDELAY*/)) < 0) {
      Ws("Couldn't open serial port "); Ws(portname); Fail(".");
    }
    struct termios2 config = {0};
    if (ioctl(*SerialPort, TCGETS2, &config)) {Close(*SerialPort); *SerialPort = 0; return;}
    config.c_cflag     = CS8 | BOTHER | CLOCAL;
    config.c_iflag     = IGNPAR | IGNBRK;
    config.c_oflag     = 0;
    config.c_lflag     = 0;
    config.c_ispeed    = baudrate;
    config.c_ospeed    = baudrate;
    config.c_cc[VMIN]  = 0;           // Return as soon as one byte is available
    config.c_cc[VTIME] = 5;           // 0.5 seconds timeout per byte
    if (ioctl(*SerialPort, TCSETS2, &config)) {Close(*SerialPort); *SerialPort = 0; return;}
    usleep(10000); // Allow 10ms for USB to settle.
    ioctl(*SerialPort, TCFLSH, TCIOFLUSH);
  }
#endif


void SerialWrite(FileHandle port, const u8 *bytes, int length) {
  Write(port, bytes, length);
}

void SerialRead(FileHandle port, u8 *buf, int len) {
  int totalRead = 0;
  do {
    int lengthRead = Read(port, buf+totalRead, len-totalRead);
    if (lengthRead == 0) {
      Ws("SerialRead expected ");
      Wd(len,1); Ws(" bytes, received ");
      Wd(totalRead,1); Ws(" bytes ");
      if (totalRead) {Wl(); for (int i=0; i<totalRead; i++) {Wx(buf[i],2); Ws("  ");}}
      Fail("");
    }
    totalRead += lengthRead;
  } while(totalRead < len);
}

void SerialBreak(FileHandle port, int period) {
if (period < 1) period = 1;
#ifdef windows
  WinOK(SetCommBreak(port));
  Sleep(period);
  WinOK(ClearCommBreak(port));
#else
  ioctl(port, TCFLSH, TCIOFLUSH);
  ioctl(port, TIOCSBRK);
  usleep(period*1000);
  ioctl(port, TIOCCBRK);
#endif

#if defined(__APPLE__)
  usleep(50*1000); //let 1st byte arrive on mac
#endif
}

void SerialDump(FileHandle port) {
  u8 byte;
  while (1) {
    if (Read(port, &byte, 1)) {Wx(byte,2); Wc(' ');}
    else                            {Wsl("."); return;}
  }
}
