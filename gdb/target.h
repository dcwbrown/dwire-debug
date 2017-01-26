#pragma once

#include "rsp.h"

int target_write_registers(uint8_t *r, int first, int last);
int target_read_registers(struct registers *r);
int target_reset(void);
int target_step(void);
int target_read_addr(uint32_t addr, uint8_t *buf, uint16_t len);
int target_write_addr(uint32_t addr, uint8_t *buf, uint16_t len);
int target_continue(int fd);
int target_set_breakpoint(uint16_t addr);
int target_clear_breakpoint();
