/// DumpCommand.c





void ParseDumpParameters(char *context, int limit, int *addr, int *len) {
  Sb(); if (IsDwDebugNumeric(NextCh())) {*addr = ReadNumber(1);}
  Sb(); if (IsDwDebugNumeric(NextCh())) {*len  = ReadNumber(1);}

  Sb(); if (!DwEoln()) {Ws("Unrecognised parameters on "); Ws(context); Fail(" command.");}

  *len = min(*addr + *len, DataLimit()) - *addr;

  if (*len <= 0) {Ws("Requested addresses beyond range available for "); Ws(context); Fail(" command.");}
}




void DumpDataBytesCommand() {
  int length = 128; // Default data byte count to display
  ParseDumpParameters("dump bytes", DataLimit(), &DBaddr, &length);

  u8 buf[length];
  DwReadAddr(DBaddr, length, buf);

  DumpBytes(DBaddr, length, buf);
  DBaddr += length;
}




void DumpDataWordsCommand() {
  int length = 128; // Default data byte count to display
  ParseDumpParameters("dump bytes", DataLimit(), &DWaddr, &length);

  u8 buf[length];
  DwReadAddr(DWaddr, length, buf);

  DumpWords(DWaddr, length, buf);
  DWaddr += length;
}