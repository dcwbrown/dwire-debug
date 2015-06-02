/// DeviceCommand.c



#ifdef windows

  char *DosDevices = 0;
  char *CurrentDevice = 0;

  void LoadDosDevices() {
    if (DosDevices == (char*)-1) {CurrentDevice=0; return;}
    int size = 24576;
    int used = 0;
    while (1) {
      DosDevices = Allocate(size);
      used = QueryDosDevice(0, DosDevices, size);
      if (used) {break;}
      int error = GetLastError();
      Free(DosDevices);
      if (error != ERROR_INSUFFICIENT_BUFFER) {break;}
      size *= 2;
    }
    if (used) {CurrentDevice = DosDevices;}
  }

  void AdvanceCurrentDevice() {while (*CurrentDevice) {CurrentDevice++;} CurrentDevice++;}

  void NextUsbSerialPort() {
    if (!DosDevices) {LoadDosDevices();}
    while (CurrentDevice  &&  strncmp("COM", CurrentDevice, 3)) {AdvanceCurrentDevice();}
    if (CurrentDevice) {
      strncpy(UsbSerialPortName, CurrentDevice, 6);
      UsbSerialPortName[6] = 0;
      AdvanceCurrentDevice();
    } else {
      UsbSerialPortName[0] = 0;
    }
  }

#else

  #include <dirent.h>

  DIR *DeviceDir = 0;

  void NextUsbSerialPort() {
    UsbSerialPortName[0] = 0;
    if (!DeviceDir) {DeviceDir = opendir("/dev");
    if (!DeviceDir) {return;}}
    while (!UsbSerialPortName[0]) {
      struct dirent *entry = readdir(DeviceDir);
      if (!entry) {closedir(DeviceDir); DeviceDir=0; return;}
      if (!strncmp("ttyUSB", entry->d_name, 6))
        {strncpy(UsbSerialPortName, entry->d_name, 255); UsbSerialPortName[255] = 0;}
    }
  }

#endif






void TryConnectSerialPort() {
  jmp_buf oldFailPoint;
  memcpy(oldFailPoint, FailPoint, sizeof(FailPoint));

  if (setjmp(FailPoint)) {
    SerialPort = 0;
  } else {
    MakeSerialPort(UsbSerialPortName, 1000000/128);
    if (!SerialPort) {return;}
    DwBreak();
    DwConnect();
  }

  memcpy(FailPoint, oldFailPoint, sizeof(FailPoint));
}




// Implement ConnectSerialPort called from DwPort.c
void ConnectSerialPort() {
  SerialPort = 0;
  if (UsbSerialPortName[0]) {
    TryConnectSerialPort();
    if (!SerialPort) {Ws("Couldn't connect to DebugWIRE device on "); Fail(UsbSerialPortName);}
  } else {
    while (!SerialPort) {
      NextUsbSerialPort();
      if (!UsbSerialPortName[0]) {Fail("Couldn't find a DebugWIRE device.");}
      TryConnectSerialPort();
    }
  }
}




void DeviceCommand() {
  if (SerialPort) {CloseSerialPort();}
  Sb();
  Ran(ArrayAddressAndLength(UsbSerialPortName));
  ConnectSerialPort();
}