// SimpleInputTests

char testbuf[128];

void runtests() {
  while (Available()) {
    if      (IsAlpha(NextCh()))   {Ws("Alpha:   '"); Ra(testbuf, sizeof(testbuf)); Ws(testbuf); Wsl("'");}
    else if (IsNumeric(NextCh())) {Ws("Numeric: '"); Rn(testbuf, sizeof(testbuf)); Ws(testbuf); Wsl("'");}
    else if (IsBlank(NextCh()))   {Ws("Blank:   '"); Rb(testbuf, sizeof(testbuf)); Ws(testbuf); Wsl("'");}
    else if (Eoln())              {Wsl("Eoln."); Sl();}
    else                          {Ws("Other:   '"); Ro(testbuf, sizeof(testbuf)); Ws(testbuf); Wsl("'");}
  }
  Wsl("Eof.");
}


