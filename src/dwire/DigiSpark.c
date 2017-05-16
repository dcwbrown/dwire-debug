/// Digispark.c - Low level interface to digispark running as little-wire with gebugWIRE support

// DebugWire output command flag bits:
//
//     00000001    1     Send break
//     00000010    2     Set timing parameter
//     00000100    4     Send bytes
//     00001000    8     Wait for start bit
//     00010000   16     Read bytes
//     00100000   32     Read pulse widths
//
// Supported combinations
//    33 - Send break and read pulse widths
//     2 - Set timing parameters
//     4 - Send bytes
//    20 - Send bytes and read response (normal command)
//    28 - Send bytes, wait and read response (e.g. after programming, run to BP)
//    36 - Send bytes and receive 0x55 pulse widths
//
// Note that the wait for start bit loop also monitors the dwState wait for start
// bit flag, and is arranged so that sending a 33 (send break and read pulse widths)
// will abort a pending wait.



#define USB_TIMEOUT 5000
#define OUT_TO_LW   USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT
#define IN_FROM_LW  USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN
#define VENDOR_ID   0x1781
#define PRODUCT_ID  0x0c9f


void FindUsbtinys() {

  int usbtinyindex = 1;

  if (UsbInit()) {
    usb_find_busses();
    usb_find_devices();

    struct usb_bus *bus = usb_get_busses();
    while (bus) {

      struct usb_device *device = bus->devices;
      while (device) {

        if (    device->descriptor.idVendor  == VENDOR_ID
            &&  device->descriptor.idProduct == PRODUCT_ID) {
          Assert(PortCount < countof(Ports));
          Ports[PortCount] = malloc(sizeof(struct UPort));
          Assert(Ports[PortCount]);
          Ports[PortCount]->kind = 'u';
          Ports[PortCount]->index = usbtinyindex++;
          Ports[PortCount]->character = -1;              // Currently undetermined
          ((struct UPort*)Ports[PortCount])->handle = 0; // Currently unconnected
          ((struct UPort*)Ports[PortCount])->device = device;
          PortCount++;
        }

        device = device->next;
      }

      bus = bus->next;
    }
  }
}



void PortFail(char *msg) {usb_close(DigisparkPort); DigisparkPort = 0; Fail(msg);}

static uint32_t CyclesPerPulse;

int SetDwireBaud() { // returns 1 iff success
  int status;
  int tries;
  int i;
  uint16_t times[64];

  // Wsl(" -- SetDwireBaud().");
  tries = 0;  status = 0;  CyclesPerPulse = 0;

  while ((tries < 5)  &&  (status <= 0)) {
    tries++;
    delay(20);
    // Read back timings
    status = usb_control_msg(DigisparkPort, IN_FROM_LW, 60, 0, 0, (char*)times, sizeof(times), USB_TIMEOUT);
    //  Ws(" -- Read back timimgs status: "); Wd(status,1); Wsl(".");
  }
  if (status < 18) {return 0;}
  uint32_t measurementCount = status / 2;

  //  Ws(" -- Pulse times:");
  //  for (i=0; i<measurementCount; i++) {Wc(' '); Wd(times[i],1);}
  //  Wsl(".");

  // Average measurements and determine baud rate as pulse time in device cycles.

  for (i=measurementCount-9; i<measurementCount; i++) CyclesPerPulse += times[i];

  // Pulse cycle time for each measurement is 6*measurement + 8 CyclesPerPulse.
  CyclesPerPulse = (6*CyclesPerPulse) / 9  +  8;

  // Determine timing loop iteration counts for sending and receiving bytes

  times[0] = (CyclesPerPulse-8)/4;  // dwBitTime

  // Send timing parameters to digispark
  status = usb_control_msg(DigisparkPort, OUT_TO_LW, 60, 2, 0, (char*)times, 2, USB_TIMEOUT);
  if (status < 0) {PortFail("Failed to set debugWIRE port baud rate");}

  // Wsl(" -- SetDwireBaud() completed successfully.");
  return 1;
}




void DigisparkBreakAndSync() {
  // Wsl(" -- DigisparkBreakAndSync().");
  for (int tries=0; tries<25; tries++) {
    // Tell digispark to send a break and capture any returned pulse timings
    //  Wsl(" -- Commanding digispark break and capture.");
    int status = usb_control_msg(DigisparkPort, OUT_TO_LW, 60, 33, 0, 0, 0, USB_TIMEOUT);
    if (status < 0) {
      if (status == -1) {
        Wsl("Access denied sending USB message to Digispark. Run dwdebug as root, or add a udev rule such as");
        Wsl("file /etc/udev/rules.d/60-usbtiny.rules containing:");
        Wsl("SUBSYSTEM==\"usb\", ATTR{idVendor}==\"1781\", ATTR{idProduct}==\"0c9f\", GROUP=\"plugdev\", MODE=\"0666\"");
        Wsl("And make sure that you are a member of the plugdev group.");
      } else {
        Ws("Digispark did not accept 'break and capture' command. Error code "); Wd(status,1); Wsl(".");
      }
      usb_close(DigisparkPort); DigisparkPort = 0;
    } else {
      delay(120); // Wait while digispark sends break and reads back pulse timings
      if (SetDwireBaud()) {return;}
    }
    Wc('.'); Wflush();
  }
  Wl(); Wsl("Digispark/LittleWire could not capture pulse timings after 25 break attempts.");
  usb_close(DigisparkPort); DigisparkPort = 0;
}




int DigisparkReachedBreakpoint() {
  char dwBuf[10];
  int status = usb_control_msg(DigisparkPort, IN_FROM_LW, 60, 0, 0, dwBuf, sizeof(dwBuf), USB_TIMEOUT);
  //  if (status < 0) {
  //    Ws(" -- DigisparkReachedBreakpoint: dwBuf read returned "); Wd(status,1);
  //    Ws(", dwBuf[0] = $"); Wx(dwBuf[0],2); Wsl(".");
  //  }
  return status >= 0  &&  dwBuf[0] != 0;
}


// Low level send to device.

// state = 0x04 - Just send the bytes
// state = 0x14 - Send bytes and read response bytes
// state = 0x24 - Send bytes and record response pulse widths

void digisparkUSBSendBytes(u8 state, char *out, int outlen) {

  int tries  = 0;
  int status = usb_control_msg(DigisparkPort, OUT_TO_LW, 60, state, 0, out, outlen, USB_TIMEOUT);

  while ((tries < 50) && (status <= 0)) {
    // Wait for previous operation to complete
    tries++;
    delay(20);
    status = usb_control_msg(DigisparkPort, OUT_TO_LW, 60, state, 0, out, outlen, USB_TIMEOUT);
  }
  if (status < outlen) {Ws("Failed to send bytes to AVR, status "); Wd(status,1); PortFail("");}
  delay(3); // Wait at least until digispark starts to send the data.
}


// Buffer accumulating debugWIRE data to be sent to the device.
// We buffer data in order to minimise the number of USB transactions used,
// but we also guarantee that a debugWIRE read transaction includes at leasr
// one byte of data to be sent first.

char DigisparkOutBufBytes[128];
int  DigisparkOutBufLength = 0;

void digisparkBufferFlush(u8 state) {
  if (DigisparkOutBufLength > 0) {
    digisparkUSBSendBytes(state, DigisparkOutBufBytes, DigisparkOutBufLength);
    DigisparkOutBufLength = 0;
  }
}



void DigisparkSend(const u8 *out, int outlen) {
  while (DigisparkOutBufLength + outlen > sizeof(DigisparkOutBufBytes)) {
    // Total (buffered and passed here) exceeds maximum transfer length (128
    // bytes = DigisparkOutBuf size). Send buffered and new data until there remains
    // between 1 and 128 bytes still to send in the buffer.
    int lenToCopy = sizeof(DigisparkOutBufBytes)-DigisparkOutBufLength;
    memcpy(DigisparkOutBufBytes+DigisparkOutBufLength, out, lenToCopy);
    digisparkUSBSendBytes(0x04, DigisparkOutBufBytes, sizeof(DigisparkOutBufBytes));
    DigisparkOutBufLength = 0;
    out += lenToCopy;
    outlen -= lenToCopy;
  }
  Assert(DigisparkOutBufLength + outlen <= sizeof(DigisparkOutBufBytes));
  memcpy(DigisparkOutBufBytes+DigisparkOutBufLength, out, outlen);
  DigisparkOutBufLength += outlen;
  // Remainder stays in buffer to be sent with next read request, or on a
  // flush call.
}


void DigisparkFlush() {
  digisparkBufferFlush(0x14);
}


int DigisparkReceive(u8 *in, int inlen) {
  Assert(inlen <= 128);
  int tries  = 0;
  int status = 0;

  digisparkBufferFlush(0x14);

  while ((tries < 50) && (status <= 0)) {
    tries++;
    delay(20);
    // Read back dWIRE bytes
    status = usb_control_msg(DigisparkPort, IN_FROM_LW, 60, 0, 0, (char*)in, inlen, USB_TIMEOUT);
  }
  return status;
}


void DigisparkSync() {
  digisparkBufferFlush(0x24);
  if (!SetDwireBaud()) {PortFail("Could not read back timings following transfer and sync command");}
}

void DigisparkWait() {
  digisparkBufferFlush(0x0C);  // Send bytes and wait for dWIRE line state change
}

void SelectUsbtinyHandlers() {
  DwBreakAndSync      = DigisparkBreakAndSync;
  DwReachedBreakpoint = DigisparkReachedBreakpoint;
  DwSend              = DigisparkSend;
  DwFlush             = DigisparkFlush;
  DwReceive           = DigisparkReceive;
  DwSync              = DigisparkSync;
  DwWait              = DigisparkWait;
}


void ConnectUsbtinyPort(struct UPort *p) {
  Assert(p->port.kind == 'u');
  SelectUsbtinyHandlers();
  if (!p->handle) {
    p->handle = usb_open(p->device);
    if (!p->handle) Fail("Could not open usbisp/littlewire/digispark port.");
  }
  DigisparkPort = p->handle;
  DigisparkBreakAndSync();
  if (!DigisparkPort) p->port.kind = 0; // Couldn't use this port
}