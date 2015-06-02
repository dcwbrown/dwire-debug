// GoCommand.c



void DeviceBreak() {
  Wsl(" Device reached breakpoint.");
  DwSync();
  DwReconnect();
}

void KeyboardBreak() {
  Wsl("Keyboard requested break."); SkipEoln();
  DwBreak(); DwReconnect();
}



#ifdef windows

  void WindowsGoWaitLoop() {
    while (1) {
      // See if there's anything ready at the serial port
      u8 ch;
      int lengthRead = Read(SerialPort, &ch, 1);  // Note - waits for the comm timeout
      if (lengthRead) {
        // Device has hit a breakpoint
        Assert(ch == 0);
        DeviceBreak();
        break;
      }
      // See if there's a user pressing a key
      if (IsUser(Input)) {
        DWORD bytesAvailable;
        WinOK(PeekNamedPipe(Input, 0,0,0, &bytesAvailable, 0));
        if (bytesAvailable) {
          KeyboardBreak();
          break;
        }
      }
    }
  }

#else

  #include <sys/select.h>
  void LinuxGoWaitLoop() {
    fd_set readfds;
    struct timeval timeout;
    while (1) {
      FD_ZERO(&readfds);
      FD_SET(0, &readfds);  // stdin
      FD_SET(SerialPort, &readfds);
      timeout = (struct timeval){10,0}; // 10 seconds
      if(select(SerialPort+1, &readfds, 0,0, &timeout) > 0) {
        // Something became available
        if (FD_ISSET(SerialPort, &readfds)) {DeviceBreak();   break;}
        if (FD_ISSET(0,          &readfds)) {KeyboardBreak(); break;}
      } else {
        Ws("."); Flush();
      }
    }
  }

#endif





void GoCommand() {
  DwGo();

  // Wait for input from either the serial port or the keyboard.

  Ws("Running. ");
  if (IsUser(Input)) {Ws("Press return key to force break. Waiting ..."); Flush();}


  // Wait for either serial port or keyboard input

  #ifdef windows
    WindowsGoWaitLoop();
  #else
    LinuxGoWaitLoop();
  #endif
}
