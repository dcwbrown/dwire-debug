#ifndef command_hpp
#define command_hpp

void SysAbort(const char* e, ...);
void Debug(const char* format, ...);
int SysConWrite(const char* s);
int SysConAvail();
int SysConRead();

extern char* serialnumber;

#define RunInit(module) struct _##module##_Init{_##module##_Init();}__##module##_Init;_##module##_Init::_##module##_Init()
#define RunExit(module) struct _##module##_Exit{~_##module##_Exit();}__##module##_Exit;_##module##_Exit::~_##module##_Exit()

#endif
