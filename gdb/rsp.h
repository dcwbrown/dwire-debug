#pragma once
#include <stdint.h>
#include <unistd.h>

ssize_t read_command(int connfd, char *buf, size_t size);
void handle_command(int connfd, const char *cmd);

struct registers {
    uint8_t r0;
    uint8_t r1;
    uint8_t r2;
    uint8_t r3;
    uint8_t r4;
    uint8_t r5;
    uint8_t r6;
    uint8_t r7;
    uint8_t r8;
    uint8_t r9;
    uint8_t r10;
    uint8_t r11;
    uint8_t r12;
    uint8_t r13;
    uint8_t r14;
    uint8_t r15;
    uint8_t r16;
    uint8_t r17;
    uint8_t r18;
    uint8_t r19;
    uint8_t r20;
    uint8_t r21;
    uint8_t r22;
    uint8_t r23;
    uint8_t r24;
    uint8_t r25;
    uint8_t r26;
    uint8_t r27;
    uint8_t r28;
    uint8_t r29;
    uint8_t r30;
    uint8_t r31;
    uint8_t sreg;
    uint16_t sp;
    uint8_t pc;
};
