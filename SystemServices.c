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






/// Simple text writing interface headers.

void Flush();
void Wc(char c);
void Ws(const char *s);
void Wl();
void Wsl(const char *s);
void Wd(int i, int w);
void Wx(unsigned int i, int w);
void Fail(const char *message);

/// Simple text writing interface headers end.






/// Minimal file access support.

#ifdef windows
  void WWinError(DWORD winError);
  typedef HANDLE FileHandle;
  void Write (FileHandle handle, const void *buffer, int length) {WriteFile(handle, buffer, length, 0,0);}
  void Seek  (FileHandle handle, long offset)                    {SetFilePointer(handle, offset, 0, FILE_BEGIN);}
  void Close (FileHandle handle)                                 {CloseHandle(handle);}
  int  Read  (FileHandle handle,       void *buffer, int length) {
    DWORD bytesRead = 0;
    return ReadFile(handle, buffer, length, &bytesRead, 0) ? bytesRead : 0;
  }
  FileHandle Open  (const char *filename) {
    FileHandle h = CreateFile(filename, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
    if (h==INVALID_HANDLE_VALUE) {
      DWORD winError = GetLastError();
      Ws("Couldn't open "); Ws(filename); Ws(": "); WWinError(winError);
      Fail("");
    }
    return h;
  }
#else
  typedef int FileHandle;
  void Write (FileHandle handle, const void *buffer, int length) {write(handle, buffer, length);}
  void Seek  (FileHandle handle, long offset)                    {lseek(handle, offset, SEEK_SET);}
  int  Read  (FileHandle handle,       void *buffer, int length) {return read(handle, buffer, length);}
  void Close (FileHandle handle)                                 {close(handle);}
  FileHandle Open  (const char *filename) {
    FileHandle h = open(filename, O_RDONLY);
    if (h<0) {Ws("Couldn't open "); Fail(filename);}
    return h;
  }

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






/// Get command line parameters as a single char string.

char **ArgVector;
int    ArgCount;

#ifdef windows
  char *GetCommandParameters() {
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
  char *GetCommandParameters() {
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

  ArgVector = argVector;
  ArgCount  = argCount;

  if (setjmp(FailPoint)) {Exit(3);}
  Exit(Program(0,0));
}
