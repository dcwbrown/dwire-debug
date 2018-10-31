/*        System services

          A minimal layer providing basic system services on Windows and Linux
          without using platform specific libraries.
*/

#ifdef windows
  #include <winsock2.h>
  #include <windows.h>
  #include <time.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <setjmp.h>
  #undef min
  #undef max
  void delay(unsigned int ms) {Sleep(ms);}
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdint.h>
  #include <string.h>
  #include <errno.h>
  #include <sys/select.h>
  #include <dirent.h>
#ifdef __APPLE__
  #include <termios.h>
#else
  #include <stropts.h>
  #include <asm/termios.h>
#endif
  #include <setjmp.h>
  #include <dlfcn.h>
  void delay(unsigned int ms) {usleep(ms*1000);}
#endif



typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed   char      s8;
typedef signed   short     s16;
typedef signed   int       s32;
typedef signed   long long s64;


#define countof(array) (sizeof(array)/(sizeof(array)[0]))
#define ArrayAddressAndLength(array) array, sizeof(array)
#define Bytes(...) (u8[]){__VA_ARGS__}, sizeof((u8[]){__VA_ARGS__})

int min(int a, int b) {return a<b ? a : b;}
int max(int a, int b) {return a>b ? a : b;}






/// Simple text writing interface headers.

int Verbose = 0;  // Set the verbose flag to flush all outputs, and to enable
                  // the Vc, Vs, Vl etc. versions of the output functions.

void Wflush(void);
void Wc(char c);
void Ws(const char *s);
void Wl(void);
void Wsl(const char *s);
void Wd(s64 i, int w);
void Wx(u64 i, int w);
void Fail(const char *message);
void DrainInput(void);
void DumpInputState(void);

/// Simple text writing interface headers end.






/// Minimal file access support.

#ifdef windows
  void WWinError(DWORD winError);
  typedef HANDLE FileHandle;
  FileHandle Open  (const char *filename) {
    FileHandle h = CreateFile(filename, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
    if (h==INVALID_HANDLE_VALUE) {
      DWORD winError = GetLastError();
      Ws("Couldn't open "); Ws(filename); Ws(": "); WWinError(winError);
      Fail("");}
    return h;}
  void Seek  (FileHandle handle, long offset)                    {SetFilePointer(handle, offset, 0, FILE_BEGIN);}
  int  Read  (FileHandle handle, void *buffer, int length)       {DWORD bytesRead = 0; return ReadFile(handle, buffer, length, &bytesRead, 0) ? bytesRead : 0;}
  int  Write (FileHandle handle, const void *buffer, int length) {DWORD lengthWritten; WriteFile(handle, buffer, length, &lengthWritten,0); return lengthWritten;}
  void Close (FileHandle handle)                                 {CloseHandle(handle);}
  int Socket(int domain, int type, int protocol)                 {return WSASocket(domain, type, protocol, 0,0,0);}
#else
  typedef int FileHandle;
  FileHandle Open  (const char *filename) {
    FileHandle h = open(filename, O_RDONLY);
    if (h<0) {Ws("Couldn't open "); Fail(filename);}
    return h;
  }
  void Seek  (FileHandle handle, long offset)                    {lseek(handle, offset, SEEK_SET);}
  int  Read  (FileHandle handle,       void *buffer, int length) {return read(handle, buffer, length);}
  int  Write (FileHandle handle, const void *buffer, int length) {return write(handle, buffer, length);}
  void Close (FileHandle handle)                                 {close(handle);}
  int  Interactive(FileHandle handle)                            {return isatty(handle);}
  void PrintLastError(const char *msg)                           {perror(msg);}
  int  Socket(int domain, int type, int protocol)                {return socket(domain, type, protocol);}
#endif

#undef  SetFilePointer
#define SetFilePointer "Do not use SetFilePointer, use Seek."
#undef  ReadFile
#define ReadFile i     "Do not use ReadFile, use Read."
#undef  WriteFile
#define WriteFile      "Do not use WriteFile, use Write."
#define socket         "Do not use socket, use Socket."
#define read           "Do not use read, use Read."
#define write          "Do not use write, use Write."
#define perror         "Do not use perror, use PrintLastError."


FileHandle Input  = 0;
FileHandle Output = 0;
FileHandle Error  = 0;

/// Minimal file access support.






/// Minimal memory allocation support

#ifdef windows
  void Exit(int exitCode) {timeEndPeriod(1); WSACleanup(); ExitProcess(exitCode);}
#else
  void Exit(int exitCode) {_exit(exitCode);}
#endif


void *Allocate(int length) {
  #ifdef windows
    void *ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, length);
  #else
    void *ptr = malloc(length);
  #endif
  if (ptr <= 0) {Write(Error, "Out of memory.\r\n", 16); Exit(1);}
  return ptr;
}

void Free(void *ptr) {
  #ifdef windows
    HeapFree(GetProcessHeap(), 0, ptr);
  #else
    free(ptr);
  #endif
}

/// Minimal memory allocation support end.






/// Stack backtrace report on Linux

#ifdef windows
void StackTrace(void) {}
#else
#include <execinfo.h>
void StackTrace(void) {
  Wsl("Backtrace:");
  char  *p = 0;
  void  *functions[32];
  int    count = backtrace(functions, countof(functions));
  char **strings = backtrace_symbols(functions, count);
  for (int i=2; i<count; i++) { // Skip 2 references: 'StackTrace' and 'Fail'.
    if (!strncmp(strings[i], "./dwdebug(", 10)) {strings[i] += 10; if ((p=strchr(strings[i], ')'))) {*p = 0;}}
    if (!strncmp(strings[i], "main+", 5)) {break;}  // Stop when we reach our main prgram wrapper
    Ws("  "); Wsl(strings[i]);
  }
  free(strings);
}

#endif

/// Stack backtrace report on Linux.






/// Simple error handling.

static jmp_buf FailPoint;

void Fail(const char *message) {
  Wsl(message);
  if (Verbose) StackTrace();
  longjmp(FailPoint,1);
}

#define S(x) #x
#define S_(x) S(x)
#define __SLINE__ S_(__LINE__)
#define Assert(test) if(!(test)) {Fail("Assertion not met in " __FILE__ "[" __SLINE__ "]: " #test);}
#define AssertMessage(test, message) if(!(test)) {Fail("Assertion not met in " __FILE__ "[" __SLINE__ "]: " #test ": " message);}

/// Simple error handling end.






/// Get command line parameters as a single char string.

char **ArgVector;
int    ArgCount;

#ifdef windows
  char *GetCommandParameters(void) {
    char *command = GetCommandLine();
    while (*command  &&  *command <= ' ') {command++;}     // Skip leading spaces
    while (*command > ' ') {                               // Skip to blank after command filename
      if (*command == '"') {                               // Skip up to second double-quote of a pair, and anything in between
        command++;
        while (*command  &&  *command != '"') {command++;}
      }
      if (*command) {command++;}
    }
    while (*command  &&  *command <= ' ') {command++;}     // Skip spaces before first parameter
    return command;
  }
#else
  // On Linux, piece the command line together from the arguments. We don't
  // bother to put quotes around arguments with spaces ...
  char CommandParameters[256];
  char *GetCommandParameters(void) {
    int arg = 1;
    int p   = 0;
    while (arg < ArgCount) {
      int argLength = strlen(ArgVector[arg]);
      if (p + argLength + 1 >= sizeof(CommandParameters) - 1) {break;}
      if (p) {CommandParameters[p++] = ' ';}
      memcpy(CommandParameters+p, ArgVector[arg], argLength);
      p += argLength;
      arg++;
    }
    CommandParameters[p] = 0;
    return CommandParameters;
  }
#endif

/// Get command line parameters as a single char string end.






/// Simple string handling

void TrimTrailingSpace(char *s) {
  char *p = 0; // First char of potential trailing space
  while (*s) {if (*s > ' ') {p = 0;} else if (!p)  {p = s;}; s++;}
  if (p) {*p=0;}
}

/// Simple string handling end.






#ifdef windows

/// Windows specific error handling.

void WWinError(DWORD winError) {
  char *lastErrorMessage = 0;
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    0,
    winError,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR) &lastErrorMessage,
    0,0
  );
  Ws(lastErrorMessage);
  Wflush();
  LocalFree(lastErrorMessage);
}

void PrintLastError(const char *msg) {
  if (msg) {Ws(msg); Ws(": ");}
  WWinError(GetLastError());
}

#define WinOK(fn) if(!(fn)) {DWORD winError = GetLastError(); Ws("Couldn't " #fn ": "); WWinError(winError); Exit(2);}

/// Windows specific error handling end.






/// Interactive file handle test

struct FILE_NAME_INFO {DWORD length; WCHAR name[1000];};
typedef NTSTATUS (NTAPI *tNtQueryInformationFile) (HANDLE, PVOID, struct FILE_NAME_INFO*, ULONG, DWORD);
int Interactive(FileHandle handle) {
  DWORD fileType = GetFileType(handle);
  if (fileType == FILE_TYPE_CHAR) {return 1;}
  if (fileType != FILE_TYPE_PIPE) {return 0;}
  PVOID io[2];
  tNtQueryInformationFile pNtQueryInformationFile;
  if (!(pNtQueryInformationFile = (tNtQueryInformationFile) GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationFile"))) {return 0;}
  struct FILE_NAME_INFO info;
  if (pNtQueryInformationFile(handle, &io, &info, sizeof(info) - sizeof(WCHAR), 9) & 0x80000000) {return 0;}
  if (info.length / sizeof(WCHAR) < 28) {return 0;}
  for (int i=0; i<8; i++) {if (info.name[i] != "\\cygwin-"[i]) {return 0;}}
  for (int i=0; i<4; i++) {if (info.name[24+i] != "-pty"[i]) {return 0;}}
  return 1;
}

/// Interactive file handle test end.

#endif




/// Main entry point wrapper

int systemstartup(int argCount, char **argVector) {
  #ifdef windows
    Input  = GetStdHandle(STD_INPUT_HANDLE);
    Output = GetStdHandle(STD_OUTPUT_HANDLE);
    Error  = GetStdHandle(STD_ERROR_HANDLE);
    timeBeginPeriod(1);
  #else
    Input  = 0;
    Output = 1;
    Error  = 2;
  #endif

  ArgVector = argVector;
  ArgCount  = argCount;

  if (setjmp(FailPoint)) {Exit(3);}
  return 0;
}
