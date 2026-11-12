#include "bf16.h"

/* Integer-only implementations of the contract in bf16.h. */
uint16_t bf16_from_f32(uint32_t f32bits)
{
    if ((f32bits & 0x7F800000u) != 0x7F800000u)
        f32bits += 0x7FFFu + ((f32bits >> 16) & 1u);
    return (uint16_t) (f32bits >> 16);
}

uint16_t bf16_mul(uint16_t a, uint16_t b)
{
    uint32_t sign = (a ^ b) & 0x8000u;
    int ea = (a >> 7) & 255, eb = (b >> 7) & 255;
    uint32_t ma = a & 127u, mb = b & 127u;

    /* Operand order is significant: an infinite a takes precedence over a
     * NaN b, whereas a NaN a preserves its complete original pattern.
     */
    if (ea == 255) {
        if (ma)
            return a;
        return (uint16_t) ((eb | mb) ? sign | 0x7F80u : 0x7FC0u);
    }
    if (eb == 255) {
        if (mb)
            return b;
        return (uint16_t) ((ea | ma) ? sign | 0x7F80u : 0x7FC0u);
    }

    if (ea) {
        ma |= 128u;
    } else {
        if (!ma)
            return (uint16_t) sign;
        ea = 1;
        do {
            ma <<= 1;
            --ea;
        } while (ma < 128u);
    }
    if (eb) {
        mb |= 128u;
    } else {
        if (!mb)
            return (uint16_t) sign;
        eb = 1;
        do {
            mb <<= 1;
            --eb;
        } while (mb < 128u);
    }

    int exponent = ea + eb - 127;
    uint32_t product = ma * mb;
    if (product >= 0x8000u) {
        product >>= 8;
        ++exponent;
    } else {
        product >>= 7;
    }
    if (exponent >= 255)
        return (uint16_t) (sign | 0x7F80u);
    /* This assignment shifts the stored seven-bit fraction on underflow,
     * not the implicit leading one. That behavior is intentionally distinct
     * from a correctly rounded IEEE-style subnormal multiplication.
     */
    product &= 127u;
    if (exponent <= 0) {
        if (exponent < -6)
            return (uint16_t) sign;
        return (uint16_t) (sign | (product >> (1 - exponent)));
    }
    return (uint16_t) (sign | ((uint32_t) exponent << 7) | product);
}

uint16_t bf16_sqrt(uint16_t a)
{
    uint32_t magnitude = a & 0x7FFFu;
    if (magnitude > 0x7F80u)
        return a;
    if (a & 0x8000u)
        return magnitude ? 0x7FC0u : 0;
    if (magnitude == 0x7F80u)
        return a;
    if (magnitude < 128u)
        return 0;

    uint32_t exponent = a >> 7;
    uint32_t mantissa = 128u | (a & 127u);
    if (!(exponent & 1u))
        mantissa <<= 1;

    /* floor(mid*mid/128) <= mantissa is equivalent to
     * mid*mid <= 128*mantissa+127. The restoring integer square root gives
     * that largest mid directly, without a multiply at each search step.
     * The radicand is in [0x4000,0xFFFF], so the leading step is known.
     */
    uint32_t remainder = (mantissa << 7) + 127u - 0x4000u;
    uint32_t root = 0x4000u;
    for (uint32_t bit = 0x1000u; bit; bit >>= 2) {
        uint32_t trial = root + bit;
        root >>= 1;
        if (remainder >= trial) {
            remainder -= trial;
            root += bit;
        }
    }

    /* floor((exponent-127)/2)+127, without shifting a negative integer. */
    exponent = (exponent + 127u) >> 1;
    return (uint16_t) ((exponent << 7) | (root & 127u));
}
