#include "command.hpp"
#include "hostusb.hpp"
#include "usbmon.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>

void reset() {
  LuXactUsb luXactUsb(serialnumber);
  if (!luXactUsb.Open()) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  luXactUsb.Reset();
}

void power() {
  LuXactLw luXactLw(serialnumber);
  if (!luXactLw.Open()) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  luXactLw.ResetPower();
}

void monitor() {
  LuXactUsb luXact(serialnumber);
  luXact.Open();

  luXact.Sink(SysConWrite);
  for (;;) {
    luXact.Recv();
    if (SysConAvail()) {
      int c = SysConRead();
      if (c <= 0) continue;
      luXact.Send(c);
      continue;
    }
    usleep(100 * 1000);
  }
}

void monitor_dw() {
  LuXactLw luXact(serialnumber);
  if (!luXact.Open() || !luXact.ResetDw()) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  luXact.Sink(SysConWrite);
  for (;;) {
    luXact.Recv();
    if (SysConAvail()) {
      int c = SysConRead();
      if (c <= 0) continue;
      luXact.Send(c);
      continue;
    }
    usleep(100 * 1000);
  }
}

void change_serialnumber(const char* new_serialnumber) {
  LuXactLw luXactLw(serialnumber);
  if (!luXactLw.Open()) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  char16_t* serial = luXactLw.Serial(new_serialnumber);
  if (luXactLw.Xfer
      (55/*Change serial number*/,
       ((uint32_t)serial[0]&0xFF)
       |(((uint32_t)serial[1]&0xFF)<<8)
       |(((uint32_t)serial[2]&0xFF)<<16)) < 0) {
    std::cerr << "error changing serial number" << std::endl;
    return;
  }
}

void test() {
}
