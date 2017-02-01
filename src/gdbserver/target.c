typedef u8 Registers[39];


int target_write_registers(Registers regs, int len)
{
    DwWriteRegisters(regs, 0, len <= 32 ? len : 32);
    if (len > 33) DwWriteAddr(0x5E, 1, regs+34); // sreg
    if (len > 34) DwWriteAddr(0x5D, 2, regs+32); // spl, sph
    if (len > 36) PC = regs[35] + 256*regs[36];

    return 0;
}

int target_read_registers(Registers regs) // retuns r0..r1, sreg, spl, sph, pcl, pc2l, pcum, pch
{
    DwReadRegisters(regs, 0, 30);
    regs[30] = R31;
    regs[31] = R31;
    DwReadAddr(0x5E, 1, regs+32); // SREG
    DwReadAddr(0x5D, 2, regs+33); // SPL, SPH
    regs[35] = PC % 256;          // PC 0..7
    regs[36] = PC / 256;          // PC 8..15
    regs[37] = 0;                 // PC 16..23
    regs[38] = 0;                 // PC 24..31
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
        DwWriteAddr(addr % 65536, len, buf);
    }

    return 0;
}

int target_read_addr(u32 addr, u8 *buf, u16 len)
{
    if (addr < 0x800000) {
        DwReadFlash(addr, len, buf);
    } else {
        DwReadAddr(addr % 65536, len, buf);
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
