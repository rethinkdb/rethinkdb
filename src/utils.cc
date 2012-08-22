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

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#endif

#include <boost/tokenizer.hpp>

#include "arch/runtime/runtime.hpp"
#include "config/args.hpp"
#include "containers/archive/archive.hpp"
#include "containers/printf_buffer.hpp"
#include "logger.hpp"
#include "thread_local.hpp"

// fast non-null terminated string comparison
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

    char bd_sample[16] = { 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD,
                           0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD };
    char zero_sample[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char ff_sample[16] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                           0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    bool skipped_last = false;
    while (length > 0) {
        bool skip = length >= 16 && (
                    memcmp(buf, bd_sample, 16) == 0 ||
                    memcmp(buf, zero_sample, 16) == 0 ||
                    memcmp(buf, ff_sample, 16) == 0
                    );
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

void home_thread_mixin_debug_only_t::assert_thread() const {
    rassert(get_thread_id() == real_home_thread);
}

home_thread_mixin_debug_only_t::home_thread_mixin_debug_only_t(int specified_home_thread UNUSED) 
#ifndef NDEBUG
    : real_home_thread(specified_home_thread)
#endif
{ };

home_thread_mixin_debug_only_t::home_thread_mixin_debug_only_t()
#ifndef NDEBUG
    : real_home_thread(get_thread_id())
#endif
{ };

void home_thread_mixin_t::assert_thread() const {
    rassert(home_thread() == get_thread_id());
}

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
    int res __attribute__((unused)) = gettimeofday(&t, NULL);
    rassert(0 == res);
    return uint64_t(t.tv_sec) * (1000 * 1000) + t.tv_usec;
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
    struct timespec t;

    int res = clock_gettime(CLOCK_REALTIME, &t);
    guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");

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

rng_t::rng_t( UNUSED int seed) {
    memset(&buffer_, 0, sizeof(buffer_));
#ifndef NDEBUG
    if (seed == -1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        seed = tv.tv_usec;
    }
    srand48_r(seed, &buffer_);
#else
    srand48_r(314159, &buffer_);
#endif
}

int rng_t::randint(int n) {
    // We return int, and use it in the moduloizer, so the long here
    // (which is necessary for the lrand48_r API) is okay.

    long x;  // NOLINT(runtime/int)
    lrand48_r(&buffer_, &x);

    return x % n;
}

TLS_with_init(bool, rng_initialized, false)
TLS(drand48_data, rng_data)

int randint(int n) {
    drand48_data buffer;
    if (!TLS_get_rng_initialized()) {
        struct timespec ts;
        int res = clock_gettime(CLOCK_REALTIME, &ts);
        guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");
        srand48_r(ts.tv_nsec, &buffer);
        TLS_set_rng_initialized(true);
    } else {
        buffer = TLS_get_rng_data();
    }
    long x;   // NOLINT(runtime/int)
    lrand48_r(&buffer, &x);
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
    CT_ASSERT(sizeof(long) == sizeof(int64_t));  // NOLINT(runtime/int)
    long result = strtol(string, const_cast<char **>(end), base);  // NOLINT(runtime/int)
    if ((result == LONG_MAX || result == LONG_MIN) && errno == ERANGE) {
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
    CT_ASSERT(sizeof(unsigned long) == sizeof(uint64_t));  // NOLINT(runtime/int)
    unsigned long result = strtoul(string, const_cast<char **>(end), base);  // NOLINT(runtime/int)
    if (result == ULONG_MAX && errno == ERANGE) {
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


ticks_t secs_to_ticks(float secs) {
    // The timespec struct used in clock_gettime has a tv_nsec field.
    // That's why we use a billion.
    return ticks_t(secs) * 1000000000L;
}

ticks_t get_ticks() {
    timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

time_t get_secs() {
    timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    return tv.tv_sec;
}

int64_t get_ticks_res() {
    timespec tv;
    clock_getres(CLOCK_MONOTONIC, &tv);
    return int64_t(secs_to_ticks(tv.tv_sec)) + tv.tv_nsec;
}

double ticks_to_secs(ticks_t ticks) {
    return ticks / 1000000000.0;
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
    } while(count == sizeof(buffer));

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
                                                  it++) {
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
