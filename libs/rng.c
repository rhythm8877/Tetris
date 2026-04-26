/*
 * libs/rng.c
 * Tiny PRNG for the Tetris game. We are not allowed to call rand() from
 * <stdlib.h>, so we implement xorshift32 — fast, ~5 lines, perfectly
 * adequate for game purposes.
 *
 * Also exposes a 7-bag piece generator (Tetris standard): every batch
 * of 7 pieces contains exactly one of each tetromino in random order,
 * with no duplicates until the bag refills.
 */

#include <sys/time.h>
#include "rng.h"
#include "math.h"

static unsigned int g_state = 0xCAFEBABEu;

static int g_bag[7];
static int g_bag_idx = 0;

void rng_seed(unsigned int seed)
{
    /* xorshift32 gets stuck on state 0 — substitute a non-zero default. */
    if (seed == 0u)
        seed = 0xCAFEBABEu;
    g_state = seed;
}

unsigned int rng_next_u32(void)
{
    g_state ^= g_state << 13;
    g_state ^= g_state >> 17;
    g_state ^= g_state << 5;
    return g_state;
}

int rng_range(int lo, int hi)
{
    unsigned int r;
    int          span;
    int          positive;

    r        = rng_next_u32();
    span     = my_max(1, hi - lo + 1);
    /* r >> 1 is in [0, 2^31 - 1] which fits in int without sign loss. */
    positive = (int)(r >> 1);
    return lo + my_mod(positive, span);
}

unsigned int rng_seed_from_clock(void)
{
    struct timeval tv;

    gettimeofday(&tv, 0);
    /* Mix tv_sec and tv_usec via XOR per spec. tv_usec only fills the
     * low ~20 bits, so the result has weak entropy in the high half —
     * good enough for seeding xorshift32, which fans out within a few
     * iterations. */
    return ((unsigned int)tv.tv_sec) ^ ((unsigned int)tv.tv_usec);
}

static void rng_bag_refill(void)
{
    int i;
    int j;
    int tmp;

    for (i = 0; i < 7; i++)
        g_bag[i] = i;

    /* Fisher-Yates: walk i from 6 down to 1, swap bag[i] with bag[j]
     * where j is uniform in [0, i]. */
    for (i = 6; i >= 1; i--) {
        j        = rng_range(0, i);
        tmp      = g_bag[i];
        g_bag[i] = g_bag[j];
        g_bag[j] = tmp;
    }

    g_bag_idx = 0;
}

int rng_bag_next(void)
{
    int v;

    /* idx == 0 covers both the very first call (statics zero-init) and
     * a freshly-exhausted bag where refill just reset idx to 0.
     * idx == 7 is the "exhausted just before this call" trigger. */
    if (g_bag_idx == 0 || g_bag_idx == 7)
        rng_bag_refill();

    v = g_bag[g_bag_idx];
    g_bag_idx++;
    return v;
}

#ifdef TEST_RNG
#include <stdio.h>

int main(void)
{
    int i;
    int v;
    int seen[7];
    int j;
    int run;
    int run_pass;
    int all_pass;

    rng_seed(rng_seed_from_clock());

    printf("== rng_range(0, 9) x 100 ==\n");
    for (i = 0; i < 100; i++) {
        v = rng_range(0, 9);
        printf("%d ", v);
        if (((i + 1) % 10) == 0)
            printf("\n");
    }
    printf("\n");

    printf("== rng_bag_next() x 14 (two consecutive bags) ==\n");
    all_pass = 1;
    for (run = 0; run < 2; run++) {
        for (j = 0; j < 7; j++)
            seen[j] = 0;

        for (j = 0; j < 7; j++) {
            v = rng_bag_next();
            printf("%d ", v);
            if (v < 0 || v > 6) {
                all_pass = 0;
            } else {
                seen[v]++;
            }
        }
        printf(" | ");

        run_pass = 1;
        for (j = 0; j < 7; j++) {
            if (seen[j] != 1)
                run_pass = 0;
        }
        printf(run_pass ? "[PASS]\n" : "[FAIL]\n");

        if (!run_pass)
            all_pass = 0;
    }

    printf("\n%s\n", all_pass ? "OVERALL PASS" : "OVERALL FAIL");
    return all_pass ? 0 : 1;
}
#endif
