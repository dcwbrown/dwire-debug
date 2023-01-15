#ifdef RPC_ELEMENT // list all of the rpc structures
/******************************************************************************/
// here is the leaf of the recursion, each class has an entry here
// this part is multiply evaluated with different RPC_ELEMENT definitions

#define RPC_ELEMENT_BASE
// the abstract base class has index 0
RPC_ELEMENT
(Rpc,
 // return the size of each class
 inline static uint8_t Size(uint8_t i) { return size[i]; }
 static uint8_t size[RpcNumber];

 // run the Act member function via a 'virtual table'
 inline void VtAct(uint8_t i) { (this->*vtAct[i])(); }
 static void (Rpc::*vtAct[RpcNumber])();
)
#undef RPC_ELEMENT_BASE
#define RPC_ELEMENT_BASE :Rpc

// index 1
RPC_ELEMENT
(RpcConsole,
 union {
   uint32_t value;
   char text[6];
 };
 // sign bit of radix indicates signed value
 // a zero doubles as terminator for text
 uint8_t radix;
)

#undef RPC_ELEMENT_BASE
#undef RPC_ELEMENT
#else // RPC_ELEMENT
/******************************************************************************/
#ifndef RPC_DECLARE
#define RPC_DECLARE // declare items by recursive include

#include <stdint.h>

// __attribute__((packed)); doesn't pack the union in RpcConsole
#pragma pack(push,1)

// declare the struct indicies
enum Rpc_index_type : uint8_t {
#define RPC_ELEMENT(NAME,ARGUMENT) NAME##_index,
#include "rpc.hpp"
// declare the total number of structs
RpcNumber = 0
#define RPC_ELEMENT(NAME,ARGUMENT) +1
#include "rpc.hpp"
};

// declare the structs
#define RPC_ELEMENT(NAME,DECLARATION) \
struct NAME RPC_ELEMENT_BASE { \
  NAME() = delete; /* deleted constructor */ \
  void Act(); /* called after receive */ \
  DECLARATION };
#include "rpc.hpp"

constexpr inline uint8_t RpcElementMax(uint8_t s0, uint8_t s1)
  { return s0 < s1 ? s1 : s0; };
// declare max size to be checked by static_assert elsewhere
enum { RpcPayloadMax =
#define RPC_ELEMENT(NAME,ARGUMENT) RpcElementMax(sizeof(NAME),
#include "rpc.hpp"
  0
#define RPC_ELEMENT(NAME,ARGUMENT) )
#include "rpc.hpp"
};

// declare helper types

// must be used to allocate rpc structures to create space for encoding
#define RPCALLOCA(type,var) type&var=*(type*)memset \
(alloca(Rpc::Size(type##_index)+1),0,Rpc::Size(type##_index)+1); \
((char*)&var)[Rpc::Size(type##_index)]=type##_index;

struct fstr_t;
class RpcSend {
public:
  virtual void send(Rpc& rpc, uint8_t index) = 0;
  void sendConsole(RpcConsole& rpc);

  template<typename T> void out(T data) { print(data); }
  RpcSend& operator<<(const char* s) { out<const char*>(s); return *this; }
  RpcSend& operator<<(const fstr_t* s) { out<const fstr_t*>(s); return *this; }
  RpcSend& operator<<(const char s) { out<const char>(s); return *this; }
  RpcSend& operator<<(const int8_t s) { out<const int8_t>(s); return *this; }
  RpcSend& operator<<(const uint8_t s) { out<const uint8_t>(s); return *this; }
  RpcSend& operator<<(const int16_t s) { out<const int16_t>(s); return *this; }
  RpcSend& operator<<(const uint16_t s) { out<const uint16_t>(s); return *this; }
  RpcSend& operator<<(const int32_t s) { out<const int32_t>(s); return *this; }
  RpcSend& operator<<(const uint32_t s) { out<const uint32_t>(s); return *this; }

  void print(const char s[]);
  void print(const fstr_t* s);
  void print(char v, uint8_t r = 0);
  void print(int8_t v, uint8_t r = 10);
  void print(uint8_t v, uint8_t r = 10);
  void print(int16_t v, uint8_t r = 10);
  void print(uint16_t v, uint8_t r = 10);
  void print(int32_t v, uint8_t r = 10);
  void print(uint32_t v, uint8_t r = 10);
  void println();
  void println(const char s[]);
  void println(const fstr_t* s);
  void println(char v, uint8_t r = 0);
  void println(int8_t v, uint8_t r = 10);
  void println(uint8_t v, uint8_t r = 10);
  void println(int16_t v, uint8_t r = 10);
  void println(uint16_t v, uint8_t r = 10);
  void println(int32_t v, uint8_t r = 10);
  void println(uint32_t v, uint8_t r = 10);

/* http://www.stuartcheshire.org/papers/COBSforToN.pdf

COBS encoding allows a zero byte to be used as a delimiter by producing an encoded
stream with no zeros. The maximum overhead is one byte per 254 of unencoded message
(size / 254 + 1).

Search up to 254 bytes for a zero, place the length into the first byte and copy up to,
but not including the zero. If there are no zeros, place 255 into the first byte and
copy the 254 nonzero bytes. Encode entire buffer as if there was a zero appended unless
the last block is a run of 254, then don't append.
*/
  static uint32_t COBSencode(uint8_t* dest, const uint8_t* sour, uint32_t size);
  static uint32_t COBSencode(void (*sink)(uint8_t value), const uint8_t* sour, uint32_t size);
  static uint32_t COBSdecode(uint8_t* dest, const uint8_t* sour, uint32_t size);
};

#pragma pack(pop)
#endif // RPC_DECLARE
/******************************************************************************/
#ifdef RPC_DEFINE
#undef RPC_DEFINE // define items by recursive include

// define size array
uint8_t Rpc::size[] = {
#define RPC_ELEMENT(NAME,ARGUMENT) sizeof(NAME),
#include "rpc.hpp"
};

extern "C" void Rpc_dummy() { }
// define jump table for Act()
#define RPC_ELEMENT(NAME,ARGUMENT) void NAME::Act() __attribute__ ((weak, alias ("Rpc_dummy")));
#include "rpc.hpp"
void (Rpc::*Rpc::vtAct[])() = {
#define RPC_ELEMENT(NAME,ARGUMENT) (void (Rpc::*)())&NAME::Act,
#include "rpc.hpp"
};

#endif // RPC_DEFINE
#endif // RPC_ELEMENT
