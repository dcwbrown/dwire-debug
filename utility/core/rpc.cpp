#define RPC_DEFINE
#include "rpc.hpp"

#include <alloca.h>
#include <string.h>

void RpcSend::sendConsole(RpcConsole& rpc) {
  send(rpc, RpcConsole_index);
}

void RpcSend::print(const char s[])
{
  RPCALLOCA(RpcConsole,rpc)
  rpc.radix = 0;
  for (char* d = rpc.text;;) {
    char c = *d++ = *s;
    if (c) s++;
    if (d == rpc.text + sizeof(rpc.text)) {
      sendConsole(rpc);
      d = rpc.text;
      if (!c)
        break;
    }
  }
}

#ifdef core_hpp
void RpcSend::print(const fstr_t* s)
{
  RPCALLOCA(RpcConsole,rpc)
  rpc.radix = 0;
  for (char* d = rpc.text;;) {
    char c = *d++ = pgm_read_byte(s);
    if (c) s++;
    if (d == rpc.text + sizeof(rpc.text)) {
      sendConsole(rpc);
      d = rpc.text;
      if (!c)
        break;
    }
  }
}
void RpcSend::println(const fstr_t* s) { print(s); println(); }
#endif

void RpcSend::print(int32_t v, uint8_t r) {
  RPCALLOCA(RpcConsole,rpc)
  rpc.value = v;
  rpc.radix = r ? r | 0x80 : 0;
  sendConsole(rpc);
}

void RpcSend::print(uint32_t v, uint8_t r) {
  RPCALLOCA(RpcConsole,rpc)
  rpc.value = v;
  rpc.radix = r;
  sendConsole(rpc);
}

void RpcSend::print(char v, uint8_t r) { print((int32_t)v, r); }
void RpcSend::print(int8_t v, uint8_t r) { print((int32_t)v, r); }
void RpcSend::print(uint8_t v, uint8_t r) { print((uint32_t)v, r); }
void RpcSend::print(int16_t v, uint8_t r) { print((int32_t)v, r); }
void RpcSend::print(uint16_t v, uint8_t r) { print((uint32_t)v, r); }
void RpcSend::println() { print((int8_t)'\n', 0); }
void RpcSend::println(char v, uint8_t r) { print((int32_t)v, r); println(); }
void RpcSend::println(const char s[]) { print(s); println(); }
void RpcSend::println(int8_t v, uint8_t r) { print(v, r); println(); }
void RpcSend::println(uint8_t v, uint8_t r) { print(v, r); println(); }
void RpcSend::println(int16_t v, uint8_t r) { print(v, r); println(); }
void RpcSend::println(uint16_t v, uint8_t r) { print(v, r); println(); }
void RpcSend::println(int32_t v, uint8_t r) { print(v, r); println(); }
void RpcSend::println(uint32_t v, uint8_t r) { print(v, r); println(); }

uint32_t RpcSend::COBSencode(uint8_t* dest, const uint8_t* sour, uint32_t size) {

  uint8_t* d = dest;
  for (const uint8_t* p = sour; p < sour + size; ) {
    uint8_t code = size - (p - sour) < 254 ? size - (p - sour) : 254;
    const uint8_t* e = (const uint8_t*)memchr(p, 0, code);
    code = e ? e - p + 1 : code < 254 ? code + 1 : 255;
    *d++ = code;
    for (uint8_t i = 1; i < code; i++)
      *d++ = *p++;
    if (code < 0xFF)
      p++;
  }
  // Add the phantom zero when source is zero length or ends with zero.
  if (!size || !sour[size - 1])
    *d++ = 1;
  return d - dest;
}

uint32_t RpcSend::COBSencode(void (*sink)(uint8_t value), const uint8_t* sour, uint32_t size) {

  uint32_t n = 0;
  for (const uint8_t* p = sour; p < sour + size; ) {
    uint8_t code = size - (p - sour) < 254 ? size - (p - sour) : 254;
    const uint8_t* e = (const uint8_t*)memchr(p, 0, code);
    sink(code = e ? e - p + 1 : code < 254 ? code + 1 : 255);
    ++n;
    for (uint8_t i = 1; i < code; i++) {
      sink(*p++);
      ++n;
    }
    if (code < 0xFF)
      p++;
  }
  // Add the phantom zero when source is zero length or ends with zero.
  if (!size || !sour[size - 1]) {
    sink(1);
    ++n;
  }
  return n;
}

uint32_t RpcSend::COBSdecode(uint8_t* dest, const uint8_t* sour, uint32_t size) {

  uint8_t* d = dest;
  const uint8_t* end = sour + size;
  while (sour < end) {
    uint8_t code = *sour++;
    if (sour + code - 1 > end)
      // invalid
      return 0;
    for (uint8_t i = 1; i < code; i++)
      *d++ = *sour++;
    if (code < 0xFF && sour < end)
      *d++ = 0;
  }
  return d - dest;
}
