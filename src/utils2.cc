#include "utils2.hpp"

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/time.h>

#include <boost/scoped_array.hpp>


#include "arch/arch.hpp"

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


const repli_timestamp repli_timestamp::invalid = { -1 };

repli_timestamp repli_time(time_t t) {
    repli_timestamp ret;
    uint32_t x = t;
    ret.time = (x == (uint32_t)-1 ? 0 : x);
    return ret;
}

repli_timestamp current_time() {
    // Get the current time, cast it to 32 bits.  The lack of
    // precision will not break things in 2038 or 2106 if we compare
    // times correctly.

    // time(NULL) does not do a system call (on Linux), last time we
    // checked, but it's still kind of slow.
    return repli_time(time(NULL));
}

microtime_t current_microtime() {
    // This could be done more efficiently, surely.
    struct timeval t;
    int res __attribute__((unused)) = gettimeofday(&t, NULL);
    rassert(0 == res);
    return uint64_t(t.tv_sec) * (1000 * 1000) + t.tv_usec;
}

int repli_compare(repli_timestamp x, repli_timestamp y) {
    return int(int32_t(x.time - y.time));
}

repli_timestamp repli_max(repli_timestamp x, repli_timestamp y) {
    return repli_compare(x, y) < 0 ? y : x;
}


void *malloc_aligned(size_t size, size_t alignment) {
    void *ptr = NULL;
    int res = posix_memalign(&ptr, alignment, size);
    if (res != 0) {
        if (res == EINVAL) {
            crash_or_trap("posix_memalign with bad alignment: %zu.", alignment);
        } else if (res == ENOMEM) {
            crash_or_trap("Out of memory.");
        } else {
            crash_or_trap("posix_memalign failed with unknown result: %d.", res);
        }
    }
    return ptr;
}

/* Function to create a random delay. Defined in .cc instead of in .tcc because it
uses the IO layer, and it must be safe to include utils2 from within the IO layer. */

int choose_random_delay() {
    /* In one in ten thousand requests, we delay up to 10 seconds. In half of the remaining
    requests, we delay up to 50 milliseconds; in the other half we delay a very short time. */
    int kind = randint(10000), ms;
    if (kind == 0) ms = randint(10000);
    else if (kind % 2 == 0) ms = 0;
    else ms = randint(50);
    return ms;
}

void random_delay(void (*fun)(void*), void *arg) {
    fire_timer_once(choose_random_delay(), fun, arg);
}

void debugf(const char *msg, ...) {
    flockfile(stderr);
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "Thread %d: ", get_thread_id());
    vfprintf(stderr, msg, args);
    va_end(args);
    funlockfile(stderr);
}

/* This object exists only to call srand(time(NULL)) in its constructor, before main() runs. */
struct rand_initter_t {
    rand_initter_t() {
        srand(time(NULL));
    }
} rand_initter;

int randint(int n) {
    rassert(n > 0 && n < RAND_MAX);
    return rand() % n;
}

bool begins_with_minus(const char *string) {
    while (isspace(*string)) string++;
    return *string == '-';
}

bool strtobool_strict(const char *string, char **end) {
    unsigned long res = strtoul_strict(string, end, 2);
    if (*end == string || res > 1) {
        *end = const_cast<char *>(string);
        return false;
    }
    return res == 1;
}

long strtol_strict(const char *string, char **end, int base) {
    long result = strtol(string, end, base);
    if ((result == LONG_MAX || result == LONG_MIN) && errno == ERANGE) {
        *end = const_cast<char *>(string);
        return 0;
    }
    return result;
}

unsigned long strtoul_strict(const char *string, char **end, int base) {
    if (begins_with_minus(string)) {
        *end = const_cast<char *>(string);
        return 0;
    }
    unsigned long result = strtoul(string, end, base);
    if (result == ULONG_MAX && errno == ERANGE) {
        *end = const_cast<char *>(string);
        return 0;
    }
    return result;
}

unsigned long long strtoull_strict(const char *string, char **end, int base) {
    if (begins_with_minus(string)) {
        *end = const_cast<char *>(string);
        return 0;
    }
    unsigned long long result = strtoull(string, end, base);
    if (result == ULLONG_MAX && errno == ERANGE) {
        *end = const_cast<char *>(string);
        return 0;
    }
    return result;
}

ticks_t secs_to_ticks(float secs) {
    return (unsigned long long)secs * 1000000000L;
}

ticks_t get_ticks() {
    timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

long get_ticks_res() {
    timespec tv;
    clock_getres(CLOCK_MONOTONIC, &tv);
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

float ticks_to_secs(ticks_t ticks) {
    return ticks / 1000000000.0f;
}

float ticks_to_ms(ticks_t ticks) {
    return ticks / 1000000.0f;
}

float ticks_to_us(ticks_t ticks) {
    return ticks / 1000.0f;
}

std::string strprintf(const char *format, ...) {
    va_list ap;

    boost::scoped_array<char> arr;

    va_start(ap, format);
    va_list aq;
    va_copy(aq, ap);


    // the snprintfs return the number of characters they _would_ have
    // written, not including the '\0', so we use that number to
    // allocate an appropriately sized array.
    char buf[1];
    int size = vsnprintf(buf, sizeof(buf), format, ap);

    guarantee_err(size >= 0, "vsnprintf failed, bad format string?");

    arr.reset(new char[size + 1]);

    int newsize = vsnprintf(arr.get(), size + 1, format, aq);
    (void)newsize;
    assert(newsize == size);

    va_end(aq);
    va_end(ap);

    return std::string(arr.get(), arr.get() + size);
}
