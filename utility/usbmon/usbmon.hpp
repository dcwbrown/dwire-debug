#ifndef usbmon_hpp
#define usbmon_hpp

void reset();
void power();
void monitor();
void monitor_dw();
void change_serialnumber(const char* new_serialnumber);
void test();

#endif
