/// DataMemory.c





void ParseDumpParameters(char *context, int limit, int *addr, int *len) {
  Sb(); if (IsDwDebugNumeric(NextCh())) {*addr = ReadNumber(1);}
  Sb(); if (IsDwDebugNumeric(NextCh())) {*len  = ReadNumber(1);}

  Sb(); if (!DwEoln()) {Ws("Unrecognised parameters on "); Ws(context); Fail(" command.");}

  *len = min(*addr + *len, limit) - *addr;

  if (*len <= 0) {Ws("Requested addresses beyond range available for "); Ws(context); Fail(" command.");}
}




void DumpDataBytesCommand() {
  int length = 128; // Default data byte count to display
  ParseDumpParameters("dump data bytes", DataLimit(), &DBaddr, &length);

  u8 buf[length];
  DwReadAddr(DBaddr, length, buf);

  DumpBytes(DBaddr, length, buf);
  DBaddr += length;
}




void DumpDataWordsCommand() {
  int length = 128; // Default data byte count to display
  ParseDumpParameters("dump data words", DataLimit(), &DWaddr, &length);

  u8 buf[length];
  DwReadAddr(DWaddr, length, buf);

  DumpWords(DWaddr, length, buf);
  DWaddr += length;
}




void ParseWriteParameters(char *context, int *addr, int limit, u8 *buf, int *len) {
  Sb(); if (!IsDwDebugNumeric(NextCh())) {Ws("Missing address parameter on "); Ws(context); Fail(" command.");}
  *addr = ReadNumber(1);

  *len = 0;
  Sb();
  while (IsDwDebugNumeric(NextCh())  &&  (*len < 16)) {
    int byte = ReadNumber(1);
    if (byte < 0  ||  byte > 255) {Ws("Invalid byte value on "); Ws(context); Fail(" command.");}
    buf[(*len)++] =byte;
    Sb();
  }

  if (!DwEoln()) {Ws("Unrecognised parameters on "); Ws(context); Fail(" command.");}

  if (*len < 1) {Ws("Missing data bytes on "); Ws(context); Fail(" command.");}
  if (*addr + *len > limit) {Ws("Requested write address beyond range available for "); Ws(context); Fail(" command.");}
}




void WriteDataBytesCommand() {
  u8 buf[16];
  int addr;
  int len;
  ParseWriteParameters("Data memory write", &addr, DataLimit(), buf, &len);
  if (len) {DwWriteAddr(addr, len, buf);}
}

