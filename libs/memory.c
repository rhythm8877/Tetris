#include "memory.h"

typedef struct {
    int size;
    int is_free;
} block_header_t;

#define HEADER_SIZE ((int)sizeof(block_header_t))
#define ALIGN4(n)   (((n) + 3) & ~3)

static char virtual_ram[RAM_SIZE];

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

    return 0;
}

void my_dealloc(void *ptr)
{
    block_header_t *hdr;
    block_header_t *next;
    block_header_t *prev;
    int             hdr_offset;
    int             pos;

    if (!ptr)
        return;

    hdr          = (block_header_t *)((char *)ptr - HEADER_SIZE);
    hdr->is_free = 1;

    /* Forward coalesce */
    for (;;)
    {
        next = (block_header_t *)((char *)hdr + HEADER_SIZE + hdr->size);

        if ((char *)next + HEADER_SIZE > virtual_ram + RAM_SIZE)
            break;

        if (!next->is_free)
            break;

        hdr->size += HEADER_SIZE + next->size;
    }

    /* Backward coalesce: walk from heap start to find predecessor */
    hdr_offset = (int)((char *)hdr - virtual_ram);
    if (hdr_offset == 0)
        return;

    pos = 0;
    while (pos + HEADER_SIZE <= RAM_SIZE)
    {
        prev = (block_header_t *)(virtual_ram + pos);

        if (pos + HEADER_SIZE + prev->size == hdr_offset)
        {
            if (prev->is_free)
                prev->size += HEADER_SIZE + hdr->size;
            return;
        }

        pos += HEADER_SIZE + prev->size;
        if (pos >= hdr_offset)
            break;
    }
}

void mem_stats(int *used, int *free_bytes, int *largest_free)
{
    block_header_t *hdr;
    int             pos;

    *used         = 0;
    *free_bytes   = 0;
    *largest_free = 0;

    pos = 0;
    while (pos + HEADER_SIZE <= RAM_SIZE)
    {
        hdr = (block_header_t *)(virtual_ram + pos);
        if (hdr->is_free)
        {
            *free_bytes += hdr->size;
            if (hdr->size > *largest_free)
                *largest_free = hdr->size;
        }
        else
        {
            *used += hdr->size;
        }
        pos += HEADER_SIZE + hdr->size;
    }
}

void *my_realloc(void *ptr, int new_size)
{
    block_header_t *hdr;
    void           *new_ptr;
    char           *src;
    char           *dst;
    int             old_size;
    int             i;

    if (!ptr)
        return my_alloc(new_size);

    if (new_size == 0)
    {
        my_dealloc(ptr);
        return 0;
    }

    hdr      = (block_header_t *)((char *)ptr - HEADER_SIZE);
    old_size = hdr->size;

    if (new_size <= old_size)
        return ptr;

    new_ptr = my_alloc(new_size);
    if (!new_ptr)
        return 0;

    src = (char *)ptr;
    dst = (char *)new_ptr;
    for (i = 0; i < old_size; i++)
        dst[i] = src[i];

    my_dealloc(ptr);
    return new_ptr;
}

#ifdef TEST_MEMORY
#include <stdio.h>

static int g_pass = 0;
static int g_fail = 0;

static void check(const char *name, int cond)
{
    if (cond) {
        printf("[PASS] %s\n", name);
        g_pass++;
    } else {
        printf("[FAIL] %s\n", name);
        g_fail++;
    }
}

int main(void)
{
    void           *b0, *b1, *b2, *b3, *b4;
    void           *merged;
    void           *r;
    int             merged_size;
    int             used, free_bytes, largest;
    int             total, n_headers, pos;
    block_header_t *hdr;
    char           *p;
    int             i;

    mem_init();

    /* --- Allocate 5 blocks of varying sizes --- */
    b0 = my_alloc(64);
    b1 = my_alloc(128);
    b2 = my_alloc(256);
    b3 = my_alloc(64);
    b4 = my_alloc(128);

    check("alloc b0", b0 != 0);
    check("alloc b1", b1 != 0);
    check("alloc b2", b2 != 0);
    check("alloc b3", b3 != 0);
    check("alloc b4", b4 != 0);

    /* --- Free b1 and b3, then b2 to trigger bidirectional coalesce --- */
    my_dealloc(b1);
    my_dealloc(b3);
    my_dealloc(b2);

    /*
     * Merged payload spans former b1 + H + b2 + H + b3.
     * Forward coalesce absorbs b3 into b2, then backward
     * coalesce absorbs the result into b1.
     */
    merged_size = ALIGN4(128) + HEADER_SIZE + ALIGN4(256)
                + HEADER_SIZE + ALIGN4(64);

    merged = my_alloc(merged_size);
    check("coalesce: merged block not NULL", merged != 0);
    check("coalesce: merged block at b1 address", merged == b1);

    /* --- mem_stats accounting --- */
    mem_stats(&used, &free_bytes, &largest);

    n_headers = 0;
    pos       = 0;
    while (pos + HEADER_SIZE <= RAM_SIZE)
    {
        hdr = (block_header_t *)(virtual_ram + pos);
        n_headers++;
        pos += HEADER_SIZE + hdr->size;
    }

    total = used + free_bytes + n_headers * HEADER_SIZE;
    check("mem_stats: accounting equals RAM_SIZE", total == RAM_SIZE);
    printf("       used=%d  free=%d  headers=%d*%d=%d  total=%d\n",
           used, free_bytes, n_headers, HEADER_SIZE,
           n_headers * HEADER_SIZE, total);

    check("mem_stats: largest_free > 0", largest > 0);
    check("mem_stats: largest_free <= free_bytes", largest <= free_bytes);

    /* --- my_realloc --- */

    /* NULL ptr behaves like alloc */
    r = my_realloc(0, 32);
    check("realloc(NULL, 32) != NULL", r != 0);

    /* Write a pattern so we can verify data preservation */
    p = (char *)r;
    for (i = 0; i < 32; i++)
        p[i] = (char)(i + 1);

    /* Shrink returns same pointer */
    r = my_realloc(r, 16);
    check("realloc shrink: same ptr", r == (void *)p);

    /* Grow copies data to new block */
    r = my_realloc(r, 512);
    check("realloc grow: not NULL", r != 0);
    p = (char *)r;
    check("realloc grow: data[0] preserved",  p[0] == 1);
    check("realloc grow: data[15] preserved", p[15] == 16);
    check("realloc grow: data[31] preserved", p[31] == 32);

    /* new_size == 0 behaves like dealloc */
    r = my_realloc(r, 0);
    check("realloc(ptr, 0) returns NULL", r == 0);

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
#endif
