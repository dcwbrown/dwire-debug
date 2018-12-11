

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <util/delay.h>

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny13__)
#define DW_DDR DDRB
#define DW_PORT PORTB
#define DW_BIT 5
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
#define DW_DDR DDRB
#define DW_PORT PORTB
#define DW_BIT 3
#else // defined(__AVR_*)
#error DW ports
#endif // defined(__AVR_*)

void RpcDw_begin() {
  DW_DDR &=~ (1<<DW_BIT); // input
  DW_PORT |= (1<<DW_BIT); // pullup
}

void RpcDw_write(uint8_t _c, uint8_t divisor) {
  // divisor >= 16 and multiple of 4
  divisor = (divisor - 12) >> 2;

  // c is passed in r24, with r25 allows adiw instruction
  register uint8_t c asm ("r24") = _c;
  register uint8_t ch asm ("r25");
  uint8_t sreg;
  __asm__ __volatile__ (R"string-literal(

    ; ch:c = 0x200 | (uint16_t(c) << 1);
    ldi   %[ch],0x02
    add   %[c],%[c]
    adc   %[ch],__zero_reg__

    ; uint8_t sreg = SREG; cli();
    in    %[sreg],__SREG__
    cli

    ; shift right and output lsb
loop%=:					;0. 00
    lsr   %[ch]         ;1. 01
    ror   %[c]          ;1. 02
    brcc  set_low%=     ;12 03
set_high%=:             ;0. 03
    cbi   %[ddr],%[bit] ;2. 05 ;input
    sbi   %[port],%[bit];2. 07 ;pullup
    rjmp  set_done%=    ;.2 09
set_low%=:              ;0. 04
    cbi   %[port],%[bit];2. 06 ;low
    sbi   %[ddr],%[bit] ;2. 08 ;output
    nop                 ;1. 09
set_done%=:             ;0. 09

    ; loop until all bits sent
    adiw  %[c],0        ;2. 11
    breq  all_done%=    ;12 12

    ; delay = 4*count
    mov   __tmp_reg__,%[count];1.
delay%=:
    dec   __tmp_reg__         ;1.
    breq  loop%=              ;12
    rjmp  delay%=             ;.2

    ; SREG = sreg;
all_done%=:
    out   __SREG__,%[sreg]
)string-literal"
     : [sreg] "=&r" (sreg)
       ,[c] "+w" (c), [ch] "+w" (ch)
     : [ddr] "I" (_SFR_IO_ADDR(DW_DDR))
       ,[port] "I" (_SFR_IO_ADDR(DW_PORT))
       ,[bit] "I" (DW_BIT)
       ,[count] "r" (divisor)
     );
}

void RpcDw_puts(const char* s) {
  while (*s) {
    RpcDw_write(*s++, 128);
    _delay_us(20);
  }
}

int main(void) {
  RpcDw_begin();
  char str[] = "hellodw 0\n";
  for (;;) {
    RpcDw_puts(str);
    if (++str[8] > '9')
      str[8] = '0';
    _delay_ms(1000);
  }
}
