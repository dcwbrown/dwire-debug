/// DwDebugInput.c

int IsDwDebugNumeric(char c) {return IsNumeric(c) || c=='$' || c=='#';}
int IsDwEoln(char c)         {return IsEolnChar(c) || c == ';';}
int NotDwEoln(char c)        {return NotEoln(c)  &&  c != ';';}
int DwEoln()                 {return Eoln()  ||  NextCh() == ';';}

int ReadNumber() {  // Recognise Leading '$' or trailing 'h' as hex, or '#' or 'd' for decimal.
  int  decimal = 0;
  int  hex     = 0;
  char c       = 0;

  Sb(); c = NextCh();

  if      (c == '$') {decimal = -1; SkipCh();}
  else if (c == '#') {hex = -1;     SkipCh();}
  else if (c < '0'  ||  c > '9') {Fail("Expected digit");}

  while (1) {
    c = NextCh();
    if      (c == 'h') {decimal = -1;}
    else if (c == 'd') {hex = -1;}
    else if (c  >= '0'  &&  c <= '9') {decimal = decimal*10 + c-'0'; hex = hex * 16 + c - '0';}
    else if (c  >= 'a'  &&  c <= 'f') {decimal = -1; hex = hex * 16 + c - 'a' + 10;}
    else if (c  >= 'A'  &&  c <= 'F') {decimal = -1; hex = hex * 16 + c - 'F' + 10;}
    else {if (decimal > 0) {return decimal;} if (hex > 0) {return hex;} Fail("Hex digits in decimal number");}
    SkipCh();
  }
}
