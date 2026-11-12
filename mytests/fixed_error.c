/* Independent host-side numerical analysis of the actual q_mul C function.
 * Uses exact signed 64-bit multiplication/division, unavailable on RV32I.
 * The reference performs mathematical floor without signed-right-shift
 * assumptions. No target harness file is modified.
 */
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include "src/fixed/fixed.h"

static uint64_t cases, wraps, nonwraps;
static int64_t min_error = INT64_MAX, max_error = INT64_MIN;
static int32_t min_a, min_b, max_a, max_b;
static uint64_t max_wrapped_abs_error;
static int32_t wrap_a, wrap_b, wrap_result;

static uint32_t xs32(uint32_t x)
{
    x ^= x << 13;
    x ^= x >> 17;
    return x ^ (x << 5);
}

static int check(int32_t a, int32_t b)
{
    const int64_t scale = INT64_C(1) << Q;
    int64_t product = (int64_t) a * b;
    int64_t rounded = product + scale / 2;
    int64_t exact = rounded / scale;
    if (rounded < 0 && rounded % scale)
        exact--;
    int32_t got = q_mul(a, b);
    cases++;
    if ((uint32_t) got != (uint32_t) exact) {
        fprintf(stderr, "q_mul mismatch for (%" PRId32 ",%" PRId32 ")\n", a, b);
        return 1;
    }
    int64_t delta = (int64_t) got * scale - product;
    if (exact < INT32_MIN || exact > INT32_MAX) {
        wraps++;
        uint64_t magnitude = delta < 0 ? (uint64_t) (-delta) : (uint64_t) delta;
        if (magnitude > max_wrapped_abs_error) {
            max_wrapped_abs_error = magnitude;
            wrap_a = a;
            wrap_b = b;
            wrap_result = got;
        }
    } else {
        nonwraps++;
        if (delta < -(scale / 2 - 1) || delta > scale / 2) {
            fprintf(stderr, "nonwrapping error bound violated\n");
            return 1;
        }
        if (delta < min_error) {
            min_error = delta;
            min_a = a;
            min_b = b;
        }
        if (delta > max_error) {
            max_error = delta;
            max_a = a;
            max_b = b;
        }
    }
    return 0;
}

int main(void)
{
    for (int32_t r = 0; r < (1 << Q); r++) {
        if (check(1, r) || check(-1, r))
            return 1;
    }
    static const int32_t edge[] = {
        INT32_MIN,
        INT32_MIN + 1,
        INT32_MAX,
        INT32_MAX - 1,
        -(1 << Q),
        -((1 << Q) - 1),
        -(1 << (Q - 1)),
        -3,
        -1,
        0,
        1,
        3,
        1 << (Q - 1),
        (1 << Q) - 1,
        1 << Q,
        2 << Q,
    };
    for (unsigned i = 0; i < sizeof edge / sizeof edge[0]; i++) {
        for (unsigned j = 0; j < sizeof edge / sizeof edge[0]; j++) {
            if (check(edge[i], edge[j]))
                return 1;
        }
    }
    uint32_t state = UINT32_C(0x6d2b79f5);
    for (unsigned i = 0; i < 65536; i++) {
        state = xs32(state);
        int32_t a = (int32_t) state;
        state = xs32(state);
        int32_t b = (int32_t) state;
        if (check(a, b) || check(a / 256, b / 256))
            return 1;
    }
    printf("{\n  \"cases\": %" PRIu64 ",\n  \"nonwrapping_cases\": %" PRIu64
           ",\n  \"wrapping_cases\": %" PRIu64
           ",\n  \"mismatches\": 0,\n"
           "  \"error_denominator\": 1099511627776,\n"
           "  \"min_nonwrapping_error_numerator\": %" PRId64
           ",\n"
           "  \"min_error_at\": [%" PRId32 ", %" PRId32
           "],\n"
           "  \"max_nonwrapping_error_numerator\": %" PRId64
           ",\n"
           "  \"max_error_at\": [%" PRId32 ", %" PRId32
           "],\n"
           "  \"max_wrapping_abs_error_numerator\": %" PRIu64
           ",\n"
           "  \"max_wrapping_error_at\": [%" PRId32 ", %" PRId32
           "],\n"
           "  \"max_wrapping_error_result\": %" PRId32 "\n}\n",
           cases, nonwraps, wraps, min_error, min_a, min_b, max_error, max_a,
           max_b, max_wrapped_abs_error, wrap_a, wrap_b, wrap_result);
    return 0;
}
