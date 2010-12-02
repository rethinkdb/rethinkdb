
#include <time.h>
#include <stdlib.h>
#include "utils.hpp"

/* Really fast random function */
static unsigned long x=123456789, y=362436069, z=521288629;

unsigned long xorshf96(void) {          //period 2^96-1
unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

   t = x;
   x = y;
   y = z;
   z = t ^ x ^ y;

  return z;
}

static unsigned long saltx=123456789, salty=362436069, saltz=521288629;
unsigned long seeded_xorshf96(unsigned long seed) {
    unsigned long X = saltx ^ seed << 2, Y = salty ^ seed << 13, Z = saltz ^ seed << 5;

    X ^= X << 17;
    X ^= Y >> 13;
    X ^= Z << 43;

    X ^= X >> 5;
    X ^= X >> 16;
    X ^= X >> 44;

    return X;
}

/* Returns random number between [min, max] */
size_t random(size_t min, size_t max) {
    //return rand() % (max - min + 1) + min;
    return xorshf96() % (max - min + 1) + min;
}

size_t seeded_random(size_t min, size_t max, unsigned long seed) {
    return seeded_xorshf96(seed) % (max - min + 1) + min;
}
