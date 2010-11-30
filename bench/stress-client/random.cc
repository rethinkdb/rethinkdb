
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

/* Returns random number between [min, max] */
size_t random(size_t min, size_t max) {
    //return rand() % (max - min + 1) + min;
    return xorshf96() % (max - min + 1) + min;
}

