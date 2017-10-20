// GlobalData.c




// Current device state

int PC  = 0;  // PC as a flash address, twice the value used over the dwire interface
int BP  =-1;  // BP as a flash address, twice the value used over the dwire interface
u8  R[32];    // r28-r31 cached at break, others loaded on demand


int TimerEnable = 0;




// Current dump instruction states

int DBaddr = 0;  // Data area by byte
int DWaddr = 0;  // Data area by word
int EBaddr = 0;  // EEPROM by byte
int EWaddr = 0;  // EEPROM by word
int FBaddr = 0;  // Flash by byte
int FWaddr = 0;  // Flash by word
int Uaddr  = 0;  // Disassembly (Unassemble command)


void ResetDumpStates() {
  DBaddr = 0;
  DWaddr = 0;
  EBaddr = 0;
  EWaddr = 0;
  FBaddr = 0;
  FWaddr = 0;
  Uaddr  = 0;
}