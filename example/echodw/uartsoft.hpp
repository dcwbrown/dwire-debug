#ifndef uartsoft_hpp
#define uartsoft_hpp

#include "core.hpp"
#include "rpc.hpp"

#ifndef UARTSOFT_BUFF_SIZE
#define UARTSOFT_BUFF_SIZE 16
#endif

// UARTSOFT_ARGS (port,bit#,pci#,pcint), REG (DDR|PORT|PIN)
#define UARTSOFT_REG(REG,ARGS) MUAPK_CAT(REG,MUAPK_4_0 ARGS)
#define UARTSOFT_BIT(ARGS) MUAPK_4_1 ARGS
#define UARTSOFT_PCMSK(ARGS) MUAPK_CAT(PCMSK,MUAPK_4_2 ARGS)
#define UARTSOFT_PCIE(ARGS) MUAPK_CAT(PCIE,MUAPK_4_2 ARGS)
#define UARTSOFT_INT(ARGS) MUAPK_4_3 ARGS

#if defined(UARTSOFT_ARGS)
// ok
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) \
  || defined(__AVR_ATtiny13__)
#define UARTSOFT_ARGS (B,5,,PCINT0_vect)

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#define UARTSOFT_ARGS (B,3,1,PCINT1_vect)

#else // defined(UARTSOFT_ARGS)
#error DW ports
#endif // defined(UARTSOFT_ARGS)

extern class UartSoft : public RpcSend {
public:

  static void begin(uint8_t divisor = 128, uint8_t break_timeout = 12);

#ifdef core_hpp
  static uint8_t write(const fstr_t* s, uint8_t n, uint8_t break_bits = 8);
#endif
  static uint8_t write(const char* s, uint8_t n, uint8_t break_bits = 8);
  static uint8_t read(char* d, uint8_t n);
  static uint8_t read(char** d);
  static bool eof();

  void send(Rpc& rpc, uint8_t index);

  // divisor >= 16 and multiple of 4
  static uint8_t divisor;
  static uint8_t break_timeout;
  static volatile enum Flag : uint8_t {
    flNone = 0,
    flBreakActive = 0x01,     // during break
    flBreak = 0x02,           // after break
    flError = 0x04,           // framing
  } flag;
  static bool clear(Flag f);

  // double buffer, next gets loaded
  // last byte has number of chars received
  // 1st byte has command, 0: text, 0xff: reset
  static volatile char buff[UARTSOFT_BUFF_SIZE*2];
  static volatile char* next;

} uartSoft;

#endif // uartsoft_hpp
