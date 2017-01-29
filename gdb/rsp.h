#pragma once

ssize_t read_command(int connfd, char *buf, size_t size);
void handle_command(int connfd, const char *cmd);

struct registers {
    u8  r0;
    u8  r1;
    u8  r2;
    u8  r3;
    u8  r4;
    u8  r5;
    u8  r6;
    u8  r7;
    u8  r8;
    u8  r9;
    u8  r10;
    u8  r11;
    u8  r12;
    u8  r13;
    u8  r14;
    u8  r15;
    u8  r16;
    u8  r17;
    u8  r18;
    u8  r19;
    u8  r20;
    u8  r21;
    u8  r22;
    u8  r23;
    u8  r24;
    u8  r25;
    u8  r26;
    u8  r27;
    u8  r28;
    u8  r29;
    u8  r30;
    u8  r31;
    u8  sreg;
    u16 sp;
    u8  pc;
};
