/// WindowsServices.c - Windows specific system services






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
  Flush();
  LocalFree(lastErrorMessage);
}

#define WinOK(fn) if(!(fn)) {DWORD winError = GetLastError(); Ws("Couldn't " #fn ": "); WWinError(winError); Exit(2);}

/// Windows specific error handling end.






/// IsUser file handle test

struct FILE_NAME_INFO {
  DWORD FileNameLength;
  WCHAR FileName[1000];
} FileNameInfo;

typedef NTSTATUS (NTAPI *tNtQueryInformationFile) (HANDLE, PVOID, struct FILE_NAME_INFO*, ULONG, DWORD);

int IsUser(FileHandle handle) {
  DWORD fileType = GetFileType(handle);
  if (fileType == FILE_TYPE_CHAR) {return 1;}
  if (fileType != FILE_TYPE_PIPE) {return 0;}
  PVOID io[2];
  tNtQueryInformationFile pNtQueryInformationFile;
  if (!(pNtQueryInformationFile = (tNtQueryInformationFile) GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationFile"))) {return 0;}
  if (pNtQueryInformationFile(handle, &io, &FileNameInfo, sizeof(FileNameInfo) - sizeof(WCHAR), 9) & 0x80000000) {return 0;}
  if (FileNameInfo.FileNameLength / sizeof(WCHAR) < 28) {return 0;}
  for (int i=0; i<8; i++) {if (FileNameInfo.FileName[i] != "\\cygwin-"[i]) {return 0;}}
  for (int i=0; i<4; i++) {if (FileNameInfo.FileName[24+i] != "-pty"[i]) {return 0;}}
  return 1;
}

/// IsUser file handle test end.