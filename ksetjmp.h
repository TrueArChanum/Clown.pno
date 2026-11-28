
//  ksetjmp.h   replacement for #include <setjmp.h>

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#ifndef KSETJMP_H
#define KSETJMP_H

#include <stdint.h>

typedef struct
{
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
} k_jmp_state;

typedef k_jmp_state k_jmp_buf[1];

_Static_assert(sizeof(k_jmp_state) == 28, "k_jmp_state size");

__attribute__((returns_twice))
int ksetjmp(k_jmp_buf env);

__attribute__((noreturn))
void klongjmp(k_jmp_buf env, int value);

#endif
