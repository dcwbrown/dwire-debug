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
    	if (    (strncmp("\\Device\\Serial", name, 14) == 0)
    		  ||  (strncmp("COM", value, 3)              != 0)
    	){ // Ignore motherboard RS232 ports - we're looking for USB virtual serial ports
    	  NextUsbSerialPort(UsbSerialPortName);
    	  return;
    	}
      strncpy(UsbSerialPortName, value, 6); UsbSerialPortName[6] = 0;
    }
  }
#else
  void NextUsbSerialPort() {
  }
#endif

void ConnectDefaultDevice() {
  #ifdef windows

  #else
	#endif
}


void DeviceCommand() {

}