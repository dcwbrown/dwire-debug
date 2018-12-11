#include "command.hpp"
#include "hostser.hpp"
#include "hostusb.hpp"
#include "targexec.hpp"
#include "usbmon.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>

void monitor() {
  switch (tolower(*opt_device)) {
  case 'u': {
    LuXactUsbRpc luXact(opt_device + 1);
    luXact.Open();

    if (opt_power)
      luXact.ResetPower();
    if (opt_reset)
      luXact.Reset();
    if (opt_set_serial)
      luXact.SetSerial(opt_set_serial);
    if (opt_nomon)
      break;

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
    break;
  }

  case 't': {
    LuXactLw luXact(opt_device + 1);
    if (!luXact.Open()) {
      std::cerr << "error opening device" << std::endl;
      break;
    }

    if (opt_power)
      luXact.ResetPower();
    if (opt_reset)
      luXact.Reset();
    if (opt_set_serial)
      luXact.SetSerial(opt_set_serial);
    if (opt_nomon)
      break;

    if (!luXact.ResetDw()) {
      std::cerr << "error connecting to device" << std::endl;
      break;
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
    break;
  }

  case 's': {
    LuXactSer luXact(opt_device + 1);
    if (!luXact.Open()) {
      std::cerr << "error opening device" << std::endl;
      break;
    }

    if (opt_power)
      luXact.ResetPower();
    if (opt_reset)
      luXact.Reset();
    if (opt_set_serial)
      luXact.SetSerial(opt_set_serial);
    if (opt_nomon)
      break;

    if (!luXact.ResetDw()) {
      std::cerr << "error connecting to device" << std::endl;
      break;
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
    break;
  }

  }
}

void load(const char* file) {

  TargExec targExec(file);
  if (!targExec.data.blob) {
    std::cerr << "no data" << std::endl;
    return;
  }
  for (uint8_t* p = targExec.data.blob; p < targExec.data.blob + targExec.data.load_length; ) {
    printf("%02X", *p++);
    if (!((p - targExec.data.blob) & 0xF))
      printf("\n");
  }
}

  class Label : public std::string {

  public:

    // don't hide assignment operator
    using std::string::operator=;

    // add blank segments to end
    void AddSegment(size_t n);

    // return string for display
    std::string Display();

    // return, set segment
    std::string Segment(size_t n);
    void Segment(size_t n, std::string v);

    // return position, length of segment
    size_t SegmentPos(size_t n);
    size_t SegmentLen(size_t pos);

  } label;

void Label::AddSegment(size_t n) {
  for (size_t i = 0; i < n; ++i)
    push_back(0);
}

std::string Label::Display() {
  std::string ret{*this};
  for (size_t pos = 0; (pos = ret.find_first_of((char)0, pos)) != std::string::npos;) {
    char space = ' ';
    ret.replace(pos++, 1, &space, 1);
  }
  return ret;
}

std::string Label::Segment(size_t n) {
  size_t pos = SegmentPos(n);
  //  std::cerr << "pos:" << pos << '\n';
  if (pos == std::string::npos)
    return std::string{};
  size_t len = SegmentLen(pos);
  //std::cerr << "len:" << len << '\n';
  return substr(pos, len);
}

void Label::Segment(size_t n, std::string v) {
  size_t pos = SegmentPos(n);
  if (pos == std::string::npos)
    return;
  size_t len = SegmentLen(pos);
  replace(pos, len, v);
}

size_t Label::SegmentPos(size_t n) {
  size_t pos = 0;
  for (size_t i = 0; i < n; ++i) {
    pos = find_first_of((char)0, pos);
    if (pos == std::string::npos)
      return pos;
    ++pos;
  }
  return pos;
}

size_t Label::SegmentLen(size_t pos) {
  size_t len = find_first_of((char)0, pos);
  return len == std::string::npos ? len : len -= pos;
}

void test() {

    LuXactSer luXact("");
    if (!luXact.Open()) {
      std::cerr << "error opening device" << std::endl;
      exit(0);
    }

    luXact.Debug();

  exit(0);
}
