
// udiv64.c     NOTE: to get rid of "undefined reference to `__udivmoddi4'" and "__udivdi3"

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include <stdint.h>

/*
 * Freestanding 64-bit unsigned division helpers for 32-bit targets.
 * No compiler runtime library required.
 * No 64-bit division or modulo operators used internally.
 */

typedef struct
{
    uint64_t quotient;
    uint64_t remainder;

} udiv64_result_t;

uint64_t __udivdi3(uint64_t numerator, uint64_t denominator);
uint64_t __umoddi3(uint64_t numerator, uint64_t denominator);
uint64_t __udivmoddi4(uint64_t numerator, uint64_t denominator, uint64_t *remainder);

static udiv64_result_t udiv64_core(uint64_t numerator, uint64_t denominator)
{
    udiv64_result_t result = {0, 0};
    int bit;

    if (denominator == 0)
    {   // Division by zero is undefined in C. Return zero here to avoid a hardware fault in this helper.
        return result;
    }

    for (bit = 63; bit >= 0; --bit)
    {
        uint64_t carry = result.remainder >> 63;

        result.remainder = (result.remainder << 1) | ((numerator >> bit) & 1ULL);

        if (carry || result.remainder >= denominator)
        {
            result.remainder -= denominator;
            result.quotient |= (1ULL << bit);
        }
    }

    return result;
}

// GCC unsigned 64-bit division helper.
uint64_t __udivdi3(uint64_t numerator, uint64_t denominator)
{
    return udiv64_core(numerator, denominator).quotient;
}

// GCC unsigned 64-bit modulo helper.
uint64_t __umoddi3(uint64_t numerator, uint64_t denominator)
{
    return udiv64_core(numerator, denominator).remainder;
}

// GCC unsigned 64-bit division-with-remainder helper.
uint64_t __udivmoddi4(uint64_t numerator, uint64_t denominator, uint64_t *remainder)
{
    udiv64_result_t result = udiv64_core(numerator, denominator);

    if (remainder != 0)
    {
        *remainder = result.remainder;
    }

    return result.quotient;
}
