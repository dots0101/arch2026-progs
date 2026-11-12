#include "fixed.h"

/* The observable pass count requires the actual carry-propagation loop. */
uint32_t op1(uint32_t a, uint32_t b, int *n)
{
    int passes = 0;
    while (b) {
        uint32_t carry = (a & b) << 1;
        a ^= b;
        b = carry;
        passes++;
    }
    *n = passes;
    return a;
}

uint32_t op2(uint32_t a, uint32_t b)
{
    return a - b;
}

uint32_t op3(uint32_t a, uint32_t b)
{
    return (a & b) + ((a ^ b) >> 1);
}

int32_t q_mul(int32_t a, int32_t b)
{
    uint32_t ua = (uint32_t) a, ub = (uint32_t) b;
    uint32_t lo = 1u << (Q - 1), hi = 0;
    uint32_t term_lo = ua, term_hi = 0;

    /* Compute the unsigned product plus the half-unit rounding bias.
     * Explicit words are necessary here: GCC -Os recognizes the equivalent
     * uint64_t shift/add loop as multiplication and calls __muldi3.
     */
    for (uint32_t bits = ub; bits; bits >>= 1) {
        if (bits & 1u) {
            uint32_t sum = lo + term_lo;
            hi += term_hi + (sum < lo);
            lo = sum;
        }
        term_hi = (term_hi << 1) | (term_lo >> 31);
        term_lo <<= 1;
    }

    /* A signed 32-bit operand equals its unsigned representation minus
     * 2^32 when negative. The cross terms affect only the high word;
     * the 2^64 term vanishes modulo 2^64.
     */
    if (a < 0)
        hi -= ub;
    if (b < 0)
        hi -= ua;

    /* Only bits 20..51 survive the specified wrapping result. Thus this
     * unsigned shift is also valid for negative signed products.
     */
    return (int32_t) ((lo >> Q) | (hi << (32 - Q)));
}
