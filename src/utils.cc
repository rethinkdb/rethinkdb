// Copyright 2010-2013 RethinkDB, all rights reserved.
#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include "utils.hpp"

#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef __MACH__
#include <mach/mach_time.h>
#endif

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#endif

#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "arch/runtime/runtime.hpp"
#include "config/args.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/file_stream.hpp"
#include "containers/printf_buffer.hpp"
#include "logger.hpp"
#include "thread_local.hpp"

void run_generic_global_startup_behavior() {
    install_generic_crash_handler();

    rlimit file_limit;
    int res = getrlimit(RLIMIT_NOFILE, &file_limit);
    guarantee_err(res == 0, "getrlimit with RLIMIT_NOFILE failed");

    // We need to set the file descriptor limit maximum to a higher value.  On OS X, rlim_max is
    // RLIM_INFINITY and, with RLIMIT_NOFILE, it's illegal to set rlim_cur to RLIM_INFINITY.  On
    // Linux, maybe the same thing is illegal, but rlim_max is set to a finite value (65K - 1)
    // anyway.  OS X has OPEN_MAX defined to limit the highest possible file descriptor value, and
    // that's what'll end up being the new rlim_cur value.  (The man page on OS X suggested it.)  I
    // don't know if Linux has a similar thing, or other platforms, so we just go with rlim_max, and
    // if we ever see a warning, we'll fix it.  Users can always deal with the problem on their end
    // for a while using ulimit or whatever.)

#ifdef __MACH__
    file_limit.rlim_cur = std::min<rlim_t>(OPEN_MAX, file_limit.rlim_max);
#else
    file_limit.rlim_cur = file_limit.rlim_max;
#endif
    res = setrlimit(RLIMIT_NOFILE, &file_limit);

    if (res != 0) {
        logWRN("The call to set the open file descriptor limit failed (errno = %d - %s)\n",
            errno, errno_string(errno).c_str());
    }

}

// fast-ish non-null terminated string comparison
int sized_strcmp(const uint8_t *str1, int len1, const uint8_t *str2, int len2) {
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

    uint8_t bd_sample[16] = { 0xBD, 0xBD, 0xBD, 0xBD,
                              0xBD, 0xBD, 0xBD, 0xBD,
                              0xBD, 0xBD, 0xBD, 0xBD,
                              0xBD, 0xBD, 0xBD, 0xBD };
    uint8_t zero_sample[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t ff_sample[16] = { 0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff,
                              0xff, 0xff, 0xff, 0xff };

    bool skipped_last = false;
    while (length > 0) {
        bool skip = length >= 16 && (
                    memcmp(buf, bd_sample, 16) == 0 ||
                    memcmp(buf, zero_sample, 16) == 0 ||
                    memcmp(buf, ff_sample, 16) == 0);
        if (skip) {
            if (!skipped_last) fprintf(stderr, "*\n");
        } else {
            fprintf(stderr, "%.8x  ", (unsigned int)offset);
            for (ssize_t i = 0; i < 16; ++i) {
                if (i < length) {
                    fprintf(stderr, "%.2hhx ", buf[i]);
                } else {
                    fprintf(stderr, "   ");
                }
            }
            fprintf(stderr, "| ");
            for (ssize_t i = 0; i < 16; ++i) {
                if (i < length) {
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

void format_time(struct timespec time, append_only_printf_buffer_t *buf) {
    struct tm t;
    struct tm *res1 = localtime_r(&time.tv_sec, &t);
    guarantee_err(res1 == &t, "gmtime_r() failed.");
    buf->appendf(
        "%04d-%02d-%02dT%02d:%02d:%02d.%09ld",
        t.tm_year+1900,
        t.tm_mon+1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        time.tv_nsec);
}

std::string format_time(struct timespec time) {
    printf_buffer_t<formatted_time_length + 1> buf;
    format_time(time, &buf);
    return std::string(buf.c_str());
}

struct timespec parse_time(const std::string &str) THROWS_ONLY(std::runtime_error) {
    struct tm t;
    struct timespec time;
    int res1 = sscanf(str.c_str(),
        "%04d-%02d-%02dT%02d:%02d:%02d.%09ld",
        &t.tm_year,
        &t.tm_mon,
        &t.tm_mday,
        &t.tm_hour,
        &t.tm_min,
        &t.tm_sec,
        &time.tv_nsec);
    if (res1 != 7) {
        throw std::runtime_error("badly formatted time");
    }
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    t.tm_isdst = -1;
    time.tv_sec = mktime(&t);
    if (time.tv_sec == -1) {
        throw std::runtime_error("invalid time");
    }
    return time;
}

#ifndef NDEBUG
void home_thread_mixin_debug_only_t::assert_thread() const {
    rassert(get_thread_id() == real_home_thread);
}
#endif

home_thread_mixin_debug_only_t::home_thread_mixin_debug_only_t(DEBUG_VAR int specified_home_thread) 
#ifndef NDEBUG
    : real_home_thread(specified_home_thread)
#endif
{ }

home_thread_mixin_debug_only_t::home_thread_mixin_debug_only_t()
#ifndef NDEBUG
    : real_home_thread(get_thread_id())
#endif
{ }

#ifndef NDEBUG
void home_thread_mixin_t::assert_thread() const {
    rassert(home_thread() == get_thread_id());
}
#endif

home_thread_mixin_t::home_thread_mixin_t(int specified_home_thread)
    : real_home_thread(specified_home_thread) {
    assert_good_thread_id(specified_home_thread);
}
home_thread_mixin_t::home_thread_mixin_t()
    : real_home_thread(get_thread_id()) { }


on_thread_t::on_thread_t(int thread) {
    coro_t::move_to_thread(thread);
}
on_thread_t::~on_thread_t() {
    coro_t::move_to_thread(home_thread());
}

microtime_t current_microtime() {
    // This could be done more efficiently, surely.
    struct timeval t;
    DEBUG_VAR int res = gettimeofday(&t, NULL);
    rassert(0 == res);
    return uint64_t(t.tv_sec) * MILLION + t.tv_usec;
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

void debug_print_quoted_string(append_only_printf_buffer_t *buf, const uint8_t *s, size_t n) {
    buf->appendf("\"");
    for (size_t i = 0; i < n; ++i) {
        uint8_t ch = s[i];

        switch (ch) {
        case '\"':
            buf->appendf("\\\"");
            break;
        case '\\':
            buf->appendf("\\\\");
            break;
        case '\n':
            buf->appendf("\\n");
            break;
        case '\t':
            buf->appendf("\\t");
            break;
        case '\r':
            buf->appendf("\\r");
            break;
        default:
            if (ch <= '~' && ch >= ' ') {
                // ASCII dependency here
                buf->appendf("%c", ch);
            } else {
                const char *table = "0123456789ABCDEF";
                buf->appendf("\\x%c%c", table[ch / 16], table[ch % 16]);
            }
            break;
        }
    }
    buf->appendf("\"");
}

#ifndef NDEBUG
// Adds the time/thread id prefix to buf.
void debugf_prefix_buf(printf_buffer_t<1000> *buf) {
    struct timespec t = clock_realtime();

    format_time(t, buf);

    buf->appendf(" Thread %d: ", get_thread_id());
}

void debugf_dump_buf(printf_buffer_t<1000> *buf) {
    // Writing a single buffer in one shot like this makes it less
    // likely that stderr debugfs and stdout printfs get mixed
    // together, and probably makes it faster too.  (We can't simply
    // flockfile both stderr and stdout because there's no established
    // rule about which one should be locked first.)
    size_t nitems = fwrite(buf->data(), 1, buf->size(), stderr);
    guarantee_err(nitems == size_t(buf->size()), "trouble writing to stderr");
    int res = fflush(stderr);
    guarantee_err(res == 0, "fflush(stderr) failed");
}

void debugf(const char *msg, ...) {
    printf_buffer_t<1000> buf;
    debugf_prefix_buf(&buf);

    va_list ap;
    va_start(ap, msg);

    buf.vappendf(msg, ap);

    va_end(ap);

    debugf_dump_buf(&buf);
}

#endif

void debug_print(append_only_printf_buffer_t *buf, uint64_t x) {
    buf->appendf("%" PRIu64, x);
}

void debug_print(append_only_printf_buffer_t *buf, const std::string& s) {
    const char *data = s.data();
    debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(data), s.size());
}

debugf_in_dtor_t::debugf_in_dtor_t(const char *msg, ...) {
    va_list arguments;
    va_start(arguments, msg);
    message = vstrprintf(msg, arguments);
    va_end(arguments);
}

debugf_in_dtor_t::~debugf_in_dtor_t() {
    debugf("%s", message.c_str());
}

rng_t::rng_t(int seed) {
#ifndef NDEBUG
    if (seed == -1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        seed = tv.tv_usec;
    }
#else
    seed = 314159;
#endif
    xsubi[2] = seed / (1 << 16);
    xsubi[1] = seed % (1 << 16);
    xsubi[0] = 0x330E;
}

int rng_t::randint(int n) {
    guarantee(n > 0, "non-positive argument for randint's [0,n) interval");
    long x = nrand48(xsubi);  // NOLINT(runtime/int)
    return x % n;
}

struct nrand_xsubi_t {
    unsigned short xsubi[3];  // NOLINT(runtime/int)
};

TLS_with_init(bool, rng_initialized, false)
TLS(nrand_xsubi_t, rng_data)

void get_dev_urandom(void *out, int64_t nbytes) {
    blocking_read_file_stream_t urandom;
    guarantee(urandom.init("/dev/urandom"), "failed to open /dev/urandom to initialize thread rng");
    int64_t readres = force_read(&urandom, out, nbytes);
    guarantee(readres == nbytes);
}

int randint(int n) {
    nrand_xsubi_t buffer;
    if (!TLS_get_rng_initialized()) {
        CT_ASSERT(sizeof(buffer.xsubi) == 6);
        get_dev_urandom(&buffer.xsubi, sizeof(buffer.xsubi));
        TLS_set_rng_initialized(true);
    } else {
        buffer = TLS_get_rng_data();
    }
    long x = nrand48(buffer.xsubi);  // NOLINT(runtime/int)
    TLS_set_rng_data(buffer);
    return x % n;
}

std::string rand_string(int len) {
    std::string res;

    int seed = randint(RAND_MAX);

    while (len --> 0) {
        res.push_back((seed % 26) + 'A');
        seed ^= seed >> 17;
        seed += seed << 11;
        seed ^= seed >> 29;
    }

    return res;
}

bool begins_with_minus(const char *string) {
    while (isspace(*string)) string++;
    return *string == '-';
}

int64_t strtoi64_strict(const char *string, const char **end, int base) {
    CT_ASSERT(sizeof(long long) == sizeof(int64_t));  // NOLINT(runtime/int)
    long long result = strtoll(string, const_cast<char **>(end), base);  // NOLINT(runtime/int)
    if ((result == LLONG_MAX || result == LLONG_MIN) && errno == ERANGE) {
        *end = string;
        return 0;
    }
    return result;
}

uint64_t strtou64_strict(const char *string, const char **end, int base) {
    if (begins_with_minus(string)) {
        *end = string;
        return 0;
    }
    CT_ASSERT(sizeof(unsigned long long) == sizeof(uint64_t));  // NOLINT(runtime/int)
    unsigned long long result = strtoull(string, const_cast<char **>(end), base);  // NOLINT(runtime/int)
    if (result == ULLONG_MAX && errno == ERANGE) {
        *end = string;
        return 0;
    }
    return result;
}

bool strtoi64_strict(const std::string &str, int base, int64_t *out_result) {
    const char *end;
    int64_t result = strtoi64_strict(str.c_str(), &end,  base);
    if (end != str.c_str() + str.length() || (result == 0 && end == str.c_str())) {
        *out_result = 0;
        return false;
    } else {
        *out_result = result;
        return true;
    }
}

bool strtou64_strict(const std::string &str, int base, uint64_t *out_result) {
    const char *end;
    uint64_t result = strtou64_strict(str.c_str(), &end,  base);
    if (end != str.c_str() + str.length() || (result == 0 && end == str.c_str())) {
        *out_result = 0;
        return false;
    } else {
        *out_result = result;
        return true;
    }
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

int64_t round_up_to_power_of_two(int64_t x) {
    rassert(x >= 0);

    --x;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;

    rassert(x < INT64_MAX);

    return x + 1;
}

ticks_t secs_to_ticks(time_t secs) {
    return static_cast<ticks_t>(secs) * BILLION;
}

#ifdef __MACH__
__thread mach_timebase_info_data_t mach_time_info;
#endif  // __MACH__

timespec clock_monotonic() {
#ifdef __MACH__
    if (mach_time_info.denom == 0) {
        mach_timebase_info(&mach_time_info);
        guarantee(mach_time_info.denom != 0);
    }
    const uint64_t t = mach_absolute_time();
    uint64_t nanosecs = t * mach_time_info.numer / mach_time_info.denom;
    timespec ret;
    ret.tv_sec = nanosecs / BILLION;
    ret.tv_nsec = nanosecs % BILLION;
    return ret;
#else
    timespec ret;
    int res = clock_gettime(CLOCK_MONOTONIC, &ret);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTIC, ...) failed");
    return ret;
#endif
}

timespec clock_realtime() {
#ifdef __MACH__
    struct timeval tv;
    int res = gettimeofday(&tv, NULL);
    guarantee_err(res == 0, "gettimeofday failed");

    timespec ret;
    ret.tv_sec = tv.tv_sec;
    ret.tv_nsec = THOUSAND * tv.tv_usec;
    return ret;
#else
    timespec ret;
    int res = clock_gettime(CLOCK_REALTIME, &ret);
    guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");
    return ret;
#endif
}


ticks_t get_ticks() {
    timespec tv = clock_monotonic();
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

time_t get_secs() {
    timespec tv = clock_realtime();
    return tv.tv_sec;
}

double ticks_to_secs(ticks_t ticks) {
    return ticks / static_cast<double>(BILLION);
}


bool notf(bool x) {
    return !x;
}

std::string vstrprintf(const char *format, va_list ap) {
    printf_buffer_t<500> buf(ap, format);

    return std::string(buf.data(), buf.data() + buf.size());
}

std::string strprintf(const char *format, ...) {
    std::string ret;

    va_list ap;
    va_start(ap, format);

    printf_buffer_t<500> buf(ap, format);

    ret.assign(buf.data(), buf.data() + buf.size());

    va_end(ap);

    return ret;
}

bool hex_to_int(char c, int *out) {
    if (c >= '0' && c <= '9') {
        *out = c - '0';
        return true;
    } else if (c >= 'a' && c <= 'f') {
        *out = c - 'a' + 10;
        return true;
    } else if (c >= 'A' && c <= 'F') {
        *out = c - 'A' + 10;
        return true;
    } else {
        return false;
    }
}

char int_to_hex(int x) {
    rassert(x >= 0 && x < 16);
    if (x < 10) {
        return '0' + x;
    } else {
        return 'A' + x - 10;
    }
}

std::string read_file(const char *path) {
    std::string s;
    FILE *fp = fopen(path, "rb");
    char buffer[4096];
    int count;
    do {
        count = fread(buffer, 1, sizeof(buffer), fp);
        s.append(buffer, buffer + count);
    } while (count == sizeof(buffer));

    rassert(feof(fp));

    fclose(fp);

    return s;
}

static const char * unix_path_separator = "/";

path_t parse_as_path(const std::string &path) {
    path_t res;
    res.is_absolute = (path[0] == unix_path_separator[0]);

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    typedef tokenizer::iterator tok_iterator;

    boost::char_separator<char> sep(unix_path_separator);
    tokenizer tokens(path, sep);

    res.nodes.assign(tokens.begin(), tokens.end());

    return res;
}

std::string render_as_path(const path_t &path) {
    std::string res;
    for (std::vector<std::string>::const_iterator it =  path.nodes.begin();
                                                  it != path.nodes.end();
                                                  ++it) {
        if (it != path.nodes.begin() || path.is_absolute) {
            res += unix_path_separator;
        }
        res += *it;
    }

    return res;
}

std::string sanitize_for_logger(const std::string &s) {
    std::string sanitized = s;
    for (size_t i = 0; i < sanitized.length(); ++i) {
        if (sanitized[i] == '\n' || sanitized[i] == '\t') {
            sanitized[i] = ' ';
        } else if (sanitized[i] < ' ' || sanitized[i] > '~') {
            sanitized[i] = '?';
        }
    }
    return sanitized;
}

std::string errno_string(int errsv) {
    char buf[250];
    const char *errstr = errno_string_maybe_using_buffer(errsv, buf, sizeof(buf));
    return std::string(errstr);
}


// The last thread is a service thread that runs an connection acceptor, a log writer, and possibly
// similar services, and does not run any db code (caches, serializers, etc). The reasoning is that
// when the acceptor (and possibly other utils) get placed on an event queue with the db code, the
// latency for these utils can increase significantly. In particular, it causes timeout bugs in
// clients that expect the acceptor to work faster.
int get_num_db_threads() {
    return get_num_threads() - 1;
}


bool ptr_in_byte_range(const void *p, const void *range_start, size_t size_in_bytes) {
    const uint8_t *p8 = static_cast<const uint8_t *>(p);
    const uint8_t *range8 = static_cast<const uint8_t *>(range_start);
    return range8 <= p8 && p8 < range8 + size_in_bytes;
}

bool range_inside_of_byte_range(const void *p, size_t n_bytes, const void *range_start, size_t size_in_bytes) {
    const uint8_t *p8 = static_cast<const uint8_t *>(p);
    return ptr_in_byte_range(p, range_start, size_in_bytes) &&
        (n_bytes == 0 || ptr_in_byte_range(p8 + n_bytes - 1, range_start, size_in_bytes));
}




// GCC and CLANG are smart enough to optimize out strlen(""), so this works.
// This is the simplist thing I could find that gave warning in all of these
// cases:
// * RETHINKDB_VERSION=
// * RETHINKDB_VERSION=""
// * RETHINKDB_VERSION=1.2
// (the correct case is something like RETHINKDB_VERSION="1.2")
UNUSED static const char _assert_RETHINKDB_VERSION_nonempty = 1/(!!strlen(RETHINKDB_VERSION));
