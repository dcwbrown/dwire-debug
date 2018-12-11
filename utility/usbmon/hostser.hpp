#ifndef hostser_hpp
#define hostser_hpp

#include "hostio.hpp"

class LuXactSer : public LuXact {
public:

  LuXactSer(const char* serialnumber = nullptr);
  ~LuXactSer();

  bool Open();
  bool IsOpen() const;
  bool Close();

  void Send(char data);
  void Send(struct Rpc& rpc, uint8_t index);
  void Recv();

  void Break(uint8_t length);
  bool Reset();
  bool ResetDw();
  bool ResetPower();

  bool SetSerial(const char* serial);

  void Debug();

protected:

  // label segments
  enum {
    sgDevice,
    sgSerial,
    sgBaudrate,
    sgNumber
  };

  void Baudrate(uint32_t value);
  void Ioctl(unsigned long request, ...);
  uint32_t Gio();
  void Gio(uint32_t value, uint32_t mask);

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

  uint8_t I2cWrite(uint8_t regn, const char* data, uint8_t size);
  uint8_t I2cRead(uint8_t regn, char* data, uint8_t size);
  void I2cSCL(bool v);
  void I2cSDA(bool v);
  bool I2cSDA();
  void I2cInit();
  void I2cStart();
  void I2CRestart();
  void I2cStop();
  void I2cAck();
  void I2cNak();
  bool I2cSend(uint8_t data);
  uint8_t I2cRecv();
  void I2cReset();
  void I2cDebug();

  int fd = -1;
  uint32_t gio;
  uint32_t recv_skip;
  uint8_t recv_buff[254]; // RpcPayloadMax+1
  uint8_t recv_size;

  // gio outputs: TXD, DTR, RTS, inputs: RXD, RI, DSR, CD, CTS
  uint32_t I2cAddr; // i2c address
  uint32_t I2cSCLo; // SCL output
  uint32_t I2cSDAo; // SDA output
  uint32_t I2cSDAi; // SDA input

};

#endif
