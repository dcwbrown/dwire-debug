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
} Elf32_Shdr;



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
    Ws("Program header. Type "); Wd(header->type,1);
    Ws(", offset ");             Wd(header->offset,1);
    Ws(", vaddr ");              Wd(header->vaddr,1);
    Ws(", paddr ");              Wd(header->paddr,1);
    Ws(", filesize ");           Wd(header->filesize,1);
    Ws(", memsize ");            Wd(header->memsize,1);
    Ws(", flags ");              Wd(header->flags,1);
    Ws(", align ");              Wd(header->align,1);
    Wl();
  }

  // Section header table







  return 1;
}

void LoadElf() {

}

void LoadBinary() {

}

void LoadFile() {if (IsLoadableElf()) {LoadElf();} else {LoadBinary();}}