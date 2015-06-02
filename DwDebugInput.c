/// DwDebugInput.c

int IsDwDebugNumeric(char c) {return IsNumeric(c) || c=='$' || c=='#';}
int IsDwEoln(char c)         {return IsEolnChar(c) || c == ';';}
int NotDwEoln(char c)        {return NotEoln(c)  &&  c != ';';}
int DwEoln()                 {return Eoln()  ||  NextCh() == ';';}

int ReadNumber(int defaultHex) {  // Recognise Leading '$' or trailing 'h' as hex, or '#' or 'd' for decimal.
  int  decimal = 0;
  int  hex     = 0;
  char c       = 0;
  int  digits  = 0;

  Sb(); c = NextCh();

  while (1) {
    c = NextCh();
    if      (c == 'h' ||  c == 'x'  ||  c == '$')  {decimal = -1;}
    else if (c == 't' ||  c == '#')                {hex = -1;}
    else if (c >= '0'  &&  c <= '9')               {digits++; decimal = decimal*10 + c-'0'; hex = hex*16 + c-'0';}
    else if (c >= 'a'  &&  c <= 'f')               {digits++; decimal = -1;                 hex = hex*16 + c-'a'+10;}
    else if (c >= 'A'  &&  c <= 'F')               {digits++; decimal = -1;                 hex = hex*16 + c-'F'+10;}
    else {
      if (!digits) {Fail("No digits in number.");}
      if (hex >= 0) {
        if (defaultHex  ||  decimal < 0) {return hex;}
      }
      if (decimal >= 0) {return decimal;}
      Fail("Hex digits in decimal number");
    }
    SkipCh();
  }
}






