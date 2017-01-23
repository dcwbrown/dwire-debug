/*        Simple output

          A very simple implementation of the output headers defined in
          SystemServices.c.
*/

/// Simple standard output text writing and buffering.

char OutputBuffer[100] = {0};
int  OutputPosition    = 0;

void Flush() {
  if (OutputPosition) {
    Write(Output, OutputBuffer, OutputPosition);
    OutputPosition = 0;
  }
}

void Wc(char c) {
  if (OutputPosition >= sizeof OutputBuffer) {Flush();}
  OutputBuffer[OutputPosition++] = c;
}


void Wt(int tab) {
  tab = min(max(tab, OutputPosition), countof(OutputBuffer));
  while (OutputPosition < tab) {OutputBuffer[OutputPosition++] = ' ';}
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
  Flush();
}

void Wsl(const char *s) {Ws(s); Wl();}

void Wd(int i, int w) {
  char r[20] = "00000000000000000000";
  int  n = 0;
  if (i<0) {i = -i;  Wc('-');}
  while (i && n<sizeof(r)) {r[n++] = (i%10) + '0';  i = i/10;}
  if (n<w) {n=w;}
  if (n>=sizeof(r)) {n=sizeof(r)-1;}
  while (n) {Wc(r[--n]);}
}

char HexChar(int i) {return (i<10) ? '0'+i : 'a'+i-10;}

void Wx(unsigned int i, int w) {
  char r[20] = "00000000000000000000";
  int  n = 0;
  while (i && n<sizeof(r)) {r[n++] = HexChar(i&15);  i = (i >> 4) & 0x0fffffff;}
  if (n<w) {n=w;}
  if (n>=sizeof(r)) {n=sizeof(r)-1;}
  while (n) {Wc(r[--n]);}
}
