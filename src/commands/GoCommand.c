// GoCommand.c



//void DeviceBreak() {
//  Wsl("\rDevice reached breakpoint.                                        ");
//  DwSync();
//  DwReconnect();
//}

void KeyboardBreak() {
  Ws("Keyboard requested break. "); SkipEoln();
  DwBreakAndSync();
  DwReconnect();
}



#ifdef windows

  void GoWaitLoop(FileHandle fd) {
    while (1) {
      //  // See if there's anything ready at the serial port
      //  u8 ch;
      //  int lengthRead = Read(SerialPort, &ch, 1);  // Note - waits for the comm timeout
      //  if (lengthRead) {
      //    // Device has hit a breakpoint
      //    Assert(ch == 0);
      //    DeviceBreak();
      //    break;
      //  }

      if (dwReachedBreakpoint()) {
        Wsl("\rDevice reached breakpoint.                                        ");
        DwReconnect();
        break;
      }

      // See if there's a user pressing a key
      if (Interactive(Input)) {
        DWORD bytesAvailable;
        if (PeekNamedPipe(Input, 0,0,0, &bytesAvailable, 0)) {
          if (bytesAvailable) {
            KeyboardBreak(); break; // Running under cygwin or through other sort of pipe
          }
        } else {
          if (WaitForSingleObject(Input, 0) == WAIT_OBJECT_0) {
            KeyboardBreak(); break; // Running in console
          }
        }
      }
    }
  }

#else

  void GoWaitLoop(FileHandle fd) {
    fd_set readfds;
    fd_set excpfds;
    struct timeval timeout;
    while (1) {
      FD_ZERO(&readfds);
      FD_ZERO(&excpfds);
      FD_SET(fd, &readfds);  // either stdin or GDB command socket
      FD_SET(SerialPort, &readfds);
      timeout = (struct timeval){10,0}; // 10 seconds
      if(select(max(fd, SerialPort)+1, &readfds, 0, &excpfds, &timeout) > 0) {
        // Something became available
        //if (FD_ISSET(SerialPort, &readfds)) {DeviceBreak();   break;}
        if (FD_ISSET(fd,         &readfds)) {KeyboardBreak(); break;}
        if (FD_ISSET(fd,         &excpfds)) {KeyboardBreak(); break;}
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
  if (Interactive(Input)) {Ws("Press return key to force break. Waiting ..."); Flush();}


  // Wait for either serial port or keyboard input
  GoWaitLoop(Input);
}
