struct SPort {  // Serial port
  struct Port port;
  char        portname[32];
  FileHandle  handle;
};


//  // Debugging
//
//  void WriteSPort(struct SPort *sp) {
//    Ws("SPort: kind "); Wc(sp->port.kind);
//    Ws(", index ");     Wd(sp->port.index,1);
//    Ws(", character "); Wd(sp->port.character,1);
//    Ws(", baud ");      Wd(sp->port.baud,1);
//    Ws(", handle $");   Wx((int)sp->handle,1);
//    Ws(", portname ");  Ws(sp->portname); Wsl(".");
//  }



#ifdef windows

  char *LoadDosDevices(void) {
    char *DosDevices = 0;
    int   size       = 24576;
    int   used       = 0;
    while (1) {
      DosDevices = Allocate(size);
      used = QueryDosDevice(0, DosDevices, size);
      if (used) {break;}
      int error = GetLastError();
      Free(DosDevices);
      DosDevices = 0;
      if (error != ERROR_INSUFFICIENT_BUFFER) {break;}
      size *= 2;
    }
    return DosDevices;
  }

  void FindSerials(void) {
    char *DosDevices = LoadDosDevices();
    char *CurrentDevice = DosDevices;
    struct SPort *p;

    while (*CurrentDevice) {
      if (strncmp("COM", CurrentDevice, 3) == 0) {
        Assert(PortCount < countof(Ports));
        Assert((p = malloc(sizeof(*p))));

        p->port.kind      = 's';
        p->port.index     = strtol(CurrentDevice+3, 0, 10);
        p->port.character = -1;              // Currently undetermined
        p->port.baud      = 0;               // Currently undetermined
        p->handle = 0; // Currently unconnected
        snprintf(p->portname, sizeof(p->portname), "COM%d", p->port.index);

        Ports[PortCount] = (struct Port *)p;
        PortCount++;
      }
      while (*CurrentDevice) {CurrentDevice++;}  // Skip over device string
      CurrentDevice++;                           // SKip over one zero terminator
    }

    Free(DosDevices);
  }
  
#elif defined(__APPLE__)  

  void FindSerials(void) {
    DIR *DeviceDir = opendir("/dev");
    if (!DeviceDir) return;

    struct dirent *entry = 0;
    struct SPort *p;
    while ((entry = readdir(DeviceDir))) {
      if (!strncmp("tty.usbserial", entry->d_name, 6)) {
        Assert((p = malloc(sizeof(struct SPort))));

        p->port.kind      = 's';
        p->port.index     = strtol(entry->d_name+14, 0, 16);
        p->port.character = -1;   // Currently undetermined
        p->port.baud      = 0;    // Currently undetermined
        p->handle         = 0;    // Currently unconnected
        snprintf(p->portname, sizeof(p->portname), "%s", entry->d_name);

        Ports[PortCount] = (struct Port *)p;
        PortCount++;
      }
    }
    closedir(DeviceDir);
  }

#else

  void FindSerials(void) {
    DIR *DeviceDir = opendir("/dev");
    if (!DeviceDir) return;

    struct dirent *entry = 0;
    struct SPort *p;
    while ((entry = readdir(DeviceDir))) {
      if (!strncmp("ttyUSB", entry->d_name, 6)) {
        Assert((p = malloc(sizeof(struct SPort))));

        p->port.kind      = 's';
        p->port.index     = strtol(entry->d_name+6, 0, 10);
        p->port.character = -1;   // Currently undetermined
        p->port.baud      = 0;    // Currently undetermined
        p->handle         = 0;    // Currently unconnected
        snprintf(p->portname, sizeof(p->portname), "/dev/ttyUSB%d", p->port.index);

        Ports[PortCount] = (struct Port *)p;
        PortCount++;
      }
    }
    closedir(DeviceDir);
  }

#endif


void SerialSendBytes(struct SPort *port, const u8 *out, int outlen) {
  Write(port->handle, out, outlen);
  // Since txd and rxd share the same line, everything we transmit will turn
  // up in the receive buffer (unless there is a collision). Drain this
  // echoed input and check that it has not been changed.
  u8 actual[outlen];
  #if defined(__APPLE__)
  usleep(50*1000); //let byte arrive on mac
  #endif
  SerialRead(port->handle, actual, outlen);
  for (int i=0; i<outlen; i++) {
    if (actual[i] != out[i]) {
      Ws("WriteDebug, byte "); Wd(i+1,1); Ws(" of "); Wd(outlen,1);
      Ws(": Read "); Wx(actual[i],2); Ws(" expected "); Wx(out[i],2); Wl(); Fail("");
    }
  }
}

// Buffer accumulating debugWIRE data to be sent to the device to minimize
// the number of USB transactions used,

u8   SerialOutBufBytes[256];
int  SerialOutBufLength = 0;

void SerialFlush(struct SPort *port) {
  if (SerialOutBufLength > 0) {
    SerialSendBytes(port, SerialOutBufBytes, SerialOutBufLength);
    SerialOutBufLength = 0;
  }
}

void SerialSend(struct SPort *sp, const u8 *out, int outlen) {
  while (SerialOutBufLength + outlen >= sizeof(SerialOutBufBytes)) {
    // Total (buffered and passed here) meets or exceeds SerialOutBuf size.
    // Send buffered and new data until there remains between 0 and 127
    // bytes still to send in the buffer.
    int lenToCopy = sizeof(SerialOutBufBytes)-SerialOutBufLength;
    memcpy(SerialOutBufBytes+SerialOutBufLength, out, lenToCopy);
    SerialSendBytes(sp, SerialOutBufBytes, sizeof(SerialOutBufBytes));
    SerialOutBufLength = 0;
    out += lenToCopy;
    outlen -= lenToCopy;
  }
  Assert(SerialOutBufLength + outlen <= sizeof(SerialOutBufBytes));
  memcpy(SerialOutBufBytes+SerialOutBufLength, out, outlen);
  SerialOutBufLength += outlen;
  // Remainder stays in buffer to be sent with next read request, or on a
  // SerialFlush call.
}



int SerialReceive(struct SPort *sp, u8 *in, int inlen) {
  SerialFlush(sp);
  SerialRead(sp->handle, in, inlen);
  return inlen;
}




int MaybeReadByte(struct SPort *port) {
  u8 byte = 0;
  int bytesRead;

  bytesRead = Read(port->handle, &byte, 1);
  if (bytesRead == 1) return byte; else return -1;
}


int GetSyncByte(struct SPort *sp, int verbose) {
  int byte  = 0;

  byte = MaybeReadByte(sp);
  if (byte < 0) {
    Close(sp->handle);
    sp->handle = 0;
    sp->port.baud = -1;
    Fail("Expecting break byte 0x00, but no bytes read.");
  }
  else if (byte != 0) {Ws("Warning, expected to read zero byte after break, but got $"); Wx((u8)byte,2); Fail(".");}

  if (verbose) Ws(", skipping [");
  if    (byte == 0)    {if (verbose) Wc('0'); byte = MaybeReadByte(sp);} // Skip zero bytes generated by break.
  if    (byte == 0)    {if (verbose) Wc('0'); byte = MaybeReadByte(sp);} // Skip zero bytes generated by break.
  if    (byte == 0)    {if (verbose) Wc(']'); return 0;}                   // This many zeroes means we're very much faster than the chip
  while (byte == 0xFF) {if (verbose) Wc('F'); byte = MaybeReadByte(sp);} // Skip 0xFF bytes generated by line going high following break.
  if (verbose) Ws("]");

  return byte;
}



void Wbits(int byte) {
  if (byte < 0) {Wd(byte,1);}
  else for (int i=7; i>=0; i--) {Wc(((byte >> i) & 1) ? '1' : '0');}
}




int scaleby(int byte) {
  // Return amount to adjust baudrate by before the next test.
  // The scale is returned as a percentage.
  // E.g. Returns 95 to suggest a reduction by 5%.
  // Returns 100 only when the byte is exactly as expected.

  if (byte == 0x55) return 100;
  if (byte == 0   ) return 20;

  int lengthcount[9] = {0}; // Number of runs of each length

  int i = 7;
  while (((byte >> i) & 1) && (i > 1)) i--;  // Ignore leading ones as they likely come from preceeding stop bits.

  int remaining = i+1;      // Number of remaining bits we have to work with.

  int c = (byte >> i) & 1;  // Current bit value
  int s = i;                // Start of current run
  while (i > 0) {
    i--;
    int b = (byte >> i) & 1;
    if (b != c) {           // Change of line polarity
      lengthcount[s-i]++;
      c = b;
      s = i;
    }
  }
  lengthcount[s+1]++; // Length of final run

  if (Verbose) {
    Ws(": "); Wd(remaining,1); Wc(' ');
    for (int j=0; j<9; j++) Wd(lengthcount[j],1);
  }

  // If all of the remaining bits are 0, it means the first '0' bit of the
  // actual 01010101 response is at least that wide at the current baud clock.

  int remainingmask = (1<<(remaining))-1;

  if ((byte & remainingmask) == 0) // Other than leading '1's, all bits are '0'
    switch(remaining) {
      case 1:  return 20;
      case 2:  return 20;
      case 3:  return 20;
      case 4:  return 30;
      case 5:  return 40;
      case 6:  return 60;
      default: return 95;
    }

  // Now interpret run lengths
  if (lengthcount[3] > 1) return 60; // There are at least 2 runs 3 bits wide
  if (lengthcount[2] > 1) return 85; // There are at least 2 runs 2 bits wide
  if (lengthcount[1] > 1) return 95; // at least two runs just 1 bit long, we're very close.
  if (lengthcount[4])     return 40; // There is a 4 bit run
  if (lengthcount[5])     return 30; // There is a 6 bit run
  if (lengthcount[6])     return 25; // There is a 7 bit run
  if (lengthcount[7])     return 25; // There is a 8 bit run

  return 95;  // Only short runs, we're very close
}




int TryBaudRate(struct SPort *port, int baudrate, int breaklength) {
  // Returns: > 100 - approx factor by which rate is too high (as multiple of 10)
  //          = 100 - rate generates correct 0x55 response
  //          = 0   - port does not support this baud rate

  if (Verbose) {
    Ws("Trying ");          Ws(port->portname);
    Ws(", baud rate ");     Wd(baudrate, 6);
    Ws(", break length ");  Wd(breaklength, 4);
  } else {
    Wc('.');
  }
  Wflush();
  MakeSerialPort(port->portname, baudrate, &port->handle);
  if (!port->handle) {
    Vsl(". Cannot set this baud rate, probably not an FT232.");
    return 0;
  }

  SerialBreak(port->handle, breaklength);
  int byte = GetSyncByte(port, Verbose);

  Close(port->handle);
  port->handle = 0;

  if (byte < 0) {
    Fail(" No response, no debugWire device detected."); return 0;
  } else if (Verbose) {
    Ws(", received "); Wbits(byte);
  }

  int scale = scaleby(byte);
  if (scale == 100) Vsl(": expected result.");
  return scale;
}




int FindBaudRate(struct SPort *port) {

  int baudrate = 150000; // Start well above the fastest dwire baud rate based
                         // on the max specified ATtiny clock of 20MHz.
  int breaklength = 50;  // 50ms allows for clocks down to 320KHz.
                         // For 8 MHz break len can be as low as 2ms.

  // First try lower and lower clock rates until we get a good result.
  // The baud rate for each attempt is based on a rough measurement of
  // the relative size of pulses seen in the byte returned after break.

  int scale = TryBaudRate(port, baudrate, breaklength);
  if (scale == 0) {return 0;}

  while (scale != 100) {
    Vs(", scale "); Vd(scale,1); Vsl("%");
    baudrate = (baudrate * scale) / 100;
    scale = TryBaudRate(port, baudrate, breaklength);
  }

  if (scale == 0) return 0;

  // We have hit a baudrate that returns the right result this one time.
  // Now find a lower and upper bound of working rates in order to
  // finally choose the middle rate.

  breaklength = 100000 / baudrate; // Now we have the approx byte len we can use a shorter break.
  if (breaklength < 2) breaklength = 2;

  Vsl("Finding upper bound.");
  int upperbound = baudrate;
  do {
    int trial = (upperbound * 102) / 100;
    scale = TryBaudRate(port, trial, breaklength);
    if (scale == 100) upperbound = trial;
  } while (scale == 100);
  Vl();

  Vsl("Finding lower bound.");
  int lowerbound = baudrate;
  do {
    int trial = (lowerbound * 98) / 100;
    scale = TryBaudRate(port, trial, breaklength);
    if (scale == 100) lowerbound = trial;
  } while (scale == 100);
  Vl();

  // Return baud rate in the middle of the working range.

  return (lowerbound + upperbound) / 2;
}




void TryConnectSerialPort(struct SPort *sp) {
  jmp_buf oldFailPoint;
  memcpy(oldFailPoint, FailPoint, sizeof(FailPoint));

  if (sp->handle) {Close(sp->handle);}
  sp->handle = 0;

  if (setjmp(FailPoint)) {
    sp->handle = 0;
    sp->port.baud   = (sp->port.baud > 0) ? 0 : -1;
  } else {
    if (sp->port.baud <= 0) {sp->port.baud = FindBaudRate(sp);}
    int byte;
    if (sp->port.baud <= 0) {
      byte = 0;
    } else {
      MakeSerialPort(sp->portname, sp->port.baud, &sp->handle);
      int breaklength = 100000 / sp->port.baud; if (breaklength < 2) breaklength = 2;
      SerialBreak(sp->handle, breaklength);
      byte = GetSyncByte(sp, 0);
    }
    if (byte != 0x55) {
      Close(sp->handle);
      sp->handle = 0;
      sp->port.baud   = (sp->port.baud > 0) ? 0 : -1;
    }
  }

  memcpy(FailPoint, oldFailPoint, sizeof(FailPoint));
  // Ws(" -- TryConnectSerialPort complete. "); WriteSPort(sp);
}


int SerialReadByte(struct SPort *sp) {
  u8 byte = 0;
  int result;
  while ((result = Read(sp->handle, &byte, 1)) == 0) {Wc('n'); Wflush();}
  if (result<0) {Ws("SerialReadByte received error "); Wd(result,1); Fail(" from Read() on sp->handle.");}
  return byte;
}


void SerialSync(struct SPort *sp) {
  u8 byte = 0;

  SerialFlush(sp);
  while ((byte = SerialReadByte(sp)) == 0x00) {}    // Eat 0x00 bytes generated by line at break (0v)
  while (byte == 0xFF) {byte = SerialReadByte(sp);} // Eat 0xFF bytes generated by line going high following break.
  if (byte != 0x55) {
    Ws("Didn't receive 0x55 on reconnection, got "); Wx(byte,2); Wsl(".");
    Wsl("Clock speed may have changed, trying to re-sync.");
    Close(sp->handle);
    sp->port.baud = 0;
    TryConnectSerialPort(sp);
    if (!sp->handle) {Ws("Couldn't reconnect to DebugWIRE device on "); Fail(sp->portname);}
  }
}

void SerialBreakAndSync(struct SPort *sp) {
  Assert(SerialOutBufLength == 0);
  int breaklength = 100000 / sp->port.baud; if (breaklength < 2) breaklength = 2;
  SerialBreak(sp->handle, breaklength);
  SerialSync(sp);
}

void SerialWait(struct SPort *sp) {
  SerialFlush(sp);
}




void ConnectSerialPort(struct SPort *sp, int baud) {
  Assert(sp->port.kind == 's');

  // Ws(" -- ConnectSerialPort entry. "); WriteSPort(sp);

  Ws(sp->portname); Ws(" ");

  if (baud  &&  sp->port.baud != baud) { // Change of baud
    if (sp->handle) Close(sp->handle);
    sp->handle    = 0;
    sp->port.baud = baud;
  }

  if (sp->handle) {
    SerialBreakAndSync(sp);
  } else {
    TryConnectSerialPort(sp);
  }
  Ws("\r                                        \r");

  // Ws(" -- ConnectSerialPort complete. "); WriteSPort(sp);
}