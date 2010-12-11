
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

/* Returns random number between [min, max] */
size_t random(size_t min, size_t max);
size_t seeded_random(size_t min, size_t max, unsigned long seed);

/* Timing related functions */
typedef unsigned long long ticks_t;
ticks_t secs_to_ticks(float secs);
ticks_t get_ticks();
long get_ticks_res();
float ticks_to_secs(ticks_t ticks);
float ticks_to_ms(ticks_t ticks);
float ticks_to_us(ticks_t ticks);

#endif // __UTILS_HPP__

