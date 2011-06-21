#include "utils2.hpp"

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/time.h>
#include <boost/scoped_array.hpp>

#include "arch/arch.hpp"



const repli_timestamp_t repli_timestamp_t::invalid = { -1 };
const repli_timestamp_t repli_timestamp_t::distant_past = { 0 };

microtime_t current_microtime() {
    // This could be done more efficiently, surely.
    struct timeval t;
    int res __attribute__((unused)) = gettimeofday(&t, NULL);
    rassert(0 == res);
    return uint64_t(t.tv_sec) * (1000 * 1000) + t.tv_usec;
}

repli_timestamp_t repli_max(repli_timestamp_t x, repli_timestamp_t y) {
    return int32_t(x.time - y.time) < 0 ? y : x;
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

void debugf(const char *msg, ...) {
    flockfile(stderr);
    precise_time_t t = get_time_now();
    char formatted_time[formatted_precise_time_length+1];
    format_precise_time(t, formatted_time, sizeof(formatted_time));
    fprintf(stderr, "%s Thread %d: ", formatted_time, get_thread_id());

    va_list args;
    va_start(args, msg);
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
    // The timespec struct used in clock_gettime has a tv_nsec field.
    // That's why we use a billion.
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

double ticks_to_secs(ticks_t ticks) {
    return ticks / 1000000000.0;
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
