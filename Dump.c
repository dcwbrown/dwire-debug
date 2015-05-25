/// Dump.c




void DumpBytes(int addr, int len, const u8 *buf) {
	// Dump len bytes form buf, addr being the address to annotate the first.
	if (len < 16) {
    // Dump up to 16 bytes in one line
    Wx(addr,4); Ws(":  ");
    for (int i=0; i<len; i++) {Wx(buf[i],2); Wc(' ');}

	} else {
		// Dump a formatted block with each row aligned to a multiple of 16 bytes.
		int base  = addr & 0xfffffff0;

		while (1) {
			Wx(base, 4); Wc(':');
			int first = max(base, addr);
			int limit = min(base+16, addr+len);

      // Hex dump
			for (int i=first; i<limit; i++) {
				int column = i-base;
				int pos    = 8 + column*3 + column/4 + column/8;
				Wt(pos);
				Wx(buf[i-addr], 2);
			}

      // ASCII dump
			for (int i=first; i<limit; i++) {
				int column = i-base;
				int pos    = 62 + column;
				Wt(pos);
				u8 byte = buf[i-addr];
				Wc((byte >=32  &&  byte < 127) ? byte: '.');
			}

			base += 16;
			if (base >= addr+len) {break;} else {Wl();}
		}
	}
	Wt(80); Prompted = 1;
}
