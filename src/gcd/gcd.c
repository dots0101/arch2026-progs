#include "gcd.h"

/* Stein's algorithm. Removing powers of two and halving every nonzero
 * difference bounds the work by the operand width, not the operand value.
 */
uint32_t gcd_u32(uint32_t a, uint32_t b)
{
    if (!a)
        return b;
    if (!b)
        return a;

    unsigned shift = 0;
    while (!((a | b) & 1u)) {
        a >>= 1;
        b >>= 1;
        shift++;
    }
    while (!(a & 1u))
        a >>= 1;

    do {
        while (!(b & 1u))
            b >>= 1;
        if (a == 1u || b == 1u)
            return 1u << shift;
        if (a > b) {
            uint32_t t = a;
            a = b;
            b = t;
        }
        b -= a;
    } while (b);

    return a << shift;
}
