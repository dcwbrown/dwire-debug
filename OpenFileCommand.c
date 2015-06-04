// OpenFileCommand.c

HANDLE CurrentFile                 = 0;
char   CurrentFileName[MAX_PATH+2] = "";

OPENFILENAME OpenFileName = {sizeof(OpenFileName), 0};


void OpenFileCommand()
{
  if (CurrentFile) {CloseHandle(CurrentFile);}

  OpenFileName.lpstrFile   = CurrentFileName;
  OpenFileName.nMaxFile    = countof(CurrentFileName);
  OpenFileName.Flags       = OFN_FILEMUSTEXIST;
  OpenFileName.lpstrFilter = "All files\0*.*\0*.bin\0*.bin\0\0";
  if (GetOpenFileName(&OpenFileName)) {
    CurrentFile = CreateFile(CurrentFileName, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
    if (CurrentFile == INVALID_HANDLE_VALUE) {
      DWORD winError = GetLastError();
      Ws("Couldn't open ");
      Ws(CurrentFileName);
      Ws(": ");
      WWinError(winError);
      Fail("");
    }
  }
}
