#include "command.hpp"
#include "hostusb.hpp"
#include "usbmon.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>

void reset() {
  LuXactUsb luXactUsb(serialnumber);
  if (!luXactUsb.Open(false)) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  luXactUsb.Reset();
}

void power() {
  LuXactDw luXactDw(serialnumber);
  if (!luXactDw.Open()) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  luXactDw.ResetPower();
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
  LuXactDw luXact(serialnumber);
  if (!luXact.Open() || !luXact.ResetDw()) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  luXact.Sink(SysConWrite);
  luXact.ReadDw();
  while (!SysConAvail()) {
    luXact.Recv();
    usleep(100 * 1000);
  }
}

void change_serialnumber(const char* new_serialnumber) {
  LuXactDw luXactDw(serialnumber);
  if (!luXactDw.Open()) {
    std::cerr << "error opening device" << std::endl;
    return;
  }

  char16_t* serial = luXactDw.Serial(new_serialnumber);
  if (luXactDw.Xfer
      (55/*Change serial number*/,
       ((uint32_t)serial[0]&0xFF)
       |(((uint32_t)serial[1]&0xFF)<<8)
       |(((uint32_t)serial[2]&0xFF)<<16)) < 0) {
    std::cerr << "error changing serial number" << std::endl;
    return;
  }
}

void test() {
  LuXactDw luXactDw(serialnumber);
  if (!luXactDw.Open())
    return;

  if (!luXactDw.ResetDw())
    return;

  for (;;) {
    while (!SysConAvail()) { }
    SysConRead();
    luXactDw.Send(0x55);
  }
}
