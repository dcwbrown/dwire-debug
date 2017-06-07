/// DeviceCommand.c


void DeviceFail(char *msg) {
  Wsl(msg);
  Wl();
  Wsl("device command format:");
  Wl();
  Wsl("  device deviceid [baud]");
  Wl();
  Wsl("where deviceid is a device name and number such as:");
  Wl();
  Wsl("  com5");
  Wsl("  usbtiny1");
  Wsl("  ttyUSB3");
  Wl();
  Wsl("The device name may be abbreviated to as little as its first letter, thus");
  Wsl("the examples above could be specified instead as c5, u1 and t3.");
  Wl();
  Wsl("The optional baud is ignored on usbtiny ports.");
  Wl();
  Wsl("Use the ls command to list available devices.");
  Wl();
  Fail("");
}


void DeviceCommand(void) {

  Sb();

  int n    = 1;
  int baud = 0;

  char devicename[32];
  Ra(devicename, sizeof(devicename));
  if (IsNumeric(NextCh())) n = ReadNumber(0);
  Sb();
  if (IsNumeric(NextCh())) baud = ReadNumber(0);

  int l = strlen(devicename);
  if (l <= 0) DeviceFail("Missing deviceid on device command.");

  // Recognise the name part of the port from as many letters of the following
  // names as we have been givien, even if it is just one.

  if (    (strncasecmp(devicename, "littlewire", l) == 0)
      ||  (strncasecmp(devicename, "digispark",  l) == 0)
      ||  (strncasecmp(devicename, "usbtinyspi", l) == 0)) {
    DwFindPort('u', n, 0);
  } else if (    (strncasecmp(devicename, "com",    l) == 0)
             ||  (strncasecmp(devicename, "ttyusb", l) == 0)) {
    DwFindPort('s', n, baud);
  } else {
    DeviceFail("Unrecognised device id.");
  }

  if (CurrentPort >= 0) {
    Ws("Connected to "); DescribePort(CurrentPort);
  } else Fail("Cannot find requested device.");
}
