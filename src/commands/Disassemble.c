// Disassembler


int next; // Next instruction word value, for 2 word instructions.

/// AVR disassembly

//  0000 0000 0000 0000  N0P
//  0000 0001 dddd rrrr  M0VW
//  0000 0010 dddd rrrr  MULS
//  0000 0011 0ddd 0rrr  MULSU
//  0000 0011 0ddd 1rrr  FMUL
//  0000 0011 1ddd 0rrr  FMULS
//  0000 0011 1ddd 1rrr  FMULSU
//  0000 01rd dddd rrrr  CPC
//  0000 10rd dddd rrrr  SBC
//  0000 11rd dddd rrrr  ADD

void Wreg(int n) {
  Wc('r'); Wd(n, 1);
}

void WregPair(int n) {
  Wreg(n+1); Wc(':'); Wreg(n);
}

void WSramAddr(int addr) {
  if (SramSymbol[addr]) {
    Ws(SramSymbol[addr]);
    Ws(" ($"); Wx(addr,1); Wc(')');
  } else {
    Wc('$'); Wx(addr,1);
  }
}

int Winst0(int i, int code) {
  u8 rrrrr = ((code & 0x200) >> 5) | (code & 0xF);
  u8 ddddd = (code >> 4) & 0x1F;
  switch ((code>>8) & 0xF) {
    case 0: Ws("nop   "); return 0; break;
    case 1: Ws("movw  "); WregPair((code>>3) & 0x1e);   Wc(','); WregPair((code<<1) & 0x1e); return 0; break;
    case 2: Ws("muls  "); Wreg(((code>>4) & 0xf) + 16); Wc(','); Wreg((code & 0xf) + 16);    return 0; break;
    case 3: Ws("mulsu/fmul/fmuls/fmulsu?"); break;
    default:
      switch ((code>>10) & 3) {
        case 1: Ws("cpc   "); break;
        case 2: Ws("sbc   "); break;
        case 3: Ws("add   "); break;
      }
      Wreg(ddddd); Ws(", "); Wreg(rrrrr);
    break;
  }
  return 0;
}

int Winst8(int i, int code) {Ws("Inst 8xxx"); return 0;}
int WinstA(int i, int code) {Ws("Inst Axxx"); return 0;}

//  0001 00rd dddd rrrr  CPSE
//  0001 01rd dddd rrrr  CP
//  0001 10rd dddd rrrr  SUB
//  0001 11rd dddd rrrr  ADC

//  0010 00rd dddd rrrr  AND
//  0010 01rd dddd rrrr  E0R
//  0010 10rd dddd rrrr  0R
//  0010 11rd dddd rrrr  M0V

void wrdddddrrrr(int code) {
  int d = (code >> 4) & 0x1f;
  int r = ((code >> 5) & 0x10) | (code & 0x0f);
  Wreg(d); Ws(", "); Wreg(r);
}

int Winst1(int i, int code) {
  switch ((code>>10) & 3) {
    case 0: Ws("cpse  "); break;
    case 1: Ws("cp    "); break;
    case 2: Ws("sub   "); break;
    case 3: Ws("adc   "); break;
  }
  wrdddddrrrr(code);
  return 0;
}

int Winst2(int i, int code) {
  int d = (code >> 4) & 0x1f;
  int r = ((code >> 5) & 0x10) | (code & 0x0f);
  u8  c = (code >> 10) & 3;
  if (d == r  &&  c < 2) {
    switch (c) {
      case 0: Ws("tst   "); Wreg(d); break;
      case 1: Ws("clr   "); Wreg(d); break;
    }
  } else {
    switch ((code>>10) & 3) {
      case 0: Ws("and   "); break;
      case 1: Ws("eor   "); break;
      case 2: Ws("or    "); break;
      case 3: Ws("mov   "); break;
    }
    wrdddddrrrr(code);
  }
  return 0;
}



//  0011 kkkk dddd kkkk  CPI
//  0100 kkkk dddd kkkk  SBCI
//  0101 kkkk dddd kkkk  SUBI
//  0110 kkkk dddd kkkk  ORI  (aka SBR)
//  0111 kkkk dddd kkkk  ANDI

int Winst3(int i, int code) {Ws("cpi   "); Wreg(((code >> 4) & 0xF) + 16); Ws(", $"); Wx(((code >> 4) & 0xF0) | (code & 0x0F),1); return 0;}
int Winst4(int i, int code) {Ws("sbci  "); Wreg(((code >> 4) & 0xF) + 16); Ws(", $"); Wx(((code >> 4) & 0xF0) | (code & 0x0F),1); return 0;}
int Winst5(int i, int code) {Ws("subi  "); Wreg(((code >> 4) & 0xF) + 16); Ws(", $"); Wx(((code >> 4) & 0xF0) | (code & 0x0F),1); return 0;}
int Winst6(int i, int code) {Ws("ori   "); Wreg(((code >> 4) & 0xF) + 16); Ws(", $"); Wx(((code >> 4) & 0xF0) | (code & 0x0F),1); return 0;}
int Winst7(int i, int code) {Ws("andi  "); Wreg(((code >> 4) & 0xF) + 16); Ws(", $"); Wx(((code >> 4) & 0xF0) | (code & 0x0F),1); return 0;}



//  1000 000d dddd 0000  LD Z
//  1000 000d dddd 1000  LD Y

//  10q0 qq0d dddd 0qqq  LDD Z
//  10q0 qq0d dddd 1qqq  LDD Y
//  10q0 qq1d dddd 0qqq  STD Z
//  10q0 qq1d dddd 1qqq  STD Y



//  1001 000d dddd 0000  LDS *
//  1001 000d dddd 00-+  LD –Z+
//  1001 000d dddd 010+  LPM Z
//  1001 000d dddd 011+  ELPM Z
//  1001 000d dddd 10-+  LD –Y+
//  1001 000d dddd 11-+  LD X
//  1001 000d dddd 1111  P0P
int Winst90(int i, int code) { // 90xx and 91xx
  int dst = (code >> 4) & 0x1f;
  switch (code & 0xf) {
    case 0x0: Ws("lds   "); Wreg(dst); Ws(", $");  Wx(next,4); return 1; break;
    case 0x1: Ws("ld    "); Wreg(dst); Ws(", Z+");             return 0; break;
    case 0x2: Ws("ld    "); Wreg(dst); Ws(", -Z");             return 0; break;
    case 0x4: Ws("lpm   "); Wreg(dst); Ws(", Z");              return 0; break;
    case 0x5: Ws("lpm   "); Wreg(dst); Ws(", Z+");             return 0; break;
    case 0x6: Ws("elpm  "); Wreg(dst); Ws(", Z");              return 0; break;
    case 0x7: Ws("elpm  "); Wreg(dst); Ws(", Z+");             return 0; break;
    case 0x9: Ws("ld    "); Wreg(dst); Ws(", Y+");             return 0; break;
    case 0xA: Ws("ld    "); Wreg(dst); Ws(", -Y");             return 0; break;
    case 0xC: Ws("ld    "); Wreg(dst); Ws(", X");              return 0; break;
    case 0xD: Ws("ld    "); Wreg(dst); Ws(", X+");             return 0; break;
    case 0xE: Ws("ld    "); Wreg(dst); Ws(", -X");             return 0; break;
    case 0xF: Ws("pop   "); Wreg(dst);                         return 0; break;
    default:  Ws("???   ");                                    return 0; break;
  }
  return 0;
}

//  1001 001r rrrr 0000  STS *
//  1001 001r rrrr 00-+  ST –Z+
//  1001 001r rrrr 01xx  ???
//  1001 001r rrrr 10-+  ST –Y+
//  1001 001r rrrr 11-+  ST X
//  1001 001d dddd 1111  PUSH
int Winst92(int i, int code) { // 92xx and 93xx
  int dst = (code >> 4) & 0x1f;
  switch (code & 0xf) {
    case 0x0: Ws("sts   "); Wreg(dst); Ws(", $");  Wx(next,4); return 1; break;
    case 0x1: Ws("st    "); Wreg(dst); Ws(", Z+");             return 0; break;
    case 0x2: Ws("st    "); Wreg(dst); Ws(", -Z");             return 0; break;
    case 0x9: Ws("st    "); Wreg(dst); Ws(", Y+");             return 0; break;
    case 0xA: Ws("st    "); Wreg(dst); Ws(", -Y");             return 0; break;
    case 0xC: Ws("st    "); Wreg(dst); Ws(", X");              return 0; break;
    case 0xD: Ws("st    "); Wreg(dst); Ws(", X+");             return 0; break;
    case 0xE: Ws("st    "); Wreg(dst); Ws(", -X");             return 0; break;
    case 0xF: Ws("push  "); Wreg(dst);                         return 0; break;
    default:  Ws("???   ");                                    return 0; break;
  }
  return 0;
}


int Winst94(int i, int code) { // 94xx and 95xx
  if ((code & 0xE) == 8) {
    if (code & 0x100) {
      //  1001 0101 0000 1000  RET
      //  1001 0101 0000 1001  ICALL
      //  1001 0101 0001 1000  RETI
      //  1001 0101 0001 1001  EICALL
      //  1001 0101 1000 1000  SLEEP
      //  1001 0101 1001 1000  BREAK
      //  1001 0101 1010 1000  WDR
      //  1001 0101 1100 1000  LPM
      //  1001 0101 1101 1000  ELPM
      //  1001 0101 1110 1000  SPM
      //  1001 0101 1111 1000  ESPM
      switch (code & 0xFF) {
        case 0x08: Ws("ret   "); break;
        case 0x09: Ws("icall "); break;
        case 0x18: Ws("reti  "); break;
        case 0x19: Ws("eicall"); break;
        case 0x88: Ws("sleep "); break;
        case 0x98: Ws("break "); break;
        case 0xA8: Ws("wdr   "); break;
        case 0xC8: Ws("lpm   "); break;
        case 0xD8: Ws("elpm  "); break;
        case 0xE8: Ws("spm   "); break;
        case 0xF8: Ws("espm  "); break;
      }
    } else {
        if (code & 1) {
        //  1001 0100 0000 1001  IJMP
        //  1001 0100 0001 1001  EIJMP
        if (code & 0x10) {Ws("eijmp ");} else {Ws("ijmp  ");}
      } else { // bset, blcr
        //  1001 0100 0sss 1000  BSET
        //  1001 0100 1sss 1000  BCLR
        switch((code>>4) & 0xF) {
          case 0x0: Ws("sec   "); break;
          case 0x1: Ws("sez   "); break;
          case 0x2: Ws("sen   "); break;
          case 0x3: Ws("sev   "); break;
          case 0x4: Ws("ses   "); break;
          case 0x5: Ws("seh   "); break;
          case 0x6: Ws("set   "); break;
          case 0x7: Ws("sei   "); break;
          case 0x8: Ws("clc   "); break;
          case 0x9: Ws("clz   "); break;
          case 0xA: Ws("cln   "); break;
          case 0xB: Ws("clv   "); break;
          case 0xC: Ws("cls   "); break;
          case 0xD: Ws("clh   "); break;
          case 0xE: Ws("clt   "); break;
          case 0xF: Ws("cli   "); break;
        }
      }
    }
  } else {
    if ((code & 0xC) != 0xC) {
      //  1001 010d dddd 0000  COM
      //  1001 010d dddd 0001  NEG
      //  1001 010d dddd 0010  SWAP
      //  1001 010d dddd 0011  INC
      //  1001 010d dddd 0100  ???
      //  1001 010d dddd 0101  ASR
      //  1001 010d dddd 0110  LSR
      //  1001 010d dddd 0111  ROR
      //  1001 010d dddd 1010  DEC
      switch (code & 0xf) {
        case 0x0: Ws("com   "); break;
        case 0x1: Ws("neg   "); break;
        case 0x2: Ws("swap  "); break;
        case 0x3: Ws("inc   "); break;
        case 0x5: Ws("asr   "); break;
        case 0x6: Ws("lsr   "); break;
        case 0x7: Ws("ror   "); break;
        case 0xA: Ws("dec   "); break;
        default:  Ws("???   "); break;
      }
      Wreg((code >> 4) & 0x1f);
    } else { // jmp, call
      //  1001 010k kkkk 110k    kkkk kkkk kkkk kkkk  JMP *
      //  1001 010k kkkk 111k    kkkk kkkk kkkk kkkk  CALL *
      Ws((code & 2) ? "call  $" : "jmp   $");
      int addrhi = ((code >> 3) & 0x3E) | (code & 1);
      Wx(((addrhi<<16)+next)*2,6); return 1;
    }
  }
  return 0;
}




//  1001 0110 kkdd kkkk  ADIW
int Winst96(int i, int code) {
  int regpair = ((code >> 3) & 6) + 24;
  int value  = ((code >> 2) & 0x30) | (code & 0xf);
  Ws("adiw  "); WregPair(regpair); Ws(", $"); Wx(value,2);
  return 0;
}

//  1001 0111 kkdd kkkk  SBIW
int Winst97(int i, int code) {
  int regpair = ((code >> 3) & 6) + 24;
  int value  = ((code >> 2) & 0x30) | (code & 0xf);
  Ws("sbiw  "); WregPair(regpair); Ws(", $"); Wx(value,2);
  return 0;
}

//  1001 1000 AAAA Abbb  CBI
int Winst98(int i, int code) {Ws("cbi   "); WSramAddr((code >> 3) & 0x1F); Ws(", "); Wd(code & 7,1); return 0;}

//  1001 1001 AAAA Abbb  SBIC
int Winst99(int i, int code) {Ws("sbic  "); WSramAddr((code >> 3) & 0x1F); Ws(", "); Wd(code & 7,1); return 0;}

//  1001 1010 AAAA Abbb  SBI
int Winst9A(int i, int code) {Ws("sbi   "); WSramAddr((code >> 3) & 0x1F); Ws(", "); Wd(code & 7,1); return 0;}

//  1001 1011 AAAA Abbb  SBIS
int Winst9B(int i, int code) {Ws("sbis  "); WSramAddr((code >> 3) & 0x1F); Ws(", "); Wd(code & 7,1); return 0;}

//  1001 11rd dddd rrrr  MUL
int Winst9C(int i, int code) {
  u8 rrrrr = ((code & 0x200) >> 5) | (code & 0xF);
  u8 ddddd = (code >> 4) & 0x1F;
  Ws("mul   "); Wreg(ddddd); Wc(','); Wreg(rrrrr);
  return 0;
}


int Winst9(int i, int code) {
  switch ((code >> 8) & 0xf) {
    case 0x0:
    case 0x1: return Winst90(i, code); break;
    case 0x2:
    case 0x3: return Winst92(i, code); break;
    case 0x4:
    case 0x5: return Winst94(i, code); break;
    case 0x6: return Winst96(i, code); break;
    case 0x7: return Winst97(i, code); break;
    case 0x8: return Winst98(i, code); break;
    case 0x9: return Winst99(i, code); break;
    case 0xA: return Winst9A(i, code); break;
    case 0xB: return Winst9B(i, code); break;
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF: return Winst9C(i, code); break;  }
  return 0;
}


//  1011 0AAd dddd AAAA  IN
//  1011 1AAr rrrr AAAA  0UT
int WinstB(int i, int code) {
  int reg = (code >> 4) & 0x1f;
  int adr = ((code >> 5) & 0x30) | (code & 0xf);
  if (code & 0x800) {
    Ws("out   "); WSramAddr(adr); Ws(", "); Wreg(reg);
  } else {
    Ws("in    "); Wreg(reg); Ws(", "); WSramAddr(adr);
  }
  return 0;
}


void wRelative(int i, int delta) {
  if (CodeSymbol[i + delta*2]) {Ws(CodeSymbol[i + delta*2]);}
  else                         {Wx(i + delta*2, 4);}
  Ws(" (");
  if (delta >= 0) {Wc('+');}
  Wd(delta, 1);
  Wc(')');
}

void wRelative12(int i, int code) {
  if (code & 0x0800) {
    wRelative(i, ((code & 0x7ff) - 0x800) + 1);
  } else {
    wRelative(i, (code & 0x7ff) + 1);
  }
}

//  1100 kkkk kkkk kkkk  RJMP
int WinstC(int i, int code) {
  Ws("rjmp  ");
  wRelative12(i, code);
  return 0;
}


//  1101 kkkk kkkk kkkk  RCALL
int WinstD(int i, int code) {
  Ws("rcall ");
  wRelative12(i, code);
  return 0;
}


//  1110 kkkk dddd kkkk  LDI
int WinstE(int i, int code) {
  int reg = ((code >> 4) & 0xf) + 16;
  int val = ((code >> 4) & 0xf0) | (code & 0xf);
  Ws("ldi   ");
  Wreg(reg); Ws(", "); WSramAddr(val);
  return 0;
}



void wRelative7(int i, int code) {
  if (code & 0x40) {
    wRelative(i, ((code & 0x3f) - 0x40) + 1);
  } else {
    wRelative(i, (code & 0x3f) + 1);
  }
}


int WinstF(int i, int code) {
  if (!(code & 0x800)) {
    //  1111 00kk kkkk ksss  BRBS
    //  1111 01kk kkkk ksss  BRBC
    int s = code & 7;
    int k = (code >> 3) & 0x7f;

    if (code & 0x400) { // brbc
      switch (s) {
        case 0: Ws("brsh  "); break; // aka brcc
        case 1: Ws("brne  "); break;
        case 2: Ws("brpl  "); break;
        case 3: Ws("brvc  "); break;
        case 4: Ws("brge  "); break;
        case 5: Ws("brhc  "); break;
        case 6: Ws("brtc  "); break;
        case 7: Ws("brid  "); break;
      }
    } else { // brbs
      switch (s) {
        case 0: Ws("brlo  "); break; // aka brcs
        case 1: Ws("breq  "); break;
        case 2: Ws("brmi  "); break;
        case 3: Ws("brvs  "); break;
        case 4: Ws("brlt  "); break;
        case 5: Ws("brhs  "); break;
        case 6: Ws("brts  "); break;
        case 7: Ws("brie  "); break;
      }
    }
    wRelative7(i, k);
  } else if ((code & 8) == 0) {
    //  1111 100d dddd 0bbb  BLD
    //  1111 101d dddd 0bbb  BST
    //  1111 110r rrrr 0bbb  SBRC
    //  1111 111r rrrr 0bbb  SBRS
    int bit = code & 7;
    int reg = (code >> 4) & 0x1f;
    switch ((code >> 9) & 3) {
      case 0: Ws("bld   "); break;
      case 1: Ws("bst   "); break;
      case 2: Ws("sbrc  "); break;
      case 3: Ws("sbrs  "); break;
    }
    Wreg(reg); Ws(", "); Wd(bit,1);
  } else {
    if (code != 0xffff) {Ws("???");}  // Special casse - show nothing for uninitialised memory
  }
  return 0;
}


int Winstruction(int i, int code) {
  switch (code>>12) {
    case 0x0:  return Winst0(i, code); break;
    case 0x1:  return Winst1(i, code); break;
    case 0x2:  return Winst2(i, code); break;
    case 0x3:  return Winst3(i, code); break;
    case 0x4:  return Winst4(i, code); break;
    case 0x5:  return Winst5(i, code); break;
    case 0x6:  return Winst6(i, code); break;
    case 0x7:  return Winst7(i, code); break;
    case 0x8:  return Winst8(i, code); break;
    case 0x9:  return Winst9(i, code); break;
    case 0xA:  return WinstA(i, code); break;
    case 0xB:  return WinstB(i, code); break;
    case 0xC:  return WinstC(i, code); break;
    case 0xD:  return WinstD(i, code); break;
    case 0xE:  return WinstE(i, code); break;
    case 0xF:  return WinstF(i, code); break;
  }
  return 0;
}

char *SkipPath(char *name) { // Return pointer to name with any leading path skipped
  char *p     = name;
  char *start = name;
  while (*p) {
    if (((*p == '/') || (*p == '\\'))  &&  *(p+1)) {start = p+1;}
    p++;
  }
  return start;
}

int DisassembleInstruction(int addr, u8 *buf) { // Returns instruction length in words
  Assert((addr & 1) == 0);
  if (CodeSymbol[addr]) {Wl(); Ws(CodeSymbol[addr]); Wsl(":");}
  if (HasLineNumbers) {
    if (FileName[addr])   {Ws(SkipPath(FileName[addr]));}
    if (LineNumber[addr]) {Wc('['); Wd(LineNumber[addr],1); Wc(']');}
    Wt(20);
  }
  Wx(addr, 4); Ws(": ");
  int code = (buf[1] << 8) | buf[0];
  Wx(code, 4); Ws("  ");     // Instruction code
  next = (buf[3] << 8) | buf[2];
  if (Winstruction(addr, code)) return 4;
  return 2;
}

