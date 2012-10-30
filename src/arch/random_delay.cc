// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/arch.hpp"
#include "arch/random_delay.hpp"

/* Function to create a random delay. Defined in .cc because it uses
the IO layer, and it must be safe to include this from within the IO
layer.

Things could be made cleaner but we're just moving this out of utils2.cc.

TODO: Make this cleaner in v1.2.
*/

void random_delay(void (*fun)(void *), void *arg) {  // NOLINT
    /* In one in ten thousand requests, we delay up to 10 seconds. In half of the remaining
    requests, we delay up to 50 milliseconds; in the other half we delay a very short time. */
    int kind = randint(10000), ms;
    if (kind == 0) {
        ms = randint(10000);
    } else if (kind % 2 == 0) {
        ms = 0;
    } else {
        ms = randint(50);
    }
    fire_timer_once(ms, fun, arg);
}

