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


struct UPort {
  struct Port        port;
  struct usb_device *device;
  usb_dev_handle    *handle;
};


//  // Debugging
//
//  void WriteUPort(struct UPort *up) {
//    Ws("UPort: kind "); Wc(up->port.kind);
//    Ws(", index ");     Wd(up->port.index,1);
//    Ws(", character "); Wd(up->port.character,1);
//    Ws(", baud ");      Wd(up->port.baud,1);
//    Ws(", device $");   Wx((u64)up->device,1);
//    Ws(", handle $");   Wx((u64)up->handle,1); Wsl(".");
//  }



void FindUsbtinys(void) {

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
          Ports[PortCount]->kind      = 'u';
          Ports[PortCount]->index     = usbtinyindex++;
          Ports[PortCount]->character = -1;              // Currently undetermined
          Ports[PortCount]->baud      = 0;               // Currently undetermined
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



void PortFail(struct UPort *up, char *msg) {
  usb_close(up->handle);
  up->handle = 0;
  up->port.baud = -1;
  Fail(msg);
}

int SetDwireBaud(struct UPort *up) { // returns 1 iff success
  int status  = 0;
  int tries   = 0;
  int i;
  uint16_t times[64];
  uint32_t cyclesperpulse = 0;

  // Wsl(" -- SetDwireBaud entry.");

  up->port.baud = 0;

  while ((tries < 5)  &&  (status <= 0)) {
    tries++;
    delay(20);
    // Read back timings
    status = usb_control_msg(up->handle, IN_FROM_LW, 60, 0, 0, (char*)times, sizeof(times), USB_TIMEOUT);
    // Ws(" -- Read timings status: "); Wd(status,1); Wsl(".");
  }

  if (status < 18) {return 0;}
  uint32_t measurementCount = status / 2;

  // Average measurements and determine baud rate as pulse time in device cycles.

  for (i=measurementCount-9; i<measurementCount; i++) cyclesperpulse += times[i];

  // Pulse cycle time for each measurement is 6*measurement + 8 cyclesperpulse.
  cyclesperpulse = (6*cyclesperpulse) / 9  +  8;

  // Determine timing loop iteration counts for sending and receiving bytes

  times[0] = (cyclesperpulse-8)/4;  // dwBitTime

  // Send timing parameters to digispark
  status = usb_control_msg(up->handle, OUT_TO_LW, 60, 2, 0, (char*)times, 2, USB_TIMEOUT);
  if (status < 0) {PortFail(up, "Failed to set debugWIRE port baud rate");}

  up->port.baud = 16500000 / cyclesperpulse;

  // Ws(" -- SetDWireBaud complete. "); WriteUPort(up);
  return 1;
}




void DigisparkBreakAndSync(struct UPort *up) {
  // Ws(" -- DigisparkBreakAndSync entry. "); WriteUPort(up);
  for (int tries=0; tries<25; tries++) {
    // Tell digispark to send a break and capture any returned pulse timings
    int status = usb_control_msg(up->handle, OUT_TO_LW, 60, 33, 0, 0, 0, USB_TIMEOUT);
    if (status < 0) {
      if (status == -1) {
        Wsl("Access denied sending USB message to Digispark. Run dwdebug as root, or add a udev rule such as");
        Wsl("file /etc/udev/rules.d/60-usbtiny.rules containing:");
        Wsl("SUBSYSTEM==\"usb\", ATTR{idVendor}==\"1781\", ATTR{idProduct}==\"0c9f\", GROUP=\"plugdev\", MODE=\"0666\"");
        Wsl("And make sure that you are a member of the plugdev group.");
      } else {
        Ws("Digispark did not accept 'break and capture' command. Error code "); Wd(status,1); Wsl(".");
      }
      PortFail(up, "");
    } else {
      delay(120); // Wait while digispark sends break and reads back pulse timings
      if (SetDwireBaud(up)) {return;}
    }
    Wc('.'); Wflush();
  }
  Wl(); PortFail(up, "Digispark/LittleWire/UsbTiny could not capture pulse timings after 25 break attempts.");
}



int DigisparkReachedBreakpoint(struct UPort *up) {
  char dwBuf[10];
  int status = usb_control_msg(up->handle, IN_FROM_LW, 60, 0, 0, dwBuf, sizeof(dwBuf), USB_TIMEOUT);
  int reachedBreakpoint = status >= 0  &&  dwBuf[0] != 0;
  if (reachedBreakpoint) DigisparkBreakAndSync(up);  // Re-establish baud rate in case code change clock speed
  return reachedBreakpoint;
}


// Low level send to device.

// state = 0x04 - Just send the bytes
// state = 0x14 - Send bytes and read response bytes
// state = 0x24 - Send bytes and record response pulse widths

void digisparkUSBSendBytes(struct UPort *up, u8 state, char *out, int outlen) {

  int tries  = 0;
  int status = usb_control_msg(up->handle, OUT_TO_LW, 60, state, 0, out, outlen, USB_TIMEOUT);

  while ((tries < 200) && (status <= 0)) {
    // Wait for previous operation to complete
    tries++;
    delay(5);
    status = usb_control_msg(up->handle, OUT_TO_LW, 60, state, 0, out, outlen, USB_TIMEOUT);
  }
  if (status < outlen) {Ws("Failed to send bytes to AVR, status "); Wd(status,1); PortFail(up, "");}
  delay(3); // Wait at least until digispark starts to send the data.
}


// Buffer accumulating debugWIRE data to be sent to the device.
// We buffer data in order to minimise the number of USB transactions used,
// but we also guarantee that a debugWIRE read transaction includes at leasr
// one byte of data to be sent first.

char DigisparkOutBufBytes[128];
int  DigisparkOutBufLength = 0;

void digisparkBufferFlush(struct UPort *up, u8 state) {
  if (DigisparkOutBufLength > 0) {
    digisparkUSBSendBytes(up, state, DigisparkOutBufBytes, DigisparkOutBufLength);
    DigisparkOutBufLength = 0;
  }
}



void DigisparkSend(struct UPort *up, const u8 *out, int outlen) {
  while (DigisparkOutBufLength + outlen > sizeof(DigisparkOutBufBytes)) {
    // Total (buffered and passed here) exceeds maximum transfer length (128
    // bytes = DigisparkOutBuf size). Send buffered and new data until there remains
    // between 1 and 128 bytes still to send in the buffer.
    int lenToCopy = sizeof(DigisparkOutBufBytes)-DigisparkOutBufLength;
    memcpy(DigisparkOutBufBytes+DigisparkOutBufLength, out, lenToCopy);
    digisparkUSBSendBytes(up, 0x04, DigisparkOutBufBytes, sizeof(DigisparkOutBufBytes));
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


void DigisparkFlush(struct UPort *up) {
  digisparkBufferFlush(up, 0x04);
}


int DigisparkReceive(struct UPort *up, u8 *in, int inlen) {
  Assert(inlen <= 128);
  int tries  = 0;
  int status = 0;

  if (OutputPosition > 0) Wl();

  digisparkBufferFlush(up, 0x14);

  while ((tries < 200) && (status <= 0)) {
    tries++;
    delay(5);
    // Read back dWIRE bytes
    status = usb_control_msg(up->handle, IN_FROM_LW, 60, 0, 0, (char*)in, inlen, USB_TIMEOUT);
  }
  return status;
}


void DigisparkSync(struct UPort *up) {
  digisparkBufferFlush(up, 0x24);
  if (!SetDwireBaud(up)) {PortFail(up, "Could not read back timings following transfer and sync command");}
}

void DigisparkWait(struct UPort *up) {
  digisparkBufferFlush(up, 0x0C);  // Send bytes and wait for dWIRE line state change
}



void ConnectUsbtinyPort(struct UPort *up) {
  Assert(up->port.kind == 'u');

  // Ws(" -- ConnectUsbtinyPort entry. "); WriteUPort(up);

  Ws("UsbTiny"); Wd(up->port.index,1); Wc(' ');

  jmp_buf oldFailPoint;
  memcpy(oldFailPoint, FailPoint, sizeof(FailPoint));

  if (setjmp(FailPoint)) {
    // Wsl(" -- Fail caught in ConnectUsbtinyPort.");
    up->port.baud = -1;
  } else {
    up->port.baud = 0;
    if (!up->handle) {up->handle = usb_open(up->device);}
    if (!up->handle) {PortFail(up, "Couldn't open UsbTiny port.");}
    DigisparkBreakAndSync(up);
  }

  memcpy(FailPoint, oldFailPoint, sizeof(FailPoint));

  Ws("\r                                        \r");

  // Ws(" -- ConnectUsbtinyPort complete. "); WriteUPort(up);
}