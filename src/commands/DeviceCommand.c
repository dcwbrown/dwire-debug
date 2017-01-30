/// DeviceCommand.c

void DeviceCommand() {
  if (SerialPort) {Close(SerialPort);}
  Sb();
  Ran(ArrayAddressAndLength(UsbSerialPortName));
  Sb();
  int baud;
  if (!IsDwEoln(NextCh())) {baud = ReadNumber(0);} else {baud = 0;}
  ConnectSerialPort(baud);
}
