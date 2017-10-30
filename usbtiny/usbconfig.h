/* Name: usbconfig.h
 * Project: AVR USB driver
 * Author: Christian Starkjohann
 * Creation Date: 2007-06-23
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * This Revision: $Id: usbconfig.h 537 2008-02-28 21:13:01Z cs $
 */

#ifndef __usbconfig_h_included__
#define __usbconfig_h_included__

/* ---------------------------- Hardware Config ---------------------------- */

#define USB_CFG_IOPORTNAME      B
/* This is the port where the USB bus is connected. When you configure it to
 * "B", the registers PORTB, PINB and DDRB will be used.
 */
#define USB_CFG_DMINUS_BIT      3
/* This is the bit number in USB_CFG_IOPORT where the USB D- line is connected.
 * This may be any bit in the port.
 */
#define USB_CFG_DPLUS_BIT       4
/* This is the bit number in USB_CFG_IOPORT where the USB D+ line is connected.
 * This may be any bit in the port. Please note that D+ must also be connected
 * to interrupt pin INT0!
 */
#define USB_CFG_CLOCK_KHZ       (F_CPU/1000)
/* Clock rate of the AVR in MHz. Legal values are 12000, 16000 or 16500.
 * The 16.5 MHz version of the code requires no crystal, it tolerates +/- 1%
 * deviation from the nominal frequency. All other rates require a precision
 * of 2000 ppm and thus a crystal!
 * Default if not specified: 12 MHz
 */

/* ----------------------- Optional Hardware Config ------------------------ */

/* #define USB_CFG_PULLUP_IOPORTNAME   D */
/* If you connect the 1.5k pullup resistor from D- to a port pin instead of
 * V+, you can connect and disconnect the device from firmware by calling
 * the macros usbDeviceConnect() and usbDeviceDisconnect() (see usbdrv.h).
 * This constant defines the port on which the pullup resistor is connected.
 */
/* #define USB_CFG_PULLUP_BIT          4 */
/* This constant defines the bit number in USB_CFG_PULLUP_IOPORT (defined
 * above) where the 1.5k pullup resistor is connected. See description
 * above for details.
 */

/* --------------------------- Functional Range ---------------------------- */

#define USB_CFG_HAVE_INTRIN_ENDPOINT    1
/* Define this to 1 if you want to compile a version with two endpoints: The
 * default control endpoint 0 and an interrupt-in endpoint 1.
 */
#define USB_CFG_HAVE_INTRIN_ENDPOINT3   0
/* Define this to 1 if you want to compile a version with three endpoints: The
 * default control endpoint 0, an interrupt-in endpoint 1 and an interrupt-in
 * endpoint 3. You must also enable endpoint 1 above.
 */
#define USB_CFG_IMPLEMENT_HALT          0
/* Define this to 1 if you also want to implement the ENDPOINT_HALT feature
 * for endpoint 1 (interrupt endpoint). Although you may not need this feature,
 * it is required by the standard. We have made it a config option because it
 * bloats the code considerably.
 */
#define USB_CFG_INTR_POLL_INTERVAL      100
/* If you compile a version with endpoint 1 (interrupt-in), this is the poll
 * interval. The value is in milliseconds and must not be less than 10 ms for
 * low speed devices.
 */
#define USB_CFG_IS_SELF_POWERED         0
/* Define this to 1 if the device has its own power supply. Set it to 0 if the
 * device is powered from the USB bus.
 */
#define USB_CFG_MAX_BUS_POWER           200
/* Set this variable to the maximum USB bus power consumption of your device.
 * The value is in milliamperes. [It will be divided by two since USB
 * communicates power requirements in units of 2 mA.]
 */
#define USB_CFG_IMPLEMENT_FN_WRITE      1
/* Set this to 1 if you want usbFunctionWrite() to be called for control-out
 * transfers. Set it to 0 if you don't need it and want to save a couple of
 * bytes.
 */
#define USB_CFG_IMPLEMENT_FN_READ       1
/* Set this to 1 if you need to send control replies which are generated
 * "on the fly" when usbFunctionRead() is called. If you only want to send
 * data from a static buffer, set it to 0 and return the data from
 * usbFunctionSetup(). This saves a couple of bytes.
 */
#define USB_CFG_IMPLEMENT_FN_WRITEOUT   0
/* Define this to 1 if you want to use interrupt-out (or bulk out) endpoint 1.
 * You must implement the function usbFunctionWriteOut() which receives all
 * interrupt/bulk data sent to endpoint 1.
 */
#define USB_CFG_HAVE_FLOWCONTROL        0
/* Define this to 1 if you want flowcontrol over USB data. See the definition
 * of the macros usbDisableAllRequests() and usbEnableAllRequests() in
 * usbdrv.h.
 */
#ifndef __ASSEMBLER__
extern void usbEventResetReady(void);
#endif
#define USB_RESET_HOOK(isReset)             if(!isReset){usbEventResetReady();}
/* This macro is a hook if you need to know when an USB RESET occurs. It has
 * one parameter which distinguishes between the start of RESET state and its
 * end.
 */
#define USB_CFG_HAVE_MEASURE_FRAME_LENGTH   1
/* define this macro to 1 if you want the function usbMeasureFrameLength()
 * compiled in. This function can be used to calibrate the AVR's RC oscillator.
 */

/* -------------------------- Device Description --------------------------- */

/*
	Please note: Do not use the Adafruit USB VID/PID without written permission
	from Adafruit Industries, LLC and Limor "Ladyada" Fried
	(support@adafruit.com). Permission is granted for littlewire, Ihsan Kehribar
	and Seeed Studio by Adafruit Industries, LLC to use the Adafruit
	USB VID/PID for littlewire (SKU:AVR06071P)
*/

#define  USB_CFG_VENDOR_ID       0x81, 0x17
/* USB vendor ID for the device, low byte first. If you have registered your
 * own Vendor ID, define it here. Otherwise you use obdev's free shared
 * VID/PID pair. Be sure to read USBID-License.txt for rules!
 * This template uses obdev's shared VID/PID pair for HIDs: 0x16c0/0x5df.
 * Use this VID/PID pair ONLY if you understand the implications!
 */
#define  USB_CFG_DEVICE_ID       0x9f, 0x0c
/* This is the ID of the product, low byte first. It is interpreted in the
 * scope of the vendor ID. If you have registered your own VID with usb.org
 * or if you have licensed a PID from somebody else, define it here. Otherwise
 * you use obdev's free shared VID/PID pair. Be sure to read the rules in
 * USBID-License.txt!
 * This template uses obdev's shared VID/PID pair for HIDs: 0x16c0/0x5df.
 * Use this VID/PID pair ONLY if you understand the implications!
 */
#define USB_CFG_DEVICE_VERSION  0x04, 0x01
/* Version number of the device: Minor number first, then major number.
 */
#define USB_CFG_VENDOR_NAME     'o', 'b', 'd', 'e', 'v', '.', 'a', 't'
#define USB_CFG_VENDOR_NAME_LEN 8
#undef USB_CFG_VENDOR_NAME
#undef USB_CFG_VENDOR_NAME_LEN
/* These two values define the vendor name returned by the USB device. The name
 * must be given as a list of characters under single quotes. The characters
 * are interpreted as Unicode (UTF-16) entities.
 * If you don't want a vendor name string, undefine these macros.
 * ALWAYS define a vendor name containing your Internet domain name if you use
 * obdev's free shared VID/PID pair. See the file USBID-License.txt for
 * details.
 */
#define USB_CFG_DEVICE_NAME     'U', 'S', 'B', 't', 'i', 'n', 'y', 'S', 'P', 'I'
#define USB_CFG_DEVICE_NAME_LEN 10
/* Same as above for the device name. If you don't want a device name, undefine
 * the macros. See the file USBID-License.txt before you assign a name if you
 * use a shared VID/PID.
 */
#define USB_CFG_SERIAL_NUMBER   '5', '1' , '2'
#define USB_CFG_SERIAL_NUMBER_LEN   3
/* Same as above for the serial number. If you don't want a serial number,
 * undefine the macros.
 * It may be useful to provide the serial number through other means than at
 * compile time. See the section about descriptor properties below for how
 * to fine tune control over USB descriptors such as the string descriptor
 * for the serial number.
 */
#define USB_CFG_DEVICE_CLASS        0
#define USB_CFG_DEVICE_SUBCLASS     0
/* See USB specification if you want to conform to an existing device class.
 */
#define USB_CFG_INTERFACE_CLASS     0xff   /* HID */
#define USB_CFG_INTERFACE_SUBCLASS  0   /* no boot interface */
#define USB_CFG_INTERFACE_PROTOCOL  0   /* no protocol */
/* See USB specification if you want to conform to an existing device class or
 * protocol.
 */
#define USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH    0  /* total length of report descriptor */
/* Define this to the length of the HID report descriptor, if you implement
 * an HID device. Otherwise don't define it or define it to 0.
 * Since this template defines a HID device, it must also specify a HID
 * report descriptor length. You must add a PROGMEM character array named
 * "usbHidReportDescriptor" to your code which contains the report descriptor.
 * Don't forget to keep the array and this define in sync!
 */

/* #define USB_PUBLIC static */
/* Use the define above if you #include usbdrv.c instead of linking against it.
 * This technique saves a couple of bytes in flash memory.
 */

/* ------------------- Fine Control over USB Descriptors ------------------- */
/* If you don't want to use the driver's default USB descriptors, you can
 * provide our own. These can be provided as (1) fixed length static data in
 * flash memory, (2) fixed length static data in RAM or (3) dynamically at
 * runtime in the function usbFunctionDescriptor(). See usbdrv.h for more
 * information about this function.
 * Descriptor handling is configured through the descriptor's properties. If
 * no properties are defined or if they are 0, the default descriptor is used.
 * Possible properties are:
 *   + USB_PROP_IS_DYNAMIC: The data for the descriptor should be fetched
 *     at runtime via usbFunctionDescriptor().
 *   + USB_PROP_IS_RAM: The data returned by usbFunctionDescriptor() or found
 *     in static memory is in RAM, not in flash memory.
 *   + USB_PROP_LENGTH(len): If the data is in static memory (RAM or flash),
 *     the driver must know the descriptor's length. The descriptor itself is
 *     found at the address of a well known identifier (see below).
 * List of static descriptor names (must be declared PROGMEM if in flash):
 *   char usbDescriptorDevice[];
 *   char usbDescriptorConfiguration[];
 *   char usbDescriptorHidReport[];
 *   char usbDescriptorString0[];
 *   int usbDescriptorStringVendor[];
 *   int usbDescriptorStringDevice[];
 *   int usbDescriptorStringSerialNumber[];
 * Other descriptors can't be provided statically, they must be provided
 * dynamically at runtime.
 *
 * Descriptor properties are or-ed or added together, e.g.:
 * #define USB_CFG_DESCR_PROPS_DEVICE   (USB_PROP_IS_RAM | USB_PROP_LENGTH(18))
 *
 * The following descriptors are defined:
 *   USB_CFG_DESCR_PROPS_DEVICE
 *   USB_CFG_DESCR_PROPS_CONFIGURATION
 *   USB_CFG_DESCR_PROPS_STRINGS
 *   USB_CFG_DESCR_PROPS_STRING_0
 *   USB_CFG_DESCR_PROPS_STRING_VENDOR
 *   USB_CFG_DESCR_PROPS_STRING_PRODUCT
 *   USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER
 *   USB_CFG_DESCR_PROPS_HID
 *   USB_CFG_DESCR_PROPS_HID_REPORT
 *   USB_CFG_DESCR_PROPS_UNKNOWN (for all descriptors not handled by the driver)
 *
 */

#define USB_CFG_DESCR_PROPS_DEVICE                  0
#define USB_CFG_DESCR_PROPS_CONFIGURATION           0
#define USB_CFG_DESCR_PROPS_STRINGS                 0
#define USB_CFG_DESCR_PROPS_STRING_0                0
#define USB_CFG_DESCR_PROPS_STRING_VENDOR           0
#define USB_CFG_DESCR_PROPS_STRING_PRODUCT          0
// #define USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER    0
#define USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER    (USB_PROP_IS_RAM | (2 * USB_PROP_LENGTH(3)+2))
#define USB_CFG_DESCR_PROPS_HID                     0
#define USB_CFG_DESCR_PROPS_HID_REPORT              0
#define USB_CFG_DESCR_PROPS_UNKNOWN                 0

/* ----------------------- Optional MCU Description ------------------------ */

/* The following configurations have working defaults in usbdrv.h. You
 * usually don't need to set them explicitly. Only if you want to run
 * the driver on a device which is not yet supported or with a compiler
 * which is not fully supported (such as IAR C) or if you use a differnt
 * interrupt than INT0, you may have to define some of these.
 */
/* #define USB_INTR_CFG            MCUCR */
/* #define USB_INTR_CFG_SET        ((1 << ISC00) | (1 << ISC01)) */
/* #define USB_INTR_CFG_CLR        0 */
/* #define USB_INTR_ENABLE         GIMSK */
/* #define USB_INTR_ENABLE_BIT     INT0 */
/* #define USB_INTR_PENDING        GIFR */
/* #define USB_INTR_PENDING_BIT    INTF0 */
// use PCINT1 instead of INT0
#define USB_INTR_CFG            PCMSK
#define USB_INTR_CFG_SET        (1<<USB_CFG_DPLUS_BIT)
#define USB_INTR_ENABLE_BIT     PCIE
#define USB_INTR_PENDING_BIT    PCIF
#define USB_INTR_VECTOR         SIG_PIN_CHANGE

//#define USB_INTR_VECTOR 		INT0_vect
//#define USB_INTR_VECTOR 		PCINT9_vect




/* Record non-USB pin change interrupt by setting a flag.

   This assembler code is included by usbdrvasm*.inc where the KJKJKJKK sync
   detect code has failed to detect a sync.

   Where the USB line is inactive, as in the case we are interested in, this
   code is reached in 31 cycles, the sbis sampling happening 33 cycles after
   the pin change.

   This is a 2 microsecond delay. Given a normal dWIRE baud of 1/128th of the
   device clock speed, and allowing a fast max clock speed of 32 MHz, that
   gives a baud rate of 250 kbaud, that is, a bit time of 4 microseconds.

   Therefore we are successfully sampling the pin well within the shortest
   reasonable expected bit time.

   If the 0x55 from the device hits during USB packet handling it depends
   whether the USB packet handling completes before the last low pulse of
   the 0x55 arrives.

   The 0x55 is sent as start bit, 55 lsb first, stop bit:
   ___   _   _   _   _   _ ___
      |_| |_| |_| |_| |_| |___
       S 1 0 1 0 1 0 1 0 S
       t                 t
       a                 o
       r                 p
       t

   At our worst case o 4μs per bit, that's 36μs from the first transition
   from idle to 0 until the last one.

   USB start of frame and token packets are 32 bits long, and should be
   handled within around 25μs (?), so these whould be fine.

   However a USB data packet, including a payload of the maximimum 8 bytes,
   can take 70μs, so if the device reports break during one of these, we
   could miss it.

   Then again, the 0x55 should be preceeded by a break, whiich must also last
   at least one byte time, or 40μs. Therefore we should test for dwpin
   low at end of usb token handling.

   On detecting a non-idle dwdebugWIRE line, set dwBuf[0] to 1 and disable
   further interrupts from the debugWIRE pin changing.
*/

// debugWIRE bit normally 5, but can be changed for debugging
#define DW_BIT 2

#ifdef __ASSEMBLER__
macro nonUsbPinChange
    .global dwBuf
    sbic  PINB,DW_BIT
    rjmp  dWirePinIdle

    ldi   YL,1
    sts   dwBuf,YL
    cbi   PCMSK,DW_BIT   ; Disable further interrupts on dwire pin change

dWirePinIdle:
    endm
#endif

#define USB_SOF_HOOK nonUsbPinChange




#endif /* __usbconfig_h_included__ */
