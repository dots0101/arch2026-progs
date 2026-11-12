/* Host-only exhaustive oracle. The BFS model below comes directly from the
 * published R/B/D source and twist tables, independently of the H48 data.
 * Including the solver lets this program inspect its private B8 predicate;
 * no full-state distance table is linked into a submitted target program.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/rubik/h48_solver.c"
#include "../src/rubik/rubik.h"

enum { OPERM = 5040, OORI = 729, OSTATES = OPERM * OORI, TOP = 40 };
struct ocube {
    uint8_t p[7], o[7];
};
static const uint8_t osource[3][7] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t otwist[3][7] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};
static const char *const onames[9] = {
    "R", "R2", "R'", "B", "B2", "B'", "D", "D2", "D'",
};
static const uint8_t omap[7] = {3, 1, 5, 2, 0, 4, 6};
static const uint8_t omove_to_old[9] = {2, 1, 0, 8, 7, 6, 5, 4, 3};
static const unsigned factorial[7] = {1, 1, 2, 6, 24, 120, 720};
static uint16_t pnext[OPERM][9], onext[OORI][9];
static uint32_t histogram[12];

static double now(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) {
        perror("clock_gettime");
        exit(1);
    }
    return (double) t.tv_sec + 1e-9 * (double) t.tv_nsec;
}

static void fail(const char *what, unsigned state)
{
    fprintf(stderr, "%s at oracle state %u\n", what, state);
    exit(1);
}

static void pdecode(unsigned rank, uint8_t p[7])
{
    uint8_t left[7] = {0, 1, 2, 3, 4, 5, 6};
    for (unsigned i = 0; i < 7; i++) {
        unsigned at = rank / factorial[6 - i];
        rank %= factorial[6 - i];
        p[i] = left[at];
        for (unsigned j = at; j + 1 < 7 - i; j++)
            left[j] = left[j + 1];
    }
}

static unsigned prank(const uint8_t p[7])
{
    unsigned rank = 0;
    for (unsigned i = 0; i < 6; i++) {
        unsigned less = 0;
        for (unsigned j = i + 1; j < 7; j++)
            less += p[j] < p[i];
        rank += less * factorial[6 - i];
    }
    return rank;
}

static void odecode(unsigned rank, uint8_t o[7])
{
    unsigned sum = 0;
    for (unsigned i = 6; i-- > 0;) {
        o[i] = rank % 3;
        sum += o[i];
        rank /= 3;
    }
    o[6] = (3 - sum % 3) % 3;
}

static unsigned orank(const uint8_t o[7])
{
    unsigned rank = 0;
    for (unsigned i = 0; i < 6; i++)
        rank = 3 * rank + o[i];
    return rank;
}

static struct ocube decode(unsigned state)
{
    struct ocube c;
    pdecode(state / OORI, c.p);
    odecode(state % OORI, c.o);
    return c;
}

static unsigned rank_cube(const struct ocube *c)
{
    return prank(c->p) * OORI + orank(c->o);
}

static struct ocube omove(struct ocube c, unsigned m)
{
    unsigned face = m / 3;
    for (unsigned n = 0; n <= m % 3; n++) {
        struct ocube next;
        for (unsigned i = 0; i < 7; i++) {
            unsigned from = osource[face][i];
            next.p[i] = c.p[from];
            next.o[i] = (c.o[from] + otwist[face][i]) % 3;
        }
        c = next;
    }
    return c;
}

static WorkState mapped(const struct ocube *c)
{
    WorkState w;
    for (unsigned i = 0; i < 7; i++) {
        w.p[omap[i]] = omap[c->p[i]];
        w.o[omap[i]] = c->o[i];
    }
    w.p[7] = 7;
    w.o[7] = 0;
    return w;
}

static void check_conjugacy(struct ocube c, unsigned state)
{
    WorkState w = mapped(&c);
    for (unsigned m = 0; m < 9; m++) {
        struct ocube nc = omove(c, m);
        WorkState expected = mapped(&nc), got;
        state_move(&w, omove_to_old[m], &got);
        if (memcmp(&expected, &got, sizeof got))
            fail("coordinate conjugacy mismatch", state);
    }
}

static void transitions(void)
{
    for (unsigned p = 0; p < OPERM; p++) {
        struct ocube c = decode(p * OORI);
        if (prank(c.p) != p)
            fail("permutation rank/decode mismatch", p * OORI);
        check_conjugacy(c, p * OORI);
        for (unsigned m = 0; m < 9; m++) {
            struct ocube n = omove(c, m);
            pnext[p][m] = prank(n.p);
        }
    }
    for (unsigned o = 0; o < OORI; o++) {
        struct ocube c = decode(o);
        if (orank(c.o) != o)
            fail("orientation rank/decode mismatch", o);
        check_conjugacy(c, o);
        for (unsigned m = 0; m < 9; m++) {
            struct ocube n = omove(c, m);
            onext[o][m] = orank(n.o);
        }
    }
    for (unsigned m = 0; m < 9; m++) {
        unsigned inverse = 3 * (m / 3) + 2 - m % 3;
        for (unsigned p = 0; p < OPERM; p++) {
            if (pnext[pnext[p][m]][inverse] != p)
                fail("permutation inverse mismatch", p * OORI);
        }
        for (unsigned o = 0; o < OORI; o++) {
            if (onext[onext[o][m]][inverse] != o)
                fail("orientation inverse mismatch", o);
        }
    }
}

static uint8_t *bfs(void)
{
    uint8_t *dist = malloc(OSTATES);
    uint32_t *queue = malloc(sizeof *queue * OSTATES);
    if (!dist || !queue) {
        perror("malloc");
        exit(1);
    }
    memset(dist, 255, OSTATES);
    dist[0] = 0;
    queue[0] = 0;
    unsigned head = 0, tail = 1;
    while (head < tail) {
        unsigned s = queue[head++], p = s / OORI, o = s % OORI;
        for (unsigned m = 0; m < 9; m++) {
            unsigned n = pnext[p][m] * OORI + onext[o][m];
            if (dist[n] == 255) {
                dist[n] = dist[s] + 1;
                if (tail == OSTATES)
                    fail("BFS queue overflow", n);
                queue[tail++] = n;
            }
        }
    }
    if (tail != OSTATES)
        fail("BFS did not reach every legal state", tail);
    for (unsigned s = 0; s < OSTATES; s++) {
        if (dist[s] >= 12)
            fail("diameter exceeds 11", s);
        histogram[dist[s]]++;
    }
    free(queue);
    return dist;
}

static void state_text(const struct ocube *c, char text[15])
{
    for (unsigned i = 0; i < 7; i++) {
        text[i] = '1' + c->p[i];
        text[i + 7] = '1' + c->o[i];
    }
    text[14] = '\0';
}

static FILE *output(const char *directory, const char *name, const char *mode)
{
    char path[4096];
    int n = snprintf(path, sizeof path, "%s/%s", directory, name);
    if (n < 0 || (size_t) n >= sizeof path) {
        fprintf(stderr, "output path too long\n");
        exit(1);
    }
    FILE *f = fopen(path, mode);
    if (!f) {
        perror(path);
        exit(1);
    }
    return f;
}

static unsigned replay(struct ocube c,
                       const char *text,
                       const uint8_t *dist,
                       unsigned wanted)
{
    unsigned count = 0;
    while (*text) {
        const char *end = text;
        while (*end && *end != ' ')
            end++;
        unsigned m;
        for (m = 0; m < 9; m++) {
            if ((size_t) (end - text) == strlen(onames[m]) &&
                !memcmp(text, onames[m], (size_t) (end - text)))
                break;
        }
        if (m == 9 || count >= wanted)
            fail("invalid solution token/count", rank_cube(&c));
        c = omove(c, m);
        count++;
        if (dist[rank_cube(&c)] != wanted - count)
            fail("solution does not descend exact distance", rank_cube(&c));
        if (*end && !end[1])
            fail("solution has trailing space", rank_cube(&c));
        text = *end ? end + 1 : end;
    }
    if (count != wanted || rank_cube(&c) != 0)
        fail("solution is not shortest and solved", rank_cube(&c));
    return count;
}

struct hard {
    unsigned state, search, total;
};
static struct hard hardest[TOP];

static void retain(struct hard v)
{
    for (unsigned i = 0; i < TOP; i++) {
        if (v.total > hardest[i].total ||
            (v.total == hardest[i].total && v.search > hardest[i].search)) {
            for (unsigned j = TOP - 1; j > i; j--)
                hardest[j] = hardest[j - 1];
            hardest[i] = v;
            return;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 2;
    }
    double begin = now();
    transitions();
    uint8_t *dist = bfs();
    double after_bfs = now();
    printf("BFS: %u states, histogram", (unsigned) OSTATES);
    for (unsigned d = 0; d < 12; d++)
        printf(" %u", histogram[d]);
    printf("\n");
    fflush(stdout);
    FILE *df = output(argv[1], "rubik_oracle_dist.bin", "wb");
    if (fwrite(dist, 1, OSTATES, df) != OSTATES || fclose(df)) {
        perror("distance output");
        return 1;
    }
    unsigned inside = 0, outside = 0;
    for (unsigned p = 0; p < OPERM; p++) {
        struct ocube c;
        pdecode(p, c.p);
        for (unsigned o = 0; o < OORI; o++) {
            unsigned s = p * OORI + o;
            odecode(o, c.o);
            WorkState w = mapped(&c);
            uint8_t got = 255;
            int found = b8_probe(&w, &got);
            if (found != (dist[s] <= 8) || (found && got != dist[s])) {
                char text[15];
                state_text(&c, text);
                fprintf(stderr, "%s: oracle %u, B8 found %d distance %u\n",
                        text, dist[s], found, got);
                fail("B8 exact-radius predicate mismatch", s);
            }
            if (found)
                inside++;
            else
                outside++;
        }
        if ((p + 1) % 720 == 0) {
            printf("B8: %u / %u states checked\n", (p + 1) * OORI,
                   (unsigned) OSTATES);
            fflush(stdout);
        }
    }
    double after_b8 = now();
    FILE *f = output(argv[1], "rubik_d11.csv", "w");
    fprintf(
        f,
        "state,oracle_index,distance,search_probes,total_b8_probes,solution\n");
    unsigned d11 = 0, max_search = 0, max_total = 0;
    unsigned max_search_state = 0, max_total_state = 0;
    for (unsigned s = 0; s < OSTATES; s++) {
        if (dist[s] != 11)
            continue;
        struct ocube c = decode(s);
        char text[15], solution[96];
        state_text(&c, text);
        memset(solution, '#', sizeof solution);
        int n = rubik_solve(text, solution, sizeof solution);
        if (n != 11 || !memchr(solution, 0, sizeof solution))
            fail("public rubik_solve failed distance-11 state", s);
        replay(c, solution, dist, 11);
        WorkState w = mapped(&c);
        N2State state;
        compress_public(&w, &state);
        uint8_t moves[11], count = 0;
        N2Stats stats = {0, 0, 0};
        if (!n2_solve(&state, moves, &count, &stats) || count != 11 ||
            stats.prefix_depth != 3 || stats.solution_depth != 11)
            fail("solver prefix/statistics mismatch", s);
        unsigned total = stats.search_nodes;
        for (unsigned k = stats.prefix_depth; k < count; k++)
            total += moves[k] + 1;
        if (stats.search_nodes > max_search) {
            max_search = stats.search_nodes;
            max_search_state = s;
        }
        if (total > max_total) {
            max_total = total;
            max_total_state = s;
        }
        retain((struct hard) {s, stats.search_nodes, total});
        fprintf(f, "%s,%u,11,%u,%u,%s\n", text, s, stats.search_nodes, total,
                solution);
        d11++;
    }
    if (fclose(f)) {
        perror("d11 output");
        return 1;
    }
    if (d11 != 2644 || d11 != histogram[11])
        fail("unexpected number of distance-11 states", d11);
    f = output(argv[1], "rubik_worst_probe_states.csv", "w");
    fprintf(f, "state,oracle_index,search_probes,total_b8_probes\n");
    for (unsigned i = 0; i < TOP; i++) {
        struct ocube c = decode(hardest[i].state);
        char text[15];
        state_text(&c, text);
        fprintf(f, "%s,%u,%u,%u\n", text, hardest[i].state, hardest[i].search,
                hardest[i].total);
    }
    fclose(f);
    double end = now();
    struct ocube worst_s = decode(max_search_state),
                 worst_t = decode(max_total_state);
    char worst_search[15], worst_total[15];
    state_text(&worst_s, worst_search);
    state_text(&worst_t, worst_total);
    f = output(argv[1], "rubik_oracle_summary.json", "w");
    fprintf(f, "{\n  \"states\": %u,\n  \"histogram\": [", (unsigned) OSTATES);
    for (unsigned d = 0; d < 12; d++)
        fprintf(f, "%s%u", d ? ", " : "", histogram[d]);
    fprintf(f,
            "],\n  \"b8_exact_matches\": %u,\n  \"b8_inside\": %u,\n"
            "  \"b8_outside\": %u,\n  \"d11_solutions_replayed\": %u,\n"
            "  \"max_search_probes\": %u,\n  \"max_search_state\": \"%s\",\n"
            "  \"max_total_b8_probes\": %u,\n  \"max_total_state\": \"%s\",\n"
            "  \"transition_and_bfs_seconds\": %.6f,\n"
            "  \"all_state_b8_seconds\": %.6f,\n"
            "  \"d11_solve_and_replay_seconds\": %.6f,\n"
            "  \"total_seconds\": %.6f,\n  \"mismatches\": 0\n}\n",
            (unsigned) OSTATES, inside, outside, d11, max_search, worst_search,
            max_total, worst_total, after_bfs - begin, after_b8 - after_bfs,
            end - after_b8, end - begin);
    fclose(f);
    printf(
        "PASS: B8 %u exact checks; %u distance-11 shortest solutions "
        "replayed\n",
        (unsigned) OSTATES, d11);
    printf("max search %u at %s; max total probes %u at %s\n", max_search,
           worst_search, max_total, worst_total);
    printf("elapsed: BFS %.3f s, B8 %.3f s, d11 %.3f s, total %.3f s\n",
           after_bfs - begin, after_b8 - after_bfs, end - after_b8,
           end - begin);
    free(dist);
    return 0;
}
