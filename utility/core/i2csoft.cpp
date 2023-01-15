#include "i2csoft.hpp"
#include <avr/interrupt.h>
#include <string.h>
#include <util/atomic.h>

#ifdef I2CSOFT_ARGS

I2cSoft i2cSoft;

volatile uint8_t I2cSoft::buff[I2CSOFT_BUFF_SIZE];
uint8_t I2cSoft::state;
uint8_t I2cSoft::ibit;
uint8_t I2cSoft::shft;
uint8_t I2cSoft::regn;
volatile I2cSoft::Flag I2cSoft::flag = flNone;

void I2cSoft::begin() {
  // input, enable pullup
  DIO_OP(I2CSOFT_SCL,DDR,&=~);
  DIO_OP(I2CSOFT_SCL,PORT,|=);
  DIO_OP(I2CSOFT_SDA,DDR,&=~);
  DIO_OP(I2CSOFT_SDA,PORT,|=);

  // enable pc interrupt
  GIMSK |= (1<<DIO_PC(I2CSOFT_SCL,PCIE)) | (1<<DIO_PC(I2CSOFT_SDA,PCIE));
  DIO_PC(I2CSOFT_SCL,PCMSK) |= (1<<DIO_BIT(I2CSOFT_SCL)) | (1<<DIO_BIT(I2CSOFT_SDA));
  sei();
}

bool I2cSoft::set(Flag f, bool v) {
  bool ret;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ret = (flag & f) != 0;
    flag = (Flag)((flag & ~f) | (v ? f : 0));
  }
  return ret;
}

bool I2cSoft::test(Flag f) {
  return (flag & f) != 0;
}

void I2cSoft::release() {
  // release the bus if it's held
  if (!(flag & flHold))
    return;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // set input, enable pullup
    DIO_OP(I2CSOFT_SDA,DDR,&=~);
    DIO_OP(I2CSOFT_SDA,PORT,|=);
    flag = (Flag)(flag & ~flHold);
  }
}

extern "C" __attribute__ ((signal, used))
void I2CSOFT_INT() {
  I2cSoft::interrupt();
}
void I2cSoft::interrupt() {
  enum {
    sIdle,
    sIdleWait,
    sStopWait,
    sStopWaitPre,
    sStart,
    sRecvAddr,
    sRecvRegn,
    sRecvData,
    sRecvAddrWait,
    sRecvRegnWait,
    sRecvDataWait,
    sAckAddr,
    sAckRecv,
    sAckSend,
    sAckAddrWait,
    sAckRecvWait,
    sAckSendWait,
    sSendData,
    sSendDataWait,
    sAckSent,
  };
  switch (state) {

  default:
  case sIdle:
    // idle with SDA and SCL high
    if (!DIO_OP(I2CSOFT_SCL,PIN,&)) {
      // SCL fell
      state = sIdleWait; break; }
    if ( DIO_OP(I2CSOFT_SDA,PIN,&))
      // SDA and SCL still high
      break;
    // SDA fell
    state = sStart;
    break;

  case sIdleWait:
    // wait for idle with SDA and SCL any
    if (!DIO_OP(I2CSOFT_SCL,PIN,&))
      break;
    if (!DIO_OP(I2CSOFT_SDA,PIN,&))
      break;
    state = sIdle;
    break;

  case sStopWait:
    // wait for stop with SDA low and SCL high
    // only used after a write
    if (!DIO_OP(I2CSOFT_SCL,PIN,&)) {
      // SCL fell
      state = sStopWaitPre;
      break;
    }
    if (!DIO_OP(I2CSOFT_SDA,PIN,&))
      // SDA still low
      break;
    // SDA rose
  Stop:
    if (flag & flHoldRecv) {
      // hold the bus
      flag = (Flag)(flag | flHold);
      // set low, output
      DIO_OP(I2CSOFT_SDA,PORT,&=~);
      DIO_OP(I2CSOFT_SDA,DDR,|=);
    }
    state = sIdle;
    break;

  case sStopWaitPre:
    // wait for stop with SCL low or SDA high
    if (!DIO_OP(I2CSOFT_SCL,PIN,&))
      break;
    if ( DIO_OP(I2CSOFT_SDA,PIN,&))
      break;
    // SCL rose with SDA low
    state = sStopWait;
    break;

  case sStart:
    // SDA low, SCL high
    if ( DIO_OP(I2CSOFT_SDA,PIN,&)) {
      // SDA rose
      state = sIdle; break; }
    if ( DIO_OP(I2CSOFT_SCL,PIN,&)) {
      // SCL still high
      state = sIdleWait; break; }
    // SCL fell, SDA still low
    ibit = 8;
    state = sRecvAddr;
    break;

  case sRecvAddr:
  case sRecvRegn:
  case sRecvData:
    // SCL low, SDA any
    if (!DIO_OP(I2CSOFT_SCL,PIN,&))
      // SCL still low
      break;
    // SCL rose, read bit
    shft = (shft << 1) | (DIO_OP(I2CSOFT_SDA,PIN,&) != 0);
    state = state == sRecvAddr ? sRecvAddrWait
      : state == sRecvRegn ? sRecvRegnWait
      : sRecvDataWait;
    break;

  case sRecvAddrWait:
  case sRecvRegnWait:
  case sRecvDataWait:
    // SCL high, SDA any
    if ( DIO_OP(I2CSOFT_SCL,PIN,&)) {
      if (!(shft & 1) &&  DIO_OP(I2CSOFT_SDA,PIN,&))
        // SDA rose, stop condition
        goto Stop;
      if ( (shft & 1) && !DIO_OP(I2CSOFT_SDA,PIN,&))
        // SDA fell, restart condition
        state = sStart;
      break;
    }
    // SCL fell, get next bit unless done
    state = state == sRecvAddrWait ? sRecvAddr
      : state == sRecvRegnWait ? sRecvRegn
      : sRecvData;
    if (--ibit) break;
    // finished byte
    if (state == sRecvAddr) {
      if ((shft >> 1) != I2CSOFT_ADDRESS) {
        // not our address
        state = sIdleWait; break;
      }
      state = (shft & 1) ? sAckSend : sAckAddr;
    }
    else if (state == sRecvRegn) {
      regn = shft;
      state = sAckRecv;
    }
    else if (regn < I2CSOFT_BUFF_SIZE) {
      buff[regn++] = shft;
      state = sAckRecv;
    }
    if (state == sAckAddr || regn < I2CSOFT_BUFF_SIZE) {
      // ack set low, output
      DIO_OP(I2CSOFT_SDA,PORT,&=~);
      DIO_OP(I2CSOFT_SDA,DDR,|=);
    }
    break;

  case sAckAddr:
  case sAckRecv:
  case sAckSend:
    // SCL low, SDA output
    if (!DIO_OP(I2CSOFT_SCL,PIN,&))
      break;
    // SCL rose
    state = state == sAckAddr ? sAckAddrWait
      : state == sAckRecv ? sAckRecvWait
      : sAckSendWait;
    break;

  case sAckAddrWait:
  case sAckRecvWait:
  case sAckSendWait:
    // SCL high, SDA output (or input after send)
    if ( DIO_OP(I2CSOFT_SCL,PIN,&))
      break;
    // SCL fell
    if (state == sAckSendWait && regn < I2CSOFT_BUFF_SIZE) {
      shft = buff[regn++];
      ibit = 8;
      goto SendData;
    }
    // set input, enable pullup
    DIO_OP(I2CSOFT_SDA,DDR,&=~);
    DIO_OP(I2CSOFT_SDA,PORT,|=);
    ibit = 8;
    state = state == sAckAddrWait ? sRecvRegn
      : state == sAckRecvWait ? sRecvData
      : sStopWaitPre;
    break;

  case sSendData:
    // SCL low
    if (!DIO_OP(I2CSOFT_SCL,PIN,&))
      break;
    // SCL rose
    state = sSendDataWait;
    break;

  case sSendDataWait:
    // SCL high
    if ( DIO_OP(I2CSOFT_SCL,PIN,&))
      break;
    // SCL fell, send next bit unless done
  SendData:
    if (!ibit || (shft & 0x80)) {
      // set input, enable pullup
      DIO_OP(I2CSOFT_SDA,DDR,&=~);
      DIO_OP(I2CSOFT_SDA,PORT,|=);
    }
    else {
      // set low, output
      DIO_OP(I2CSOFT_SDA,PORT,&=~);
      DIO_OP(I2CSOFT_SDA,DDR,|=);
    }
    shft <<= 1;
    state = ibit-- ? sSendData : sAckSent;
    break;

  case sAckSent:
    // SCL low, SDA input
    if (!DIO_OP(I2CSOFT_SCL,PIN,&))
      break;
    // SCL rose, check for ack from host
    state = !DIO_OP(I2CSOFT_SDA,PIN,&)
      ? sAckSendWait : sIdleWait;
    break;

  }
}

#endif // I2CSOFT_ARGS
