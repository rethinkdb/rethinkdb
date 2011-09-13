#include "utils.hpp"

#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <cxxabi.h>

#include <boost/scoped_array.hpp>

#include "arch/runtime/runtime.hpp"
#include "db_thread_info.hpp"

#include <boost/uuid/uuid_generators.hpp>

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#endif

// fast non-null terminated string comparison
int sized_strcmp(const char *str1, int len1, const char *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0) {
        res = len1 - len2;
    }
    return res;
}

void print_hd(const void *vbuf, size_t offset, size_t ulength) {
    flockfile(stderr);

    const char *buf = reinterpret_cast<const char *>(vbuf);
    ssize_t length = ulength;

    char bd_sample[16] = { 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 
                           0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD };
    char zero_sample[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char ff_sample[16] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                           0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    bool skipped_last = false;
    while (length > 0) {
        bool skip = memcmp(buf, bd_sample, 16) == 0 ||
                    memcmp(buf, zero_sample, 16) == 0 ||
                    memcmp(buf, ff_sample, 16) == 0;
        if (skip) {
            if (!skipped_last) fprintf(stderr, "*\n");
        } else {
            fprintf(stderr, "%.8x  ", (unsigned int)offset);
            for (int i = 0; i < 16; i++) {
                if (i < (int)length) {
                    fprintf(stderr, "%.2hhx ", buf[i]);
                } else {
                    fprintf(stderr, "   ");
                }
            }
            fprintf(stderr, "| ");
            for (int i = 0; i < 16; i++) {
                if (i < (int)length) {
                    if (isprint(buf[i])) {
                        fputc(buf[i], stderr);
                    } else {
                        fputc('.', stderr);
                    }
                } else {
                    fputc(' ', stderr);
                }
            }
            fprintf(stderr, "\n");
        }
        skipped_last = skip;

        offset += 16;
        buf += 16;
        length -= 16;
    }

    funlockfile(stderr);
}

// Precise time functions

// These two fields are initialized with current clock values (roughly) at the same moment.
// Since monotonic clock represents time since some arbitrary moment, we need to correlate it
// to some other clock to print time more or less precisely.
//
// Of course that doesn't solve the problem of clocks having different rates.
static struct {
    timespec hi_res_clock;
    time_t low_res_clock;
} time_sync_data;

void initialize_precise_time() {
    int res = clock_gettime(CLOCK_MONOTONIC, &time_sync_data.hi_res_clock);
    guarantee(res == 0, "Failed to get initial monotonic clock value");
    (void) time(&time_sync_data.low_res_clock);
}

timespec get_uptime() {
    timespec now;

    int err = clock_gettime(CLOCK_MONOTONIC, &now);
    rassert_err(err == 0, "Failed to get monotonic clock value");
    if (err == 0) {
        // Compute time difference between now and origin of time
        now.tv_sec -= time_sync_data.hi_res_clock.tv_sec;
        now.tv_nsec -= time_sync_data.hi_res_clock.tv_nsec;
        if (now.tv_nsec < 0) {
            now.tv_nsec += (long long int)1e9;
            now.tv_sec--;
        }
        return now;
    } else {
        // fallback: we can't get nanoseconds value, so we fake it
        time_t now_low_res = time(NULL);
        now.tv_sec = now_low_res - time_sync_data.low_res_clock;
        now.tv_nsec = 0;
        return now;
    }
}

precise_time_t get_absolute_time(const timespec& relative_time) {
    precise_time_t result;
    time_t sec = time_sync_data.low_res_clock + relative_time.tv_sec;
    uint32_t nsec = time_sync_data.hi_res_clock.tv_nsec + relative_time.tv_nsec;
    if (nsec > 1e9) {
        nsec -= (long long int)1e9;
        sec++;
    }
    (void) gmtime_r(&sec, &result);
    result.ns = nsec;
    return result;
}

precise_time_t get_time_now() {
    return get_absolute_time(get_uptime());
}

void format_precise_time(const precise_time_t& time, char* buf, size_t max_chars) {
    int res = snprintf(buf, max_chars,
        "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
        time.tm_year+1900,
        time.tm_mon+1,
        time.tm_mday,
        time.tm_hour,
        time.tm_min,
        time.tm_sec,
        (int) (time.ns/1e3));
    (void) res;
    rassert(0 <= res);
}

std::string format_precise_time(const precise_time_t& time) {
    char buf[formatted_precise_time_length+1];
    format_precise_time(time, buf, sizeof(buf));
    return std::string(buf);
}

#ifndef NDEBUG
void home_thread_mixin_t::assert_thread() const {
    rassert(home_thread() == get_thread_id());
}
#endif

home_thread_mixin_t::home_thread_mixin_t() : real_home_thread(get_thread_id()) { }


on_thread_t::on_thread_t(int thread) {
    coro_t::move_to_thread(thread);
}
on_thread_t::~on_thread_t() {
    coro_t::move_to_thread(home_thread());
}


const repli_timestamp_t repli_timestamp_t::invalid = { -1 };
const repli_timestamp_t repli_timestamp_t::distant_past = { 0 };

microtime_t current_microtime() {
    // This could be done more efficiently, surely.
    struct timeval t;
    int res __attribute__((unused)) = gettimeofday(&t, NULL);
    rassert(0 == res);
    return uint64_t(t.tv_sec) * (1000 * 1000) + t.tv_usec;
}

boost::uuids::uuid generate_uuid() {
    boost::mt19937 number_generator;
    boost::uuids::detail::seed(number_generator);
    /* `boost::uuids::detail::seed()` uses uninitialized memory as one source of
    entropy. This means that Valgrind thinks that `number_generator` contains
    uninitialized contents. Explicitly tell Valgrind that it's OK. */
#ifdef VALGRIND
    VALGRIND_MAKE_MEM_DEFINED_IF_ADDRESSABLE(&number_generator, sizeof(number_generator));
#endif
    boost::uuids::random_generator uuid_generator(number_generator);
    return uuid_generator();
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

#ifndef NDEBUG
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
#endif

/* This object exists only to call srand(time(NULL)) in its constructor, before main() runs. */
struct rand_initter_t {
    rand_initter_t() {
        srand(time(NULL));
    }
} rand_initter;

rng_t::rng_t() {
    memset(&buffer_, 0, sizeof(buffer_));
    srand48_r(314159, &buffer_);
}

int rng_t::randint(int n) {
    long int x;
    lrand48_r(&buffer_, &x);

    return x % n;
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

int gcd(int x, int y) {
    rassert(x >= 0);
    rassert(y >= 0);

    while (y != 0) {
        int tmp = y;
        y = x % y;
        x = tmp;
    }

    return x;
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

std::string vstrprintf(const char *format, va_list ap) {
    boost::scoped_array<char> arr;
    int size;

    va_list aq;
    va_copy(aq, ap);

    // the snprintfs return the number of characters they _would_ have
    // written, not including the '\0', so we use that number to
    // allocate an appropriately sized array.
    char buf[1];
    size = vsnprintf(buf, sizeof(buf), format, ap);

    guarantee_err(size >= 0, "vsnprintf failed, bad format string?");

    arr.reset(new char[size + 1]);

    int newsize = vsnprintf(arr.get(), size + 1, format, aq);
    (void)newsize;
    rassert(newsize == size);

    va_end(aq);

    return std::string(arr.get(), arr.get() + size);
}

std::string strprintf(const char *format, ...) {
    va_list ap;

    std::string ret;

    va_start(ap, format);

    ret = vstrprintf(format, ap);

    va_end(ap);

    return ret;
}
