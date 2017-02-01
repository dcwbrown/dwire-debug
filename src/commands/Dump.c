/// Dump.c


// TODO - DOn't force alignment to multiple of 16.



void DumpBytes(int addr, int len, const u8 *buf) {
  int first = addr;
  while (1) {
    Wx(first, 4); Wc(':');
    int limit = min(first+16, addr+len);

    // Hex dump
    for (int i=first; i<limit; i++) {
      int column = i-first;
      int pos    = 8 + column*3 + column/4 + column/8;
      Wt(pos);
      Wx(buf[i-addr], 2);
    }

    // ASCII dump
    for (int i=first; i<limit; i++) {
      int column = i-first;
      int pos    = 62 + column;
      Wt(pos);
      u8 byte = buf[i-addr];
      Wc((byte >=32  &&  byte < 127) ? byte: '.');
    }

    first += 16;
    if (first >= addr+len) {break;} else {Wl();}
  }
  Wt(80);
}




void DumpWords(int addr, int len, const u8 *buf) {
  int first = addr;
  while (1) {
    Wx(first, 4); Wc(':');
    int limit = min(first+16, addr+len);

    // Hex dump
    for (int i=first; i<limit; i+=2) {
      int column = i-first;
      int pos    = 8 + column*3 + column/4 + column/8;
      Wt(pos);
      Wx(buf[i-addr+1], 2); Wx(buf[i-addr], 2);
    }

    // ASCII dump
    for (int i=first; i<limit; i++) {
      int column = i-first;
      int pos    = 62 + column;
      Wt(pos);
      u8 byte = buf[i-addr];
      Wc((byte >=32  &&  byte < 127) ? byte: '.');
    }

    first += 16;
    if (first >= addr+len) {break;} else {Wl();}
  }
  Wt(80);
}
