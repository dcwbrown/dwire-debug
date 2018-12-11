#ifndef i2csoft_hpp
#define i2csoft_hpp

#include "core.hpp"

// I2CSOFT_ARGS (SCL,SDA,pcinterrupt#,pcinterrupt)
#define I2CSOFT_ADDRESS   MUAPK_EXP(MUAPK_5_0 I2CSOFT_ARGS)
#define I2CSOFT_BUFF_SIZE MUAPK_EXP(MUAPK_5_1 I2CSOFT_ARGS)
#define I2CSOFT_SCL MUAPK_EXP(MUAPK_5_2 I2CSOFT_ARGS)
#define I2CSOFT_SDA MUAPK_EXP(MUAPK_5_3 I2CSOFT_ARGS)
#define I2CSOFT_INT MUAPK_EXP(MUAPK_5_4 I2CSOFT_ARGS)

#if defined(I2CSOFT_ARGS)
// SCL and SDA must be in same register
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) \
  || defined(__AVR_ATtiny13__)
#define I2CSOFT_ARGS (0x25,2,(B,0,),(B,1,),PCINT0_vect)

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#define I2CSOFT_ARGS (0x25,2,(A,0,0),(A,1,0),PCINT0_vect)

#else // defined(I2CSOFT_ARGS)
#error DW ports
#endif // defined(I2CSOFT_ARGS)

extern class I2cSoft {
public:

  static void begin();

  static volatile uint8_t buff[I2CSOFT_BUFF_SIZE];
  static volatile enum Flag : uint8_t {
    flNone = 0,
    flHold = 0x01,            // bus held until release()
    flHoldRecv = 0x02,        // hold after receive
  } flag;
  static bool set(Flag f, bool v);
  static bool test(Flag f);
  static void release();

  static void interrupt();
  static uint8_t state;
  static uint8_t ibit;
  static uint8_t shft;
  static uint8_t regn;

} i2cSoft;

#endif // i2csoft_hpp
