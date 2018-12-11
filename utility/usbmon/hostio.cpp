#include "hostio.hpp"

#if __GNUC__ > 4
#include <codecvt>
#endif
#include <iostream>
#include <locale>
#include <string.h>
#include <unistd.h>

void LuXact::Label()
{
  // only output label to tty
  if (!isatty(fileno(stdout)) || !isatty(fileno(stderr))) return;

  std::cerr << "\e]0;";
  if (!IsOpen()) std::cerr << '[';
  std::cerr << label.Display();
  if (!IsOpen()) std::cerr << ']';
  std::cerr << '\a';
}

bool LuXact::Reset() {
  return false;
};

bool LuXact::ResetDw() {
  return false;
};

bool LuXact::ResetPower() {
  return false;
};

bool LuXact::SetSerial(const char* serial) {
  return false;
}

void LuXact::Label::AddSegment(size_t n) {
  for (size_t i = 0; i < n; ++i)
    push_back(0);
}

std::string LuXact::Label::Display() {
  std::string ret{*this};
  for (size_t pos = 0; (pos = ret.find_first_of((char)0, pos)) != std::string::npos;) {
    char space = ' ';
    ret.replace(pos++, 1, &space, 1);
  }
  return ret;
}

std::string LuXact::Label::Segment(size_t n) {
  size_t pos = SegmentPos(n);
  //  std::cerr << "pos:" << pos << '\n';
  if (pos == std::string::npos)
    return std::string{};
  size_t len = SegmentLen(pos);
  //std::cerr << "len:" << len << '\n';
  return substr(pos, len);
}

void LuXact::Label::Segment(size_t n, std::string v) {
  size_t pos = SegmentPos(n);
  if (pos == std::string::npos)
    return;
  size_t len = SegmentLen(pos);
  replace(pos, len, v);
}

size_t LuXact::Label::SegmentPos(size_t n) {
  size_t pos = 0;
  for (size_t i = 0; i < n; ++i) {
    pos = find_first_of((char)0, pos);
    if (pos == std::string::npos)
      return pos;
    ++pos;
  }
  return pos;
}

size_t LuXact::Label::SegmentLen(size_t pos) {
  size_t len = find_first_of((char)0, pos);
  return len == std::string::npos ? len : len -= pos;
}

std::string LuXact::to_bytes(const char16_t* wptr) {
  if (!wptr) return std::string{};
#if __GNUC__ > 4
  // convert utf16 to utf8
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
#else
  // this only handles ASCII
  struct { std::string (*to_bytes)(const char16_t* p); } convert;
  convert.to_bytes = [&](const char16_t* p)->std::string {
    std::string r;
    for (; *p; ++p)
      r += (char)(*p & 0xFF);
    return r; };
#endif
  return convert.to_bytes(wptr);
}
