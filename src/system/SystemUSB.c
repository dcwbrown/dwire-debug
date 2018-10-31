// SystemUSB - Provide common USB access on Linux and on Windows (via LibUSBw).


#ifdef windows

  // Minimal interface to libusb-win32 for Windows

  #define LIBUSB_PATH_MAX   512
  #define USB_TYPE_VENDOR   (0x02 << 5)
  #define USB_RECIP_DEVICE  0x00
  #define USB_ENDPOINT_IN   0x80
  #define USB_ENDPOINT_OUT  0x00

  struct usb_device_descriptor
  {
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned short bcdUSB;
    unsigned char  bDeviceClass;
    unsigned char  bDeviceSubClass;
    unsigned char  bDeviceProtocol;
    unsigned char  bMaxPacketSize0;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char  iManufacturer;
    unsigned char  iProduct;
    unsigned char  iSerialNumber;
    unsigned char  bNumConfigurations;
  };

  struct usb_device
  {
    struct usb_device             *next, *prev;
    char                           filename[LIBUSB_PATH_MAX];
    struct usb_bus                *bus;
    struct usb_device_descriptor   descriptor;
    void                          *config;
    void                          *dev;    /* Darwin support */
    unsigned char                  devnum;
    unsigned char                  num_children;
    struct usb_device            **children;
  };

  struct usb_bus
  {
    struct usb_bus    *next, *prev;
    char               dirname[LIBUSB_PATH_MAX];
    struct usb_device *devices;
    unsigned long      location;
    struct usb_device *root_dev;
  };

  struct usb_dev_handle;
  typedef struct usb_dev_handle usb_dev_handle;

  static int             (*usb_find_busses) (void) = NULL;
  static void            (*_usb_init)       (void) = NULL;
  static int             (*usb_find_devices)(void) = NULL;
  static struct usb_bus *(*usb_get_busses)  (void) = NULL;
  static usb_dev_handle *(*usb_open)        (struct usb_device *dev) = NULL;
  static int             (*usb_close)       (usb_dev_handle *dev) = NULL;
  static int             (*usb_control_msg)(
                           usb_dev_handle *dev,
                           int requesttype, int request,
                           int value, int index,
                           char *bytes, int size,
                           int timeout
                         ) = NULL;

  int UsbInit(void) {
    HINSTANCE libusb0 = LoadLibrary("libusb0.dll");
    if (libusb0) {
      if (    (usb_find_busses  = (void *)GetProcAddress(libusb0, "usb_find_busses"))
          &&  (usb_find_devices = (void *)GetProcAddress(libusb0, "usb_find_devices"))
          &&  (usb_get_busses   = (void *)GetProcAddress(libusb0, "usb_get_busses"))
          &&  (usb_open         = (void *)GetProcAddress(libusb0, "usb_open"))
          &&  (usb_close        = (void *)GetProcAddress(libusb0, "usb_close"))
          &&  (usb_control_msg  = (void *)GetProcAddress(libusb0, "usb_control_msg"))
          &&  (_usb_init        = (void *)GetProcAddress(libusb0, "usb_init"))) {
        _usb_init();
        return 1; // Success.
      }
    }
    return 0;  // Failure.
  }

#else

  // Linux - USB support is built in.

  #include <usb.h>
  
  // run the following command on mac to fix 'usb.h' file not found
  // brew install libusb libusb-compat 

  int UsbInit() {usb_init(); return 1;}

#endif




