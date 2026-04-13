#include "memory.h"

void  mem_init(void)        {}
void *my_alloc(int size)    { (void)size; return 0; }
void  my_dealloc(void *ptr) { (void)ptr; }
