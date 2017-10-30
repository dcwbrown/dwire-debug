/* Name: main.c

 -----------------------------------------------------------
 - Little Wire
 - Firmware Version: 1.3
 - Modified by: ihsan Kehribar, 2013 September
 -----------------------------------------------------------

 -----------------------------------------------------------
 - Little Wire
 - Firmware Version: 1.2
 - Modified by: ihsan Kehribar, 2013 April
 -----------------------------------------------------------

 -----------------------------------------------------------
 - Little Wire
 - Firmware Version: 1.1
 - Modified by: ihsan Kehribar, 2012 July
 -----------------------------------------------------------

  modified by ihsan Kehribar, 2011 November

  created by chris chung, 2010 April

  based on the works found in

  v-usb framework http://www.obdev.at/vusb/
   Project: Thermostat based on AVR USB driver
   Author: Christian Starkjohann

  usbtiny isp http://www.xs4all.nl/~dicks/avr/usbtiny/
  Dick Streefland

  please observe licensing term from the above two projects

  Copyright (C) 2010  chris chung

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */


#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdlib.h>
#include "usbdrv.h"
#include "digital.h"
#define delayMicroseconds(value) _delay_us(value);
#define sbi(register,bit) (register|=(1<<bit))
#define cbi(register,bit) (register&=~(1<<bit))
const uint8_t LITTLE_WIRE_VERSION = 0x13;
enum
{
  // Generic requests
  USBTINY_ECHO  = 0,  // echo test
  USBTINY_READ  = 1,  // read byte (wIndex:address)
  USBTINY_WRITE = 2,  // write byte (wIndex:address, wValue:value)
  USBTINY_CLR   = 3,  // clear bit (wIndex:address, wValue:bitno)
  USBTINY_SET   = 4,  // set bit (wIndex:address, wValue:bitno)

  // Programming requests
  USBTINY_POWERUP      = 5,   // apply power (wValue:SCK-period, wIndex:RESET)
  USBTINY_POWERDOWN    = 6,   // remove power from chip
  USBTINY_SPI          = 7,   // issue SPI command (wValue:c1c0, wIndex:c3c2)
  USBTINY_POLL_BYTES   = 8,   // set poll bytes for write (wValue:p1p2)
  USBTINY_FLASH_READ   = 9,   // read flash (wIndex:address)
  USBTINY_FLASH_WRITE  = 10,  // write flash (wIndex:address, wValue:timeout)
  USBTINY_EEPROM_READ  = 11,  // read eeprom (wIndex:address)
  USBTINY_EEPROM_WRITE = 12,  // write eeprom (wIndex:address, wValue:timeout)

  // Additional requests - ihsanKehribar
  USBTINY_PIN_SET_INPUT       = 13,
  USBTINY_PIN_SET_OUTPUT      = 14,
  USBTINY_READ_ADC            = 15,
  USBTINY_SETUP_PWM           = 16,
  USBTINY_UPDATE_PWM_COMPARE  = 17,
  USBTINY_PIN_SET_HIGH        = 18,
  USBTINY_PIN_SET_LOW         = 19,
  USBTINY_PIN_READ            = 20,
  USBTINY_SINGLE_SPI          = 21,
  USBTINY_CHANGE_PWM_PRESCALE = 22,
  USBTINY_SETUP_SPI           = 23,

  USBTINY_SETUP_I2C           = 24,
  USBTINY_I2C_BEGIN_TX        = 25,
  USBTINY_I2C_ADD_BUFFER      = 26,
  USBTINY_I2C_SEND_BUFFER     = 27,
  USBTINY_SPI_ADD_BUFFER      = 28,
  USBTINY_SPI_SEND_BUFFER     = 29,
  USBTINY_I2C_REQUEST_FROM    = 30,

  USBTINY_SPI_UPDATE_DELAY    = 31,
  USBTINY_STOP_PWM            = 32,
  USBTINY_DEBUG_SPI           = 33,
  USBTINY_VERSION_QUERY       = 34,
};

#define PORT  PORTB
#define DDR DDRB
#define PIN PINB

uint8_t* EE_addr = (uint8_t*)32;
static inline void initSerialNumber();
int usbDescriptorStringSerialNumber[] = {
  USB_STRING_DESCRIPTOR_HEADER( USB_CFG_SERIAL_NUMBER_LEN ),
  USB_CFG_SERIAL_NUMBER
};

//
// to reduce pin count so that this can fit in a 8 pin tiny
// . no power nor ground pins to target, they are to be connected always
// . no reset control pin to target, target reset always grounded
//   * this had caused problem and there are two solutions
//     1. provide a toggle switch to off-on-off target reset to ground
//     2. introduce reset control and use reset pin as io
//
#define POWER_MASK  0x00
#define GND_MASK  0x00

#define RESET_MASK  (1 << 5)
#define SCK_MASK  (1 << 2)
#define MISO_MASK (1 << 1)
#define MOSI_MASK (1 << 0)

#define SPI_BUFFER_SIZE 16
#define checkBit(reg,bit) (reg&(1<<bit))

// ----------------------------------------------------------------------
// Local data
// ----------------------------------------------------------------------
static uchar    sck_period=10; // SCK period in microseconds (1..250)
static uchar    poll1;         // first poll byte for write
static uchar    poll2;         // second poll byte for write
static unsigned address;       // read/write address
static unsigned timeout;       // write timeout in usec
static uchar    cmd0;          // current read/write command byte
static uchar    cmd[4];        // SPI command buffer
static uchar    res[4];        // SPI result buffer
static uint16_t SPI_DELAY=10;  // in microseconds. USI driven SPI mode
static uint16_t I2C_DELAY=0;   // in microseconds. USI driven SPI mode
// ----------------------------------------------------------------------
volatile uint8_t sendBuffer[9];
volatile uint8_t rxBuffer[8];
static   uint8_t t,q;
static   uint8_t counter=0;
static   uint8_t softPWM=0;
static   uint8_t cmp0,cmp1,cmp2,compare0,compare1,compare2;
static   uint8_t adcSetting=0;
volatile uint8_t jobState=0;
// ----------------------------------------------------------------------
#define DATA_PIN 2
// ----------------------------------------------------------------------
// ws2812 support
#define ws2812_maxleds 64
static uint8_t ws2812_grb[ws2812_maxleds*3];
static uint8_t ws2812_mask;
static uint8_t ws2812_ptr=0;
// ----------------------------------------------------------------------
// debugWIRE support
volatile uint8_t dwBuf[128];
volatile uint8_t dwLen;        // Length being received from host or avr device
volatile uint8_t dwIn;         // Input pointer: where usbDwFunctionWrite writes into dwBuf
volatile uint8_t dwState;      // Current debugWIRE action underway, 0 if none
// ----------------------------------------------------------------------



// -----------------------------------------------------------------------
// Delay support.
// Since by design we are fixed at 16.5MHz, we don't need the generality
// of utility/delay.h, and can avoid the overhead of many math calculations.
// -----------------------------------------------------------------------

void _delay_us(uint16_t us) {  // microsecond parameter is passed in r25:r24
  asm(
    "                                                                       \n"
    ";       Delay 1 to 65536 microseconds (pass 0 for 65536 microseconds)  \n"
    "                                                                       \n"
    ";       Assume we enter with 5 cycles already used:                    \n"
    ";         Microsecond parameter load (e.g. 2 x ldi) has taken 2 cycles \n"
    ";         Call has taken 3 cycles (4 if more than 2k words distant)    \n"
    "                                                                       \n"
    ";       Want to reach dus10 on a multiple of 16.5 + 12.5 including     \n"
    ";       call time but excluding return time.                           \n"
    "                                                                       \n"
    ";       E.g. cycles taken inc setup, call and return will be:          \n"
    ";actual 16,   33, 49,   66, 82,   99, 115    ...                       \n"
    ";ideal  16.5, 33, 49.5, 66, 82.5, 99, 115.5  ...                       \n"
    "                                                                       \n"
    "        rjmp  dus8            ; 2. 7  Enter loop allowing for call     \n"
    "                              ;       and returnn overhead             \n"
    "                                                                       \n"
    ";       One microsecond takes 16.5 cycles, so use a loop of alternate  \n"
    ";       16 and 17 cycle delays.                                        \n"
    "                                                                       \n"
    "                              ;          13  46  79                    \n"
    "dus2:   ldi   r18,4           ; +                                      \n"
    "dus4:   dec   r18             ;  > 12.                                 \n"
    "        brne  dus4            ; +        25  58  91                    \n"
    "        sbiw  r24,1           ; 2.       27  60  93                    \n"
    "        brcs  dus10           ; If complete reaches dus10 at 29 + n*33 \n"
    "                              ;          28  61  94                    \n"
    "        ldi   r18,4           ; +                                      \n"
    "dus6:   dec   r18             ;  > 12.                                 \n"
    "        brne  dus6            ; +        40  73  106                   \n"
    "dus8:   rjmp  .               ; 2.    9  42  75  108                   \n"
    "        sbiw  r24,1           ; 2.   11  44  77  110                   \n"
    "        brcc  dus2            ; If more delay required                 \n"
    "                                                                       \n"
    "dus10:                        ;      12  45  78  111                   \n"
    "                                                                       \n"
    ";       Return takes 4 cycles        16  49  82  115                   \n"
  :::"r18");
}

void _delay_ms(uint16_t ms) {  // millisecond parameter is passed in r25:r24
  asm(
    "                                                                       \n"
    ";       Delay 1 to 65536 milliseconds (pass 0 for 65536 milliseconds)  \n"
    "                                                                       \n"
    "        movw  r26,r24         ; Loop count in r27:r26                  \n"
    "                                                                       \n"
    "dms2:   rjmp  .               ; 2. 2                                   \n"
    "        rjmp  .               ; 2. 4                                   \n"
    "        rjmp  .               ; 2. 6                                   \n"
    "        rjmp  .               ; 2. 8                                   \n"
    "        rjmp  .               ; 2. 10                                  \n"
    "        rjmp  .               ; 2. 12                                  \n"
    "        ldi   r24,lo8(999)    ;                                        \n"
    "        ldi   r25,hi8(999)    ;                                        \n"
    "        rcall _delay_us       ;    16495                               \n"
    "        nop                   ; 1. 16496                               \n"
    "        sbiw  r26,1           ; 2. 16498                               \n"
    "        brcc  dms2            ; if more, 16500                         \n"
  :::"r18","r26", "r27");
}




// -----------------------------------------------------------------------



#if 0
// MCLR 0x04 (100)  => SCK  -pin2
// PGD  0x02 (010)  => MISO -pin1
// PGC  0x01 (001)  => MOSI -pin4

static inline void ClockControlBits(uint8_t value)
{
  uint8_t i;
  DDRB |= 0x02;
  uint8_t mask = 0x01;
  while (mask != 0x10)
  {
    if(value & mask)
      PORTB = (PORTB & ~0x01) | 0x02;
    else
      PORTB &= 0xFC;
    mask <<= 1;
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
    PORTB |= 0x01; //pop clock
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
  }
  PORTB &= ~0x03; // drop clock & PGD
}

static inline void ClockLowXTimes(uint8_t value)
{
  uint8_t i;
  DDRB |= 0x02;
  uint8_t length = value;
  while(length)
  {
    PORTB &= ~0x03; //PGD + PGC low
    length--;
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
    PORTB |= 0x01;
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
  }
  PORTB &= ~0x03;
}

static inline void ClockByte(uint8_t value)
{
  uint8_t i;
  DDRB |= 0x02; // switch PGD to output

  uint8_t mask = 0x01;
  while(mask)
  {
    if(value & mask)
      PORTB = (PORTB & ~0x01) | 0x02;
    else
      PORTB &= ~0x03;
    mask <<= 1;
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
    PORTB |= 0x01; //pop clock
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
  }
  PORTB &= ~0x03;
}

static inline void PinByte(uint8_t *value)
{
  uint8_t i;
  DDRB &= ~0x02; // Switch PGD to input
  PORTB |= 0x02; // activate pull-up...

  uint8_t mask = 0x01;
  while(mask)
  {
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
    PORTB |= 0x01; //pop clock
    for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
    if(PINB & 0x02)
      *value |= mask;
    else
      *value &= ~mask;
    PORTB &= ~0x01; //drop clock
    mask <<= 1;
  }
  PORTB &= ~0x02; //deactivate pull-up...
}

#endif




// ----------------------------------------------------------------------
// Delay exactly <sck_period> times 0.485 microseconds (8 cycles).
// ----------------------------------------------------------------------
__attribute__((always_inline))
static inline void spi_delay ( void )
{
  asm volatile(
    "        mov   __tmp_reg__,%0    \n"
    "spd%=:  rjmp  .                 \n"
    "        rjmp  .                 \n"
    "        nop                     \n"
    "        dec   __tmp_reg__       \n"
    "        brne  spd%=             \n"
    : : "r" (sck_period) );
}




// ----------------------------------------------------------------------
// Issue one SPI command.
// ----------------------------------------------------------------------
static  void  spi ( uchar* cmd, uchar* res )
{
  uchar i;
  uchar c;
  uchar r;
  uchar mask;

  for ( i = 0; i < 4; i++ )
  {
    c = *cmd++;
    r = 0;
    for ( mask = 0x80; mask; mask >>= 1 )
    {
      if  ( c & mask )
      {
        PORT |= MOSI_MASK;
      }
      spi_delay();
      PORT |= SCK_MASK;
      spi_delay();
      r <<= 1;
      if  ( PIN & MISO_MASK )
      {
        r++;
      }
      PORT &= ~MOSI_MASK;
      PORT &= ~SCK_MASK;
    }
    *res++ = r;
  }
}

// ----------------------------------------------------------------------
// Create and issue a read or write SPI command.
// ----------------------------------------------------------------------
static  void  spi_rw ( void )
{
  unsigned  a;

  a = address++;
  if  ( cmd0 & 0x80 )
  { // eeprom
    a <<= 1;
  }
  cmd[0] = cmd0;
  if  ( a & 1 )
  {
    cmd[0] |= 0x08;
  }
  cmd[1] = a >> 9;
  cmd[2] = a >> 1;
  spi( cmd, res );
}

// ----------------------------------------------------------------------
// Handle an spi IN packet.
// ----------------------------------------------------------------------
uchar usbFunctionRead(uchar *data, uchar len)
{
  uchar i;

  for ( i = 0; i < len; i++ )
  {
    spi_rw();
    data[i] = res[3];
  }
  return len;
}

// ----------------------------------------------------------------------
// Handle a dWIRE or spi OUT packet.
// ----------------------------------------------------------------------
uchar usbFunctionWrite(uchar *data, uchar len)
{
  uchar i;
  unsigned  usec;
  uchar r;
  //uchar last = (len != 8);

  if (dwState) { // Handle a debugWIRE OUT data packet

    uint8_t isLastBlock = dwIn + len >= dwLen;
    if (isLastBlock) {
      len = dwLen - dwIn;
      jobState = 20;
    }
    for (i=0; i<len; i++) dwBuf[dwIn++] = data[i];
    return isLastBlock;

  } else { // Handle an SPI OUT data packet

    for ( i = 0; i < len; i++ )
    {
      cmd[3] = data[i];
      spi_rw();
      cmd[0] ^= 0x60; // turn write into read
      //
      for ( usec = 0; usec < timeout; usec += 32 * sck_period )
      { // when timeout > 0, poll until byte is written
        spi( cmd, res );
        r = res[3];
        if  ( r == cmd[3] && r != poll1 && r != poll2 )
        {
          break;
        }
      }
    }
    //return last;
    return 1;
  }
}




/*
  Bitbang WS2812 write - T. Böscke May 26th, 2013
             - clean up May 27th, 2013

  This routine writes an array of bytes with RGB values to the Dataout pin
  using the fast 800kHz clockless WS2811/2812 protocol.

  The order of the color-data is GRB 8:8:8. Serial data transmission begins
  with the most significant bit in each byte.

  The total length of each bit is 1.25µs (20 cycles @ 16Mhz)
  * At 0µs the dataline is pulled high.
  * To send a zero the dataline is pulled low after 0.375µs (6+1 cycles).
  * To send a one the dataline is pulled low after 0.625µs (10+1 cycles).

  After the entire bitstream has been written, the dataout pin has to remain low
  for at least 50µs (reset condition).

*/
#define ws2812_port PORTB   // Data port register

void ws2812_sendarray_mask(uint8_t *data,uint16_t datlen,uint8_t maskhi)
{
  uint8_t curbyte,ctr,masklo;
  masklo  =~maskhi&ws2812_port;
  maskhi |=ws2812_port;

  while (datlen--) {
    curbyte=*data++;

    asm volatile(
      "                                                \n"
      "         ldi   %0,8     ; 0                     \n"
      "loop%=:  out   %2,%3    ; 1                     \n"
      "         lsl   %1       ; 2                     \n"
      "         dec   %0       ; 3                     \n"
      "                                                \n"
      "         rjmp  .+0      ; 5                     \n"
      "                                                \n"
      "         brcs  .+2      ; 6l / 7h               \n"
      "         out   %2,%4    ; 7l / -                \n"
      "                                                \n"
      "         rjmp  .+0      ; 9                     \n"
      "                                                \n"
      "         nop            ; 10                    \n"
      "         out   %2,%4    ; 11                    \n"
      "         breq  end%=    ; 12      nt. 13 taken  \n"
      "                                                \n"
      "         rjmp  .+0      ; 14                    \n"
      "         rjmp  .+0      ; 16                    \n"
      "         rjmp  .+0      ; 18                    \n"
      "         rjmp  loop%=   ; 20                    \n"
      "end%=:                                          \n"

      :  "=&d" (ctr)
      :  "r" (curbyte), "I" (_SFR_IO_ADDR(ws2812_port)), "r" (maskhi), "r" (masklo)
    );

    // loop overhead including byte load is 6+1 cycles
  }
}


/* ------------------------------------------------------------------------- */
/* ------------------------ interface to USB driver ------------------------ */
/* ------------------------------------------------------------------------- */


// ----------------------------------------------------------------------
// Handle a non-standard SETUP packet.
// ----------------------------------------------------------------------

uchar usbFunctionSetup(uchar data[8])
{
// ----------------------------------------------------------------------
//
// Nominal packet format:
//
//   data[0] - Request type
//   data[1] - Request id
//   data[2] - lo8(Value)
//   data[3] - hi8(Value)
//   data[4] - lo8(Index)
//   data[5] - hi8(Index)
//   data[6] - lo8(Length)
//   data[7] - hi8(Length)
//
// usbFunctionSetup is called when request type is not USBRQ_TYPE_STANDARD.
//
// 'Value' and 'Index' are names assigned by the USB standard for some
// request types, but we are free to use them as 4 arbitrary data bytes.
//
// ----------------------------------------------------------------------
  uint8_t i;
  uchar bit;
  uchar mask;
  uchar req;

  // Generic requests
  req = data[1];
  if  ( req == USBTINY_ECHO )
  {
    data[1]=0x21;
    usbMsgPtr = data;
    return 8;
  }

  if  ( req == USBTINY_READ )
  {
    data[0] = PIN;
    usbMsgPtr = data;
    return 1;
  }

  if  ( req == USBTINY_WRITE )
  {
    PORT = data[2];
    return 0;
  }

  bit = data[2] & 7;
  mask = 1 << bit;
  if  ( req == USBTINY_CLR )
  {
    PORT &= ~ mask;
    return 0;
  }

  if  ( req == USBTINY_SET )
  {
    PORT |= mask;
    return 0;
  }

  // Programming requests
  if  ( req == USBTINY_POWERUP )
  {
    sck_period = data[2];
    mask = POWER_MASK;
    if  ( data[4] )
    {
      mask |= RESET_MASK;
    }
    DDR  &= ~MISO_MASK;
    DDR  |= (RESET_MASK|SCK_MASK|MOSI_MASK);
    PORT &= ~(RESET_MASK|SCK_MASK|MOSI_MASK|MISO_MASK);
    return 0;
  }

  if  ( req == USBTINY_POWERDOWN )
  {
    /* set all the pins to Hi-Z */
    pinMode(B,0,INPUT);
    internalPullup(B,0,DISABLE);
    pinMode(B,1,INPUT);
    internalPullup(B,1,DISABLE);
    pinMode(B,2,INPUT);
    internalPullup(B,2,DISABLE);
    pinMode(B,5,INPUT);
    internalPullup(B,5,DISABLE);
    return 0;
  }

  if  ( req == USBTINY_SPI )
  {
    spi( data + 2, data + 0 );
    usbMsgPtr = data;
    return 4;
  }

  if  ( req == USBTINY_POLL_BYTES )
  {
    poll1 = data[2];
    poll2 = data[3];
    return 0;
  }

  address = * (unsigned*) & data[4];
  if  ( req == USBTINY_FLASH_READ )
  {
    cmd0 = 0x20;
    return 0xff;  // usb_in() will be called to get the data
  }

  if  ( req == USBTINY_EEPROM_READ )
  {
    cmd0 = 0xa0;
    return 0xff;  // usb_in() will be called to get the data
  }

  timeout = * (unsigned*) & data[2];
  if  ( req == USBTINY_FLASH_WRITE )
  {
    cmd0 = 0x40;
    return 0xff;  // data will be received by usb_out()
  }

  if  ( req == USBTINY_EEPROM_WRITE )
  {
    cmd0 = 0xc0;
    return 0xff;  // data will be received by usb_out()
  }

  // - ihsan Kehribar -
  if( req == USBTINY_PIN_SET_INPUT ) // 13
  {
    DDR &= ~(1<<bit);
    return 0;
  }

  if( req == USBTINY_PIN_SET_OUTPUT ) // 14
  {
    DDR |= (1<<bit);
    return 0;
  }

  if( req == USBTINY_READ_ADC ) // 15
  {
    // unsigned char temp=DDR;

    if(data[2]==0) // Read ADC from RESET pin
    {
      ADMUX = adcSetting | 0; // Set the desired channel
      sbi(DIDR0,ADC0D); // Disable digital buffer ...
      // DDR &= ~(1<<5);
    }
    else if(data[2]==1) // Read ADC from SCK pin
    {
      ADMUX = adcSetting | 1; // Set the desired channel
      sbi(DIDR0,ADC1D); // Disable digital buffer ...
      // DDR &= ~(1<<2);
    }
    else if(data[2]==2) // Read ADC from internal Temperature sensor
    {
      ADMUX=0b10001111;
    }
    sbi(ADCSRA,ADSC); // Start conversion
    while(checkBit(ADCSRA,ADSC)); // Wait for conversion
    data[0]=(unsigned char)ADCL;
    data[1]=(unsigned char)ADCH;
    usbMsgPtr = data;
    // DDR=temp;
    DIDR0=0x00;
    return 2;
  }

  if( req == USBTINY_SETUP_PWM ) // 16
  {
    DDR |= (1<<0); // Set PORTB0 Output
    DDR |= (1<<1); // Set PORTB1 Output
    TCCR0A |= (1<<COM0A1)|(0<<COM0A0)|(1<<COM0B1)|(0<<COM0B0); // Clear OC0A/OC0B on Compare Match, set OC0A/OC0B at BOTTOM (non-inverting mode)
    TCCR0A |= (1<<WGM01)|(1<<WGM00); // Fast PWM mode
    TCCR0B |= (1<<CS02)|(0<<CS01)|(1<<CS00); // Timer prescale: 1024 => PWM signal update frequency: ( 16.500.000 / ( 255 * 1024) ) = ~ 63 Hertz
    return 0;
  }

  if( req == USBTINY_UPDATE_PWM_COMPARE ) // 17
  {
    OCR0A=data[2];
    OCR0B=data[4];
    return 0;
  }

  if( req == USBTINY_PIN_SET_HIGH ) // 18
  {
    PORT |= (1<<bit);
    return 0;
  }

  if( req == USBTINY_PIN_SET_LOW ) // 19
  {
    PORT &= ~(1<<bit);
    return 0;
  }

  if( req == USBTINY_PIN_READ ) // 20
  {
    data[0]=(PIN&(1<<bit))>>bit;
    usbMsgPtr = data;
    return 1;
  }

  //-----------------------------------------------//
  if( req == USBTINY_CHANGE_PWM_PRESCALE ) // 22
  {
    if(data[2]==0) // ~ 64kHz update frequency
    {
       cbi(TCCR0B,CS02);
       cbi(TCCR0B,CS01);
       sbi(TCCR0B,CS00);
    }
    else if(data[2]==1) // ~ 8kHz Hertz update frequency
    {
       cbi(TCCR0B,CS02);
       sbi(TCCR0B,CS01);
       cbi(TCCR0B,CS00);
    }
    else if(data[2]==2) // ~ 1 kHz update frequency
    {
       cbi(TCCR0B,CS02);
       sbi(TCCR0B,CS01);
       sbi(TCCR0B,CS00);
    }
    else if(data[2]==3) // ~ 252 Hertz update frequency
    {
       sbi(TCCR0B,CS02);
       cbi(TCCR0B,CS01);
       cbi(TCCR0B,CS00);
    }
    else if(data[2]==4) // ~ 63 Hertz update frequency -> Suitable for Servo driving. -> Default
    {
       sbi(TCCR0B,CS02);
       cbi(TCCR0B,CS01);
       sbi(TCCR0B,CS00);
    }
    return 0;
  }

  if( req == USBTINY_SPI_UPDATE_DELAY ) // 31
  {
    SPI_DELAY = (data[3]<<8) + data[2] ;
    return 0;
  }

  if( req == USBTINY_STOP_PWM ) // 32
  {
    TCCR0A = 0;
    TCCR0B = 0;
    return 0;
  }

  if( req == USBTINY_DEBUG_SPI ) // 33
  {
    rxBuffer[0] = data[2]; // Data to send
    jobState = 1;
    return 0;
  }

  if( req == 34 ) // This has to be hardcoded to 34!
  {
    data[0]=LITTLE_WIRE_VERSION;
    usbMsgPtr = data;
    return 1;
  }

  if ( req == 35 ) /* init ADC */
  {
    sbi(ADCSRA,ADEN); // Enable the ADC peripheral
    ADCSRA |= data[2]; // Least significant three bits are prescale settings
    switch(data[3])
    {
      case 0: // Use VCC as Vref.
        adcSetting = 0;
        break;
      case 1: // 1.1V
        sbi(adcSetting,REFS1);
        break;
      case 2: // 2.56V
        sbi(adcSetting,REFS1);
        sbi(adcSetting,REFS2);
        break;
    }
    return 0;
  }

  if (req == 40) /* read the results */
  {
    usbMsgPtr = (uchar*)sendBuffer;
    return sendBuffer[8];
  }

  if (req == 41) /* onewire reset pulse */
  {
    jobState = 2;
    return 0;
  }

  if ( req == 42) /* onewire send byte */
  {
    jobState = 3;
    rxBuffer[0]=data[2];
    return 0;
  }

  if (req == 43) /* onewire read byte */
  {
    jobState = 4;
    return 0;
  }

  if (req == 44) /* i2c init */
  {
    jobState=8;
    return 0;
  }

  if (req == 45) /* i2c begin */
  {
    jobState=9;
    rxBuffer[0]=data[2]; // -- address
    return 0;
  }

  if (req == 46) /* i2c read */
  {
    jobState=11;
    rxBuffer[0]=data[2]; // -- should we end with Nack ?
    rxBuffer[1]=data[3]; // -- length
    rxBuffer[2]=data[4]; // -- should we issue a Stop ?
    return 0;
  }

  if (req == 47) /* init softPWM */
  {
    if(data[2]==1)
    {
      pinMode(B,0,OUTPUT);
      pinMode(B,1,OUTPUT);
      pinMode(B,2,OUTPUT);
      softPWM=1;
    }
    else
      softPWM=0;
    return 0;
  }

  if( req == 48) /* update softPWM */
  {
    compare0=data[2];
    compare1=data[3];
    compare2=data[4];
    return 0;
  }

  if( req == 49) /* i2c update delay */
  {
    I2C_DELAY = data[2];
    return 0;
  }

  if( req == 50 ) /* onewire read bit */
  {
    jobState=5;
    return 0;
  }

  if( req == 51 ) /* onewire write bit */
  {
    rxBuffer[0]=data[2];
    jobState=6;
    return 0;
  }

#if 0
  // --- experimental --
  if( req == 52 ) /* pic24f programming? */
  {
    jobState=data[2];
    rxBuffer[0]=data[3];
    rxBuffer[1]=data[4];
    rxBuffer[2]=data[5];
    return 0;
  }
  if( req == 53 ) /* pic24f sendsix? */
  {
    rxBuffer[0]=data[2]; // NOP count
    rxBuffer[1]=data[3];
    rxBuffer[2]=data[4];
    rxBuffer[3]=data[5];
    jobState=16;
    return 0;
  }
#endif

// WS2812 Support - T. Böscke May 26th, 2013
  if( req == 54 ) /* WS2812_write */
  {
    if ((data[2]&0x20)&&(ws2812_ptr<ws2812_maxleds*3))  // bit 5 set = add to buffer
    {
      ws2812_grb[ws2812_ptr++]=data[3];
      ws2812_grb[ws2812_ptr++]=data[4];
      ws2812_grb[ws2812_ptr++]=data[5];
    }

    if ((data[2]&0x10)&&(ws2812_ptr>0))  // bit 4 set = send buffer to ws2812
    {
      jobState=17;
      DDRB|=mask;
      ws2812_mask=mask;
    }
    return 0;
  }

// end ws2812 support

  /* Change serial number ... */
  if( req == 55)
  {
    eeprom_write_byte((EE_addr+0),data[2]);
    eeprom_write_byte((EE_addr+1),data[3]);
    eeprom_write_byte((EE_addr+2),data[4]);

    // data[0] = eeprom_read_byte(EE_addr+0);
    // data[1] = eeprom_read_byte(EE_addr+1);
    // data[2] = eeprom_read_byte(EE_addr+2);
    // usbMsgPtr = data;
    // return 3;

    return 0;
  }


  if (req == 60) { // debugWIRE transfer
    if (dwState) {return 0;}   // Prior operation has not yet completed

    if (data[0] & 0x80) {

      // IN transfer - device to host: return buffered data
      usbMsgPtr = (uchar*)dwBuf; // Operation complete, result is in dwBuf
      return dwLen;

    } else {

      // OUT transfer - host to device. rq->wValue specifies action to take.
      dwState = data[2];                // action required, from low byte of rq->wValue
      dwLen   = *((uint16_t*)(data+6)); // rq->wLength
      if (dwLen == 0) {
        jobState = 20; // No out data transfer, go straight to job part
      } else {
        if (dwLen > sizeof(dwBuf)) dwLen = sizeof(dwBuf);
        dwIn = 0;
        return USB_NO_MSG;     // jobState will be set in usbFunctionDwWrite
      }
      return 0;
    }
  }


#if 0
  if ((req & 0xF0) == 0xD0) /* pic24f send bytes */
  {
    rxBuffer[0]=req&0x07; // length

    rxBuffer[1]=data[2];
    rxBuffer[2]=data[3];
    rxBuffer[3]=data[4];
    rxBuffer[4]=data[5];

    jobState=14;
    return 0;
  }
#endif

  // --- experimental --
  if ((req & 0xF0) == 0xE0) // Special multiple I2C message send function
  {
    jobState=10;

    rxBuffer[0]=req&0x07; // length
    rxBuffer[1]=req&0x08; // should we send a Stop?

    for(i=0;i<rxBuffer[0];i++)
      rxBuffer[2+i]=data[2+i];

    return 0;
  }

  if ((req & 0xF0) == 0xF0) // Special multiple SPI message send function
  {
    jobState=7;

    rxBuffer[0]=req&0x08; // auto chip select ?
    rxBuffer[1]=req&0x07; // length

    for(i=0;i<rxBuffer[1];i++)
      rxBuffer[2+i]=data[2+i];

    return 0;
  }

  return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */

/* Calibrate the RC oscillator to 8.25 MHz. The core clock of 16.5 MHz is
 * derived from the 66 MHz peripheral clock by dividing. Our timing reference
 * is the Start Of Frame signal (a single SE0 bit) available immediately after
 * a USB RESET. We first do a binary search for the OSCCAL value and then
 * optimize this value with a neighboorhod search.
 * This algorithm may also be used to calibrate the RC oscillator directly to
 * 12 MHz (no PLL involved, can therefore be used on almost ALL AVRs), but this
 * is wide outside the spec for the OSCCAL value and the required precision for
 * the 12 MHz clock! Use the RC oscillator calibrated to 12 MHz for
 * experimental purposes only!
 */
static void calibrateOscillator(void)
{
uchar       step = 128;
uchar       trialValue = 0, optimumValue;
int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

  /* do a binary search: */
  do{
    OSCCAL = trialValue + step;
    x = usbMeasureFrameLength();    /* proportional to current real frequency */
    if(x < targetValue)             /* frequency still too low */
      trialValue += step;
    step >>= 1;
  }while(step > 0);
  /* We have a precision of +/- 1 for optimum OSCCAL here */
  /* now do a neighborhood search for optimum value */
  optimumValue = trialValue;
  optimumDev = x; /* this is certainly far away from optimum */
  for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
    x = usbMeasureFrameLength() - targetValue;
    if(x < 0)
      x = -x;
    if(x < optimumDev){
      optimumDev = x;
      optimumValue = OSCCAL;
    }
  }
  OSCCAL = optimumValue;
}

void usbEventResetReady(void)
{
  calibrateOscillator();
  eeprom_write_byte(0, OSCCAL);   /* store the calibrated value in EEPROM */
}



// ----------------------------------------------------------------------------
//   bitBang i2c routines
// ----------------------------------------------------------------------------

// Port for the I2C
#define I2C_DDR DDRB
#define I2C_PIN PINB
#define I2C_PORT PORTB

// Pins to be used in the bit banging
#define I2C_CLK 2
#define I2C_DAT 0

#define I2C_DATA_HI() I2C_DDR &= ~( 1 << I2C_DAT );
#define I2C_DATA_LO() I2C_DDR |= ( 1 << I2C_DAT );

#define I2C_CLOCK_HI() I2C_DDR &= ~( 1 << I2C_CLK );
#define I2C_CLOCK_LO() I2C_DDR |= ( 1 << I2C_CLK );

void I2C_WriteBit( unsigned char c )
{
  uint8_t i;
  if ( c > 0 )
  {
    I2C_DATA_HI();
  }
  else
  {
    I2C_DATA_LO();
  }
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_CLOCK_HI();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_CLOCK_LO();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  if ( c > 0 )
  {
    I2C_DATA_LO();
  }
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */
}

unsigned char I2C_ReadBit()
{
  uint8_t i;
  I2C_DATA_HI();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_CLOCK_HI();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  unsigned char c = I2C_PIN;
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_CLOCK_LO();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  return ( c >> I2C_DAT ) & 1;
}

void I2C_Init()
{
  uint8_t i;
  I2C_PORT &= ~( ( 1 << I2C_DAT ) | ( 1 << I2C_CLK ) );

  I2C_CLOCK_HI();
  I2C_DATA_HI();

  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */
}

void I2C_Start()
{
  uint8_t i;
  // set both to high at the same time
  I2C_DDR &= ~( ( 1 << I2C_DAT ) | ( 1 << I2C_CLK ) );
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_DATA_LO();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_CLOCK_LO();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */
}

void I2C_Stop()
{
  uint8_t i;
  I2C_DATA_LO();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_CLOCK_LO();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_CLOCK_HI();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  I2C_DATA_HI();
  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */
}

unsigned char I2C_Write( unsigned char c )
{
  uint8_t i;
  for (i=0;i<8;i++)
  {
    I2C_WriteBit( c & 128 );

    c<<=1;
  }

  return I2C_ReadBit();
}

unsigned char I2C_Read( unsigned char ack )
{
  uint8_t i;
  unsigned char res = 0;

  for (i=0;i<8;i++)
  {
    res <<= 1;
    res |= I2C_ReadBit();
  }

  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  if ( ack > 0)
  {
    I2C_WriteBit( 0 );
  }
  else
  {
    I2C_WriteBit( 1 );
  }

  for(i=0;i<I2C_DELAY;i++) _delay_us(1); /* Small delay */

  return res;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



/* ------------------------------------------------------------------------- */
/* ------------------------------- DebugWIRE ------------------------------- */
/* ------------------------------------------------------------------------- */


// RS232 levels are
//
//   5v ttl  =  mark   =  1  =  idle   =  stop-bit   =  -3v..-15v line
//   0v ttl  =  space  =  0  =  break  =  start-bit  =  +3v..+15v line
//
//   Least significant bit sent first.

uint16_t dwBitTime;      // Each bit takes 4*dwBitTime+8 cycles to transmit

void dwCaptureWidths() {
  // r18 through r25 free to use without restoring
  // r27:r26 is 'x', free to use without restoring
  // r31:r30 is 'z', free to use without restoring

  asm(
    "                                                                                    \n"
    ";       Measure pulse widths and store in dwBuf                                     \n"
    "                                                                                    \n"
    "        ldi   r26,lo8(dwBuf)  ; X register addresses dwBuf                          \n"
    "        ldi   r27,hi8(dwBuf)                                                        \n"
    "        clr   r24             ; Counter for number of measured pulses               \n"
    "                                                                                    \n"
    "        cli                   ; Disable interrupts while measuring pulse widths     \n"
    "        sbi   0x18,%[bit]     ; End break                                           \n"
    "        cbi   0x17,%[bit]     ; Set DDRB 5 (reset/dwire) direction input            \n"
    "                                                                                    \n"
    ";       Cycle loop - track a high pulse followed by a low pulse                     \n"
    "                                                                                    \n"
    "dcw2:   clr   r30             ; 1.                                                  \n"
    "        clr   r31             ; 1.                                                  \n"
    "                                                                                    \n"
    ";       Loop while pin high                                                         \n"
    "                                                                                    \n"
    "dwc4:   adiw  r30,1           ; 2.                                                  \n"
    "        brcs  dwc8            ; 1/2. If no change in 65536 loops (~24ms)            \n"
    "                                                                                    \n"
    "        sbic  0x16,%[bit]     ; 1/2. Skip if Pin PB5 clear (space, zero)            \n"
    "        rjmp  dwc4            ; 2.                                                  \n"
    "                                                                                    \n"
    "        cpi   r24,64          ; 1.   Limit measurement to space available in dwBuf. \n"
    "                              ;      Logically this test would be more              \n"
    "                              ;      sensible at the start of the main dcw2         \n"
    "                              ;      loop. It is here instead using 2 cycles at     \n"
    "                              ;      the end of a low pulse in order to balance     \n"
    "                              ;      the 2 cycles taken by the 'rjmp dcw2' at       \n"
    "                              ;      the end of the high pulse.                     \n"
    "                                                                                    \n"
    "        brsh  dwc8            ; 1.   If dwBuf full                                  \n"
    "                                                                                    \n"
    "        st    x+,r30          ; 2.   Record high pulse time                         \n"
    "        st    x+,r31          ; 2.                                                  \n"
    "        inc   r24             ; 1.                                                  \n"
    "                                                                                    \n"
    "        clr   r30             ; 1.                                                  \n"
    "        clr   r31             ; 1.                                                  \n"
    "                                                                                    \n"
    ";       Loop while pin low                                                          \n"
    "                                                                                    \n"
    "dwc6:   adiw  r30,1           ; 2.                                                  \n"
    "        brcs  dwc8            ; 1/2. If no change in 65536 loops (~24ms)            \n"
    "                                                                                    \n"
    "        sbis  0x16,%[bit]     ; 1/2. Skip if Pin PB5 set (mark, one)                \n"
    "        rjmp  dwc6            ; 2.                                                  \n"
    "                                                                                    \n"
    "        st    x+,r30          ; 2.   Record low pulse time                          \n"
    "        st    x+,r31          ; 2.                                                  \n"
    "        inc   r24             ; 1.                                                  \n"
    "                                                                                    \n"
    "        rjmp  dcw2            ; 2.   Back to start of cycle loop                    \n"
    "                                                                                    \n"
    "                                                                                    \n"
    ";       Measurement complete                                                        \n"
    "                                                                                    \n"
    "dwc8:   sei                   ; Re-enable interrupts                                \n"
    "        add   r24,r24         ; Convert word count to byte count                    \n"
    "        sts   dwLen,r24                                                             \n"
  ::[bit]"I"(DW_BIT):"r24","r26","r27","r30","r31");
}

// Sample pulse widths returned by attiny85 with internal clock as supplied
//
//  pulse   cycles   state description
//  ----- ---------- ----- --------------------------------------------------------
//   [0]  0f0a  3850   1   Idle line while device gets ready to respond
//   [1]  015d  349    0   Start bit (0 = mark)
//   [2]  015a  346    1   bit 0  +
//   [3]  015b  347    0   bit 1  |
//   [4]  015a  346    1   bit 2  |
//   [5]  015d  349    0   bit 3  | 0x55 standard debugWIRE response to break
//   [6]  015a  346    1   bit 4  |
//   [7]  015c  348    0   bit 5  |
//   [8]  0159  345    1   bit 6  |
//   [9]  015d  349    0   bit 7  +




void dwSendBytes() {
  asm(
    "                                                                    \n"
    ";       Transmit dwLen bytes from dwBuf                             \n"
    ";                                                                   \n"
    ";       Register usage:                                             \n"
    ";                                                                   \n"
    ";       r21     - Remaining bit count to be sent in this byte       \n"
    ";       r22     - current byte being transmitted (shifted)          \n"
    ";       r23     - remaining byte count to be transmitted            \n"
    ";       r25:r24 - bit time as iteration count                       \n"
    ";       r27:r26 - current buffer address (x)                        \n"
    ";       r31:r30 - bit time counter                                  \n"
    "                                                                    \n"
    ";       Start with dwire pin as output in idle state                \n"
    "                                                                    \n"
    "        sbi   0x18,%[bit]     ; Make sure line is idle (high)       \n"
    "        sbi   0x17,%[bit]     ; DDRB pin5 is output                 \n"
    "                                                                    \n"
    ";       Preload registers                                           \n"
    "                                                                    \n"
    "        lds   r24,dwBitTime                                         \n"
    "        lds   r25,dwBitTime+1                                       \n"
    "        ldi   r26,lo8(dwBuf)  ; X register addresses dwBuf          \n"
    "        ldi   r27,hi8(dwBuf)                                        \n"
    "                                                                    \n"
    "        lds   r23,dwLen                                             \n"
    "        tst   r23                                                   \n"
    "        breq  dws12           ; If no bytes to transmit             \n"
    "                                                                    \n"
    ";       Disable interrupts during transmission                      \n"
    "                                                                    \n"
    "        cli                                                         \n"
    "                                                                    \n"
    ";       Loop for each byte                                          \n"
    "                                                                    \n"
    ";       Send start bit (space / 0)                                  \n"
    "                                                                    \n"
    "dws2:   movw  r30,r24         ;      Load wait count to r31:r30     \n"
    "        cbi   0x18,%[bit]     ; 1.   Set dwire port low             \n"
    "                                                                    \n"
    "        ld    r22,x+          ; 2.   Next byte to send              \n"
    "        ldi   r21,8           ; 1.   Bit count                      \n"
    "                                                                    \n"
    "dws4:   sbiw  r30,1           ; 2.   Decrement wait count           \n"
    "        brne  dws4            ; 1/2. While more waiting required    \n"
    "                                                                    \n"
    ";       Loop for each bit                                           \n"
    "                                                                    \n"
    ";       Each bit takes a total of 4*dwBitTime + 8 cycles.           \n"
    "                                                                    \n"
    "dws6:   sbrc  r22,0           ; 1/2. Skip if sending zero           \n"
    "        sbi   0x18,%[bit]     ; 1.   Set dwire port high            \n"
    "        sbrs  r22,0           ; 1/2. Skip if sending one            \n"
    "        cbi   0x18,%[bit]     ; 1.   Set dwire port low             \n"
    "        movw  r30,r24         ; 1.   Load wait count to r31:r30     \n"
    "                                                                    \n"
    ";       5 cycles from dws6 to here.                                 \n"
    "                                                                    \n"
    "dws8:   sbiw  r30,1           ; 2.   Decrement wait count           \n"
    "        brne  dws8            ; 1/2. While more waiting required    \n"
    "                                                                    \n"
    ";       4*dwBitTime+4 cycles from dws6 to here.                     \n"
    "                                                                    \n"
    "        lsr   r22             ; 1.   Shift next bit to bit 0        \n"
    "        dec   r21             ; 1.   Count transmitted bit          \n"
    "        brne  dws6            ; 1/2. While more bits to transmit    \n"
    "                                                                    \n"
    ";       Send stop bit (mark / 1)                                    \n"
    "                                                                    \n"
    "        movw  r30,r24         ; 1.   Load wait count to r31:r30     \n"
    "        sbi   0x18,%[bit]     ; 1.   Set dwire port high            \n"
    "        adiw  r30,1           ; 2.   Extra iteration                \n"
    "                                                                    \n"
    "dws10:  sbiw  r30,1           ; 2.   Decrement wait count           \n"
    "        brne  dws10           ; 1/2. While more waiting required    \n"
    "                                                                    \n"
    "        dec   r23                                                   \n"
    "        brne  dws2            ; While more bytes to transmit        \n"
    "                                                                    \n"
    "dws12:  cbi   0x17,%[bit]     ; DDRB pin5 is input                  \n"
    "                                                                    \n"
  ::[bit]"I"(DW_BIT):"r21","r22","r23","r24","r25","r26","r27","r30","r31");
}




void dwReadBytes() {
  asm(
    "                                                                    \n"
    ";       Register usage:                                             \n"
    ";                                                                   \n"
    ";       r21     - Remaining bit count to be read in this byte       \n"
    ";       r22     - current byte being received (shifted)             \n"
    ";       r23     - total bytes received so far                       \n"
    ";       r25:r24 - bit time as iteration count                       \n"
    ";       r27:r26 - current buffer address (x)                        \n"
    ";       r31:r30 - bit time counter                                  \n"
    "                                                                    \n"
    ";       Preload registers                                           \n"
    "                                                                    \n"
    "        clr   r23                                                   \n"
    "        lds   r24,dwBitTime                                         \n"
    "        lds   r25,dwBitTime+1                                       \n"
    "        ldi   r26,lo8(dwBuf)  ; X register addresses dwBuf          \n"
    "        ldi   r27,hi8(dwBuf)                                        \n"
    "                                                                    \n"
    ";       Assumes previous dwSendBytes call has left:                 \n"
    ";         Interrupts already disabled                               \n"
    ";         DDRB pin5 is input                                        \n"
    "                                                                    \n"
    ";       Wait up to 65536*6 cycles = 23.8ms for start bit            \n"
    "                                                                    \n"
    "dwr2:   clr   r30                                                   \n"
    "        clr   r31                                                   \n"
    "                                                                    \n"
    "dwr4:   sbiw  r30,1           ; 2.   Check for timeout              \n"
    "        breq  dwr14           ; 1/2. If no start bit encountered    \n"
    "                                                                    \n"
    "        sbic  0x16,%[bit]     ; 1/2. Skip if Pin PB5 clear          \n"
    "        rjmp  dwr4            ; 2.   While no start bit             \n"
    "                                                                    \n"
    ";       Wait through half of start bit                              \n"
    "                                                                    \n"
    "        movw  r30,r24         ; 1                                   \n"
    "        lsr   r31             ; 1                                   \n"
    "        ror   r30             ; 1                                   \n"
    "                                                                    \n"
    "dwr6:   sbiw  r30,1           ; 2.                                  \n"
    "        brne  dwr6            ; 1/2.                                \n"
    "                                                                    \n"
    ";       We should be half way through the start bit, check this.    \n"
    ";       If line not still low, assume it was a glitch and go        \n"
    ";       back to waiting for a start bit.                            \n"
    "                                                                    \n"
    "        sbic  0x16,%[bit]     ; 1/2. Skip if Pin PB5 clear          \n"
    "        rjmp  dwr2            ; 2.   If not a real start bit        \n"
    "                                                                    \n"
    ";       Loop for 8 bits sampling the middle of each pulse.          \n"
    "                                                                    \n"
    "        clr   r22                                                   \n"
    "        ldi   r21,8                                                 \n"
    "                                                                    \n"
    ";       Each iteration takes 4*(r25:r24) + 8 cycles.                \n"
    "                                                                    \n"
    "dwr8:   movw  r30,r24         ; 1.                                  \n"
    "                                                                    \n"
    "dwr10:  sbiw  r30,1           ; 2.                                  \n"
    "        brne  dwr10           ; 1/2.                                \n"
    "                                                                    \n"
    ";       4*(r25:r24) cycles to here                                  \n"
    "                                                                    \n"
    ";       Sample one bit to top of r22                                \n"
    "                                                                    \n"
    "        lsr   r22             ; 1.                                  \n"
    "        sbic  0x16,%[bit]     ; 1/2.                                \n"
    "        sbr   r22,128         ; 1.   Sets top bit of r22            \n"
    "        rjmp  .               ; 2.   2 cycle noop                   \n"
    "        dec   r21             ; 1.                                  \n"
    "        brne  dwr8            ; 1/2. While more bits to sample      \n"
    "                                                                    \n"
    ";       Add sampled byte to buffer                                  \n"
    "                                                                    \n"
    "        st    x+,r22                                                \n"
    "        inc   r23                                                   \n"
    "                                                                    \n"
    ";       Wait for stop bit / idle line                               \n"
    "                                                                    \n"
    "        clr   r30                                                   \n"
    "        clr   r31                                                   \n"
    "                                                                    \n"
    "dwr12:  sbiw  r30,1           ; 2.   Check for timeout              \n"
    "        breq  dwr14           ; 1/2. If line stuck low              \n"
    "                                                                    \n"
    "        sbis  0x16,%[bit]     ; 1/2. Skip if Pin PB5 set            \n"
    "        rjmp  dwr12           ; 2.   While not stop bit             \n"
    "                                                                    \n"
    ";       Check for buffer full and loop back to read next byte       \n"
    "                                                                    \n"
    "        cpi   r23,128                                               \n"
    "        brlo  dwr2            ; While buffer not full               \n"
    "                                                                    \n"
    ";       Read complete                                               \n"
    "                                                                    \n"
    "dwr14:  sei                   ; Re-enable interrupts                \n"
    "        sts   dwLen,r23                                             \n"
    "                                                                    \n"
  ::[bit]"I"(DW_BIT):"r21","r22","r23","r24","r25","r26","r27","r30","r31");
}




/* ------------------------------------------------------------------------- */
/* --------------------------------- main ---------------------------------- */
/* ------------------------------------------------------------------------- */
int main(void) {
  uchar   i;
  uchar   calibrationValue;

  DDRB  = RESET_MASK;
  PORTB = RESET_MASK;

  calibrationValue = eeprom_read_byte(0); /* calibration value from last time */
  if(calibrationValue != 0xff){
    OSCCAL = calibrationValue;
  }

  initSerialNumber();

  usbDeviceDisconnect();
  for(i=0;i<20;i++){  /* 300 ms disconnect */
    _delay_ms(15);
  }
  usbDeviceConnect();

  I2C_DELAY = 5;
  SPI_DELAY = 0;

  wdt_enable(WDTO_1S);

  cmp0=0; cmp1=0; cmp2=0;

  compare0=0; compare1=0; compare2=0;

  counter=0;

  jobState=0;

  usbInit();
  sei();

  pinMode(B,0,INPUT);
  internalPullup(B,0,DISABLE);
  pinMode(B,1,INPUT);
  internalPullup(B,1,DISABLE);
  pinMode(B,2,INPUT);
  internalPullup(B,2,DISABLE);
  pinMode(B,5,INPUT);
  internalPullup(B,5,DISABLE);

  for(;;)
  {
    wdt_reset();
    usbPoll();

    //-----------------------------------------------------------------------------
    // parse rxBuffer and store the result in the sendBuffer
    // sendBuffer[8] represents the length of the buffer
    //-----------------------------------------------------------------------------
    switch(jobState)
    {
      case 0: /* idle ... */
      break;

      case 1: /* debug spi */
        // --------------------------------------------------------------------
        // -- idle clock state: high
        // -- spiMode = 3
        // rxBuffer[0] = message to send
        // sendBuffer[0] = reply
        // ---------------------------------
        // Data out:  MOSI
        // Data in:   MISO
        // Clock:   SCK -> max ~300 kHz
        // --------------------------------------------------------------------
        DDRB |= MOSI_MASK;
        DDRB &= ~MISO_MASK;
        DDRB |= SCK_MASK;
        PORTB |= SCK_MASK;
        sendBuffer[0]=0;
        t = rxBuffer[0];
        for ( q = 0x80; q; q >>= 1 )
        {
          PORT &= ~MOSI_MASK; /* Clear send data */
          for(i=0;i<SPI_DELAY;i++) _delay_us(1); /* Small delay */
          if  ( t & q ) { PORTB |= MOSI_MASK; } /* Send the data */
          PORT &= ~SCK_MASK; /* High to low edge */
          sendBuffer[0]<<=1; /* Shift the register */
          sendBuffer[0]+=((PINB&(MISO_MASK))>>1); /* Sample the data */
          for(i=0;i<SPI_DELAY;i++) _delay_us(1); /* Small delay */
          PORT |= SCK_MASK; /* Low to high edge */
        }
        sendBuffer[8]=1; // length
        // --------------------------------------------------------------------
        jobState=0;
      break;

      case 2: /* onewire reset pulse */
        pinMode(B,DATA_PIN,OUTPUT);
        digitalWrite(B,DATA_PIN,LOW);       // Drive the bus low
        delayMicroseconds(480);           // delay 480 microsecond (us)
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          digitalWrite(B,DATA_PIN,HIGH);      // Release the bus
          delayMicroseconds(70);          // delay 70 microsecond (us)
          pinMode(B,DATA_PIN,INPUT);
          sendBuffer[0]=!(digitalRead(B,DATA_PIN)>>2);// Sample for presence pulse from slave
        }
        delayMicroseconds(410);           // delay 410 microsecond (us)
        pinMode(B,DATA_PIN,OUTPUT);           // Release the bus
        digitalWrite(B,DATA_PIN,HIGH);
        sendBuffer[8]=1;
        jobState=0;
      break;

      case 3: /* onewire send byte */
        q=rxBuffer[0];
        pinMode(B,DATA_PIN,OUTPUT);
        for (i=0;i<8;i++)
        {
          ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
          {
            digitalWrite(B,DATA_PIN,LOW);   // Drive the bus low

            if ( q & 0x01)
            {
              delayMicroseconds(6);     // delay 6 microsecond (us)
              digitalWrite(B,DATA_PIN,HIGH);  // Release the bus
              delayMicroseconds(64);      // delay 64 microsecond (us)
            }
            else
            {
              delayMicroseconds(60);      // delay 60 microsecond (us)
              digitalWrite(B,DATA_PIN,HIGH);  // Release the bus
              delayMicroseconds(10);      // delay 10 microsecond (us)
            }
            q >>= 1;              // shift the data byte for the next bit to send
          }
        }
        jobState=0;
      break;

      case 4: /* onewire read byte */
        sendBuffer[0]=0;
        for(i=0;i<8;i++)
        {
          sendBuffer[0] >>= 1;          // shift the result to get it ready for the next bit to receive
          pinMode(B,DATA_PIN,OUTPUT);
          ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
          {
            digitalWrite(B,DATA_PIN,LOW);   // Drive the bus low
            delayMicroseconds(6);       // delay 6 microsecond (us)
            digitalWrite(B,DATA_PIN,HIGH);    // Release the bus
            delayMicroseconds(10);        // delay 9 microsecond (us)
            pinMode(B,DATA_PIN,INPUT);
            q = (digitalRead(B,DATA_PIN)>>2); // Read the status of OW_PIN
          }
          delayMicroseconds(55);          // delay 55 microsecond (us)
          if (q)  sendBuffer[0] |= 0x80;      // if result is one, then set MS-bit
        }
        sendBuffer[8]=1;
        jobState=0;
      break;

      case 5: /* onewire read bit */
        pinMode(B,DATA_PIN,OUTPUT);
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          digitalWrite(B,DATA_PIN,LOW);     // Drive the bus low
          delayMicroseconds(6);         // delay 6 microsecond (us)
          digitalWrite(B,DATA_PIN,HIGH);      // Release the bus
          delayMicroseconds(10);          // delay 9 microsecond (us)
          pinMode(B,DATA_PIN,INPUT);
          sendBuffer[0]=(digitalRead(B,DATA_PIN)>>2); // Read the status of OW_PIN
        }
        sendBuffer[8]=1;
        jobState=0;
      break;

      case 6: /* onewire write bit */
        pinMode(B,DATA_PIN,OUTPUT);
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          digitalWrite(B,DATA_PIN,LOW);     // Drive the bus low
          if ( rxBuffer[0] & 0x01)
          {
            delayMicroseconds(6);       // delay 6 microsecond (us)
            digitalWrite(B,DATA_PIN,HIGH);    // Release the bus
            delayMicroseconds(64);        // delay 64 microsecond (us)
          }
          else
          {
            delayMicroseconds(60);        // delay 60 microsecond (us)
            digitalWrite(B,DATA_PIN,HIGH);    // Release the bus
            delayMicroseconds(10);        // delay 10 microsecond (us)
          }
        }
        jobState=0;
      break;

      case 7:  /* multiple spi send and receive */
        // --------------------------------------------------------------------
        // -- idle clock state: low
        // -- spiMode = 0
        // rxBuffer[0] = message to send
        // sendBuffer[0] = reply
        // ---------------------------------
        // Data out:  MISO
        // Data in:   MOSI
        // Clock:   SCK -> max ~650 kHz
        // --------------------------------------------------------------------
        DDRB |= MISO_MASK;
        DDRB &= ~MOSI_MASK;
        DDRB |= SCK_MASK;
        PORTB &= ~SCK_MASK;
        t=(1<<USIWM0)|(1<<USICS1)|(1<<USICLK)|(1<<USITC);
        USICR = (1<<USIWM0)|(1<<USICS1)|(1<<USICLK);
        if(rxBuffer[0]) PORT &= ~(1<<5); // auto chip select?
        for(q=0;q<rxBuffer[1];q++)
        {
          USIDR=rxBuffer[2+q];
          USISR=(1<<USIOIF);
          do {
            USICR = t;
            for(i=0;i<SPI_DELAY;i++)  _delay_us(1);
          } while ((USISR & (1<<USIOIF))==0);
          sendBuffer[q]=USIDR;
        }
        if(rxBuffer[0]) PORT |= (1<<5); // auto chip select?
        sendBuffer[8]=q;
        jobState=0;
      break;

      case 8: /* init i2c */
        I2C_Init();
        jobState=0;
      break;

      case 9: /* i2c send address */
        I2C_Start(0);
        q=I2C_Write(rxBuffer[0]);
        sendBuffer[0]=q;
        sendBuffer[8]=1;
        jobState=0;
      break;

      case 10: /* i2c send byte(s) */
        // ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        // {
          for(i=0;i<rxBuffer[0];i++)
            q=I2C_Write(rxBuffer[i+2]);
        // }
        if(rxBuffer[1])
          I2C_Stop();
        sendBuffer[0]=q;
        sendBuffer[8]=1;
        jobState=0;
      break;

      case 11: /* i2c read byte(s) */
        // ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        // {
          if(rxBuffer[0]) // -- end with Nack?
          {
            for(i=0;i<(rxBuffer[1]-1);i++)
              sendBuffer[i]=I2C_Read(1);
            sendBuffer[i]=I2C_Read(0);
          }
          else // -- always issue an Ack?
          {
            for(i=0;i<rxBuffer[1];i++)
              sendBuffer[i]=I2C_Read(1);
          }
        // }
        if(rxBuffer[2]) // -- should we issue a Stop?
          I2C_Stop();
        sendBuffer[8]=rxBuffer[1];
        jobState=0;
      break;

    #if 0
      case 12: /* pic24f - Clock Control Bits */
        ClockControlBits(rxBuffer[1]);
        jobState=0;
      break;
      case 13: /* pic24f - Clock Low x Times */
        ClockLowXTimes(rxBuffer[1]);
        jobState=0;
      break;
      case 14: /* pic24f - Clock Byte */
        for(i=0;i<rxBuffer[0];i++)
          ClockByte(rxBuffer[i+1]);
        jobState=0;
      break;
      case 15: /* pic24f - Pin Byte */
        for(i=0;i<rxBuffer[1];i++)
        {
          PinByte(&q);
          sendBuffer[i]=q;
        }
        sendBuffer[8]=rxBuffer[1];
        jobState=0;
      break;
      case 16: /* pic24f - sendSix */
        ClockControlBits(0x00);
        ClockByte(rxBuffer[1]);
        ClockByte(rxBuffer[2]);
        ClockByte(rxBuffer[3]);
        for(q=0;q<rxBuffer[0];q++)
        {
          ClockControlBits(0x00);
          ClockByte(0x00);
          ClockByte(0x00);
          ClockByte(0x00);
        }
        jobState=0;
      break;
    #endif

      case 17: /* write ws2812 */
        _delay_ms(1); // Hack: Make sure USB communication has finished before atomic block.
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          ws2812_sendarray_mask(ws2812_grb,ws2812_ptr,ws2812_mask);   //mask=1<<(data[2]&7)
          ws2812_ptr=0;
        }
        jobState=0;
      break;




      case 20: // DebugWIRE. dwState determines action:
        // dwState flag bits:
        //
        //     00000001   0x01     Send break
        //     00000010   0x02     Set timing parameter
        //     00000100   0x04     Send bytes
        //     00001000   0x08     Wait for start bit
        //     00010000   0x10     Read bytes
        //     00100000   0x20     Read pulse widths
        //
        // Supported combinations
        //    $21 - Send break and read pulse widths
        //    $02 - Set timing parameters
        //    $04 - Send bytes
        //    $0C - Send bytes and wait for dWIRE line state change
        //    $14 - Send bytes and read response (normal command)
        //    $24 - Send bytes and receive 0x55 pulse widths
        //
        // Note that the wait for start bit loop also monitors the dwState wait for start
        // bit flag, and is arranged so that sending a 33 (send break and read pulse widths)
        // will abort a pending wait.

        if (dwState & 0x34) {_delay_ms(2);} // Allow USB transfer to complete before
                                            // any action that may disable interrupts

        if (dwState & 0x01) {cbi(PORTB, DW_BIT); sbi(DDRB, DW_BIT); _delay_ms(100);}
        if (dwState & 0x02) {((char*)&dwBitTime)[0] = dwBuf[0]; ((char*)&dwBitTime)[1] = dwBuf[1];}
        if (dwState & 0x04) {dwSendBytes();}
        if (dwState & 0x08) {dwBuf[0]=0; dwLen=1; cbi(DDRB,DW_BIT); sbi(PCMSK,DW_BIT); sei();} // Capture dWIRE pin change
        if (dwState & 0x10) {dwReadBytes();}
        if (dwState & 0x20) {dwCaptureWidths();}

        jobState = 0;
        dwState  = 0;
        sei();
      break;


      default:
        jobState=0;
      break;
    }


    if(softPWM)
    {
      if(counter==0)
      {
        cmp0=compare0;
        cmp1=compare1;
        cmp2=compare2;
      }
      else
      {
        if(counter>cmp0)
          digitalWrite(B,0,LOW);
        else
          digitalWrite(B,0,HIGH);
        if(counter>cmp1)
          digitalWrite(B,1,LOW);
        else
          digitalWrite(B,1,HIGH);
        if(counter>cmp2)
          digitalWrite(B,2,LOW);
        else
          digitalWrite(B,2,HIGH);
      }
      counter++;
    }

  }
  return 0;
}

static inline void initSerialNumber()
{
  uint8_t eepromProblem = 0;

  char val1 = eeprom_read_byte(EE_addr+0);
  char val2 = eeprom_read_byte(EE_addr+1);
  char val3 = eeprom_read_byte(EE_addr+2);

  // ascii 48 -> '0' , 57 -> '9'

  if((val1 < 48) || (val1 > 57))
  {
    eepromProblem = 1;
  }
  else if((val2 < 48) || (val2 > 57))
  {
    eepromProblem = 1;
  }
  else if((val3 < 48) || (val3 > 57))
  {
    eepromProblem = 1;
  }

  /* default serial number ... */
  if(eepromProblem != 0)
  {
    eeprom_write_byte((EE_addr+0),'5');
    eeprom_write_byte((EE_addr+1),'1');
    eeprom_write_byte((EE_addr+2),'2');
  }

  usbDescriptorStringSerialNumber[1] = eeprom_read_byte(EE_addr+0);
  usbDescriptorStringSerialNumber[2] = eeprom_read_byte(EE_addr+1);
  usbDescriptorStringSerialNumber[3] = eeprom_read_byte(EE_addr+2);
}
