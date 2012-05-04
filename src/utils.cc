#include "utils.hpp"
#include <boost/tokenizer.hpp>

#include <cxxabi.h>
#include <execinfo.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "arch/runtime/runtime.hpp"
#include "config/args.hpp"
#include "containers/archive/archive.hpp"
#include "containers/printf_buffer.hpp"
#include "containers/scoped_malloc.hpp"
#include "db_thread_info.hpp"
#include "logger.hpp"

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#endif

write_message_t &operator<<(write_message_t &msg, repli_timestamp_t tstamp) {
    return msg << tstamp.time;
}

MUST_USE int deserialize(read_stream_t *s, repli_timestamp_t *tstamp) {
    return deserialize(s, &tstamp->time);
}



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
        bool skip = length >= 16 && (
                    memcmp(buf, bd_sample, 16) == 0 ||
                    memcmp(buf, zero_sample, 16) == 0 ||
                    memcmp(buf, ff_sample, 16) == 0
                    );
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

void format_time(struct timespec time, char* buf, size_t max_chars) {
    struct tm t;
    struct tm *res1 = localtime_r(&time.tv_sec, &t);
    guarantee_err(res1 == &t, "gmtime_r() failed.");
    int res2 = snprintf(buf, max_chars,
        "%04d-%02d-%02dT%02d:%02d:%02d.%09ld",
        t.tm_year+1900,
        t.tm_mon+1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec,
        time.tv_nsec);
    (void) res2;
    rassert(0 <= res2);
}

std::string format_time(struct timespec time) {
    char buf[formatted_time_length+1];
    format_time(time, buf, sizeof(buf));
    return std::string(buf);
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

void home_thread_mixin_t::assert_thread() const {
    if(home_thread() != get_thread_id()) {
        printf("Homethread: %d current thread:%d\n", home_thread(), get_thread_id());
        BREAKPOINT;
    }
    rassert(home_thread() == get_thread_id());
}
#endif

home_thread_mixin_t::home_thread_mixin_t(int specified_home_thread)
    : real_home_thread(specified_home_thread) { }
home_thread_mixin_t::home_thread_mixin_t()
    : real_home_thread(get_thread_id()) { }


on_thread_t::on_thread_t(int thread) {
    coro_t::move_to_thread(thread);
}
on_thread_t::~on_thread_t() {
    coro_t::move_to_thread(home_thread());
}


const repli_timestamp_t repli_timestamp_t::invalid = { static_cast<uint32_t>(-1) };
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

#ifndef NDEBUG
void debugf(const char *msg, ...) {
    flockfile(stderr);
    char formatted_time[formatted_time_length+1];
    struct timespec t;
    int res = clock_gettime(CLOCK_REALTIME, &t);
    guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");
    format_time(t, formatted_time, sizeof(formatted_time));
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

int randint(int n) {
    return thread_local_randint(n);
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
    CT_ASSERT(sizeof(long) == sizeof(int64_t));
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
    CT_ASSERT(sizeof(unsigned long) == sizeof(uint64_t));
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


static bool parse_backtrace_line(char *line, char **filename, char **function, char **offset, char **address) {
    /*
    backtrace() gives us lines in one of the following two forms:
       ./path/to/the/binary(function+offset) [address]
       ./path/to/the/binary [address]
    */

    *filename = line;

    // Check if there is a function present
    if (char *paren1 = strchr(line, '(')) {
        char *paren2 = strchr(line, ')');
        if (!paren2) return false;
        *paren1 = *paren2 = '\0';   // Null-terminate the offset and the filename
        *function = paren1 + 1;
        char *plus = strchr(*function, '+');
        if (!plus) return false;
        *plus = '\0';   // Null-terminate the function name
        *offset = plus + 1;
        line = paren2 + 1;
        if (*line != ' ') return false;
        line += 1;
    } else {
        *function = NULL;
        *offset = NULL;
        char *bracket = strchr(line, '[');
        if (!bracket) return false;
        line = bracket - 1;
        if (*line != ' ') return false;
        *line = '\0';   // Null-terminate the file name
        line += 1;
    }

    // We are now at the opening bracket of the address
    if (*line != '[') return false;
    line += 1;
    *address = line;
    line = strchr(line, ']');
    if (!line || line[1] != '\0') return false;
    *line = '\0';   // Null-terminate the address

    return true;
}

/* There has been some trouble with abi::__cxa_demangle.

Originally, demangle_cpp_name() took a pointer to the mangled name, and returned a
buffer that must be free()ed. It did this by calling __cxa_demangle() and passing NULL
and 0 for the buffer and buffer-size arguments.

There were complaints that print_backtrace() was smashing memory. Shachaf observed that
pieces of the backtrace seemed to be ending up overwriting other structs, and filed
issue #100.

Daniel Mewes suspected that the memory smashing was related to calling malloc().
In December 2010, he changed demangle_cpp_name() to take a static buffer, and fill
this static buffer with the demangled name. See 284246bd.

abi::__cxa_demangle expects a malloc()ed buffer, and if the buffer is too small it
will call realloc() on it. So the static-buffer approach worked except when the name
to be demangled was too large.

In March 2011, Tim and Ivan got tired of the memory allocator complaining that someone
was trying to realloc() an unallocated buffer, and changed demangle_cpp_name() back
to the way it was originally.

Please don't change this function without talking to the people who have already
been involved in this. */

#include <cxxabi.h>

std::string demangle_cpp_name(const char *mangled_name) {
    int res;
    char *name_as_c_str = abi::__cxa_demangle(mangled_name, NULL, 0, &res);
    if (res == 0) {
        std::string name_as_std_string(name_as_c_str);
        free(name_as_c_str);
        return name_as_std_string;
    } else {
        throw demangle_failed_exc_t();
    }
}

static bool run_addr2line(char *executable, char *address, char *line, int line_size) {
    // Generate and run addr2line command
    char cmd_buf[255] = {0};
    snprintf(cmd_buf, sizeof(cmd_buf), "addr2line -s -e %s %s", executable, address);
    FILE *fline = popen(cmd_buf, "r");
    if (!fline) return false;

    int count = fread(line, sizeof(char), line_size - 1, fline);
    pclose(fline);
    if (count == 0) return false;

    if (line[count-1] == '\n') {
        line[count-1] = '\0';
    } else {
        line[count] = '\0';
    }

    if (strcmp(line, "??:0") == 0) return false;

    return true;
}

void print_backtrace(FILE *out, bool use_addr2line) {
    // Get a backtrace
    static const int max_frames = 100;
    void *stack_frames[max_frames];
    int size = backtrace(stack_frames, max_frames);
    char **symbols = backtrace_symbols(stack_frames, size);

    if (symbols) {
        for (int i = 0; i < size; i ++) {
            // Parse each line of the backtrace
            scoped_malloc<char> line(symbols[i], symbols[i] + (strlen(symbols[i]) + 1));
            char *executable, *function, *offset, *address;

            fprintf(out, "%d: ", i+1);

            if (!parse_backtrace_line(line.get(), &executable, &function, &offset, &address)) {
                fprintf(out, "%s\n", symbols[i]);
            } else {
                if (function) {
                    try {
                        std::string demangled = demangle_cpp_name(function);
                        fprintf(out, "%s", demangled.c_str());
                    } catch (demangle_failed_exc_t) {
                        fprintf(out, "%s+%s", function, offset);
                    }
                } else {
                    fprintf(out, "?");
                }

                fprintf(out, " at ");

                char line[255] = {0};
                if (use_addr2line && run_addr2line(executable, address, line, sizeof(line))) {
                    fprintf(out, "%s", line);
                } else {
                    fprintf(out, "%s (%s)", address, executable);
                }

                fprintf(out, "\n");
            }

        }

        free(symbols);
    } else {
        fprintf(out, "(too little memory for backtrace)\n");
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

path_t parse_as_path(const std::string &path) {
    debugf("Path: %s\n", path.c_str());
    path_t res;
    if (path[0] == '/') {
        res.is_absolute = true;
    } else {
        res.is_absolute = false;
    }
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    typedef tokenizer::iterator tok_iterator;

    boost::char_separator<char> sep("/");
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
            res += "/";
        }
        res += *it;
    }

    return res;
}
