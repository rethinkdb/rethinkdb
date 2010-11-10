#include "utils2.hpp"
#include "arch/arch.hpp"
#include <unistd.h>
#include <stdlib.h>

/* System configuration*/
int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

long get_available_ram() {
    return (long)sysconf(_SC_AVPHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

long get_total_ram() {
    return (long)sysconf(_SC_PHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

/* Function to create a random delay. Defined in .cc instead of in .tcc because it
uses the IO layer, and it must be safe to include utils2 from within the IO layer. */

void random_delay(void (*fun)(void*), void *arg) {

    int ms = randint(50);
    
    fire_timer_once(ms, fun, arg);
}

void debugf(const char *msg, ...) {
    
    flockfile(stderr);
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "CPU %d: ", get_cpu_id());
    vfprintf(stderr, msg, args);
    va_end(args);
    funlockfile(stderr);
}

int randint(int n) {
    assert(n > 0 && n < RAND_MAX);
    return rand() % n;
}
