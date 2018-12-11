#include "core.hpp"

#ifdef TICKS_TIMER

volatile uint16_t ticks_count;
#ifdef __cplusplus
extern "C"
#endif
void TICKS_VECT() __attribute__ ((signal, used));
void TICKS_VECT() { ++ticks_count; }

#endif // TICKS_TIMER

volatile uint8_t wdt_ie;
#ifndef WDTCR
#define WDTCR WDTCSR
#endif
#ifndef WDIE
#define WDIE WDTIE
#endif

// start timeout of given period
void wdt_wdto(uint8_t period) {
  wdt_enable(period);
  wdt_ie = 1;
  WDTCR |= _BV(WDIE);
}
void wdt_period(uint16_t period) {
  wdt_wdto(period <= 15 ? WDTO_15MS :
           period <= 30 ? WDTO_30MS :
           period <= 60 ? WDTO_60MS :
           period <= 120 ? WDTO_120MS :
           period <= 250 ? WDTO_250MS :
           period <= 500 ? WDTO_500MS :
           period <= 1000 ? WDTO_1S :
           period <= 2000 ? WDTO_2S :
           period <= 4000 ? WDTO_4S : WDTO_8S);
}
void wdt_delay(uint16_t period) {
  wdt_period(period);
  while (wdt_running()) { }
}

// wdt timed out, restore default value
#ifdef __cplusplus
extern "C"
#endif
void WDT_vect() __attribute__ ((signal, used));
void WDT_vect() { wdt_enable(WDTO_2S); wdt_ie = 0; }

// initialize after registers, but before static data
__attribute__((naked, section(".init3"), used)) void __init3(void) {
  WDT_vect();
#ifdef TICKS_TIMER
  ticks_init();
#endif // TICKS_TIMER
}

#ifndef WProgram_h
__attribute__((naked, weak)) int main(void) {
  extern void setup(), loop();
  setup(); for (;;) loop(); }
#endif // WProgram_h
