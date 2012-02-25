#include "utils.hpp"

#include "errors.hpp"
#include <execinfo.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <cxxabi.h>
#include "arch/arch.hpp"
#include "db_thread_info.hpp"
#include "containers/scoped_malloc.hpp"

// fast non-null terminated string comparison
int sized_strcmp(const char *str1, int len1, const char *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0)
        res = len1-len2;
    return res;
}

std::string strip_spaces(std::string str) {
    while (str[str.length() - 1] == ' ')
        str.erase(str.length() - 1, 1);

    while (str[0] == ' ')
        str.erase(0, 1);

    return str;
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
                if (i < (int)length) fprintf(stderr, "%.2hhx ", buf[i]);
                else fprintf(stderr, "   ");
            }
            fprintf(stderr, "| ");
            for (int i = 0; i < 16; i++) {
                if (i < (int)length) {
                    if (isprint(buf[i])) fputc(buf[i], stderr);
                    else fputc('.', stderr);
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

