/// DigiSpark.c - Low level interface to digispark running as little-wire with gebugWIRE support

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


void PortFail(char *msg) {usb_close(DigiSparkPort); DigiSparkPort = 0; Fail(msg);}

static uint32_t cyclesPerPulse;

int SetDwireBaud() { // returns 1 iff success
  int status;
  int tries;
  int i;
  uint16_t times[64];

  tries = 0;  status = 0;
  while ((tries < 5)  &&  (status <= 0)) {
    tries++;
    delay(20);
    // Read back timings
    status = usb_control_msg(DigiSparkPort, IN_FROM_LW, 60, 0, 0, (char*)times, sizeof(times), USB_TIMEOUT);
    //Ws("Read back timimgs status: "); Wd(status,1); Wsl(".");
  }
  if (status < 18) {return 0;}
  uint32_t measurementCount = status / 2;

  //Ws("  Pulse times:");
  //for (i=0; i<measurementCount; i++) {Wc(' '); Wd(times[i],1);}
  //Wsl(".");

  // Average measurements and determine baud rate as pulse time in device cycles.

  cyclesPerPulse = 0;
  for (i=measurementCount-9; i<measurementCount; i++) cyclesPerPulse += times[i];

  // Pulse cycle time for each measurement is 6*measurement + 8 cyclesPerPulse.
  cyclesPerPulse = (6*cyclesPerPulse) / 9  +  8;

  // Determine timing loop iteration counts for sending and receiving bytes

  times[0] = (cyclesPerPulse-8)/4;  // dwBitTime

  // Send timing parameters to digispark
  status = usb_control_msg(DigiSparkPort, OUT_TO_LW, 60, 2, 0, (char*)times, 2, USB_TIMEOUT);
  if (status < 0) {PortFail("Failed to set debugWIRE port baud rate");}

  return 1;
}




void DigiSparkBreakAndSync() {
  for (int tries=0; tries<25; tries++) {
    // Tell digispark to send a break and capture any returned pulse timings
    //Wsl("Commanding digispark break and capture.");
    int status = usb_control_msg(DigiSparkPort, OUT_TO_LW, 60, 33, 0, 0, 0, USB_TIMEOUT);
    if (status < 0) {
      if (status == -1) {
        Wsl("Access denied sending USB message to DigiSpark. Run dwdebug as root, or add a udev rule such as");
        Wsl("file /etc/udev/rules.d/60-usbtiny.rules containing:");
        Wsl("SUBSYSTEM==\"usb\", ATTR{idVendor}==\"1781\", ATTR{idProduct}==\"0c9f\", GROUP=\"plugdev\", MODE=\"0666\"");
        PortFail("And make sure that you are a member of the plugdev group.");
      } else {
        Ws("Digispark did not accept 'break and capture' command. Error code "); Wd(status,1); PortFail(".");
      }
    } else {
      delay(120); // Wait while digispark sends break and reads back pulse timings
      // Get any pulse timings back from digispark
      if (SetDwireBaud()) {
        Ws("Connected at "); Wd(16500000 / cyclesPerPulse, 1); Wsl(" baud.");
        return;
      }
    }
    Wc('.'); Flush();
  }
  Wl(); PortFail("Digispark/LittleWire could not capture pulse timings after 25 break attempts.");
}




int DigiSparkReachedBreakpoint() {
  char dwBuf[10];
  int status = usb_control_msg(DigiSparkPort, IN_FROM_LW, 60, 0, 0, dwBuf, sizeof(dwBuf), USB_TIMEOUT);
  //if (status < 0) {
  //  Ws("DigiSparkReachedBreakpoint: dwBuf read returned "); Wd(status,1);
  //  Ws(", dwBuf[0] = $"); Wx(dwBuf[0],2); Wsl(".");
  //}
  return status >= 0  &&  dwBuf[0] != 0;
}


// Low level send to device.

// state = 0x04 - Just send the bytes
// state = 0x14 - Send bytes and read response bytes
// state = 0x24 - Send bytes and record response pulse widths

void digisparkUSBSendBytes(u8 state, char *out, int outlen) {

  int tries  = 0;
  int status = usb_control_msg(DigiSparkPort, OUT_TO_LW, 60, state, 0, out, outlen, USB_TIMEOUT);

  while ((tries < 50) && (status <= 0)) {
    // Wait for previous operation to complete
    tries++;
    delay(20);
    status = usb_control_msg(DigiSparkPort, OUT_TO_LW, 60, state, 0, out, outlen, USB_TIMEOUT);
  }
  if (status < outlen) {Ws("Failed to send bytes to AVR, status "); Wd(status,1); PortFail("");}
  delay(3); // Wait at least until digispark starts to send the data.
}


// Buffer accumulating debugWIRE data to be sent to the device.
// We buffer data in order to minimise the number of USB transactions used,
// but we also guarantee that a debugWIRE read transaction includes at leasr
// one byte of data to be sent first.

char OutBufBytes[128];
int  OutBufLength = 0;

void digisparkBufferFlush(u8 state) {
  if (OutBufLength > 0) {
    digisparkUSBSendBytes(state, OutBufBytes, OutBufLength);
    OutBufLength = 0;
  }
}



void DigiSparkSend(const u8 *out, int outlen) {
  while (OutBufLength + outlen > sizeof(OutBufBytes)) {
    // Total (buffered and passed here) execeeds maximum transfer length (128
    // bytes = OutBuf size). Send buffered and new data until there remains
    // between 1 and 128 bytes still to send in the buffer.
    int lenToCopy = sizeof(OutBufBytes)-OutBufLength;
    memcpy(OutBufBytes+OutBufLength, out, lenToCopy);
    digisparkUSBSendBytes(0x04, OutBufBytes, sizeof(OutBufBytes));
    OutBufLength = 0;
    out += lenToCopy;
    outlen -= lenToCopy;
  }
  Assert(OutBufLength + outlen <= sizeof(OutBufBytes));
  memcpy(OutBufBytes+OutBufLength, out, outlen);
  OutBufLength += outlen;
  // Remainder stays in buffer to be sent with next read request, or on a
  // flush call.
}


void DigiSparkFlush() {
  digisparkBufferFlush(0x14);
}


int DigiSparkReceive(u8 *in, int inlen) {
  Assert(inlen <= 128);
  int tries  = 0;
  int status = 0;

  digisparkBufferFlush(0x14);

  while ((tries < 50) && (status <= 0)) {
    tries++;
    delay(20);
    // Read back dWIRE bytes
    status = usb_control_msg(DigiSparkPort, IN_FROM_LW, 60, 0, 0, (char*)in, inlen, USB_TIMEOUT);
  }
  return status;
}


void DigiSparkSync() {
  digisparkBufferFlush(0x24);
  if (!SetDwireBaud()) {PortFail("Could not read back timings following transfer and sync command");}
}

void DigiSparkWait() {
  digisparkBufferFlush(0x0C);  // Send bytes and wait for dWIRE line state change
}





