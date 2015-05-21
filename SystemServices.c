/*        System services

          A minimal layer providing basic system services on Windows and Linux
          without using platform specific libraries.
*/

#ifdef windows
  #include <windows.h>
#else
  #include <stdio.h>
  #include <fcntl.h>
  #include <unistd.h>
#endif






#define countof(array) (sizeof(array)/(sizeof(array)[0]))
#define ArrayAddressAndLength(array) array, sizeof(array)






#include <setjmp.h>
static jmp_buf FailPoint;






/// Minimal file access support.

#ifdef windows
  typedef HANDLE FileHandle;
  void Write(FileHandle handle, const void *buffer, int length) {WriteFile(handle, buffer, length, 0,0);}
  int  Read (FileHandle handle,       void *buffer, int length) {
    DWORD bytesRead = 0;
    return ReadFile(handle, buffer, length, &bytesRead, 0) ? bytesRead : 0;
  }
#else
  typedef int FileHandle;
  void Write (FileHandle handle, const void *buffer, int length) {write(handle, buffer, length);}
  int  Read  (FileHandle handle,       void *buffer, int length) {return read(handle, buffer, length);}
  int  IsUser(FileHandle handle)                                 {return isatty(handle);}
#endif

FileHandle Input  = 0;
FileHandle Output = 0;
FileHandle Error  = 0;

/// Minimal file access support.






/// Minimal memory allocation support

#ifdef windows
  void Exit(int exitCode) {ExitProcess(exitCode);}
#else
  void Exit(int exitCode) {_exit(exitCode);}
#endif


void *Allocate(int length) {
  #ifdef windows
    void *ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, length);
  #else
    void *ptr = sbrk(length);
  #endif
  if (ptr <= 0) {Write(Error, "Out of memory.\r\n", 16); Exit(1);}
  return ptr;
}

/// Minimal memory allocation support end.






/// Simple text writing interface headers.

void Flush();
void Wc(char c);
void Ws(const char *s);
void Wl();
void Wsl(const char *s);
void Wd(int i, int w);
void Wx(unsigned int i, int w);

/// Simple text writing interface headers end.






/// Stack backtrace report on Linux

#ifndef windows
#include <execinfo.h>
void BackTrace() {
  Wsl("Backtrace:"); 
  void  *functions[32];
  int    size    = backtrace(functions, countof(functions));
  Wd(size,1); Wsl(" callers:");
  char **strings = backtrace_symbols(functions, size);
  for (int i=0; i<size; i++) {Wsl(strings[i]);}

  //free(strings);
}

#endif

/// Stack backtrace report on Linux.






/// Simple error handling.

void Fail(const char *message) {Wsl(message); BackTrace(); longjmp(FailPoint,1);}

#define S(x) #x
#define S_(x) S(x)
#define __SLINE__ S_(__LINE__)
#define Assert(test) if(!(test)) {Fail("Assertion not met in " __FILE__ "[" __SLINE__ "]: " #test);}
#define AssertMessage(test, message) if(!(test)) {Fail("Assertion not met in " __FILE__ "[" __SLINE__ "]: " #test ": " message);}

/// Simple error handling end.




#ifdef windows
  #include "WindowsServices.c"
#endif



/// Main entry point wrapper

int Main(int argCount, char **argVector);

extern void EntryPoint() asm("EntryPoint"); // Specify exact name in object file
void EntryPoint() {
  #ifdef windows
    Input  = GetStdHandle(STD_INPUT_HANDLE);
    Output = GetStdHandle(STD_OUTPUT_HANDLE);
    Error  = GetStdHandle(STD_ERROR_HANDLE);
  #else
    Input  = 0;
    Output = 1;
    Error  = 2;
  #endif

  if (setjmp(FailPoint)) {Exit(3);}
  Exit(Main(0,0));
}
