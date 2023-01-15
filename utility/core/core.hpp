#if !defined(core_hpp) && !defined(__ASSEMBLER__)
#define core_hpp
#define Arduino_h // this is an alternate for Arduino.h

#include <stdint.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "coredef.h"

#ifndef Print_h

struct fstr_t { char c; };
#define FSTR(s) ((fstr_t*)PSTR(s))
#define F(s) ((fstr_t*)PSTR(s))

#define DEC 10
#define HEX 16

#endif // Print_h

// PIN (X,N,I), REG (DDR|PORT|PIN), OP (|=|&=~|&)
#define DIO_REG(PIN,REG) MUAPK_CAT(REG,MUAPK_3_0 PIN)
#define DIO_BIT(PIN) MUAPK_3_1 PIN
#define DIO_OP(PIN,REG,OP) (DIO_REG(PIN,REG)OP(1<<DIO_BIT(PIN)))
#define DIO_PC(PIN,REG) MUAPK_CAT(REG,MUAPK_3_2 PIN) // REG (MSK|IE)

#ifndef Wiring_h

#define LOW  0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1

typedef uint8_t byte;

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny13__)

inline void pinMode(uint8_t p, uint8_t d) {
  if (d) DDRB |= _BV(p); else DDRB &= ~_BV(p); }
inline void digitalWrite(uint8_t p, uint8_t d) {
  if (d) PORTB |= _BV(p); else PORTB &= ~_BV(p); }
inline uint8_t digitalRead(uint8_t p) {
  return 0 != (PINB & _BV(p)); }

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)

#define MASK_PIN(p) _BV(p >= 3 && p <= 10 ? 10 - p : p != 11 ? p : 3)
#define PORT_PIN_REG_OP(p,r,o) ( (p >= 3 && p <= 10) ? (r##A o MASK_PIN(p)) : (r##B o MASK_PIN(p)) );

inline void pinMode(uint8_t p, uint8_t d) {
  if (d) PORT_PIN_REG_OP(p,DDR,|=) else PORT_PIN_REG_OP(p,DDR,&=~) }
inline void digitalWrite(uint8_t p, uint8_t d) {
  if (d) PORT_PIN_REG_OP(p,PORT,|=) else PORT_PIN_REG_OP(p,PORT,&=~) }
inline uint8_t digitalRead(uint8_t p) {
  return 0 != PORT_PIN_REG_OP(p,PIN,&) }

#endif // defined(__AVR_*)

inline unsigned long millis() { extern int millis_broken(); return millis_broken(); }
#define delay(ms) _delay_ms(ms)
inline void delayMicroseconds(uint16_t _us) { uint8_t us = _us >>= 4; do _delay_us(16); while (us-- > 0); }

#else // Wiring_h
#define TICKS_MILLIS
inline uint16_t ticks_atomic() { return (uint16_t)millis(); }
#define TICKS_FROM_CLOCKS ((uint32_t)F_CPU / 1000)
#endif // Wiring_h

#ifdef TICKS_TIMER

inline void ticks_init() {
#if TICKS_TIMER == 0
  TCCR0A = 0;
  TCCR0B = (_BV(CS02)|_BV(CS00));
#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  TIMSK0 = _BV(TOIE0);
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  TIMSK = _BV(TOIE0);
#else // defined(__AVR_*)
#error TIM register names
#endif // defined(__AVR_*)

#define TICKS_VECT TIM0_OVF_vect
#define TICKS_PRESCALER 1024 // CS0[2:0]: 1-1, 2-8, 3-64, 4-256, 5-1024
#define TICKS_FROM_CLOCKS ((uint32_t)TICKS_PRESCALER*256)

#else // TICKS_TIMER
#error "TICKS_TIMER 0 only"
#endif // TICKS_TIMER
}

extern volatile uint16_t ticks_count;
inline uint16_t ticks_atomic() {
  cli();
  uint16_t t = ticks_count;
  sei();
  return t;
}
#endif // TICKS_TIMER

#if defined(__cplusplus) && (defined(TICKS_TIMER) || defined(TICKS_MILLIS))
#define TICKS_FROM_MILLIS(ms) uint16_t(ms / 1000 * F_CPU / TICKS_FROM_CLOCKS)
struct ticks_timeout {
  uint16_t start;
  inline void begin() { start = ticks_atomic(); }
  inline bool done(double ms) {
    // it's important for this subtraction to wrap
    uint16_t time_elapsed = ticks_atomic() - start;
    // rely on optimizer to do the arithmetic
    return time_elapsed > TICKS_FROM_MILLIS(ms);
  }
  inline void add(double ms) {
    start += TICKS_FROM_MILLIS(ms);
  }
  inline void wait(double ms) {
    while (!done(ms)) { }
  }
};
struct ticks_timeout_begin : ticks_timeout {
  inline ticks_timeout_begin() { begin(); }
};
#endif // __cplusplus

// data offsets in eeprom
#define EEO_OSCCAL ((uint8_t*)0)
#define EEO_SERIAL ((uint16_t*)1)

// wdt_reset must be called periodically to stay alive,
// alternatively wdt_period can be used to generate a timeout

// indicates a timeout in progress, cleared by interrupt at timeout
extern volatile uint8_t wdt_ie;

// start timeout of given period
void wdt_wdto(uint8_t period);
void wdt_period(uint16_t period);
void wdt_delay(uint16_t period);

// indicates a timeout in progress, cleared by interrupt at timeout
//inline uint8_t wdt_running() { return (WDTCR | _BV(WDIE)) != 0; }
inline uint8_t wdt_running() { return wdt_ie; }

#endif // core_hpp
