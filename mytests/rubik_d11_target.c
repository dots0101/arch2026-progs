/* Every distance-11 state from the independent full-state BFS is checked on
 * RV32I. This is a test harness, never part of the solver implementation.
 */
#include <stdint.h>
#include "print.h"
#include "src/rubik/rubik.h"
#include "tperf.h"

static const char states[][15] = {
#include "rubik_d11_cases.inc"
};
static const uint8_t sources[3][7] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twists[3][7] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};
struct cube {
    uint8_t p[7], o[7];
};

static int replay(const char *state, const char *solution, unsigned cap)
{
    struct cube c;
    for (unsigned i = 0; i < 7; i++) {
        c.p[i] = state[i] - '1';
        c.o[i] = state[i + 7] - '1';
    }
    unsigned at = 0, moves = 0;
    while (at < cap && solution[at]) {
        unsigned face;
        char f = solution[at++];
        if (f == 'R')
            face = 0;
        else if (f == 'B')
            face = 1;
        else if (f == 'D')
            face = 2;
        else
            return 0;
        unsigned turns = 1;
        if (at >= cap)
            return 0;
        if (solution[at] == '2') {
            turns = 2;
            at++;
        } else if (solution[at] == '\'') {
            turns = 3;
            at++;
        }
        if (at >= cap)
            return 0;
        if (solution[at]) {
            if (solution[at] != ' ' || at + 1 >= cap || !solution[at + 1])
                return 0;
            at++;
        }
        for (unsigned t = 0; t < turns; t++) {
            struct cube next;
            for (unsigned i = 0; i < 7; i++) {
                unsigned from = sources[face][i];
                next.p[i] = c.p[from];
                unsigned o = c.o[from] + twists[face][i];
                next.o[i] = o >= 3 ? o - 3 : o;
            }
            c = next;
        }
        if (++moves > 11)
            return 0;
    }
    if (at >= cap || moves != 11)
        return 0;
    for (unsigned i = 0; i < 7; i++) {
        if (c.p[i] != i || c.o[i])
            return 0;
    }
    return 1;
}

int main(void)
{
    const unsigned count = sizeof states / sizeof states[0];
    uint64_t worst = 0, sum = 0;
    unsigned worst_index = 0;
    uint32_t checksum = 0;
    if (count != 2644) {
        bm_printf("FAIL wrong d11 count %u\n", count);
        return 1;
    }
    bm_printf("D11 target: %u independent BFS states\n", count);
    for (unsigned i = 0; i < count; i++) {
        char out[96];
        for (unsigned k = 0; k < sizeof out; k++)
            out[k] = '#';
        uint64_t used;
        int n;
        PERF_CALL(used, n = rubik_solve_asm(states[i], out, sizeof out));
        if (n != 11 || !replay(states[i], out, sizeof out)) {
            bm_printf("FAIL case=%u state=%s returned=%d\n", i, states[i], n);
            return 1;
        }
        if (used > RUBIK_INSN_BUDGET) {
            bm_printf("FAIL case=%u state=%s instructions=%llu budget=%u\n", i,
                      states[i], used, RUBIK_INSN_BUDGET);
            return 1;
        }
        sum += used;
        if (used > worst) {
            worst = used;
            worst_index = i;
            bm_printf("new maximum: case=%u state=%s instructions=%llu\n", i,
                      states[i], used);
        }
        for (unsigned k = 0; k < 14; k++)
            checksum =
                (checksum << 5) ^ (checksum >> 27) ^ (uint8_t) states[i][k];
        checksum ^= (uint32_t) used;
        if ((i + 1) % 256 == 0)
            bm_printf("progress %u/%u\n", i + 1, count);
    }
    bm_printf(
        "@@d11 cases=%u failed=0 worst=%llu budget=%u state=%s total=%llu "
        "checksum=%u\n",
        count, worst, RUBIK_INSN_BUDGET, states[worst_index], sum, checksum);
    return 0;
}
