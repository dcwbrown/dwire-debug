;;;       Bell Push - ring AC and DC bells with minimum ring period
;;
;;




;         ATTiny45 registers used in this program

          .equ   PCMSK,  0x15  ; Pin change mask
          .equ   DDRB,   0x17  ; Data direction
          .equ   PORTB,  0x18  ; Data port
          .equ   OCR0A,  0x29  ; Output compare register A
          .equ   TCCR0A, 0x2A  ; Timer/counter control register A
          .equ   TCNT0,  0x32  ; Timer/counter 0 current count
          .equ   TCCR0B, 0x33  ; Timer/counter control register B
          .equ   MCUCR,  0x35  ; MCU control register
          .equ   TIMSK,  0x39  ; Timer/counter interrupt mask register
          .equ   GIFR,   0x3A  ; General interrupt flag register
          .equ   GIMSK,  0x3B  ; General interrupt mask register
          .equ   SPL,    0x3D  ; Stack pointer (low byte)
          .equ   SPH,    0x3E  ; Stack pointer (high byte)
          .equ   SREG,   0x3F  ; Status register


;         Constants

          .equ   CYCLETIME, 39            ; Set timer to clear after 39 counts (approx 10ms)
          .equ   PUSHTIME,  CYCLETIME*3/4 ; Number of counts per cycle sufficient to recognise a bell push
          .equ   HAMMERS,   30            ; Number of hammer current reversals following bellpush release


;         Reserved registers
;
;         r18 - TCNT0 at previous bellpush state change
;         r19 - bellpush down time this cycle
;         r20 - pending bell hammer reversal count




;         Flash memory content

          .org   0
interrupt_vectors:
          rjmp   reset       ; 0x0000  RESET
          reti               ; 0x0001  INT0         - external interrupt request 0
          rjmp   bellpush    ; 0x0002  PCINT0       - pin change interrupt request 0
          reti               ; 0x0003  TIMER1_COMPA - timer/counter 1 compare match A
          reti               ; 0x0004  TIMER1_OVF   - timer/counter 1 overflow
          reti               ; 0x0005  TIMER0_OVF   - timer/counter 0 overflow
          reti               ; 0x0006  EE_RDY       - EEPROM ready
          reti               ; 0x0007  ANA_COMP     - analog comparator
          reti               ; 0x0008  ADC          - ADC conversion complete
          reti               ; 0x0009  TIMER1_COMPB - timer/counter 1 compare match B
          rjmp   cycle       ; 0x000A  TIMER0_COMPA - timer/counter 0 compare match A
          reti               ; 0x000B  TIMER0_COMPB - timer/counter 0 compare match B
          reti               ; 0x000C  WDT          - watchdog time-out
          reti               ; 0x000D  USI_START    - universal serial interface start
          reti               ; 0x000E  USI_OVF      - universal serial interface overflow

reset:

;         Set stack pointer to allow 32 bytes above IO space for stack

          ldi    r16,0x7f
          out    SPL,r16

          ldi    r16,0
          out    SPH,r16

;         Initialise PORTB:
;
;         B0 - bell push input, normally high, grounded on button press
;         B1 - unused (set as input with pullup)
;         B2 - unused (set as input with pullup)
;         B3 - HBridge output
;         B4 - HBridge output
;         B5 - unused as port (overriden as reset + dWire)

          ldi    r16,0x18    ; B3, B4 as oupts, rest as inputs
          out    DDRB,r16

          ldi    r16,7       ; Enable input pullups, zero ouputs
          out    PORTB,r16

          ldi    r16,1       ; Enable B0 pin change interrupt
          out    PCMSK,r16

          ldi    r16,0x20    ; Enable pin change interrupts
          out    GIMSK,r16


;         Set up 100hz cycle interrupt

          ldi    r16,2       ; Disable output compare pins, set CTC mode
          out    TCCR0A,r16  ; (CTC = Clear Timer on Compare match)

          ldi    r16,4       ; Set counter clock to IO clock / 256 (256 uS)
          out    TCCR0B,r16

          ldi    r16,39      ; Interrupt every 39 counts (approx 10 mS)
          out    OCR0A,r16

          ldi    r16,0x10    ; Enable interrupt on Timer/Counter0 compare match
;         out    TIMSK,r16   ; OCF0A

;         Enable sleep

          ldi    r16,0x30    ; Sleep instruction causes power down
          out    MCUCR,r16


;         Initialise permanently assigned registers

          in     r18,TCNT0   ; Reset bellpush registers for start of cycle
          clr    r19         ; Reset accumulated push time at start of cycle
          clr    r20         ; Initialise ding count to zero


;         Enable interrupts and sleep until bellpush or cycle interrupts

          sei

;         Sleep until pin change or timer/counter interrupt (approx 10ms)

sleep:    in     r10,TCNT0   ; Temporary for testing timer enable in the debugger
          rjmp   sleep






;;        Bellpush input pin change event
;
;         Called on every bellpush pin state change
;         Bellpush state is reset at every cycle
;
;         Debouncing and denoising:
;
;         Counts the pushed time. At the end of a cycle if the pushed time is
;         over 3/4 the cycle time the button is considered pushed.
;
;         Register usage for tracking bellpush down time
;
;         r18 - time at previous bellpush state change
;         r19 - bellpush down time this cycle

;         On bellpush state change

bellpush: sbic   PORTB,0     ; Skip if button just released
          rjmp   bell2

;         Button has just been released, accumulate latest press time

          in     r10,TCNT0
          sub    r10,r18     ; Calculate how long button was pressed
          add    r19,r10     ; Accumulate total press time this cycle

bell2:    in     r18,TCNT0
          reti






;;        100 Hz counter/timer interrupt handler
;
;         Dispatches to button and bell handlers as apprropriate


cycle:    push   r0
          push   r1
          in     r0,SREG

;         Check for button press in previous cycle

          sbis   PORTB,0     ; Skip if button current pressed at end of cyle
          rjmp   released

          in     r10,TCNT0   ; Include end of cyle push time in accumulated count
          sub    r10,r18
          add    r19,r10

released: cpi    r19,PUSHTIME
          brlo   nopush      ; If button was not pushed this cycle

          ldi    r20,HAMMERS ; Set number of hammer reversals required (abt 1 second)

nopush:

;         Reset bellpush tracking registers for start of cycle

          in     r18,TCNT0
          clr    r19         ; Reset accumulated push time at start of cycle

;         Check for pending hammer reversals

          tst    r20
          breq   cycX        ; If there are no dings pending


;         Dings pending - is this the last one?

cyc2:     dec    r20
          brne   cyc4        ; If this was not the last ding


;         This is the last cycle, set h-bridge to high impedance

last:     cbi    PORTB,3
          cbi    PORTB,4
          rjmp   cycX


;         This is not the last cycle, set the hbridge control according
;         to the parity of the ding count.

cyc4:     sbrc   r20,0
          rjmp   cyc6

          sbi    PORTB,3
          cbi    PORTB,4
          rjmp   cycX


cyc6:     cbi    PORTB,3
          sbi    PORTB,4

;         Exit cycle handler

cycX:     out    SREG,r0
          pop    r1
          pop    r0
          reti

