#include "command.hpp"
#include "targexec.hpp"

#include <cstring>
#include <fstream>
#include <fstream>

TargExec::TargExec(const char* file) {
  // hex file line format :nnaaaattdd+cc
  // :    only non-hex character
  // nn   number of data bytes
  // aaaa address
  // tt   00 data, 01 eof
  // dd+  nn data bytes
  // cc   checksum
  struct format{};
  try {
    std::ifstream fs(file);
    if (!fs.is_open())
      throw format{};
    for (;;) {
      char line[528];
      fs.getline(line, sizeof(line));
      if (!fs.good() || line[0] != ':')
        throw format{};
      uint16_t l = strlen(line);
      if (line[l-1] == '\r')
        line[--l] == '\0';

      // convert 1, 2 or 4 digits, advance p,
      // and calculate checksum
      uint8_t checksum = 0;
      auto cnv1 = [&](char*& p)->uint8_t {
        uint8_t ret = *p >= '0' && *p <= '9' ? *p - '0'
        : *p >= 'A' && *p <= 'Z' ? *p - 'A' + 10
        : *p - 'a' + 10;
        ++p; return ret;
      };
      auto cnv2 = [&](char*& p)->uint8_t {
        uint8_t ret = cnv1(p) << 4;
        ret |= cnv1(p);
        checksum += ret;
        return ret;
      };
      auto cnv4 = [&](char*& p)->uint16_t {
        uint16_t ret = (uint16_t)cnv2(p) << 8;
        ret |= cnv2(p);
        return ret;
      };

      char* p = line + 1;
      uint8_t n = cnv2(p);
      if (l != (uint16_t)n * 2 + 11)
        throw format{};
      uint16_t a = cnv4(p);
      uint8_t t = cnv2(p);
      if (t == 1) {
        cnv2(p);
        if (checksum)
          throw format{};
        break;
      }
      if (t != 0)
        throw format{};
      if (data.blob && data.load_address + data.load_length != a)
        // not contiguous
        throw format{};
      if (!data.blob || data.load_length + n > data.buffer_size) {
        if (!data.blob) {
          // allocate initial buffer
          data.load_address = a;
          data.load_length = 0;
          data.blob = (uint8_t*)malloc(data.buffer_size = 8192);
        }
        else
          // grow by factor of 2
          data.blob = (uint8_t*)realloc(data.blob, data.buffer_size *= 2);
        if (!data.blob)
          SysAbort("memory");
      }
      for (uint8_t i = 0; i < n; ++i)
        data.blob[data.load_length++] = cnv2(p);
      cnv2(p);
      if (checksum)
        throw format{};
    }
  } catch (format) {
    if (data.blob)
      free(data.blob);
    data.blob = nullptr;
  }
}
