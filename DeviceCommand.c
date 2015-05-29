/// DeviceCommand.c



char UsbSerialPortName[256]  = {0}; // e.g. 'COM6' or '/dev/ttyUSB0'


#ifdef windows
  int  NextUsbSerialPortIndex = 0;
  HKEY SerialComm             = 0;

  void NextUsbSerialPort() {
    UsbSerialPortName[0] = 0;
    if (NextUsbSerialPortIndex >= 0) {
    	if (!SerialComm) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM\\", 0,	KEY_READ | KEY_WOW64_64KEY, &SerialComm) != ERROR_SUCCESS)
          {Wsl("Couldn't open HKLM\\HARDWARE\\DEVICEMAP\\SERIALCOMM\\."); NextUsbSerialPortIndex = -1; return;}
      }
      char name[256];  DWORD namelen  = sizeof(name);
      char value[256]; DWORD valuelen = sizeof(value);
    	if (RegEnumValue(
    		SerialComm,
    		NextUsbSerialPortIndex++,
    		name, &namelen,
    		0,0,
    		(unsigned char*)value, &valuelen
    	  ) != ERROR_SUCCESS
    	){
    	  NextUsbSerialPortIndex = -1;
    	  RegCloseKey(SerialComm); SerialComm = 0;
    	  return;
    	}
    	if (    (strncmp("\\Device\\Serial", name, 14) == 0)  // Ignore motherboard RS232 ports - we're looking for USB virtual serial ports
    		  ||  (strncmp("COM", value, 3)              != 0)
    	){
    	  NextUsbSerialPort(UsbSerialPortName);
    	  return;
    	}
      strncpy(UsbSerialPortName, value, 6); UsbSerialPortName[6] = 0;
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
  MakeSerialPort(UsbSerialPortName, 1000000/128);
  if (!SerialPort) {return;}

  DwBreak();
  DwConnect();
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