#pragma once

#include "rsp.h"

int target_write_registers(u8 *r, int first, int last);
int target_read_registers(struct registers *r);
int target_reset(void);
int target_step(void);
int target_read_addr(u32 addr, u8 *buf, u16 len);
int target_write_addr(u32 addr, u8 *buf, u16 len);
int target_continue(int fd);
int target_set_breakpoint(u16 addr);
int target_clear_breakpoint();
