// LoadFile.c


struct {
  u8  ident[4];      // "\x7fELF"
  u8  architecture;  // 32  bit vs 64 bit.
  u8  encoding;      // SIgn format and byte sex.
  u8  elfversion;    // ELF specification version number.
  u8  os;            // Operating system application binary interface.
  u8  abiversion;    // OS specofoc ABI version.
  u8  pad[7];        // Reserved. Must be ignored.
  u16 type;          // File type (relecatable/executable etc.).
  u16 machine;       // Machine architecture.
  u32 version;       // ELF format version.
  u32 entry;         // Entry point.
  u32 phoff;         // Program header file offset.
  u32 shoff;         // Section header file offset.
  u32 flags;         // Architecture-specific flags.
  u16 ehsize;        // Size of ELF header in bytes.
  u16 phentsize;     // Size of program header entry.
  u16 phnum;         // Number of program header entries.
  u16 shentsize;     // Size of section header entry.
  u16 shnum;         // Number of section header entries.
  u16 shstrndx;      // Section name strings section.
} ElfHeader;


struct ElfProgramHeader {
  u32 type;     // Entry type.
  u32 offset;   // File offset of contents.
  u32 vaddr;    // Virtual address in memory image.
  u32 paddr;    // Physical address (not used).
  u32 filesize; // Size of contents in file.
  u32 memsize;  // Size of contents in memory.
  u32 flags;    // Access permission flags.
  u32 align;    // Alignment in memory and file.
};


struct ElfSectionHeader {
  u32 name;      // Section name (index into the section header string table).
  u32 type;      // Section type.
  u32 flags;     // Section flags.
  u32 addr;      // Address in memory image.
  u32 offset;    // Offset in file.
  u32 size;      // Size in bytes.
  u32 link;      // Index of a related section.
  u32 info;      // Depends on section type.
  u32 addralign; // Alignment in bytes.
  u32 entsize;   // Size of each entry in section.
};

struct ElfSymbol {
  u32 name;   // String table index of name.
  u32 value;  // Symbol value.
  u32 size;   // Size of associated object.
  u8  info;   // Type and binding information.
  u8  other;  // Reserved (not used).
  u16 shndx;  // Section index of symbol.
};

struct stab {
  u32 strx;    // index into string table of name.
  u8  type;    // type of symbol.
  u8  other;   // misc info (usually empty).
  u16 desc;    // description field.
  u32 value;   // value of symbol.
};


char SourceFilename[300] = {0};


int IsLoadableElf() {
  int length = Read(CurrentFile, &ElfHeader, sizeof(ElfHeader));
  Seek(CurrentFile, 0);

  if (length != sizeof(ElfHeader))                                 {return 0;}
  if (memcmp(ElfHeader.ident, "\177ELF", sizeof(ElfHeader.ident))) {return 0;}

  // The file has a 4 byte standard ELF header. From now on explain why when
  // we choose not to load it as an ELF.

  if (ElfHeader.architecture != 1)  {Fail("Cannot load ELF that is not for 32-bit architecture.");}
  if (ElfHeader.encoding     != 1)  {Fail("Cannot load ELF that is not for two's complement little ending data encoding.");}
  if (ElfHeader.elfversion   != 1)  {Wsl("Warning: ELF EI_VERSION is not current version.");}
  if (ElfHeader.version      != 1)  {Wsl("Warning: ELF version is not current version.");}
  if (ElfHeader.type         != 2)  {Wsl("Warning: ELF type is not executable.");}
  if (ElfHeader.machine      != 83) {Wsl("Warning: ELF machine architecture is not AVR.");}
  if (    ElfHeader.phoff     == 0
      ||  ElfHeader.phentsize == 0
      ||  ElfHeader.phnum     == 0)  {Fail("Cannot load ELF with no program header table.");}

  Wsl("Loading ELF file.");

  // Program header table

  int elfProgramHeaderSize = ElfHeader.phnum * ElfHeader.phentsize;

  if (elfProgramHeaderSize < sizeof(struct ElfProgramHeader))       {Fail("Cannot load ELF with incomplete program header table.");}
  if (elfProgramHeaderSize > sizeof(struct ElfProgramHeader) * 100) {Fail("Cannot load ELF with unreasonable long program header table.");}

  u8 *programHeaders = Allocate(elfProgramHeaderSize);
  Seek(CurrentFile, ElfHeader.phoff);
  int lengthRead = Read(CurrentFile, programHeaders, elfProgramHeaderSize);
  if (lengthRead != elfProgramHeaderSize) {Fail("Cannot load ELF - failed to load program header table.");}

  for (int i=0; i<ElfHeader.phnum; i++) {
    struct ElfProgramHeader *header = (struct ElfProgramHeader *) (programHeaders + (i*ElfHeader.phentsize));
    Ws("Segment type "); Wd(header->type,1);
    Ws(", offset ");     Wd(header->offset,1);
    Ws(", vaddr ");      Wd(header->vaddr,1);
    Ws(", paddr ");      Wd(header->paddr,1);
    Ws(", filesize ");   Wd(header->filesize,1);
    Ws(", memsize ");    Wd(header->memsize,1);
    Ws(", flags ");      Wd(header->flags,1);
    Ws(", align ");      Wd(header->align,1);
    Wl();
  }

  // Section header table

  int elfSectionHeaderSize = ElfHeader.shnum * ElfHeader.shentsize;

  if (elfSectionHeaderSize < sizeof(struct ElfSectionHeader))       {Fail("Cannot load ELF with incomplete section header table.");}
  if (elfSectionHeaderSize > sizeof(struct ElfSectionHeader) * 200) {Fail("Cannot load ELF with unreasonable long section header table.");}

  u8 *sectionHeaders = Allocate(elfSectionHeaderSize);
  Seek(CurrentFile, ElfHeader.shoff);
  lengthRead = Read(CurrentFile, sectionHeaders, elfSectionHeaderSize);
  if (lengthRead != elfSectionHeaderSize) {Fail("Cannot load ELF - failed to load section header table.");}

  // Load section names

  if (ElfHeader.shstrndx >= ElfHeader.shnum) {Fail("Cannot load ELF with section name table index out of range.");}
  struct ElfSectionHeader *sectionNamesHeader = (struct ElfSectionHeader *) (sectionHeaders + ElfHeader.shentsize * ElfHeader.shstrndx);

  char *sectionNames = Allocate(sectionNamesHeader->size);
  Seek(CurrentFile, sectionNamesHeader->offset);
  lengthRead = Read(CurrentFile, sectionNames, sectionNamesHeader->size);
  if (lengthRead != sectionNamesHeader->size) {Fail("CannotLoad ELF with incomplete sectionnames table.");}

  // Display section headers content

  int symbolTableIndex     = -1;
  int symbolNameTableIndex = -1;
  int stabTableIndex       = -1;
  int stabstrTableIndex    = -1;

  for (int i=0; i<ElfHeader.shnum; i++) {
    struct ElfSectionHeader *header = (struct ElfSectionHeader *) (sectionHeaders + (i*ElfHeader.shentsize));
    if (header->name >= sectionNamesHeader->size-1) {Fail("Cannot load ELF with section name string beyond end of section name table.");}
    Ws("Section ");     Ws(sectionNames[header->name] ? sectionNames + header->name : "(unnamed)");
    Ws(", type ");      if (header->type<4) {Ws((char*[]){"null","progbits","symtab","strtab"}[header->type]);} else {Wd(header->type,1);}
    Ws(", flags ");     Wd(header->flags,1);
    Ws(", addr ");      Wd(header->addr,1);
    Ws(", offset ");    Wd(header->offset,1);
    Ws(", size ");      Wd(header->size,1);
    Ws(", link ");      Wd(header->link,1);
    Ws(", info ");      Wd(header->info,1);
    Ws(", addralign "); Wd(header->addralign,1);
    Ws(", entsize ");   Wd(header->entsize,1);
    if (i == ElfHeader.shstrndx) {Ws(" (section name table)");}
    //if (header->type == 2) {symbolTableIndex = i;}
    if (memcmp(sectionNames + header->name, ".symtab",  8) == 0) {symbolTableIndex = i;}
    if (memcmp(sectionNames + header->name, ".strtab",  8) == 0) {symbolNameTableIndex = i;}
    if (memcmp(sectionNames + header->name, ".stab",    8) == 0) {stabTableIndex = i;}
    if (memcmp(sectionNames + header->name, ".stabstr", 8) == 0) {stabstrTableIndex = i;}
    Wl();
  }

  // Symbol table

  if (symbolTableIndex > 0  &&  symbolNameTableIndex > 0) {

    // Load symbol table

    if (symbolTableIndex >= ElfHeader.shnum) {Fail("Cannot load ELF with symbol table index out of range.");}
    struct ElfSectionHeader *symbolTableHeader = (struct ElfSectionHeader *) (sectionHeaders + ElfHeader.shentsize * symbolTableIndex);

    struct ElfSymbol *symbols = Allocate(symbolTableHeader->size);
    Seek(CurrentFile, symbolTableHeader->offset);
    lengthRead = Read(CurrentFile, symbols, symbolTableHeader->size);
    if (lengthRead != symbolTableHeader->size) {Fail("CannotLoad ELF with incomplete symbol table.");}

    // Load symbol name table

    if (symbolNameTableIndex >= ElfHeader.shnum) {Fail("Cannot load ELF with symbol name table index out of range.");}
    struct ElfSectionHeader *symbolNamesHeader = (struct ElfSectionHeader *) (sectionHeaders + ElfHeader.shentsize * symbolNameTableIndex);

    char *symbolNames = Allocate(symbolNamesHeader->size);
    Seek(CurrentFile, symbolNamesHeader->offset);
    lengthRead = Read(CurrentFile, symbolNames, symbolNamesHeader->size);
    if (lengthRead != symbolNamesHeader->size) {Fail("CannotLoad ELF with incomplete symbol names table.");}

    // Display symbols

    u8 *p = (u8*)symbols;
    while (p < ((u8*)symbols) + symbolTableHeader->size) {
      struct ElfSymbol *symbol = (struct ElfSymbol *)p;
      //Ws("Symbol ");  Ws(symbolNames[symbol->name] ? symbolNames + symbol->name : "(unnamed)");
      //Ws(", value "); Wd(symbol->value,1);
      //Ws(", size ");  Wd(symbol->size,1);
      //Ws(", info ");  Wd(symbol->info,1);
      //Ws(", other "); Wd(symbol->other,1);
      //Ws(", shndx "); Wd(symbol->shndx,1);
      //Wl();

      if (symbol->name  &&  symbol->info < 16) { // Named SHB_LOCAL symbol
        if (symbol->shndx == 0xFFF1) {Ws(symbolNames+symbol->name); Wc(' '); Wt(10); Ws(".equ  0x"); Wx(symbol->value,4); Wl();}
        else {
          Ws(symbolNames+symbol->name); Wc(' '); Wt(10); Ws(".equ  ");
          struct ElfSectionHeader *refHeader = (struct ElfSectionHeader *) (sectionHeaders + ElfHeader.shentsize * symbol->shndx);
          Ws(sectionNames + refHeader->name);
          Wc(':'); Wx(symbol->value,4); Wl();
        }
      }
      p += symbolTableHeader->entsize;
    }
  }


  //stab (debugging data table)

  if (stabTableIndex > 0  &&  stabstrTableIndex > 0) {

    // Load stab

    if (stabTableIndex >= ElfHeader.shnum) {Fail("Cannot load ELF with stab table index out of range.");}
    struct ElfSectionHeader *stabTableHeader = (struct ElfSectionHeader *) (sectionHeaders + ElfHeader.shentsize * stabTableIndex);

    struct stab *stabs = Allocate(stabTableHeader->size);
    Seek(CurrentFile, stabTableHeader->offset);
    lengthRead = Read(CurrentFile, stabs, stabTableHeader->size);
    if (lengthRead != stabTableHeader->size) {Fail("CannotLoad ELF with incomplete stab table.");}

    // Load stab name table

    if (stabstrTableIndex >= ElfHeader.shnum) {Fail("Cannot load ELF with stabstr table index out of range.");}
    struct ElfSectionHeader *stabstrTableHeader = (struct ElfSectionHeader *) (sectionHeaders + ElfHeader.shentsize * stabstrTableIndex);

    char *stabNames = Allocate(stabstrTableHeader->size);
    Seek(CurrentFile, stabstrTableHeader->offset);
    lengthRead = Read(CurrentFile, stabNames, stabstrTableHeader->size);
    if (lengthRead != stabstrTableHeader->size) {Fail("CannotLoad ELF with incomplete stab names table.");}

    // Display stabs

    char *filename = "";

    u8 *p = (u8*)stabs;
    while (p < ((u8*)stabs) + stabTableHeader->size) {
      struct stab *stab = (struct stab *)p;
      //Ws("Stab ");    Ws(stabNames[stab->strx] ? stabNames + stab->strx : "(unnamed)");
      //Ws(", type ");  Wd(stab->type,1);
      //Ws(", other "); Wd(stab->other,1);
      //Ws(", desc ");  Wd(stab->desc,1);
      //Ws(", value "); Wd(stab->value,1);
      //Wl();

      if (stab->strx  &&  (stab->type == 100  ||  stab->type == 132)) {filename = stabNames+stab->strx;}
      if (stab->type == 68) { // N_SLINE
        Ws("0x"); Wx(stab->value,4); Ws(": "); Ws(filename); Wc(':'); Wd(stab->desc,1); Wl();
      }

      p += stabTableHeader->entsize;
    }
  }


//struct stab {
//  u32 strx;    // index into string table of name.
//  u8  type;    // type of symbol.
//  u8  other;   // misc info (usually empty).
//  u16 desc;    // description field.
//  u32 value;   // value of symbol.
//};

  return 1;
}

void LoadElf() {

}

void LoadBinary() {

}

void LoadFile() {if (IsLoadableElf()) {LoadElf();} else {LoadBinary();}}