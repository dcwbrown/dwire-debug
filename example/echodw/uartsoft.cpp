#include "uartsoft.hpp"
#include <avr/interrupt.h>
#include <string.h>
#include <util/atomic.h>

UartSoft uartSoft;

// divisor >= 16 and multiple of 4
uint8_t UartSoft::divisor;
volatile char UartSoft::buff[UARTSOFT_BUFF_SIZE*2];
volatile char* UartSoft::next = UartSoft::buff;

extern "C" __attribute__ ((naked))
char* UartSoft_last() {
  // called from interrupt, so careful with registers

  // working registers declared as early clobbers
  register uint16_t r asm("r24");
  register uint8_t t asm("r26");
  __asm__ __volatile__ (R"assembly(

    ldi   %A[r],lo8(%[buff])
    ldi   %B[r],hi8(%[buff])
    lds   %[t],%[next]
    cp    %A[r],%[t]
    lds   %[t],%[next]+1
    cpc   %B[r],%[t]
    brne   resetbuf%=
    adiw  %A[r],%[size]
resetbuf%=:
    ret

)assembly"
    : [r] "=&r" (r)
      ,[t] "=&r" (t)
    : [buff] "m" (UartSoft::buff)
      ,[next] "m" (UartSoft::next)
      ,[size] "I" (UARTSOFT_BUFF_SIZE)
    :
  );
}

void UartSoft::begin(uint8_t _divisor) {
  divisor = _divisor;

  // input, enable pullup
  UARTSOFT_REG(DDR,UARTSOFT_ARGS) &=~ (1<<UARTSOFT_BIT(UARTSOFT_ARGS));
  UARTSOFT_REG(PORT,UARTSOFT_ARGS) |= (1<<UARTSOFT_BIT(UARTSOFT_ARGS));

  // enable pc interrupt
  GIMSK |= (1<<UARTSOFT_PCIE(UARTSOFT_ARGS));
  UARTSOFT_PCMSK(UARTSOFT_ARGS) |= (1<<UARTSOFT_BIT(UARTSOFT_ARGS));
  sei();
}

extern "C" __attribute__ ((naked))
void UartSoft_write(uint8_t _c) {
  register uint8_t divisor = UartSoft::divisor;

  // c is passed in r24, with r25 allows adiw instruction
  uint16_t c = _c;
  uint8_t sreg;
  __asm__ __volatile__ (R"assembly(

    ; divisor >= 16 and multiple of 4
    ; count = (divisor - 12) >> 2;
    subi  %[count],12
    lsr   %[count]
    lsr   %[count]

    ; one start bit (0), one stop bit (1)
    ; ch:c = 0x600 | (uint16_t(c) << 1);
    ldi   %B[c],0x06
    add   %A[c],%A[c]
    adc   %B[c],__zero_reg__

    ; uint8_t sreg = SREG; cli();
    in    %[sreg],__SREG__
    cli

    ; shift right and output lsb
loop%=:					;0. 00
    lsr   %B[c]         ;1. 01
    ror   %A[c]         ;1. 02
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
    ret
)assembly"
    : [sreg] "=&r" (sreg)
      ,[c] "+w" (c)
    : [ddr] "I" (_SFR_IO_ADDR(UARTSOFT_REG(DDR,UARTSOFT_ARGS)))
      ,[port] "I" (_SFR_IO_ADDR(UARTSOFT_REG(PORT,UARTSOFT_ARGS)))
      ,[bit] "I" (UARTSOFT_BIT(UARTSOFT_ARGS))
      ,[count] "r" (divisor)
    );
}

#ifdef core_hpp
uint8_t UartSoft::write(const fstr_t* s, uint8_t n) {
  uint8_t r = n;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    while (n--)
      UartSoft_write(pgm_read_byte(s++));
  }
  return r;
}
#endif

uint8_t UartSoft::write(const char* s, uint8_t n) {
  uint8_t r = n;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    while (n--)
      UartSoft_write(*s++);
  }
  return r;
}

extern "C" __attribute__ ((naked, used))
void UARTSOFT_INT(UARTSOFT_ARGS)() {

  // working registers declared as early clobbers
  register uint8_t delay asm("r25");
  register uint8_t ibit asm("r29"); // also high part of idel
  register uint8_t idel asm("r28");
  register uint8_t ibuf asm("r21");
  register uint8_t recv asm("r24");
  register uint16_t x asm("r26");
  __asm__ __volatile__ (R"assembly(
                              ;  00
    sbic  %[pin],%[bit]       ;?
    reti                      ;2    give up if high
    push  r24                 ;2 02
    in    r24,__SREG__        ;1 03
    push  r24                 ;2 05
    push  r27                 ;2 07
    push  r26                 ;2 09
    push  r25                 ;2 11
    push  r29                 ;2 13
    push  r28                 ;2 15
    push  r21                 ;2 17

    lds   %A[x],%[next]       ;2 19 buffer
    lds   %B[x],%[next]+1     ;2 21
    ldi   %[ibuf],%[size]-1   ;1 22 max number of bytes

          ; inter bit delay (divisor - 8) >> 2
    lds   %[delay],%[divisor] ;2 24
    subi  %[delay],8          ;1 25 delay till next bit
    lsr   %[delay]            ;1 26
    lsr   %[delay]            ;1 27

readbyte%=:; timeout = 2 * 6 * bit time
    lds   %[idel],%[divisor]  ;2 07 one bit time
    ldi   %[ibit],0           ;1 08 high part of timeout
    lsl   %[idel]             ;1 09 *2
    adc   %[ibit],%[ibit]     ;1 10
loopstart%=:                  ;  6*idel timeout
    sbis  %[pin],%[bit]       ;?
    rjmp  havestart%=         ;2
    sbiw  %[idel],1           ;2
    brne  loopstart%=         ;? 6*idel end
    rjmp  haveall%=           ;2    timeout for start bit
havestart%=:
    mov   %[idel],%[delay]    ;1    0.5 bits
    lsr   %[idel]             ;1
delays%=:                     ;     wait for 1st bit
    dec   %[idel]             ;1
    ldi   %[ibit],8           ;1    number of data bits
    brne  delays%=            ;? 4*idel wait
loop%=:   ; critical loop timing 4*delay + 8
    mov   %[idel],%[delay]    ;1
delay%=:
    dec   %[idel]             ;1 00
    nop                       ;1 01
    brne  delay%=             ;? 4*delay

    lsr   %[recv]             ;1 01 sample
    sbic  %[pin],%[bit]       ;?    skip if high
    ori   %[recv],0x80        ;1 03 set msb
    rjmp  .                   ;2 05 nop
    dec   %[ibit]             ;1 06
    brne  loop%=              ;? 08 more bits?

    mov   %[idel],%[delay]    ;1    1 bit
delayt%=:                     ;     wait past last bit
    dec   %[idel]             ;1 00
    nop                       ;1 01
    brne  delayt%=            ;? 4*delay

    st    X+,%[recv]          ;2 00 store byte
    dec   %[ibuf]             ;1 03
    brne  readbyte%=          ;? 05
                              ;     buffer full
haveall%=:
    ldi   %[recv],%[size]-1   ;1    max number of bytes
    sub   %[recv],%[ibuf]     ;1    bytes received
    lds   %A[x],%[next]       ;2
    lds   %B[x],%[next]+1     ;2
    adiw  %A[x],%[size]-1     ;2
    st    X,%[recv]           ;2    save number of characters
    rcall UartSoft_last       ;     swap buffers
    sts   %[next],r24
    sts   %[next]+1,r25

    pop   r21                 ;2
    pop   r28                 ;2
    pop   r29                 ;2
    pop   r25                 ;2
    pop   r26                 ;2
    pop   r27                 ;2
    pop   r24                 ;2
    out   __SREG__,r24        ;1
    pop   r24                 ;2
    reti                      ;4

)assembly"
    : [recv] "=&r" (recv)
      ,[delay] "=&r" (delay)
      ,[ibit] "=&r" (ibit)
      ,[idel] "=&r" (idel)
      ,[ibuf] "=&r" (ibuf)
      ,[x] "=&r" (x)
    : [divisor] "m" (UartSoft::divisor)
      ,[next] "m" (UartSoft::next)
      ,[size] "I" (UARTSOFT_BUFF_SIZE)
      ,[pin] "I" (_SFR_IO_ADDR(UARTSOFT_REG(PIN,UARTSOFT_ARGS)))
      ,[bit] "I" (UARTSOFT_BIT(UARTSOFT_ARGS))
    :
  );
}

uint8_t UartSoft::read(char* d, uint8_t n) {
  char* s;
  uint8_t r = read(&s);
  r = r < n ? r : n;
  memcpy(d, s, r);
  return r;
}

uint8_t UartSoft::read(char** d) {
  char* last = UartSoft_last();
  *d = last;
  uint8_t r = last[UARTSOFT_BUFF_SIZE-1];
  last[UARTSOFT_BUFF_SIZE-1] = 0;
  return r;
}

bool UartSoft::eof() {
  return UartSoft_last()[UARTSOFT_BUFF_SIZE-1] != 0;
}

void UartSoft::send(Rpc& rpc, uint8_t index) {
}
