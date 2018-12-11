#ifndef hostio_hpp
#define hostio_hpp

#include <stdexcept>

class LuXact {
public:

  class Exception : public std::runtime_error { using std::runtime_error::runtime_error; };

  // draw label on console
  virtual void Label();

  // set callback for characters received
  inline void Sink(int (*_sink)(const void*)) { sink = _sink; }

  virtual bool Open() = 0;
  virtual bool IsOpen() const = 0;
  virtual bool Close() = 0;

  // send a single character
  virtual void Send(char data) = 0;

  // send an Rpc object, must have been allocated by
  // RPCALLOCA to allow encoding for transmission
  virtual void Send(struct Rpc& rpc, uint8_t index) = 0;

  // receive data and process
  virtual void Recv() = 0;

  // software reset
  virtual bool Reset();

  // exit dw mode (reset) if possible
  virtual bool ResetDw();

  // power cycle if possible
  virtual bool ResetPower();

  // update serial number
  virtual bool SetSerial(const char* serial);

protected:

  // holds string for console label
  // nulls get converted to spaces
  class Label : public std::string {
  public:

    // don't hide assignment operator
    using std::string::operator=;

    // add blank segments to end
    void AddSegment(size_t n);

    // return string for display
    std::string Display();

    // return, set segment
    std::string Segment(size_t n);
    void Segment(size_t n, std::string v);

    // return position, length of segment
    size_t SegmentPos(size_t n);
    size_t SegmentLen(size_t pos);

  } label;

  // convert char16_t[] to string
  std::string to_bytes(const char16_t* wptr);

  int (*sink)(const void*) = nullptr;
};

#endif
