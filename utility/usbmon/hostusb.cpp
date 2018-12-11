#include "command.hpp"
#include "rpc.hpp"
#include "hostusb.hpp"

#include <cstring>
#include <iostream>
#include <libusb-1.0/libusb.h> // apt-get install libusb-1.0-0-dev
#include <memory>
#include <sstream>
#include <unistd.h>

static struct libusb_context* lu_ctx = nullptr;
RunExit(libusb) { if (lu_ctx) libusb_exit(lu_ctx); }

LuXactUsb::LuXactUsb(uint32_t _vidpid, const char* vendor, const char* product, const char* serial)
  : vidpid(_vidpid)
{
  if (!lu_ctx) {
    if (libusb_init(&lu_ctx) < 0)
      SysAbort(__PRETTY_FUNCTION__);
  }

  label.AddSegment(sgNumber - 1);
  storeDescriptors(vendor, product, serial);

  if (opt_debug)
    libusb_set_debug(lu_ctx, LIBUSB_LOG_LEVEL_INFO);
  Label();
}

LuXactUsb::~LuXactUsb()
{
  Close();
}

bool LuXactUsb::Open()
{
  libusb_device* *devs;
  ssize_t cnt = libusb_get_device_list(lu_ctx, &devs);
  if (cnt < 0) SysAbort(__PRETTY_FUNCTION__);
  for (ssize_t i = 0; i < cnt; i++) {
    libusb_device* dev = devs[i];

    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(dev, &desc) < 0)
      SysAbort(__PRETTY_FUNCTION__);

    // require vidpid to match
    if (!matchVidpid(((uint32_t)desc.idVendor << 16) | desc.idProduct))
      continue;

    // open with automatic handle
    struct Handle {
      ~Handle() { if (handle) libusb_close(handle); }
      operator libusb_device_handle*() { return handle; }
      void release() { handle = nullptr; }
      libusb_device_handle* handle;
    } handle;
    if (libusb_open(dev, &handle.handle) != 0) continue;

    // read Manufacturer Product SerialNumber
    const size_t desc_size = 256;
    auto desc_get = [&](char16_t* p, uint8_t index)->bool {
      int r = libusb_get_string_descriptor(handle, index, 0x0409, (unsigned char*)p, desc_size);
      if (r < 0 || r >= desc_size || (r & 1) || *p != (0x300 | r))
        return false;
      std::char_traits<char16_t>::move(p, p + 1, r - sizeof(char16_t));
      p[r / sizeof(char16_t) - 1] = '\0';
      return true;
    };
    char16_t vendor[desc_size];
    char16_t product[desc_size];
    char16_t serial[desc_size];
    // require no errors
    if (!desc_get(vendor, desc.iManufacturer) || !desc_get(product, desc.iProduct)
        || !desc_get(serial, desc.iSerialNumber))
      continue;

    // require a match
    if (!matchVidpid(vendor, product) || !matchSerial(serial))
      continue;

    if (claim) {
      // claim interface must succeed in order to own device
      if (libusb_kernel_driver_active(handle, 0) == 1
		  && libusb_detach_kernel_driver(handle, 0) != 0
		  || libusb_claim_interface(handle, 0) != 0)
        continue;
    }

    // store the strings read in case there were wild cards
    storeDescriptors(vendor, product, serial);

    lu_dev = handle;
    handle.release();
    break;
  }
  libusb_free_device_list(devs, 1);

  if (!lu_dev) return false;
  Label();
  return true;
}

bool LuXactUsb::matchVidpid(uint32_t _vidpid) {
  return vidpid == _vidpid;
}

bool LuXactUsb::matchVidpid(const char16_t* vendor, const char16_t* product) {
  // wild cards have l == 0
  std::string sVendor{label.Segment(sgVendor)};
  std::string sProduct{label.Segment(sgProduct)};
  return (sVendor.length() == 0 || sVendor == to_bytes(vendor))
    && (sProduct.length() == 0 || sProduct == to_bytes(product));
}

bool LuXactUsb::matchSerial(const char16_t* serial) {
  std::string sSerial{label.Segment(sgSerial)};
  return sSerial.length() == 0 || sSerial == to_bytes(serial);
}

void LuXactUsb::storeDescriptors(const char16_t* vendor, const char16_t* product, const char16_t* serial) {
  storeDescriptors(to_bytes(vendor).c_str(), to_bytes(product).c_str(), to_bytes(serial).c_str());
}

void LuXactUsb::storeDescriptors(const char* vendor, const char* product, const char* serial) {
  if (label.Segment(sgVendor).length() == 0)
    label.Segment(sgVendor, vendor ? vendor : "");
  if (label.Segment(sgProduct).length() == 0)
    label.Segment(sgProduct, product ? product : "");
  if (label.Segment(sgSerial).length() == 0)
    label.Segment(sgSerial, serial ? serial : "");
}

bool LuXactUsb::IsOpen() const {
  return lu_dev != nullptr;
}

bool LuXactUsb::Close()
{
	if (!lu_dev) return false;
	bool ret = true;

	if (libusb_release_interface(lu_dev, 0) != 0)
		ret = false;
	libusb_close(lu_dev);
	lu_dev = nullptr;

	Label();
	return ret;
}

int LuXactUsb::Xfer(uint8_t req, uint32_t arg, char* data, uint8_t size, bool device_to_host) {
  int ret = libusb_control_transfer
    (/* libusb_device_handle* */lu_dev,
     /* bmRequestType */ LIBUSB_REQUEST_TYPE_CLASS
     | (device_to_host ? LIBUSB_ENDPOINT_IN : LIBUSB_ENDPOINT_OUT),
     /* bRequest */ req,
     /* wValue */ (uint16_t)arg,
     /* wIndex */ (uint16_t)(arg >> 16),
     /* data */ (unsigned char*)data,
     /* wLength */ size,
     timeout);
  if (opt_debug) {
    std::cerr << (device_to_host ? "xi: " : "xo: ") << (int)req;
    std::ios::fmtflags fmtfl = std::cerr.setf(std::ios::hex, std::ios::basefield);
    std::cerr << 'x' << (uint16_t)req;
    std::cerr.setf(fmtfl, std::ios::basefield);
    std::cerr << ' ' << arg;
    fmtfl = std::cerr.setf(std::ios::hex, std::ios::basefield);
    std::cerr << 'x' << arg;
    std::cerr.setf(fmtfl, std::ios::basefield);
    std::cerr << " -> " << ret << ' ' << libusb_error_name(ret) << std::endl;
  }
  return ret;
}

uint8_t LuXactUsb::XferRetry(uint8_t req, uint32_t arg, char* data, uint8_t size, bool device_to_host)
{
  for (;;) {
    if (!lu_dev) {
      usleep(100 * 1000);
      if (!Open())
        continue;
	}
    int xfer = Xfer(req, arg, data, size, device_to_host);
    if (xfer >= 0 && xfer <= size)
      return xfer;
    Close();
  }
}

LuXactUsbRpc::LuXactUsbRpc(const char* serialnumber)
  : LuXactUsb(0x16c005df, nullptr, "RpcUsb", serialnumber) {
}

bool LuXactUsbRpc::matchSerial(const char16_t* serial) {
  std::string sSerial{label.Segment(sgSerial)};
  if (sSerial.length() == 0) return true;
  if (!serial || std::char_traits<char16_t>::length(serial) != 1)
    return LuXactUsb::matchSerial(serial);
  return (char16_t)strtol(sSerial.c_str(), nullptr, 0) == serial[0];
}

void LuXactUsbRpc::storeDescriptors(const char16_t* vendor, const char16_t* product, const char16_t* serial) {
  if (!serial || std::char_traits<char16_t>::length(serial) != 1) {
    LuXactUsb::storeDescriptors(vendor, product, serial);
    return;
  }
  // this will only set the strings if they are uninitialized
  LuXactUsb::storeDescriptors(vendor, product, nullptr);

  std::ostringstream sn;
  sn << (uint16_t)*serial << "x" << std::hex << (uint16_t)*serial;
  LuXactUsb::storeDescriptors(nullptr, nullptr, sn.str().c_str());
}

void LuXactUsbRpc::Send(char data)
{
  try {
    XferRetry(0, (uint8_t)data);
  } catch(...) { }
}

void LuXactUsbRpc::Send(struct Rpc& rpc, uint8_t index) {
  uint8_t size = rpc.Size(index);
  uint32_t arg; memcpy(&arg, (char*)&rpc, sizeof(arg) < size ? sizeof(arg) : size);
  size = sizeof(arg) < size ? size - sizeof(arg) : 0;
  uint8_t xfer = XferRetry(index, arg, (char*)&rpc + sizeof(arg), size, false);
  if (xfer != size)
    throw Exception(__PRETTY_FUNCTION__);
}

void LuXactUsbRpc::Recv() {
  for (;;) {
    char data[RpcPayloadMax+1];
    uint8_t xfer = XferRetry(1, 0, data, sizeof(data), true);
    if (xfer == 0)
      break;
    if (xfer == 1 || data[xfer-1] == 0) {
      if (xfer == 1) data[1] = '\0';
      if (sink)
        sink(data);
      return;
    }
    if (data[xfer-1] >= RpcNumber)
      throw Exception(__PRETTY_FUNCTION__);
    ((Rpc*)data)->VtAct(data[xfer-1]);
  }
}

bool LuXactUsbRpc::Reset()
{
  if (!lu_dev)
    return false;
  Xfer(0xFF, 0);
  Close();
  return true;
}

LuXactLw::LuXactLw(const char* serialnumber)
  : LuXactUsb(0x17810c9f, nullptr, nullptr, nullptr) {

  if (serialnumber && *serialnumber) {
    int sn = strtol(serialnumber, nullptr, 0);
    if (sn < 0 || sn > 999) sn = 0;
    std::string str = std::to_string(sn);
    size_t len = str.length();
    if (len < 3) str.insert(0, 3 - len, '0');
    label.Segment(sgSerial, str);
  }
  label.AddSegment(sgNumber - LuXactUsb::sgNumber);
}

bool LuXactLw::Open() {
  listening = false;
  return LuXactUsb::Open();
}

void LuXactLw::Send(char data) {
  // Cancel any dw commands
  XferRetry(60/*dw*/, 0);

  // send bytes and read if listening
  char buf[2] = ""; buf[1] = data;
  Xfer(60/*dw*/, (listening ? 0x1C : 0x04), buf, sizeof(buf), false);
}

void LuXactLw::Send(struct Rpc& rpc, uint8_t index) {
}

void LuXactLw::Recv() {
  if (listening) {
    // libusb calls taken from dwire/DigiSpark.c

    // Read back dWIRE bytes
    char data[RpcPayloadMax+1];
    int status = Xfer(60/*dw*/, 0, data, sizeof(data), true);
    // 0 means no data, PIPE means garbled command
    //static int last = 50000; if (status || last != status) std::cerr << "Recv status:" << status << '\n'; last = status;
    if (status == 0 || status == LIBUSB_ERROR_TIMEOUT || status == LIBUSB_ERROR_PIPE)
      return;
    if (status == LIBUSB_ERROR_NO_DEVICE) {
#if 1
      XferRetry(0/*echo*/, 0);
#else
      libusb_device* dev = libusb_get_device(lu_dev);
#endif
      return;
    }
    if (status > 0) {
      uint8_t xfer = status;
      if (xfer == 1 || data[xfer-1] == 0) {
        if (xfer == 1) data[1] = '\0';
        if (sink)
          sink(data);
      }
      else {
        if (data[xfer-1] >= RpcNumber)
          throw Exception(__PRETTY_FUNCTION__);
        ((Rpc*)data)->VtAct(data[xfer-1]);
      }
    }
  }

  // Wait for start bit and Read bytes
  XferRetry(60/*dw*/, 0x18);
  listening = true;
}

bool LuXactLw::Reset() {
}

bool LuXactLw::ResetDw() {
  // libusb calls taken from dwire/DigiSpark.c

  // DigisparkBreakAndSync()
  // Send break and read pulse widths
  if (Xfer(60/*dw*/, 0x21) < 0)
    return false;
  usleep(120 * 1000);

  // SetDwireBaud()
  uint16_t dwBitTime;
  for (uint8_t tries = 0; ; ++tries) {
    usleep(20 * 1000);
    uint16_t times[64];
    // Read back timings
    int status = Xfer(60/*dw*/, 0, (char*)times, sizeof(times), true);
    if (status <= 0 && tries < 5) continue;

    // 10 transitions for start, stop, and 8 data bits (20 bytes).
    // Average over last 8 bits, first one is bad since it has no start,
    // also thrown out second one to make number even since
    // rise and fall times aren't symmetric. When dw has been disabled,
    // the target will reply with 0x55, a one byte delay, then 0x00
    // which will cause status to return 24 instead of 20.
    if (status != 20 && status != 24) return false;

    // Average measurements and determine baud rate as bit time in device cycles.
    const int n = 8;
    uint32_t sum = 0;
    for (int i = 10 - n; i < 10; i++)
      sum += times[i];

    // Bit time for each measurement is 6 * measurement + 8 cycles.
    // Don't divide by n yet to preserve precision.
    sum = 6 * sum  +  8 * n;

    // Display the baud rate
    label.Segment(sgBaudrate, std::to_string(16500000 * n / sum) + " Bd");

#if 0 // done on the little wire
    // Determine timing loop iteration counts for sending and receiving bytes,
    // use rounding in the division.
    uint16_t dwBitTime = (sum - 8 * n + (4 * n) / 2) / (4 * n);
    std::cerr << "dwBitTime: " << dwBitTime << '\n';

    // Set timing parameter
    if (Xfer(60/*dw*/, 0x02, (char*)&dwBitTime, sizeof(dwBitTime), false) < 0)
      return false;
#endif

    if (status == 20) {
      // Disable Dw if it was enabled
      char data = 0x06;
      if (Xfer(60/*dw*/, 0x3C, &data, sizeof(data), false) < 0)
        return false;
    }

    break;
  }

  Label();
  return true;
}

bool LuXactLw::ResetPower() {
  const uint8_t pin = 0;
  if (Xfer(/*set high*/18, pin) < 0) return false;
  if (Xfer(/*set output*/14, pin) < 0) return false;
  usleep(100 * 1000);
  if (Xfer(/*set input*/13, pin) < 0) return false;
  if (Xfer(/*set low*/19, pin) < 0) return false;
  usleep(100 * 1000);
  return true;
}
/* power supply

V+5--------------o
                 |    _______
               | <E   | PNP |
      50R     B|/     |BC327|
PB0>---zz--o---|\     --|||--
           |   | \C     CBE
       5K  |     |
GND----zz--o     o---->V+

V+ on at boot when PB0 tristated.
V+ off when PB0 set high to turn off Q.
*/

bool LuXactLw::SetSerial(const char* _serial) {
  uint16_t serial = strtol(_serial, nullptr, 0);
  if (serial > 999) serial = 0;
  if (Xfer(55/*Change serial number*/,
           (uint32_t)('0' + serial%10)
           |((uint32_t)('0' + serial/10%10) << 8)
           |((uint32_t)('0' + serial/100%10) << 16)) < 0) {
    std::cerr << "error changing serial number" << std::endl;
    return false;
  }
  return true;
}
