#include "rubik.h"
#include "n2_solver.h"

/* Conjugacy of the published corner conventions. Twist coordinates agree. */
static const uint8_t new_to_old[7] = {3, 1, 5, 2, 0, 4, 6};
static const char names[9][3] = {"R'", "R2", "R",  "D'", "D2",
                                 "D",  "B'", "B2", "B"};

int rubik_solve(const char *state, char *out, size_t cap)
{
    N2State cube;
    uint8_t moves[N2_MAX_DEPTH], count;
    uint8_t orientations[7];
    unsigned seen = 0, sum = 0;
    if (!state)
        return -1;
    unsigned length = 0;
    while (length < 15 && state[length])
        length++;
    if (length != 14)
        return -1;
    for (unsigned i = 0; i < 7; i++) {
        unsigned p = (unsigned char) state[i] - (unsigned) '1';
        unsigned o = (unsigned char) state[i + 7] - (unsigned) '1';
        if (p >= 7 || o >= 3 || (seen & (1u << p)))
            return -1;
        seen |= 1u << p;
        cube.perm[new_to_old[i]] = new_to_old[p];
        orientations[new_to_old[i]] = (uint8_t) o;
        sum += o;
    }
    while (sum >= 3)
        sum -= 3;
    if (sum)
        return -1;
    for (unsigned i = 0; i < 6; i++)
        cube.twist[i] = orientations[i];
    if (!n2_solve(&cube, moves, &count, 0))
        return -1;
    size_t required = count ? 2u * count : 1u;
    for (unsigned i = 0; i < count; i++)
        required += names[moves[i]][1] != 0;
    if (cap < required) {
        if (cap)
            out[0] = '\0';
        return -2;
    }
    size_t k = 0;
    for (unsigned i = 0; i < count; i++) {
        out[k++] = names[moves[i]][0];
        if (names[moves[i]][1])
            out[k++] = names[moves[i]][1];
        if (i + 1 < count)
            out[k++] = ' ';
    }
    out[k] = '\0';
    return count;
}
