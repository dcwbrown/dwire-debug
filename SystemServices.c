/*        System services

          A minimal layer providing basic system services on Windows and Linux
          without using platform specific libraries.
*/

#ifdef windows
  #include <windows.h>
  #undef min
  #undef max
#else
  #include <stdio.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <stdlib.h>
  #include <string.h>
#endif



typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef signed   char  s8;
typedef signed   short s16;
typedef signed   int   s32;


#define countof(array) (sizeof(array)/(sizeof(array)[0]))
#define ArrayAddressAndLength(array) array, sizeof(array)

int min(int a, int b) {return a<b ? a : b;}
int max(int a, int b) {return a>b ? a : b;}





#include <setjmp.h>
static jmp_buf FailPoint;






/// Minimal file access support.

#ifdef windows
  typedef HANDLE FileHandle;
  void Write (FileHandle handle, const void *buffer, int length) {WriteFile(handle, buffer, length, 0,0);}
  void Seek  (FileHandle handle, long offset)                    {SetFilePointer(handle, offset, 0, FILE_BEGIN);}
  int  Read  (FileHandle handle,       void *buffer, int length) {
    DWORD bytesRead = 0;
    return ReadFile(handle, buffer, length, &bytesRead, 0) ? bytesRead : 0;
  }
#else
  typedef int FileHandle;
  void Write (FileHandle handle, const void *buffer, int length) {write(handle, buffer, length);}
  void Seek  (FileHandle handle, long offset)                    {lseek(handle, offset, SEEK_SET);}
  int  Read  (FileHandle handle,       void *buffer, int length) {return read(handle, buffer, length);}
  int  Interactive(FileHandle handle)                            {return isatty(handle);}
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

#ifdef windows
void StackTrace() {}
#else
#include <execinfo.h>
void StackTrace() {
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

void Fail(const char *message) {Wsl(message); StackTrace(); longjmp(FailPoint,1);}

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

int Program(int argCount, char **argVector);

//extern void EntryPoint() asm("EntryPoint"); // Specify exact name in object file
int main(int argCount, char **argVector) {
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
  Exit(Program(0,0));
}
