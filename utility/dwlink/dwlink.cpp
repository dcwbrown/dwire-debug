#if 0 // MAKEFILE{

define help_source =
a=t45
endef

MCU=attiny45
F_CPU = 16000000
CPPFLAGS+=-DI2CSOFT_ARGS='(0x25,10,(B,0,),(B,1,),PCINT0_vect)'

# for ticks_timeout
CPPFLAGS += -DTICKS_TIMER=0

INCLUDE_LIBS += core

#endif // MAKEFILE}

#include "../core/i2csoft.hpp"

#include <avr/eeprom.h>
#include <util/atomic.h>

  // I2c items
  const uint8_t dlVersion = 1;
  enum Register : uint8_t {
    dlrCommand,     // commands to execute
    dlrReturn,      // ok to overwrite when issuing command
    dlrData0,       // args and returns for commands
    dlrData1,
    dlrData2,
    dlrData3,
    dlrBaudrate0,   // baudrate stored until next sync
    dlrBaudrate1,
    dlrBaudrate2,
    dlrBaudrate3,
  };
  enum Command : uint8_t {
    dlcIdle,          // no command in progress
    dlcVersion,       // return dlVersion in dlrData0
    dlcGetSerial,     // return serial number in dlrData1:dlrData0
    dlcSetSerial,     // set serial number in dlrData1:dlrData0
    dlcFcpu,          // return F_CPU in dlrData3:dlrData2:dlrData1:dlrData0
    dlcSync,          // sync to 0x55, then store dlrBaudrate3:dlrBaudrate2:dlrBaudrate1:dlrBaudrate0
    dlcBreakSync,     // output break, then sync, then store
    dlcFlagUartSoft,  // return true if FlagUartSoft detected at last sync, false indicates dw
    dlcPowerOn,       // turn power on
    dlcPowerOff,      // turn power off
    dlcGioSet,        // set DDR to bit 2 and PORT to bit 1 (PORT first when DDR set high), then dlcGioGet
    dlcGioGet,        // return DDR in bit 2, PORT in bit 1, and PIN in bit 0
  };

bool flagUartSoft;

// int0 same for t84 and t85
#define SYN_PIN (B,2,x)
#define POW_PIN (B,4,x)
#define GIO_PIN (B,3,x)

void setup() {
  wdt_reset();

#if 0
  uint8_t osccal = eeprom_read_byte(EEO_OSCCAL);
  if (osccal != 0xFF)
    OSCCAL = osccal;
#endif

  // sync: set input, always
  // keep pullup disabled since
  // target may run at 3V3
  DIO_OP(SYN_PIN,DDR,&=~);
  DIO_OP(SYN_PIN,PORT,&=~);

  // power on: set low, output
  DIO_OP(POW_PIN,PORT,&=~);
  DIO_OP(POW_PIN,DDR,|=);

  // hold bus after receive
  i2cSoft.begin();
  i2cSoft.set(I2cSoft::flHoldRecv, true);
}

uint16_t SyncTimer() {
  // available r0,r18-r27, r30-r31, zero r1
  // x r27:r26, y r29:r28, z r31:r30
  register uint16_t sum asm("r24");
  register uint16_t cnt asm("r26");
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    __asm__ __volatile__ (R"assembly(
    clr   %A[sum]             ;1 init timeout
    clr   %B[sum]             ;1
    sts   flagUartSoft,%A[sum];  clear flagUartSoft
x0_%=:    ; wait for start bit
    adiw  %A[sum],1           ;2
    sbic  %[pin],%[bit]       ;?
    brne  x0_%=               ;?
    breq  timeout%=           ;?
x1_%=:    ; wait for first bit
    adiw  %A[sum],1           ;2
    sbis  %[pin],%[bit]       ;?
    brne  x1_%=               ;?
    breq  timeout%=           ;?
          ; start measurement 8 bits
    clr   %A[sum]             ;1
    clr   %B[sum]             ;1
x2_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbic  %[pin],%[bit]       ;?
    brne  x2_%=               ;?
    breq  timeout%=           ;?
x3_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbis  %[pin],%[bit]       ;?
    brne  x3_%=               ;?
    breq  timeout%=           ;?
x4_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbic  %[pin],%[bit]       ;?
    brne  x4_%=               ;?
    breq  timeout%=           ;?
x5_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbis  %[pin],%[bit]       ;?
    brne  x5_%=               ;?
    breq  timeout%=           ;?
x6_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbic  %[pin],%[bit]       ;?
    brne  x6_%=               ;?
    breq  timeout%=           ;?
x7_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbis  %[pin],%[bit]       ;?
    brne  x7_%=               ;?
    breq  timeout%=           ;?
x8_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbic  %[pin],%[bit]       ;?
    brne  x8_%=               ;?
    breq  timeout%=           ;?
x9_%=:    ; wait for # bit
    adiw  %A[sum],1           ;2
    sbis  %[pin],%[bit]       ;?
    brne  x9_%=               ;?
    breq  timeout%=           ;?
          ; end measurement, time = 5 * sum
          ; now see if we get a short break within 2 byte times
    clr   %A[cnt]
    clr   %B[cnt]
    sub   %A[cnt],%A[sum]     ;  one byte's worth of time
    sbc   %B[cnt],%B[sum]
b0_%=:    ; wait for start bit
    adiw  %A[cnt],1           ;2
    sbic  %[pin],%[bit]       ;?
    brne  b0_%=               ;?
    brne  uartSoft%=          ;?
    sub   %A[cnt],%A[sum]     ;  one byte's worth of time
    sbc   %B[cnt],%B[sum]
b1_%=:    ; wait for start bit
    adiw  %A[cnt],1           ;2
    sbic  %[pin],%[bit]       ;?
    brne  b1_%=               ;?
    brne  uartSoft%=          ;?
          ; timeout waiting for short break
    rjmp  done%=
timeout%=:
    clr   %A[sum]
    clr   %B[sum]
    rjmp  done%=
uartSoft%=:
    ldi   %A[cnt],1
    sts   flagUartSoft,%A[cnt];  set flagUartSoft
done%=:

)assembly"
    : [sum] "=&r" (sum)
      ,[cnt] "=&r" (cnt)
    : [pin] "I" (_SFR_IO_ADDR(DIO_REG(SYN_PIN,PIN)))
      ,[bit] "I" (DIO_BIT(SYN_PIN))
    :
    );
  }
  return sum;
}

void loop() {
  wdt_reset();

  // execute commands after i2c receives (held bus)
  if (!i2cSoft.test(I2cSoft::flHold)) return;

  i2cSoft.buff[dlrReturn] = 0;
  switch (i2cSoft.buff[dlrCommand]) {
  case dlcVersion:
    i2cSoft.buff[dlrReturn] = dlVersion;
    break;

  case dlcGetSerial: {
    uint16_t v = eeprom_read_word(EEO_SERIAL);
    if (v != (uint16_t)-1) {
      i2cSoft.buff[dlrData0] = v;
      i2cSoft.buff[dlrData1] = v >> 8;
    }
    break;
  }

  case dlcSetSerial: {
    uint16_t v = i2cSoft.buff[dlrData0] | ((uint16_t)i2cSoft.buff[dlrData1] << 8);
    eeprom_write_word(EEO_SERIAL, v);
    break;
  }

  case dlcFcpu:
    i2cSoft.buff[dlrData0] = (uint8_t)F_CPU;
    i2cSoft.buff[dlrData1] = (uint8_t)(F_CPU >> 8);
    i2cSoft.buff[dlrData2] = (uint8_t)(F_CPU >> 16);
    i2cSoft.buff[dlrData3] = (uint8_t)(F_CPU >> 24);
    break;

  case dlcBreakSync:
    // set low, output
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      DIO_OP(SYN_PIN,PORT,&=~);
      DIO_OP(SYN_PIN,DDR,|=);
    }

    // requires interrupts on
    wdt_delay(50);

    // now turn them off for timing
    __asm__ __volatile__ ("cli" ::: "memory");

    // set input, disable pullup
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      DIO_OP(SYN_PIN,DDR,&=~);
      DIO_OP(SYN_PIN,PORT,&=~);
    }

    // allow for rise time of 10us with no pullup
    // rise time is below 2us with 10K pullup
    // t84 @ 8MHz, divisor 128 has 160us delay before sync
    // so, there should be plenty of time
    _delay_us(10);

    // fall through
  case dlcSync: {
    uint32_t ticks = SyncTimer();
    uint32_t baud = !ticks ? 0 : (F_CPU * 8 + (ticks * 5) / 2) / (ticks * 5);
    i2cSoft.buff[dlrBaudrate0] = baud;
    i2cSoft.buff[dlrBaudrate1] = baud >> 8;
    i2cSoft.buff[dlrBaudrate2] = baud >> 16;
    i2cSoft.buff[dlrBaudrate3] = baud >> 24;
  }
    // turn ints back on
    __asm__ __volatile__ ("sei" ::: "memory");
    break;

  case dlcFlagUartSoft:
    i2cSoft.buff[dlrData0] = flagUartSoft;
    break;

  case dlcPowerOn:
    DIO_OP(POW_PIN,PORT,&=~);
    break;

  case dlcPowerOff:
    DIO_OP(POW_PIN,PORT,|=);
    break;

  case dlcGioSet:
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (i2cSoft.buff[dlrData0] & 4) {
        if (i2cSoft.buff[dlrData0] & 2)
          DIO_OP(GIO_PIN,PORT,|=);
        else
          DIO_OP(GIO_PIN,PORT,&=~);
        DIO_OP(GIO_PIN,DDR,|=);
      } else {
        DIO_OP(GIO_PIN,DDR,&=~);
        if (i2cSoft.buff[dlrData0] & 2)
          DIO_OP(GIO_PIN,PORT,|=);
        else
          DIO_OP(GIO_PIN,PORT,&=~);
      }
    }

    // fall through
  case dlcGioGet:
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      i2cSoft.buff[dlrData0]
        = (DIO_OP(GIO_PIN,DDR,&) ? 4 : 0)
        | (DIO_OP(GIO_PIN,PORT,&) ? 2 : 0)
        | (DIO_OP(GIO_PIN,PIN,&) ? 1 : 0);
    }
    break;

  }
  // clear command and release bus
  i2cSoft.buff[dlrCommand] = dlcIdle;
  i2cSoft.release();
}

/* serial port raft

      10K
5V-----zz---o--->|---<GioSDAo,RTS#
            |
SDA---------o-------->GioSDAi,CTS#
SCL<-----------------<GioSCL,DTR#

        ________            _______
      I|        |O          |  HT |
5V--o--| HT7533 |--o--3V3   | 7533|
    |  |________|  |        --|||--
    |      |G      |          GIO
    |     GND    J |
    -----------o o-o
                 |    _______
               | <E   | PNP |
      10K     B|/     |BC327|
POW>---zz--o---|\     --|||--
           |   | \C     CBE
           |     |
           --||--o--->V+
            10n  |
      .1u        |
GND----||---o-----
            |
            z
        10K z
            |
            o--->|---<TXD
            |
SYN---------o-------->RXD
            |
            ----------DW
      ________________________
      |                      | 5V
      |                      | J
------|                      | 3V3
      |                      | GIO
      |                      | POW
------|                      | DW
      |                      | GND
      |______________________| V+

*/
