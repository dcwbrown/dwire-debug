#include "rsp.h"

int target_write_registers(u8 *buf, int first, int last)
{
    DwWriteRegisters(buf, 0, last);

    return 0;
}

int target_read_registers(struct registers *r)
{
    u8 buf[32];
    u8 io[3];

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
    r->r30 = R30;
    r->r31 = R31;

    r->sreg = io[2];
    r->sp = io[1] | io[0]<<8;

    r->pc = PC;

    return 0;
}

int target_step(void)
{
    DwTrace();
    return 0;
}

int target_write_addr(u32 addr, u8 *buf, u16 len)
{
    if (addr < 0x800000) {
        WriteFlash(addr, buf, len);
    } else {
        DwWriteAddr(addr, len, buf);
    }

    return 0;
}

int target_read_addr(u32 addr, u8 *buf, u16 len)
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
    GoWaitLoop((FileHandle)fd);

    return 0;
}

int target_reset(void)
{
    DwReset();

    return 0;
}

int target_set_breakpoint(u16 addr)
{
    printf("BP %x\n", addr);
    BP = addr;

    return 0;
}

int target_clear_breakpoint()
{
    BP=-1;

    // DCWB: Commenting out the following.
    // I dont't understand why target_clear_breakpoint should have the side
    // effect of continuing execution. Assuming it should then we need to call
    // DwGo to get R30 and R31 restored correctly, and should call the
    // GoWaitLoop.

    //  DwWrite(ByteArrayLiteral(
    //    0x60, 0xD0, hi(PC), lo(PC),    // Address to restart execution at
    //    0x30                          // Continue execution (go)
    //  ));

    return 0;
}
