#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#define RAM_SIZE 65536

void  mem_init(void);
void *my_alloc(int size);
void  my_dealloc(void *ptr);
void  mem_stats(int *used, int *free_bytes, int *largest_free);
void *my_realloc(void *ptr, int new_size);

#endif
