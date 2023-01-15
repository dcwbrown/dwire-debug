#ifndef command_hpp
#define command_hpp

void SysAbort(const char* e, ...);
void Debug(const void* format, ...);
int SysConWrite(const void* s);
int SysConAvail();
int SysConRead();

extern char* opt_device;
extern int opt_power;
extern int opt_reset;
extern int opt_debug;
extern int opt_nomon;
extern char* opt_set_serial;

#define RunInit(module) struct _##module##_Init{_##module##_Init();}__##module##_Init;_##module##_Init::_##module##_Init()
#define RunExit(module) struct _##module##_Exit{~_##module##_Exit();}__##module##_Exit;_##module##_Exit::~_##module##_Exit()

#endif
