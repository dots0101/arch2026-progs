/* Host-only numerical error analysis. The submission implementations and
 * official harness remain unchanged. Build with the C implementation and -lm.
 * The reference decodes the exact input value with long double and uses sqrtl.
 */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/bf16/bf16.h"

struct witness {
    uint32_t a, b;
    uint16_t result;
    long double reference, actual, absolute, relative;
};

struct metrics {
    uint32_t count, exact, below, above;
    long double min_signed_relative, max_signed_relative;
    struct witness max_absolute, max_relative;
};

static long double value16(uint16_t x)
{
    unsigned exponent = (x >> 7) & 255u, fraction = x & 127u;
    long double value;
    if (exponent == 255)
        value = fraction ? NAN : INFINITY;
    else if (exponent)
        value = ldexpl(128u + fraction, (int) exponent - 134);
    else
        value = ldexpl(fraction, -133);
    return x & 0x8000u ? -value : value;
}

static long double value32(uint32_t x)
{
    unsigned exponent = (x >> 23) & 255u, fraction = x & 0x7FFFFFu;
    long double value;
    if (exponent == 255)
        value = fraction ? NAN : INFINITY;
    else if (exponent)
        value = ldexpl(0x800000u + fraction, (int) exponent - 150);
    else
        value = ldexpl(fraction, -149);
    return x & 0x80000000u ? -value : value;
}

static uint16_t reference_narrow(uint32_t x)
{
    uint32_t upper = x >> 16, lower = x & 65535u;
    if (((x >> 23) & 255u) != 255u &&
        (lower > 32768u || (lower == 32768u && (upper & 1u))))
        ++upper;
    return (uint16_t) upper;
}

/* The selected products are exact binary32 values: both operands have only
 * eight significand bits, and these scales stay above binary32 underflow.
 */
static uint32_t exact_product_bits(long double x)
{
    float f = (float) x;
    uint32_t bits;
    memcpy(&bits, &f, sizeof bits);
    return bits;
}

static void add_metric(struct metrics *m,
                       uint32_t a,
                       uint32_t b,
                       uint16_t result,
                       long double reference)
{
    long double actual = value16(result);
    long double error = actual - reference;
    long double absolute = fabsl(error);
    long double relative = absolute / fabsl(reference);
    long double signed_relative = error / fabsl(reference);
    struct witness w = {a, b, result, reference, actual, absolute, relative};
    if (!m->count || absolute > m->max_absolute.absolute)
        m->max_absolute = w;
    if (!m->count || relative > m->max_relative.relative)
        m->max_relative = w;
    if (!m->count || signed_relative < m->min_signed_relative)
        m->min_signed_relative = signed_relative;
    if (!m->count || signed_relative > m->max_signed_relative)
        m->max_signed_relative = signed_relative;
    ++m->count;
    m->exact += error == 0;
    m->below += error < 0;
    m->above += error > 0;
}

static void print_witness(const struct witness *w)
{
    printf(
        "{\"input_a\":\"0x%08x\",\"input_b\":\"0x%08x\","
        "\"result\":\"0x%04x\",\"reference_value\":%.21Lg,"
        "\"actual_value\":%.21Lg,\"absolute_error\":%.21Lg,"
        "\"relative_error\":%.21Lg}",
        w->a, w->b, w->result, w->reference, w->actual, w->absolute,
        w->relative);
}

static void print_metrics(const struct metrics *m)
{
    printf(
        "{\"count\":%u,\"exact\":%u,\"below_reference\":%u,"
        "\"above_reference\":%u,\"min_signed_relative_error\":%.21Lg,"
        "\"max_signed_relative_error\":%.21Lg,\"max_absolute\":",
        m->count, m->exact, m->below, m->above, m->min_signed_relative,
        m->max_signed_relative);
    print_witness(&m->max_absolute);
    printf(",\"max_relative\":");
    print_witness(&m->max_relative);
    putchar('}');
}

static unsigned analyze_sqrt(void)
{
    struct metrics normals = {0}, subnormals = {0}, all_positive = {0};
    uint32_t positive_normals = 0, negative_normals = 0;
    uint32_t positive_subnormals = 0, negative_subnormals = 0;
    uint32_t positive_zero = 0, negative_zero = 0, infinities = 0, nans = 0;
    uint32_t special_mismatches = 0;
    for (uint32_t x = 0; x < 65536u; ++x) {
        uint16_t result = bf16_sqrt((uint16_t) x);
        unsigned exponent = (x >> 7) & 255u, fraction = x & 127u;
        unsigned negative = x >> 15;
        if (exponent == 255) {
            if (fraction) {
                ++nans;
                special_mismatches += result != x;
            } else {
                ++infinities;
                special_mismatches += result != (negative ? 0x7FC0u : x);
            }
        } else if (!exponent && !fraction) {
            negative_zero += negative;
            positive_zero += !negative;
            special_mismatches += result != 0;
        } else if (negative) {
            negative_normals += exponent != 0;
            negative_subnormals += exponent == 0;
            special_mismatches += result != 0x7FC0u;
        } else {
            long double reference = sqrtl(value16((uint16_t) x));
            if (exponent) {
                ++positive_normals;
                add_metric(&normals, x, 0, result, reference);
            } else {
                ++positive_subnormals;
                add_metric(&subnormals, x, 0, result, reference);
                special_mismatches += result != 0;
            }
            add_metric(&all_positive, x, 0, result, reference);
        }
    }
    printf(
        "\"sqrt\":{\"patterns_checked\":65536,\"input_classes\":{"
        "\"positive_normals\":%u,\"negative_normals\":%u,"
        "\"positive_subnormals\":%u,\"negative_subnormals\":%u,"
        "\"positive_zero\":%u,\"negative_zero\":%u,"
        "\"infinities\":%u,\"nans\":%u},"
        "\"special_case_mismatches\":%u,\"positive_normals\":",
        positive_normals, negative_normals, positive_subnormals,
        negative_subnormals, positive_zero, negative_zero, infinities, nans,
        special_mismatches);
    print_metrics(&normals);
    printf(",\"positive_subnormals\":");
    print_metrics(&subnormals);
    printf(",\"all_positive_finite_nonzero\":");
    print_metrics(&all_positive);
    printf("}");
    return special_mismatches;
}

static void analyze_multiply(void)
{
    static const uint16_t examples[][2] = {
        {0x0080, 0x3F00}, {0x0081, 0x3F00}, {0x00FF, 0x3F00},
        {0x0001, 0x3F80}, {0x0080, 0x3F80}, {0x7F7F, 0x4000},
    };
    struct metrics normal = {0}, boundary = {0}, bands[10] = {{0}};
    uint32_t normal_rtz_mismatches = 0, boundary_rtz_mismatches = 0;
    uint32_t boundary_rne_mismatches = 0, zeros[10] = {0};
    uint32_t subnormals[10] = {0}, normals[10] = {0};
    for (unsigned ma = 128; ma < 256; ++ma) {
        for (unsigned mb = 128; mb < 256; ++mb) {
            uint16_t a = (uint16_t) (0x3F00u + ma);
            uint16_t b = (uint16_t) (0x3F00u + mb);
            uint16_t result = bf16_mul(a, b);
            long double reference = value16(a) * value16(b);
            add_metric(&normal, a, b, result, reference);
            normal_rtz_mismatches +=
                result != (exact_product_bits(reference) >> 16);

            a = (uint16_t) ma; /* exponent 1: smallest normal binade */
            for (unsigned eb = 119; eb <= 127; ++eb) {
                b = (uint16_t) ((eb << 7) | (mb - 128));
                result = bf16_mul(a, b);
                reference = value16(a) * value16(b);
                int post_exponent = (int) eb - 126 + (ma * mb >= 32768u);
                unsigned band = (unsigned) (post_exponent + 7);
                add_metric(&boundary, a, b, result, reference);
                add_metric(&bands[band], a, b, result, reference);
                zeros[band] += result == 0;
                subnormals[band] += result > 0 && result < 128;
                normals[band] += result >= 128;
                uint32_t bits = exact_product_bits(reference);
                boundary_rtz_mismatches += result != (bits >> 16);
                boundary_rne_mismatches += result != reference_narrow(bits);
            }
        }
    }
    printf(
        "\"multiply\":{\"normal_grid_description\":"
        "\"All 128x128 normalized 8-bit significands; both input biased "
        "exponents are 127\",\"normal_grid_rtz_reference_mismatches\":%u,"
        "\"normal_grid\":",
        normal_rtz_mismatches);
    print_metrics(&normal);
    printf(
        ",\"boundary_description\":\"All 128x128 significands with "
        "input exponent a=1 and b=119..127; both inputs remain normal\","
        "\"boundary_rtz_reference_mismatches\":%u,"
        "\"boundary_rne_reference_mismatches\":%u,\"boundary\":",
        boundary_rtz_mismatches, boundary_rne_mismatches);
    print_metrics(&boundary);
    printf(",\"post_normalization_exponent_bands\":[");
    for (unsigned i = 0; i < 10; ++i) {
        if (i)
            putchar(',');
        printf(
            "{\"post_exponent\":%d,\"zero_results\":%u,"
            "\"subnormal_results\":%u,\"normal_results\":%u,\"errors\":",
            (int) i - 7, zeros[i], subnormals[i], normals[i]);
        print_metrics(&bands[i]);
        putchar('}');
    }
    printf("],\"directed_boundary_examples\":[");
    for (unsigned i = 0; i < sizeof examples / sizeof examples[0]; ++i) {
        uint16_t a = examples[i][0], b = examples[i][1];
        uint16_t result = bf16_mul(a, b);
        long double reference = value16(a) * value16(b);
        long double actual = value16(result);
        if (i)
            putchar(',');
        printf(
            "{\"a\":\"0x%04x\",\"b\":\"0x%04x\","
            "\"result\":\"0x%04x\",\"reference_value\":%.21Lg",
            a, b, result, reference);
        if (isfinite(actual)) {
            uint32_t bits = exact_product_bits(reference);
            printf(
                ",\"reference_rtz\":\"0x%04x\","
                "\"reference_rne\":\"0x%04x\","
                "\"actual_value\":%.21Lg,\"relative_error\":%.21Lg",
                bits >> 16, reference_narrow(bits), actual,
                fabsl((actual - reference) / reference));
        } else {
            printf(
                ",\"output_class\":\"infinity\","
                "\"relative_error\":null");
        }
        putchar('}');
    }
    printf("]}");
}

static unsigned analyze_narrow(void)
{
    static const uint32_t lows[] = {0, 0x7FFFu, 0x8000u, 0x8001u, 0xFFFFu};
    static const uint32_t examples[] = {
        0x00000000u, 0x80000000u, 0x00000001u, 0x00007FFFu, 0x00008000u,
        0x00008001u, 0x00010000u, 0x007FFFFFu, 0x00800000u, 0x3F807FFFu,
        0x3F808000u, 0x3F808001u, 0x3F818000u, 0xBF818000u, 0x3FFF8000u,
        0x7F7F7FFFu, 0x7F7F8000u, 0x7F7FFFFFu, 0xFF7FFFFFu, 0x7F800000u,
        0x7F800001u, 0x7FC00000u, 0xFF800001u,
    };
    struct metrics normals = {0}, tiny = {0};
    uint32_t mismatches = 0, finite_overflows = 0, special_inputs = 0;
    for (uint32_t high = 0; high < 65536u; ++high) {
        for (unsigned i = 0; i < sizeof lows / sizeof lows[0]; ++i) {
            uint32_t input = (high << 16) | lows[i];
            uint16_t result = bf16_from_f32(input);
            mismatches += result != reference_narrow(input);
            if (((input >> 23) & 255u) == 255u) {
                ++special_inputs;
                continue;
            }
            if ((result & 0x7FFFu) == 0x7F80u) {
                ++finite_overflows;
                continue;
            }
            long double reference = value32(input);
            if (!reference)
                continue;
            add_metric(fabsl(reference) >= ldexpl(1, -126) ? &normals : &tiny,
                       input, 0, result, reference);
        }
    }
    printf(
        "\"from_f32\":{\"boundary_samples_checked\":327680,"
        "\"sample_rule\":\"Every upper half with lower halves "
        "0, 0x7fff, 0x8000, 0x8001, 0xffff\","
        "\"independent_rne_oracle_mismatches\":%u,"
        "\"finite_overflow_samples\":%u,\"special_input_samples\":%u,"
        "\"normal_finite_domain_relative_error_bound\":0.00390625,"
        "\"tiny_domain_absolute_error_bound\":%.21Lg,"
        "\"finite_normal_samples_excluding_overflow\":",
        mismatches, finite_overflows, special_inputs, ldexpl(1, -134));
    print_metrics(&normals);
    printf(",\"finite_tiny_nonzero_samples\":");
    print_metrics(&tiny);
    printf(",\"representative_inputs\":[");
    for (unsigned i = 0; i < sizeof examples / sizeof examples[0]; ++i) {
        uint32_t input = examples[i];
        uint16_t result = bf16_from_f32(input);
        long double reference = value32(input), actual = value16(result);
        if (i)
            putchar(',');
        printf("{\"input\":\"0x%08x\",\"result\":\"0x%04x\"", input, result);
        if (isfinite(reference))
            printf(",\"input_value\":%.21Lg", reference);
        else
            printf(",\"input_class\":\"%s\"",
                   isnan(reference) ? "NaN" : "infinity");
        if (isfinite(actual))
            printf(",\"actual_value\":%.21Lg", actual);
        else
            printf(",\"output_class\":\"%s\"",
                   isnan(actual) ? "NaN" : "infinity");
        if (isfinite(reference) && isfinite(actual)) {
            printf(",\"absolute_error\":%.21Lg", fabsl(actual - reference));
            if (reference)
                printf(",\"relative_error\":%.21Lg",
                       fabsl((actual - reference) / reference));
        }
        putchar('}');
    }
    printf("]}");
    return mismatches;
}

int main(void)
{
    _Static_assert(sizeof(float) == sizeof(uint32_t), "binary32 host required");
    _Static_assert(FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
                   "binary32 host required");
    if (exact_product_bits(1.0L) != 0x3F800000u)
        return 2;
    printf(
        "{\"schema_version\":1,\"reference\":\"Exact bit-pattern "
        "decoding to host long double; sqrtl for square-root reference\","
        "\"long_double_mantissa_bits\":%d,",
        LDBL_MANT_DIG);
    unsigned failures = analyze_sqrt();
    putchar(',');
    analyze_multiply();
    putchar(',');
    failures += analyze_narrow();
    printf(",\"semantic_check_failures\":%u}\n", failures);
    return failures ? 1 : 0;
}
