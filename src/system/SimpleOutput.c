/*        Simple output

          A very simple implementation of the output headers defined in
          SystemServices.c.
*/

int Verbose = 0;  // Set the verbose flag to flush all outputs, and to enable
                  // the Vc, Vs, Vl etc. versions of the output functions.

/// Simple standard output text writing and buffering.

char OutputBuffer[100]  = {0};
int  OutputPosition     = 0;
int  HorizontalPosition = 0;

void Flush() {
  if (OutputPosition) {
    Write(Output, OutputBuffer, OutputPosition);
    OutputPosition = 0;
  }
}

void Wc(char c) {
  if (OutputPosition >= sizeof OutputBuffer) {Flush();}
  OutputBuffer[OutputPosition++] = c;
  if (c == '\n'  ||  c == '\r') {
    Flush();
    HorizontalPosition = 0;
  } else {
    HorizontalPosition++;
  }
  if (Verbose) Flush();
}


void Wt(int tab) {
  tab = min(max(tab, HorizontalPosition), countof(OutputBuffer));
  while (HorizontalPosition < tab) {
    OutputBuffer[OutputPosition++] = ' ';
    HorizontalPosition++;
  }
  if (Verbose) Flush();
}

void Ws(const char *s) {
  while (*s) {Wc(*s); s++;}
}

void Wl() {
  #ifdef windows
    Ws("\r\n");
  #else
    Wc('\n');
  #endif
}

void Wr() {
  Wc('\r');
}

void Wsl(const char *s) {Ws(s); Wl();}

void Wd(s64 i, int w) {
  char r[22] = "0000000000000000000000";
  int  n = 0;
  if (i<0) {i = -i;  Wc('-');}
  while (i && n<sizeof(r)) {r[n++] = (i%10) + '0';  i = i/10;}
  if (n<w) {n=w;}
  if (n>=sizeof(r)) {n=sizeof(r)-1;}
  while (n) {Wc(r[--n]);}
}

char HexChar(int i) {return (i<10) ? '0'+i : 'a'+i-10;}

void Wx(u64 i, int w) {
  char r[20] = "00000000000000000000";
  int  n = 0;
  while (i && n<sizeof(r)) {r[n++] = HexChar(i&15);  i = (i >> 4) & 0x0fffffff;}
  if (n<w) {n=w;}
  if (n>=sizeof(r)) {n=sizeof(r)-1;}
  while (n) {Wc(r[--n]);}
}

void Whexbuf(const u8 *buf, int len) {
  Wx(buf[0], 2);
  for (int i=1; i<len; i++) {
    Wc(' ');
    Wx(buf[i], 2);
  }
}


// Verbose mode only versions

void Vl ()              {if (Verbose) Wl();}
void Vc (char c)        {if (Verbose) Wc(c);}
void Vs (const char* s) {if (Verbose) Ws(s);}
void Vsl(const char* s) {if (Verbose) Wsl(s);}
void Vd (int i, int w)  {if (Verbose) Wd(i, w);}
