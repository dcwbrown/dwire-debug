/// DeviceCommand.c

void DeviceCommand() {
  if (SerialPort) {Close(SerialPort);}
  Sb();
  if (IsNumeric(NextCh())) { // Just the port number is being specified
    #ifdef windows
      strncpy(UsbSerialPortName, "COM", 3); Rn(UsbSerialPortName+3, sizeof(UsbSerialPortName-3));
    #else
      strncpy(UsbSerialPortName, "ttyUSB", 6); Rn(UsbSerialPortName+6, sizeof(UsbSerialPortName-6));
    #endif
  } else {// The full port name is being specified
    Ran(ArrayAddressAndLength(UsbSerialPortName));
  }
  Sb();
  int baud;
  if (!IsDwEoln(NextCh())) {baud = ReadNumber(0);} else {baud = 0;}
  ConnectSerialPort(baud);
}
