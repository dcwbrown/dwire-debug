#include "command.hpp"
#include "rpc.hpp"
#include "hostusb.hpp"

#if __GNUC__ > 4
#include <codecvt>
#endif
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <libusb-1.0/libusb.h> // apt-get install libusb-1.0-0-dev
#include <locale>
#include <memory>
#include <string>
#include <unistd.h>

static struct libusb_context* lu_ctx = nullptr;
RunInit(libusb) { if (libusb_init(&lu_ctx) < 0) SysAbort(__PRETTY_FUNCTION__); }
RunExit(libusb) { libusb_exit(lu_ctx); }

LuXact::LuXact(uint32_t vidpid, const char16_t vendor[], const char16_t product[], const char16_t serial[])
  : id_int(vidpid)
{
	size_t vl = vendor ? std::char_traits<char16_t>::length(vendor) : 0;
	size_t pl = product ? std::char_traits<char16_t>::length(product) : 0;
	size_t sl = serial ? std::char_traits<char16_t>::length(serial) : 0;

	id_string = new char16_t[vl + 1 + pl + 1 + sl + 1];
	char16_t blank[1] = { 0 };
	std::char_traits<char16_t>::copy(id_string, vl ? vendor : blank, vl + 1);
	std::char_traits<char16_t>::copy(id_string + vl + 1, pl ? product : blank, pl + 1);
	std::char_traits<char16_t>::copy(id_string + vl + 1 + pl + 1, sl ? serial : blank, sl + 1);

    if (opt_debug)
      libusb_set_debug(lu_ctx, LIBUSB_LOG_LEVEL_INFO);
    Label();
}

LuXact::~LuXact()
{
	Close();
	delete id_string;
}

void LuXact::Label()
{
	// only output label to tty
	if (!isatty(fileno(stdout)) || !isatty(fileno(stderr))) return;

	std::cerr << "\e]0;";
	if (!lu_dev) std::cerr << '[';
#if __GNUC__ > 4
    // convert utf16 to utf8
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
#else
    // this only handles ASCII
    struct { std::string (*to_bytes)(const char16_t* p); } convert;
    convert.to_bytes = [&](const char16_t* p)->std::string {
      std::string r;
      for (; *p; ++p)
        r += (char)(*p & 0xFF);
      return r; };
#endif
	char16_t* p = id_string;
	std::cerr << convert.to_bytes(p);
	p += std::char_traits<char16_t>::length(p) + 1;
	std::cerr << ' ' << convert.to_bytes(p);
	p += std::char_traits<char16_t>::length(p) + 1;
	if (p[0])
	{
		std::cerr << ' ';
		if (p[1])
			std::cerr << convert.to_bytes(p);
		else {
            std::ios::fmtflags fmtfl = std::cerr.setf(std::ios::hex, std::ios::basefield);
            std::cerr << "0x" << (uint16_t)*p;
            std::cerr.setf(fmtfl, std::ios::basefield);
        }
	}
    if (id_more)
      std::cerr << ' ' << id_more;
	if (!lu_dev) std::cerr << ']';
	std::cerr << '\a';
}

bool LuXact::Open()
{
	libusb_device* *devs;
	ssize_t cnt = libusb_get_device_list(lu_ctx, &devs);
	if (cnt < 0) SysAbort(__PRETTY_FUNCTION__);

	for (ssize_t i = 0; i < cnt; i++)
	{
		libusb_device* dev = devs[i];

		libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(dev, &desc) < 0)
			SysAbort(__PRETTY_FUNCTION__);

		// require vidpid to match
		if ((id_int >> 16) != desc.idVendor || (id_int & 0xFFFF) != desc.idProduct)
			continue;

		// open with automatic handle
		struct _handle { ~_handle() { if (h) libusb_close(h); }
			operator libusb_device_handle*() { return h; }
			libusb_device_handle* h = nullptr; } handle;
		if (libusb_open(dev, &handle.h) != 0) continue;

		const size_t buffer_size = 256;
		std::unique_ptr<char16_t[]> buffer(new char16_t[3 * buffer_size]);
		char16_t* p = buffer.get();
{
		// read Manufacturer Product SerialNumber
		auto desc_get = [&](uint8_t index)->bool {
			int r = libusb_get_string_descriptor(handle, index, 0x0409, (unsigned char*)p, buffer_size);
			if (r < 0 || r >= buffer_size || (r & 1) || *p != (0x300 | r))
				return false;
			std::char_traits<char16_t>::move(p, p + 1, r - sizeof(char16_t));
			p[r / sizeof(char16_t) - 1] = '\0';
			p += r / sizeof(char16_t);
			return true;
		};
		// require no errors
		if (!desc_get(desc.iManufacturer) || !desc_get(desc.iProduct) || !desc_get(desc.iSerialNumber))
			continue;
}
{
		// check for a match where wild cards have l == 0
		char16_t* s = id_string; p = buffer.get();
		auto desc_eq = [&]()->bool {
			size_t ls = std::char_traits<char16_t>::length(s);
			size_t lp = std::char_traits<char16_t>::length(p);
			bool r = ls == 0 || ls == lp && std::char_traits<char16_t>::compare(s, p, lp) == 0;
			s += ls + 1; p += lp + 1;
			return r;
		};
		// require a match
		if (!desc_eq() || !desc_eq() || !desc_eq())
			continue;

		// don't copy on exact match
		if (s == p)
			p = nullptr;
}
if (claim) {
		// claim interface must succeed in order to own device
		if (libusb_kernel_driver_active(handle, 0) == 1
		  && libusb_detach_kernel_driver(handle, 0) != 0
		  || libusb_claim_interface(handle, 0) != 0)
			continue;
}
		// if there were wild cards, store the string read
		if (p)
		{
			id_string = (char16_t*)realloc(id_string, (p - buffer.get()) * sizeof(char16_t));
			if (!id_string) SysAbort(__PRETTY_FUNCTION__);
			std::char_traits<char16_t>::copy(id_string, buffer.get(), p - buffer.get());
		}

		lu_dev = handle;
		handle.h = nullptr;
		break;
	}
	libusb_free_device_list(devs, 1);

	if (!lu_dev) return false;
	Label();
	return true;
}

bool LuXact::Close()
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

int LuXact::Xfer(uint8_t req, uint32_t arg, char* data, uint8_t size, bool device_to_host) {
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

uint8_t LuXact::XferRetry(uint8_t req, uint32_t arg, char* data, uint8_t size, bool device_to_host)
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

LuXactUsb::LuXactUsb(const char* serialnumber)
  : LuXact(0x16c005df, nullptr, nullptr, Serial(serialnumber)) {
}

char16_t* LuXactUsb::Serial(const char* serialnumber) {
  serial[0] = serialnumber ? strtol(serialnumber, nullptr, 0) : 0;
  serial[1] = 0;
  return serial;
}

void LuXactUsb::Reset()
{
  if (!lu_dev)
    return;
  Xfer(0xFF, 0);
  Close();
}

void LuXactUsb::Send(char data)
{
  try {
    XferRetry(0, (uint8_t)data);
  } catch(...) { }
}

void LuXactUsb::Send(struct Rpc& rpc, uint8_t index) {
  uint8_t size = rpc.Size(index);
  uint32_t arg; memcpy(&arg, (char*)&rpc, sizeof(arg) < size ? sizeof(arg) : size);
  size = sizeof(arg) < size ? size - sizeof(arg) : 0;
  uint8_t xfer = XferRetry(index, arg, (char*)&rpc + sizeof(arg), size, false);
  if (xfer != size)
    throw Exception(__PRETTY_FUNCTION__);
}

void LuXactUsb::Recv() {
  for (;;) {
    char data[RpcPayloadMax+1];
    uint8_t xfer = XferRetry(1, 0, data, sizeof(data), true);
    if (xfer == 0)
      break;
    if (xfer == 1 || data[xfer-1] == 0) {
      data[1] = '\0';
      if (sink)
        sink(data);
      return;
    }
    if (data[xfer-1] >= RpcNumber)
      throw Exception(__PRETTY_FUNCTION__);
    ((Rpc*)data)->VtAct(data[xfer-1]);
  }
}

LuXactLw::LuXactLw(const char* serialnumber)
  : LuXact(0x17810c9f, nullptr, nullptr, Serial(serialnumber)) {
  id[0] = '\0';
  id_more = id;
}

char16_t* LuXactLw::Serial(const char* serialnumber) {
  serial[0] = 0;
  if (!serialnumber) return nullptr;
  int sn = strtol(serialnumber, nullptr, 0);
  if (sn < 0 || sn > 999) sn = 0;
  std::string str = std::to_string(sn);
  size_t len = str.length();
  if (len < 3) str.insert(0, 3 - len, '0');
  const char* p = str.c_str();
  serial[0] = *p++;
  serial[1] = *p++;
  serial[2] = *p++;
  serial[3] = 0;
  return serial;
}

bool LuXactLw::Open() {
  listening = false;

  if (!LuXact::Open())
    return false;

  // Cancel any dw commands
  if (Xfer(60/*dw*/, 0) < 0)
    return false;
  return true;
}

void LuXactLw::Reset() {
}

void LuXactLw::Send(char data) {
  // Cancel any dw commands
  XferRetry(60/*dw*/, 0);

  // send bytes
  char buf[2] = ""; buf[1] = data;
  Xfer(60/*dw*/, 0x04, buf, sizeof(buf), false);

  // Xfer cancels dw interrupt, so restart it
  if (listening) {
    listening = false;
    Recv();
  }
}

void LuXactLw::Send(struct Rpc& rpc, uint8_t index) {
}

void LuXactLw::Recv() {
  if (listening) {
    // libusb calls taken from dwire/DigiSpark.c

    // Read back dWIRE bytes
    char data[129];
    int status = Xfer(60/*dw*/, 0, data, sizeof(data)-1, true);
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
    if (status > 0 && sink) {
      data[status] = '\0';
#if 0
      std::cerr << '[';
      std::ios::fmtflags fmtfl = std::cerr.setf(std::ios::hex, std::ios::basefield);
      for (int i = 0 ; i < status; ++i) std::cerr << ((int)data[i]&0xFF) << ' ';
      std::cerr.setf(fmtfl, std::ios::basefield);
      std::cerr << ']';
#endif
      sink(data);
    }
  }

  // Wait for start bit and Read bytes
  XferRetry(60/*dw*/, 0x18);
  listening = true;
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
    if (status < 18) return false;

    // Average measurements and determine baud rate as pulse time in device cycles.
    uint32_t cyclesperpulse = 0;
    int measurementCount = status / 2;
    for (int i = measurementCount-9; i < measurementCount; i++) cyclesperpulse += times[i];

    // Pulse cycle time for each measurement is 6*measurement + 8 cyclesperpulse.
    cyclesperpulse = (6*cyclesperpulse) / 9  +  8;

    // Determine timing loop iteration counts for sending and receiving bytes
    times[0] = (cyclesperpulse-8)/4;  // dwBitTime
    strcpy(id, (std::to_string(16500000 / cyclesperpulse)+"Bd").c_str());

    // Set timing parameter
    if (Xfer(60/*dw*/, 0x02, (char*)times, sizeof(uint16_t), false) < 0)
      return false;
    break;
  }
  {
    // Disable Dw
    char data = 0x06;
    if (Xfer(60/*dw*/, 0x3C, &data, sizeof(data), false) < 0)
      return false;
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
