#include "rsp.h"
#include "string.h"

int target_write_registers(uint8_t *buf, int first, int last)
{
    DwWriteRegisters(buf, 0, last);

    return 0;
}

int target_read_registers(struct registers *r)
{
    uint8_t buf[32];
    uint8_t io[3];

    DwReadRegisters(buf, 0, 30);
    DwReadAddr(0x5D, sizeof(io), io);

    memset(r, 0, sizeof(struct registers));

    r->r0 = buf[0];
    r->r1 = buf[1];
    r->r2 = buf[2];
    r->r3 = buf[3];
    r->r4 = buf[4];
    r->r5 = buf[5];
    r->r6 = buf[6];
    r->r7 = buf[7];
    r->r8 = buf[8];
    r->r9 = buf[9];
    r->r10 = buf[10];
    r->r11 = buf[11];
    r->r12 = buf[12];
    r->r13 = buf[13];
    r->r14 = buf[14];
    r->r15 = buf[15];
    r->r16 = buf[16];
    r->r17 = buf[17];
    r->r18 = buf[18];
    r->r19 = buf[19];
    r->r20 = buf[20];
    r->r21 = buf[21];
    r->r22 = buf[22];
    r->r23 = buf[23];
    r->r24 = buf[24];
    r->r25 = buf[25];
    r->r26 = buf[26];
    r->r27 = buf[27];
    r->r28 = buf[28];
    r->r29 = buf[29];
    // TODO: r30/r31?
    /* r->r30 = buf[30]; */
    /* r->r31 = buf[31]; */

    r->sreg = io[2];
    r->sp = io[1] | io[0]<<8;

    r->pc = PC*2;

    return 0;
}

int target_step(void)
{
    DwTrace();
    return 0;
}

int target_write_addr(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (addr < 0x800000) {
        WriteFlash(addr, buf, len);
    } else {
        DwWriteAddr(addr, len, buf);
    }

    return 0;
}

int target_read_addr(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (addr < 0x800000) {
        DwReadFlash(addr, len, buf);
    } else {
        DwReadAddr(addr, len, buf);
    }

    return 0;
}

int target_continue(int fd)
{
    DwGo();

    fd_set readfds;
    fd_set expfds;
    struct timeval timeout;
    while (1) {
      FD_ZERO(&readfds);
      FD_ZERO(&expfds);
      FD_SET(fd, &readfds);
      FD_SET(SerialPort, &readfds);
      FD_SET(fd, &expfds);
      timeout = (struct timeval){10,0}; // 10 seconds
      if(select(max(SerialPort, fd)+1, &readfds, 0, &expfds, &timeout) > 0) {
        if (FD_ISSET(SerialPort, &readfds)) {DeviceBreak();   break;}
        if (FD_ISSET(fd, &readfds) || FD_ISSET(fd, &expfds)) {KeyboardBreak();   break;}
        else {printf("WTF?");}
      } else {
        Ws("."); Flush();
      }
    }
    return 0;
}

int target_reset(void)
{
    ConnectSerialPort();
    DwReset();
    return 0;
}

int target_set_breakpoint(uint16_t addr)
{
    printf("BP %x\n", addr);
    BP = addr/2;

    return 0;
}

int target_clear_breakpoint()
{
    BP=-1;
    DwWrite(ByteArrayLiteral(
      0x60, 0xD0, hi(PC), lo(PC),    // Address to restart execution at
      0x30                          // Continue execution (go)
    ));

    return 0;
}
