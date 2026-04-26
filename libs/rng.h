#ifndef MY_RNG_H
#define MY_RNG_H

void         rng_seed(unsigned int seed);
unsigned int rng_next_u32(void);
int          rng_range(int lo, int hi);
int          rng_bag_next(void);
unsigned int rng_seed_from_clock(void);

#endif
