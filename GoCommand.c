// GoCommand.c






void GoCommand() {
  SkipEoln();
  DwGo();

  // Wait for input from either the serial port or the keyboard.

  Ws("Running. Press cr to force break. Waiting ..."); Flush();


  // Wait for either serial port or keyboard input

  while (1) {
    // See if there's anything ready at the serial port
    u8 ch;
    int lengthRead = Read(SerialPort, &ch, 1);  // Note - waits for the comm timeout
    if (lengthRead) {
      // Device has hit a breakpoint
      Assert(ch == 0);
      Wsl(" Device reached break.");
      DwSync();
      DwReconnect();
      break;
    }
    // See if there's a user pressing a key
    if (IsUser(Input)) {
      DWORD bytesAvailable;
      WinOK(PeekNamedPipe(Input, 0,0,0, &bytesAvailable, 0));
      if (bytesAvailable) {
        Wsl(" Keyboard requested break.");

        DwBreak(); DwReconnect();
        break;
      }
    }
  }
}
