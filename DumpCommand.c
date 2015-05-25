/// DumpCommand.c






void DumpDataBytesCommand() {
  Sb(); if (IsDwDebugNumeric(NextCh())) {DBaddr = ReadNumber(1); Wl();}
  int length = 128; // Default data byte count to display
  Sb();  if (IsDwDebugNumeric(NextCh())) {length = ReadNumber(1);}
  if (!DwEoln()) {Wsl("Unrecognised parameters on dump data bytes command.");}

  length = min(DBaddr + length, DataLimit) - DBaddr;

  if (length <= 0) {Fail("No data to dump.");}

  u8 buf[length];
  DwReadAddr(DBaddr, length, buf);

  // Registers 30 and 31 are cached - use the cached values.
  if (DBaddr <= 30  &&  DBaddr+length > 30) {buf[30-DBaddr] = Registers[30];}
  if (DBaddr <= 31  &&  DBaddr+length > 31) {buf[31-DBaddr] = Registers[31];}

  DumpBytes(DBaddr, length, buf);
  DBaddr += length;
}




void DumpDataWordsCommand() {
  Sb(); if (IsDwDebugNumeric(NextCh())) {DWaddr = ReadNumber(1); Wl();}
  int length = 128; // Default data byte count to display
  Sb();  if (IsDwDebugNumeric(NextCh())) {length = ReadNumber(1);}
  if (!DwEoln()) {Wsl("Unrecognised parameters on dump data words command.");}

  length = min(DWaddr + length, DataLimit) - DWaddr;

  if (length <= 0) {Fail("No data to dump.");}

  u8 buf[length];
  DwReadAddr(DWaddr, length, buf);

  // Registers 30 and 31 are cached - use the cached values.
  if (DWaddr <= 30  &&  DWaddr+length > 30) {buf[30-DWaddr] = Registers[30];}
  if (DWaddr <= 31  &&  DWaddr+length > 31) {buf[31-DWaddr] = Registers[31];}

  DumpWords(DWaddr, length, buf);
  DWaddr += length;
}