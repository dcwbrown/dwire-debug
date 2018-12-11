#ifndef hostusb_hpp
#define hostusb_hpp

#include "hostio.hpp"

class LuXactUsb : public LuXact {
public:

  const uint32_t timeout = 10;

  LuXactUsb(uint32_t vidpid, const char* vendor, const char* product, const char* serial);
  ~LuXactUsb();

  bool Open();
  bool IsOpen() const;
  bool Close();
  int Xfer(uint8_t req, uint32_t arg, char* data = nullptr, uint8_t size = 0, bool device_to_host = false);
  uint8_t XferRetry(uint8_t req, uint32_t arg, char* data = nullptr, uint8_t size = 0, bool device_to_host = false);

protected:

  // check if we match vidpid, descriptors
  virtual bool matchVidpid(uint32_t vidpid);
  virtual bool matchVidpid(const char16_t* vendor, const char16_t* product);
  virtual bool matchSerial(const char16_t* serial);
  virtual void storeDescriptors(const char16_t* vendor, const char16_t* product, const char16_t* serial);
  virtual void storeDescriptors(const char* vendor, const char* product, const char* serial);

  // label segments that serve as device id
  enum {
    sgVendor,
    sgProduct,
    sgSerial,
    sgNumber
  };

  uint32_t vidpid = 0;
  bool claim = true;
  struct libusb_device_handle* lu_dev = nullptr;
};

class LuXactUsbRpc : public LuXactUsb {
public:

  LuXactUsbRpc(const char* serialnumber);
  char16_t* Serial(const char* serialnumber);

  void Send(char data);
  void Send(struct Rpc& rpc, uint8_t index);
  void Recv();

  bool Reset();

protected:
  bool matchSerial(const char16_t* serial);
  void storeDescriptors(const char16_t* vendor, const char16_t* product, const char16_t* serial);
};

class LuXactLw : public LuXactUsb {
public:

  LuXactLw(const char* serialnumber);

  bool Open();

  void Send(char data);
  void Send(struct Rpc& rpc, uint8_t index);
  void Recv();

  bool Reset();
  bool ResetDw();
  bool ResetPower();

  bool SetSerial(const char* serial);

protected:
  enum {
    sgBaudrate = LuXactUsb::sgNumber,
    sgNumber
  };

  bool listening;
};

#endif
