#include "memory.h"

#ifdef TEST_MEMORY
#include <stdio.h>
#endif

/* Header stored immediately before every allocation.
 * Two 'int' fields = 8 bytes, naturally 4-byte aligned on all target
 * platforms.  virtual_ram is a global static, so the linker places it at
 * an address aligned to at least the platform's strictest type requirement,
 * meaning every header we embed inside it is also properly aligned. */
typedef struct {
    int size;    /* usable bytes in the block, NOT counting this header */
    int is_free; /* 1 = available for allocation, 0 = in use            */
} block_header_t;

/* HEADER_SIZE is the compile-time constant for sizeof(block_header_t).
 * Casting to int avoids signed/unsigned comparison warnings throughout. */
#define HEADER_SIZE ((int)sizeof(block_header_t))

/* Round n up to the nearest multiple of 4.
 * Keeps all returned data pointers — and therefore all subsequent headers —
 * on 4-byte boundaries. */
#define ALIGN4(n)   (((n) + 3) & ~3)

/* The only global storage: a flat byte arena that replaces the heap. */
static char virtual_ram[RAM_SIZE];

/* --- mem_init -------------------------------------------------------------
 * Zero every byte of virtual_ram with a manual loop (memset is forbidden).
 * Then plant a single free block that spans the entire usable arena:
 *
 *   offset 0 : [ header: size=RAM_SIZE-HEADER_SIZE, is_free=1 ]
 *   offset 8 : [ ... 65528 bytes of free space ...            ]
 *
 * Every subsequent allocation carves pieces out of this initial block.
 * Calling mem_init() again resets the arena — useful in tests.
 * -------------------------------------------------------------------------- */
void mem_init(void)
{
    block_header_t *first;
    int             i;

    for (i = 0; i < RAM_SIZE; i++)
        virtual_ram[i] = 0;

    first          = (block_header_t *)virtual_ram;
    first->size    = RAM_SIZE - HEADER_SIZE;
    first->is_free = 1;
}

/* --- my_alloc -------------------------------------------------------------
 * First-fit allocator: scan virtual_ram from the beginning for the first
 * free block whose usable size is >= aligned_size, then:
 *
 *   1. Split: when the leftover after carving aligned_size bytes is large
 *      enough to hold a full header + at least 4 usable bytes, create a new
 *      free header in the remainder so that space is not wasted.
 *
 *   2. Mark the chosen block as in-use (is_free = 0).
 *
 *   3. Return a pointer to the first byte AFTER the header.
 *
 * size is rounded up to a 4-byte boundary so that the next header that
 * follows is always naturally aligned.
 *
 * Returns NULL (0) when size <= 0 or no suitable block exists.
 * -------------------------------------------------------------------------- */
void *my_alloc(int size)
{
    block_header_t *hdr;
    block_header_t *split;
    int             pos;
    int             aligned_size;

    if (size <= 0)
        return 0;

    aligned_size = ALIGN4(size);

    pos = 0;
    while (pos + HEADER_SIZE <= RAM_SIZE)
    {
        hdr = (block_header_t *)(virtual_ram + pos);

        if (hdr->is_free && hdr->size >= aligned_size)
        {
            /* Split only when the leftover can hold a header + >=4 data bytes.
             * Otherwise hand the whole block to the caller (minor waste, but
             * avoids creating header-only fragments that can never be used). */
            if (hdr->size >= aligned_size + HEADER_SIZE + 4)
            {
                split          = (block_header_t *)(virtual_ram + pos + HEADER_SIZE + aligned_size);
                split->size    = hdr->size - aligned_size - HEADER_SIZE;
                split->is_free = 1;
                hdr->size      = aligned_size;
            }

            hdr->is_free = 0;
            return (void *)(virtual_ram + pos + HEADER_SIZE);
        }

        pos += HEADER_SIZE + hdr->size;
    }

    return 0; /* no block is large enough — virtual RAM is exhausted */
}

/* --- my_dealloc -----------------------------------------------------------
 * Free a block by stepping back HEADER_SIZE bytes from ptr to reach its
 * header, then marking it is_free = 1.
 *
 * After marking, coalesce forward: while the block immediately following the
 * newly freed one is also free, absorb it.  This restores contiguous free
 * space that was split by earlier allocations, preventing the fragmentation
 * that would otherwise make the arena unusable after many Tetromino cycles.
 *
 * Passing NULL is a safe no-op (mirrors the behaviour of standard free()).
 * -------------------------------------------------------------------------- */
void my_dealloc(void *ptr)
{
    block_header_t *hdr;
    block_header_t *next;

    if (!ptr)
        return;

    hdr          = (block_header_t *)((char *)ptr - HEADER_SIZE);
    hdr->is_free = 1;

    /* Coalesce forward: absorb consecutive free neighbours one at a time. */
    for (;;)
    {
        next = (block_header_t *)((char *)hdr + HEADER_SIZE + hdr->size);

        /* Stop if next header would reach past the end of the arena. */
        if ((char *)next + HEADER_SIZE > virtual_ram + RAM_SIZE)
            break;

        if (!next->is_free)
            break; /* next block is still in use — nothing to merge */

        /* Absorb: current block grows to swallow next's header + data. */
        hdr->size += HEADER_SIZE + next->size;
    }
}

/* ==========================================================================
 * Self-contained test block — compile with -DTEST_MEMORY and run:
 *
 *   gcc -Wall -Wextra -DTEST_MEMORY -Ilibs libs/memory.c -o mem_test && ./mem_test
 *
 * ========================================================================== */
#ifdef TEST_MEMORY

#define PASS "\033[32mPASS\033[0m"
#define FAIL "\033[31mFAIL\033[0m"

static int g_tests  = 0;
static int g_passed = 0;

static void check_int(const char *label, int got, int expected)
{
    g_tests++;
    if (got == expected)
    {
        printf("[%s] %s  (got %d)\n", PASS, label, got);
        g_passed++;
    }
    else
    {
        printf("[%s] %s  (got %d, expected %d)\n", FAIL, label, got, expected);
    }
}

static void check_nonnull(const char *label, void *ptr)
{
    g_tests++;
    if (ptr != 0)
    {
        printf("[%s] %s  (got %p)\n", PASS, label, ptr);
        g_passed++;
    }
    else
    {
        printf("[%s] %s  (got NULL)\n", FAIL, label);
    }
}

static void check_null(const char *label, void *ptr)
{
    g_tests++;
    if (ptr == 0)
    {
        printf("[%s] %s  (correctly NULL)\n", PASS, label);
        g_passed++;
    }
    else
    {
        printf("[%s] %s  (expected NULL, got %p)\n", FAIL, label, ptr);
    }
}

static void check_ptr(const char *label, void *got, void *expected)
{
    g_tests++;
    if (got == expected)
    {
        printf("[%s] %s  (got %p)\n", PASS, label, got);
        g_passed++;
    }
    else
    {
        printf("[%s] %s  (got %p, expected %p)\n", FAIL, label, got, expected);
    }
}

int main(void)
{
    void *a;
    void *b;
    void *c;
    void *b2;
    int  *scores;
    int   i;

    /* ------------------------------------------------------------------ */
    printf("=== basic allocation ===\n");
    mem_init();

    a = my_alloc(16);
    b = my_alloc(32);
    c = my_alloc(16);

    check_nonnull("alloc A (16 bytes)", a);
    check_nonnull("alloc B (32 bytes)", b);
    check_nonnull("alloc C (16 bytes)", c);

    /* Addresses must be distinct and in ascending order within the arena. */
    check_int("A < B (ascending addresses)", (char *)a < (char *)b, 1);
    check_int("B < C (ascending addresses)", (char *)b < (char *)c, 1);

    /* ------------------------------------------------------------------ */
    printf("\n=== free-list reuse: free middle block, re-allocate ===\n");

    my_dealloc(b);          /* B is now free; A and C remain in use */
    b2 = my_alloc(32);      /* first-fit must find B's freed slot first */

    /* The allocator walks forward: A (used) → B (free, exact fit) → returns B.
     * b2 must be identical to b — this is the proof that the free list works. */
    check_ptr("b2 == b  (first-fit reuses freed slot)", b2, b);

    /* ------------------------------------------------------------------ */
    printf("\n=== edge-case guards ===\n");

    check_null("alloc(0)  returns NULL",  my_alloc(0));
    check_null("alloc(-1) returns NULL",  my_alloc(-1));

    my_dealloc(0); /* must not crash or corrupt anything */
    printf("[%s] dealloc(NULL) is a safe no-op\n", PASS);
    g_tests++;
    g_passed++;

    /* ------------------------------------------------------------------ */
    printf("\n=== write through allocated memory ===\n");

    /* Reset the arena, allocate an int array, write and read back values. */
    mem_init();
    scores = (int *)my_alloc(10 * (int)sizeof(int));
    check_nonnull("alloc 10-int array", (void *)scores);

    for (i = 0; i < 10; i++)
        scores[i] = i * i;

    check_int("scores[0] == 0",  scores[0], 0);
    check_int("scores[5] == 25", scores[5], 25);
    check_int("scores[9] == 81", scores[9], 81);

    /* ------------------------------------------------------------------ */
    printf("\n=== coalescing: free blocks merge, enabling large re-alloc ===\n");

    /*
     * Layout after two allocations:
     *
     *   [hdr|100 bytes used] [hdr|200 bytes used] [hdr|65212 bytes free]
     *    ^x                   ^y
     *
     * After dealloc(x):  x is free; y still used; no merge (next is used).
     * After dealloc(y):  y is free; its next block (the tail) is also free
     *                    → coalesce: y absorbs tail → 200+8+65212 = 65420 bytes.
     *
     * Now two separate free blocks exist (x=100 bytes, y=65420 bytes).
     * A request for 280 bytes fits inside y's coalesced block.
     */
    mem_init();
    a = my_alloc(100);
    b = my_alloc(200);
    check_nonnull("alloc 100-byte block", a);
    check_nonnull("alloc 200-byte block", b);

    my_dealloc(a); /* free first block — next (b) is still used, no merge */
    my_dealloc(b); /* free second block — coalesces forward with arena tail */

    b2 = my_alloc(280); /* fits inside b's coalesced region (65420 bytes) */
    check_nonnull("alloc 280 bytes after forward coalesce", b2);

    /* ------------------------------------------------------------------ */
    printf("\n%d / %d tests passed.\n", g_passed, g_tests);
    return (g_passed == g_tests) ? 0 : 1;
}

#endif /* TEST_MEMORY */
