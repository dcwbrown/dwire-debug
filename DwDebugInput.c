/// DwDebugInput.c

int IsDwDebugNumeric(char c)   {return IsNumeric(c)  || c=='$' || c=='#';}
int IsCommandSeparator(char c) {return c==';' || c==',';}
int IsDwEoln(char c)           {return IsEolnChar(c) || IsCommandSeparator(c);}
int NotDwEoln(char c)          {return NotEoln(c)  &&  !IsCommandSeparator(c);}
int DwEoln()                   {return Eoln()  ||  IsCommandSeparator(NextCh());}

int ReadNumber(int defaultHex) {  // Recognise Leading '$' or trailing 'h' as hex, or '#' or 'd' for decimal.
  //Ws("ReadNumber("); Wd(defaultHex,1); Ws(") ");
  int decimal = 0;
  int hex     = 0;
  int digits  = 0;
  Sb();
  while (1) {
    char c = NextCh();
    if      (c=='h' || c=='x' || c=='$')  {decimal = -1;}
    else if (c=='t' || c=='#')            {hex     = -1;}
    else if (c>='0' && c<='9')            {decimal = decimal*10 + c-'0'; hex = hex*16 + (c-'0');      digits++;}
    else if (c>='a' && c<='f')            {decimal = -1;                 hex = hex*16 + (c-'a') + 10; digits++;}
    else if (c>='A' && c<='F')            {decimal = -1;                 hex = hex*16 + (c-'A') + 10; digits++;}
    else {
      if (!digits) {Fail("No digits in number.");}
      if ((hex >= 0) && (defaultHex  ||  decimal < 0)) {return hex;}
      if (decimal >= 0)                                {return decimal;}
      Fail("Hex digits in decimal number");
    }
    SkipCh();
  }
}

int ReadInstructionAddress(const char *dest) {
  int addr = ReadNumber(1);
  if (addr & 1) {Ws("Expecting even instruction address for "); Fail(dest);}
  return addr;
}






