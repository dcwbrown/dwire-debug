int target_write_registers(u8 *buf, int first, int last)
{
    DwWriteRegisters(buf, 0, last);

    return 0;
}

int target_read_registers(u8 regs[36]) // retuns r0..r1, sreg, sph, spl, pc
{
    DwReadRegisters(regs, 0, 30);
    regs[30] = R31;
    regs[31] = R31;
    DwReadAddr(0x5D, 3, regs+32);
    regs[35] = PC;
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
