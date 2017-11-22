#include "uartsoft.hpp"
#include <avr/interrupt.h>
#include <string.h>
#include <util/atomic.h>

UartSoft uartSoft;

// divisor >= 16 and multiple of 4
uint8_t UartSoft::divisor;
uint8_t UartSoft::break_timeout;
volatile char UartSoft::buff[UARTSOFT_BUFF_SIZE*2];
volatile char* UartSoft::next = UartSoft::buff;
volatile UartSoft::Flag UartSoft::flag = flNone;

bool UartSoft::clear(Flag f) {
  bool ret;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ret = (flag & f) != 0;
    flag = (Flag)(flag & ~f);
  }
  return ret;
}

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

void UartSoft::begin(uint8_t _divisor, uint8_t _break_timeout) {
  divisor = _divisor; break_timeout = _break_timeout;

  // input, enable pullup
  UARTSOFT_REG(DDR,UARTSOFT_ARGS) &=~ (1<<UARTSOFT_BIT(UARTSOFT_ARGS));
  UARTSOFT_REG(PORT,UARTSOFT_ARGS) |= (1<<UARTSOFT_BIT(UARTSOFT_ARGS));

  // enable pc interrupt
  GIMSK |= (1<<UARTSOFT_PCIE(UARTSOFT_ARGS));
  UARTSOFT_PCMSK(UARTSOFT_ARGS) |= (1<<UARTSOFT_BIT(UARTSOFT_ARGS));
  sei();
}

extern "C" __attribute__ ((naked))
void UartSoft_break(uint8_t bits) {

  // bits is passed in r24
  register uint8_t ibit asm("r24") = bits;
  register uint8_t idel asm("r25");
  __asm__ __volatile__ (R"assembly(

    tst   %[ibit]             ;1    check for zero length
    brne  start%=             ;?
    ret
start%=:
    cbi   %[port],%[bit]      ;2    low
    sbi   %[ddr],%[bit]       ;2    output
loop%=:
    lds   %[idel],%[divisor]  ;2    idel = (divisor - 8) >> 2
    subi  %[idel],8           ;1
    lsr   %[idel]             ;1
    lsr   %[idel]             ;1
delay%=:
    nop                       ;1
    dec   %[idel]             ;1
    brne  delay%=             ;? 4*idel end
    dec   %[ibit]             ;1
    brne  loop%=              ;?

    cbi   %[ddr],%[bit]       ;2    input
    sbi   %[port],%[bit]      ;2    pullup

    lds   %[idel],%[divisor]  ;2    idel = divisor >> 2
    lsr   %[idel]             ;1
    lsr   %[idel]             ;1
delays%=:
    nop                       ;1
    dec   %[idel]             ;1
    brne  delays%=            ;? 4*idel end

    ret
)assembly"
    : [ibit] "=&r" (ibit)
      ,[idel] "=&r" (idel)
    : [divisor] "m" (UartSoft::divisor)
      ,[ddr] "I" (_SFR_IO_ADDR(UARTSOFT_REG(DDR,UARTSOFT_ARGS)))
      ,[port] "I" (_SFR_IO_ADDR(UARTSOFT_REG(PORT,UARTSOFT_ARGS)))
      ,[bit] "I" (UARTSOFT_BIT(UARTSOFT_ARGS))
    );
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
uint8_t UartSoft::write(const fstr_t* s, uint8_t n, uint8_t break_bits) {
  uint8_t r = n;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    UartSoft_break(break_bits);
    while (n--)
      UartSoft_write(pgm_read_byte(s++));
  }
  return r;
}
#endif

uint8_t UartSoft::write(const char* s, uint8_t n, uint8_t break_bits) {
  uint8_t r = n;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    UartSoft_break(break_bits);
    while (n--)
      UartSoft_write(*s++);
  }
  return r;
}

/* Interrupt

Packets of up to UARTSOFT_BUFF_SIZE-1 are received into one of two buffers: UartSoft::buff or
UartSoft::buff + UARTSOFT_BUFF_SIZE. The interrupt starts with UartSoft::next pointing to the
buffer that will receive. Location UartSoft::buff + UARTSOFT_BUFF_SIZE-1 gets the number of
chars received, and UartSoft::next is then switched to the other buffer.

Packets must consist of a short break followed by up to UARTSOFT_BUFF_SIZE-1 chars. A break is
considered short when it is shorter than UartSoft::break_timeout bits. A packet is considered
complete once 12 high bits in a row have passed. The short break is required to allow for
interrupt latency. A short break is still required after a long break for synchronization
before data can be received.

Once a break is longer than UartSoft::break_timeout bits, flag UartSoft::flBreakActive is set.
Once the long break is over, flag UartSoft::flBreakActive is cleared and flag UartSoft::flBreak
is set. The interrupt returns during the time that the long break is active.
 */
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
    push  r24                 ;2 02
    in    r24,__SREG__        ;1 03
    push  r24                 ;2 05
    push  r27                 ;2 07
    push  r26                 ;2 09
    push  r25                 ;2 11
    push  r29                 ;2 13
    push  r28                 ;2 15
    push  r21                 ;2 17

          ; handle break flags
    lds   %[idel],%[flag]     ;2 19 is long break active?
    andi  %[idel],%[brka]     ;1 20
    breq  nolong%=            ;? 22
    sbis  %[pin],%[bit]       ;     still low?
    rjmp  done%=              ;     keep waiting
          ; long break is over
    lds   %[idel],%[flag]
    andi  %[idel],~%[brka]    ;     clear active flag
    ori   %[idel],%[brk]      ;     set break flag
    sts   %[flag],%[idel]
    rjmp  done%=              ;     long break detected
nolong%=:
    sbic  %[pin],%[bit]       ;? 23
    rjmp  done%=              ;     skip uncleared interrupt
          ; short break already in progress

          ; initialize variables
    lds   %A[x],%[next]       ;2 25 buffer
    lds   %B[x],%[next]+1     ;2 27
    ldi   %[ibuf],%[size]-1   ;1 28 max number of bytes

          ; inter bit delay (divisor - 8) >> 2
    lds   %[delay],%[divisor] ;2 30
    subi  %[delay],8          ;1 31 delay till next bit
    lsr   %[delay]            ;1 32
    lsr   %[delay]            ;1 33

          ; short break up to breakto low bits
    lds   %[ibit],%[breakto]  ;2    max number of break bits
loopbrto%=:                   ;  (4*idel + 8)*ibit
    mov   %[idel],%[delay]    ;1    idel = (divisor - 8) >> 2
loopbito%=:                   ;  4*idel
    dec   %[idel]             ;1
    sbis  %[pin],%[bit]       ;?    still low?
    brne  loopbito%=          ;? 4*idel end
    rjmp  .                   ;2 02
    rjmp  .                   ;2 04
    dec   %[ibit]             ;1 05
    sbis  %[pin],%[bit]       ;? 06 still low?
    brne  loopbrto%=          ;? (4*idel + 8)*ibit end
    sbic  %[pin],%[bit]       ;?    still low?
    rjmp  readbyte%=          ;2    shorter than breakto, so ignore
          ; now we have over breakto low bits

    lds   %[idel],%[flag]     ;     set break active flag
    ori   %[idel],%[brka]
    sts   %[flag],%[idel]
    rjmp  done%=              ;     keep waiting

          ; wait for start bit
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
    sbic  %[pin],%[bit]       ;?
    ori   %[recv],0x80        ;1 03 set msb
    rjmp  .                   ;2 05 nop
    dec   %[ibit]             ;1 06
    brne  loop%=              ;? 08 more bits?

    mov   %[idel],%[delay]    ;1    1 bit
delayt%=:                     ;     wait past last bit
    dec   %[idel]             ;1 00
    nop                       ;1 01
    brne  delayt%=            ;? 4*delay
          ; entire byte received

    sbis  %[pin],%[bit]       ;?    check for low
    rjmp  error%=

    st    X+,%[recv]          ;2 00 store byte
    dec   %[ibuf]             ;1 03
    brne  readbyte%=          ;? 05
          ; buffer full
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
    rjmp  done%=

error%=:
    lds   %[idel],%[flag]     ;     set error flag
    ori   %[idel],%[err]
    sts   %[flag],%[idel]

done%=:
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
      ,[breakto] "m" (UartSoft::break_timeout)
      ,[next] "m" (UartSoft::next)
      ,[flag] "m" (UartSoft::flag)
      ,[brk] "I" (UartSoft::flBreak)
      ,[brka] "I" (UartSoft::flBreakActive)
      ,[err] "I" (UartSoft::flError)
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
