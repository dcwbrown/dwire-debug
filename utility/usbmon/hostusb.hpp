#ifndef hostusb_hpp
#define hostusb_hpp

#include <stdexcept>

class LuXact {
public:

  const uint32_t timeout = 10;

  class Exception : public std::runtime_error { public: Exception(const char* what_arg) : std::runtime_error(what_arg) { } };

  LuXact(uint32_t vidpid, const char16_t vendor[], const char16_t product[], const char16_t serial[]);
  ~LuXact();
  void Label();
  inline bool IsOpen() const { return lu_dev != nullptr; };

  bool Open(bool claim = true);
  bool Close();
  inline void Sink(int (*_sink)(const char*)) { sink = _sink; }
  int Xfer(uint8_t req, uint32_t arg, char* data = nullptr, uint8_t size = 0, bool device_to_host = false);
  uint8_t XferRetry(uint8_t req, uint32_t arg, char* data = nullptr, uint8_t size = 0, bool device_to_host = false);

  virtual void Reset() = 0;
  virtual void Send(char data) = 0;
  virtual void Send(struct Rpc& rpc, uint8_t index) = 0;
  virtual void Recv() = 0;

protected:

  uint32_t id_int;
  char16_t* id_string;
  struct libusb_device_handle* lu_dev = nullptr;
  int (*sink)(const char*) = nullptr;
};

class LuXactUsb : public LuXact {
public:

  LuXactUsb(const char* serialnumber);
  char16_t* Serial(const char* serialnumber);

  void Reset();
  void Send(char data);
  void Send(struct Rpc& rpc, uint8_t index);
  void Recv();

protected:
  char16_t serial[2];
};

class LuXactDw : public LuXact {
public:

  LuXactDw(const char* serialnumber);
  char16_t* Serial(const char* serialnumber);

  void Reset();
  void Send(char data);
  void Send(struct Rpc& rpc, uint8_t index);
  void Recv();

  bool ReadDw();
  bool ResetDw();
  bool ResetPower();

protected:
  char16_t serial[4];
};

#endif
