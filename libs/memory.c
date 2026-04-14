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

    if (!ptr)
        return;

    hdr          = (block_header_t *)((char *)ptr - HEADER_SIZE);
    hdr->is_free = 1;

    for (;;)
    {
        next = (block_header_t *)((char *)hdr + HEADER_SIZE + hdr->size);

        if ((char *)next + HEADER_SIZE > virtual_ram + RAM_SIZE)
            break;

        if (!next->is_free)
            break;

        hdr->size += HEADER_SIZE + next->size;
    }
}

