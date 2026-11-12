#include "h48_b8_dist_table.h"
#include "h48_exc_direct.h"
#include "h48_s3_base.h"
#include "h48_t_simple_data.h"
#include "n2_core_data.h"
#include "n2_solver.h"

#include <limits.h>

typedef struct {
    uint8_t p[8];
    uint8_t o[8];
} WorkState;

static uint16_t rank_mul(uint16_t x, uint8_t m)
{
    switch (m) {
    case 2:
        return (uint16_t) (x << 1);
    case 3:
        return (uint16_t) ((x << 1) + x);
    case 4:
        return (uint16_t) (x << 2);
    case 5:
        return (uint16_t) ((x << 2) + x);
    case 6:
        return (uint16_t) ((x << 2) + (x << 1));
    case 7:
        return (uint16_t) ((x << 3) - x);
    default:
        return x;
    }
}

static const uint8_t mod3_lut[13] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0};
static uint8_t mod3_small(unsigned x)
{
    return mod3_lut[x];
}
static void work_copy(const WorkState *s, WorkState *d)
{
    unsigned i;
    for (i = 0; i < 8u; ++i) {
        d->p[i] = s->p[i];
        d->o[i] = s->o[i];
    }
}


static int expand_public(const N2State *in, WorkState *out)
{
    uint8_t seen = 0;
    unsigned i, sum = 0;
    if (!in || !out)
        return 0;
    for (i = 0; i < 7; ++i) {
        uint8_t p = in->perm[i];
        if (p >= 7u || (seen & (uint8_t) (1u << p)))
            return 0;
        seen = (uint8_t) (seen | (uint8_t) (1u << p));
        out->p[i] = p;
    }
    if (seen != 0x7fu)
        return 0;
    for (i = 0; i < 6; ++i) {
        uint8_t o = in->twist[i];
        if (o >= 3u)
            return 0;
        out->o[i] = o;
        sum += o;
    }
    {
        uint8_t r = mod3_small(sum);
        out->o[6] = (uint8_t) (r == 0u ? 0u : 3u - r);
    }
    out->p[7] = 7;
    out->o[7] = 0;
    return 1;
}
static void compress_public(const WorkState *in, N2State *out)
{
    unsigned i;
    for (i = 0; i < 7; ++i)
        out->perm[i] = in->p[i];
    for (i = 0; i < 6; ++i)
        out->twist[i] = in->o[i];
}
int n2_state_valid(const N2State *state)
{
    WorkState w;
    return expand_public(state, &w);
}
int n2_is_solved(const N2State *state)
{
    unsigned i;
    if (!state)
        return 0;
    for (i = 0; i < 7; ++i)
        if (state->perm[i] != i)
            return 0;
    for (i = 0; i < 6; ++i)
        if (state->twist[i] != 0u)
            return 0;
    return 1;
}

static const uint8_t pop7[128] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3,
    3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4,
    3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2,
    2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5,
    5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7};
static uint16_t perm_rank7(const uint8_t p[8])
{
    uint16_t r = 0;
    uint8_t used = 0;
    unsigned i;
    for (i = 0; i < 6u; ++i) {
        uint8_t v = p[i], below = (uint8_t) ((1u << v) - 1u),
                less = (uint8_t) (v - pop7[used & below]);
        r = (uint16_t) (rank_mul(r, (uint8_t) (7u - i)) + less);
        used = (uint8_t) (used | (uint8_t) (1u << v));
    }
    return r;
}
static uint16_t ori_rank_after_sym(const WorkState *s, uint8_t sym)
{
    uint16_t r = 0;
    uint8_t inv = n2_b8_sym_inv[sym];
    unsigned np;
    for (np = 0; np < 6u; ++np) {
        uint8_t pos = n2_b8_sym_pos[(unsigned) inv * 8u + np],
                piece = s->p[pos];
        unsigned ti =
            ((((unsigned) sym * 8u + pos) * 8u + piece) * 3u) + s->o[pos];
        uint8_t o = n2_b8_sym_twist[ti];
        r = (uint16_t) ((r << 1) + r + o);
    }
    return r;
}
static void state_move(const WorkState *s, uint8_t mi, WorkState *d)
{
    unsigned i;
    for (i = 0; i < 8; ++i) {
        uint8_t q = n2_b8_move_src[(unsigned) mi * 8u + i], v;
        d->p[i] = s->p[q];
        v = (uint8_t) (s->o[q] + n2_b8_move_delta[(unsigned) mi * 8u + i]);
        d->o[i] = (uint8_t) (v >= 3u ? v - 3u : v);
    }
}
int n2_apply_move(N2State *state, uint8_t move_id)
{
    WorkState a, b;
    if (move_id >= N2_MOVE_COUNT || !expand_public(state, &a))
        return 0;
    state_move(&a, move_id, &b);
    compress_public(&b, state);
    return 1;
}
static void symmetry_apply(const WorkState *in, uint8_t sym, WorkState *out)
{
    unsigned pos;
    for (pos = 0; pos < 8; ++pos) {
        uint8_t np = n2_b8_sym_pos[(unsigned) sym * 8u + pos],
                piece = in->p[pos];
        unsigned ti =
            ((((unsigned) sym * 8u + pos) * 8u + piece) * 3u) + in->o[pos];
        out->p[np] = n2_b8_sym_pos[(unsigned) sym * 8u + piece];
        out->o[np] = n2_b8_sym_twist[ti];
    }
}
static uint32_t quotient_rank_from_meta(const WorkState *s, uint16_t meta)
{
    unsigned oi = meta >> 3;
    uint8_t bs = (uint8_t) (meta & 7u);
    if (oi < 814u)
        return h48_common_base[oi] + ori_rank_after_sym(s, bs);
    {
        unsigned e = oi - 814u;
        WorkState q;
        uint16_t raw;
        symmetry_apply(s, bs, &q);
        raw = ori_rank_after_sym(&q, 0);
        return h48_exc_base[e] + h48_action_rank_ptr[h48_exc_action[e]][raw];
    }
}
static uint16_t quotient_perm_meta(const WorkState *s)
{
    return n2_b8_pmeta[perm_rank7(s->p)];
}

static const uint8_t t_remtab[7][6] = {{1, 2, 3, 4, 5, 6}, {0, 2, 3, 4, 5, 6},
                                       {0, 1, 3, 4, 5, 6}, {0, 1, 2, 4, 5, 6},
                                       {0, 1, 2, 3, 5, 6}, {0, 1, 2, 3, 4, 6},
                                       {0, 1, 2, 3, 4, 5}};
static void compact_T(const WorkState *s, WorkState *d)
{
    int pos6 = 0;
    unsigned q, posp, twp, i, sumy = 0, sum;
    const uint8_t *rem, *remp;
    uint8_t y[5];
    uint32_t ctl;
    while (s->p[pos6] != 6u)
        ++pos6;
    q = (unsigned) pos6 * 3u + s->o[pos6];
    {
        uint8_t dst = h48_t_dst[q];
        posp = dst & 7u;
        twp = dst >> 3;
    }
    rem = t_remtab[pos6];
    remp = t_remtab[posp];

    d->p[7] = 7u;
    for (i = 0; i < 6u; ++i)
        d->p[remp[i]] = (uint8_t) (s->p[rem[h48_t_perm_R[q][i]]] ^ 1u);
    d->p[posp] = 6u;

    for (i = 0; i < 5u; ++i) {
        y[i] = s->o[rem[i]];
        sumy += y[i];
    }
    d->o[7] = 0u;
    d->o[posp] = (uint8_t) twp;
    sum = twp;
    ctl = h48_t_ori_ctl[q];
    for (i = 0; i < 5u; ++i) {
        uint8_t item = (uint8_t) (ctl & 31u);
        uint8_t si = item & 7u;
        unsigned term = si == 7u ? sumy : ((unsigned) y[si] << 1);
        uint8_t v = mod3_small((item >> 3) + term);
        d->o[remp[i]] = v;
        sum += v;
        ctl >>= 5;
    }
    {
        uint8_t r = mod3_small(sum);
        d->o[remp[5]] = (uint8_t) (r ? 3u - r : 0u);
    }
}
static uint32_t h48_canonical_key(const WorkState *s)
{
    WorkState r[8], a;
    uint16_t meta[8], min_oi;
    uint32_t key = UINT32_MAX;
    unsigned i;
    work_copy(s, &r[0]);
    compact_T(s, &r[1]);
    symmetry_apply(s, 1, &a);
    compact_T(&a, &r[2]);
    symmetry_apply(s, 3, &a);
    compact_T(&a, &r[3]);
    symmetry_apply(&r[1], 1, &a);
    compact_T(&a, &r[4]);
    symmetry_apply(&r[1], 3, &a);
    compact_T(&a, &r[5]);
    symmetry_apply(&r[2], 3, &a);
    compact_T(&a, &r[6]);
    symmetry_apply(&r[4], 3, &a);
    compact_T(&a, &r[7]);
    meta[0] = quotient_perm_meta(&r[0]);
    min_oi = (uint16_t) (meta[0] >> 3);
    for (i = 1; i < 8u; ++i) {
        uint16_t oi;
        meta[i] = quotient_perm_meta(&r[i]);
        oi = (uint16_t) (meta[i] >> 3);
        if (oi < min_oi)
            min_oi = oi;
    }
    for (i = 0; i < 8u; ++i) {
        if ((meta[i] >> 3) == min_oi) {
            uint32_t qidx = quotient_rank_from_meta(&r[i], meta[i]);
            if (qidx < key)
                key = qidx;
        }
    }
    return key;
}
static int work_solved(const WorkState *s)
{
    unsigned i;
    for (i = 0; i < 7u; ++i)
        if (s->p[i] != i)
            return 0;
    for (i = 0; i < 7u; ++i)
        if (s->o[i] != 0u)
            return 0;
    return 1;
}


static int b8_lookup(uint32_t key, uint8_t *distance)
{
    uint16_t lo, hi;
    uint16_t needle = (uint16_t) (key & 0x1ffu);
    unsigned b = key >> 9;
    if (b >= H48_B8_BUCKETS)
        return 0;
    lo = h48_b8_start[b];
    hi = h48_b8_start[b + 1u];
    while (lo < hi) {
        uint16_t mid = (uint16_t) (lo + ((hi - lo) >> 1));
        uint16_t e = h48_b8_entry[mid];
        uint16_t k = e & 0x1ffu;
        if (k < needle)
            lo = (uint16_t) (mid + 1u);
        else
            hi = mid;
    }
    if (lo < h48_b8_start[b + 1u]) {
        uint16_t e = h48_b8_entry[lo];
        if ((e & 0x1ffu) == needle) {
            *distance = (uint8_t) ((e >> 9) & 15u);
            return 1;
        }
    }
    return 0;
}

static int b8_probe(const WorkState *s, uint8_t *distance)
{
    return b8_lookup(h48_canonical_key(s), distance);
}

static int finish_from_b8(WorkState *cur,
                          uint8_t distance,
                          uint8_t out[N2_MAX_DEPTH],
                          uint8_t *k)
{
    while (distance != 0u) {
        unsigned face, turn;
        int found = 0;
        if (*k >= N2_MAX_DEPTH)
            return 0;
        for (face = 0; face < 3u && !found; ++face) {
            for (turn = 0; turn < 3u; ++turn) {
                uint8_t m = (uint8_t) (((face << 1) + face) + turn);
                WorkState next;
                uint8_t nd;
                state_move(cur, m, &next);
                if (b8_probe(&next, &nd) && nd + 1u == distance) {
                    out[(*k)++] = m;
                    work_copy(&next, cur);
                    distance = nd;
                    found = 1;
                    break;
                }
            }
        }
        if (!found)
            return 0;
    }
    return work_solved(cur);
}

static uint8_t move_id(unsigned face, unsigned turn)
{
    return (uint8_t) (((face << 1) + face) + turn);
}

int n2_solve(const N2State *state,
             uint8_t out[N2_MAX_DEPTH],
             uint8_t *count,
             N2Stats *stats)
{
    WorkState root;
    uint8_t dist;
    uint8_t k = 0;
    unsigned f1, f2, f3, t1, t2, t3;
    uint16_t probes = 0;

    if (!out || !count || !expand_public(state, &root))
        return 0;
    if (work_solved(&root)) {
        *count = 0;
        if (stats) {
            stats->search_nodes = 1;
            stats->solution_depth = 0;
            stats->prefix_depth = 0;
        }
        return 1;
    }

    ++probes;
    if (b8_probe(&root, &dist)) {
        WorkState cur;
        work_copy(&root, &cur);
        if (!finish_from_b8(&cur, dist, out, &k))
            return 0;
        *count = k;
        if (stats) {
            stats->search_nodes = probes;
            stats->solution_depth = k;
            stats->prefix_depth = 0;
        }
        return 1;
    }

    for (f1 = 0; f1 < 3u; ++f1)
        for (t1 = 0; t1 < 3u; ++t1) {
            uint8_t m1 = move_id(f1, t1);
            WorkState s1;
            state_move(&root, m1, &s1);
            ++probes;
            if (b8_probe(&s1, &dist)) {
                out[k++] = m1;
                if (!finish_from_b8(&s1, dist, out, &k))
                    return 0;
                *count = k;
                if (stats) {
                    stats->search_nodes = probes;
                    stats->solution_depth = k;
                    stats->prefix_depth = 1;
                }
                return 1;
            }
        }

    for (f1 = 0; f1 < 3u; ++f1)
        for (t1 = 0; t1 < 3u; ++t1) {
            uint8_t m1 = move_id(f1, t1);
            WorkState s1;
            state_move(&root, m1, &s1);
            for (f2 = 0; f2 < 3u; ++f2)
                if (f2 != f1)
                    for (t2 = 0; t2 < 3u; ++t2) {
                        uint8_t m2 = move_id(f2, t2);
                        WorkState s2;
                        state_move(&s1, m2, &s2);
                        ++probes;
                        if (b8_probe(&s2, &dist)) {
                            out[k++] = m1;
                            out[k++] = m2;
                            if (!finish_from_b8(&s2, dist, out, &k))
                                return 0;
                            *count = k;
                            if (stats) {
                                stats->search_nodes = probes;
                                stats->solution_depth = k;
                                stats->prefix_depth = 2;
                            }
                            return 1;
                        }
                    }
        }

    for (f1 = 0; f1 < 3u; ++f1)
        for (t1 = 0; t1 < 3u; ++t1) {
            uint8_t m1 = move_id(f1, t1);
            WorkState s1;
            state_move(&root, m1, &s1);
            for (f2 = 0; f2 < 3u; ++f2)
                if (f2 != f1)
                    for (t2 = 0; t2 < 3u; ++t2) {
                        uint8_t m2 = move_id(f2, t2);
                        WorkState s2;
                        state_move(&s1, m2, &s2);
                        for (f3 = 0; f3 < 3u; ++f3)
                            if (f3 != f2)
                                for (t3 = 0; t3 < 3u; ++t3) {
                                    uint8_t m3 = move_id(f3, t3);
                                    WorkState s3;
                                    state_move(&s2, m3, &s3);
                                    ++probes;
                                    if (b8_probe(&s3, &dist)) {
                                        out[k++] = m1;
                                        out[k++] = m2;
                                        out[k++] = m3;
                                        if (!finish_from_b8(&s3, dist, out, &k))
                                            return 0;
                                        *count = k;
                                        if (stats) {
                                            stats->search_nodes = probes;
                                            stats->solution_depth = k;
                                            stats->prefix_depth = 3;
                                        }
                                        return 1;
                                    }
                                }
                    }
        }
    return 0;
}
