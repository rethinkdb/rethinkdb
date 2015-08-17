// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "utils.hpp"

#include <math.h>
#include <ftw.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#include <google/protobuf/stubs/common.h>

#include "errors.hpp"
#include <boost/date_time.hpp>

#include "arch/io/disk.hpp"
#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "clustering/administration/main/directory_lock.hpp"
#include "config/args.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/file_stream.hpp"
#include "containers/printf_buffer.hpp"
#include "debug.hpp"
#include "logger.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "thread_local.hpp"

void run_generic_global_startup_behavior() {
    install_generic_crash_handler();
    install_new_oom_handler();

    // Set the locale to C, because some ReQL terms may produce different
    // results in different locales, and we need to avoid data divergence when
    // two servers in the same cluster have different locales.
    setlocale(LC_ALL, "C");

    rlimit file_limit;
    int res = getrlimit(RLIMIT_NOFILE, &file_limit);
    guarantee_err(res == 0, "getrlimit with RLIMIT_NOFILE failed");

    // We need to set the file descriptor limit maximum to a higher value.  On
    // OS X, rlim_max is RLIM_INFINITY and, with RLIMIT_NOFILE, it's illegal to
    // set rlim_cur to RLIM_INFINITY.  On Linux, maybe the same thing is
    // illegal, but rlim_max is set to a finite value (65K - 1) anyway.  OS X
    // has OPEN_MAX defined to limit the highest possible file descriptor value,
    // and that's what'll end up being the new rlim_cur value.  (The man page on
    // OS X suggested it.)  I don't know if Linux has a similar thing, or other
    // platforms, so we just go with rlim_max, and if we ever see a warning,
    // we'll fix it.  Users can always deal with the problem on their end for a
    // while using ulimit or whatever.)

#ifdef __MACH__
    file_limit.rlim_cur = std::min<rlim_t>(OPEN_MAX, file_limit.rlim_max);
#else
    file_limit.rlim_cur = file_limit.rlim_max;
#endif
    res = setrlimit(RLIMIT_NOFILE, &file_limit);

    if (res != 0) {
        logWRN("The call to set the open file descriptor limit failed (errno = %d - %s)\n",
            get_errno(), errno_string(get_errno()).c_str());
    }

}

startup_shutdown_t::startup_shutdown_t() {
    run_generic_global_startup_behavior();
}

startup_shutdown_t::~startup_shutdown_t() {
    google::protobuf::ShutdownProtobufLibrary();
}


void print_hd(const void *vbuf, size_t offset, size_t ulength) {
    flockfile(stderr);

    if (ulength == 0) {
        fprintf(stderr, "(data length is zero)\n");
    }

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

void format_time(struct timespec time, printf_buffer_t *buf, local_or_utc_time_t zone) {
    struct tm t;
    if (zone == local_or_utc_time_t::utc) {
        boost::posix_time::ptime as_ptime = boost::posix_time::from_time_t(time.tv_sec);
        t = boost::posix_time::to_tm(as_ptime);
    } else {
        struct tm *res1;
        res1 = localtime_r(&time.tv_sec, &t);
        guarantee_err(res1 == &t, "localtime_r() failed.");
    }
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

std::string format_time(struct timespec time, local_or_utc_time_t zone) {
    printf_buffer_t buf;
    format_time(time, &buf, zone);
    return std::string(buf.c_str());
}

bool parse_time(const std::string &str, local_or_utc_time_t zone,
                struct timespec *out, std::string *errmsg_out) {
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
        *errmsg_out = "badly formatted time";
        return false;
    }
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    t.tm_isdst = -1;
    if (zone == local_or_utc_time_t::utc) {
        boost::posix_time::ptime as_ptime = boost::posix_time::ptime_from_tm(t);
        boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        /* Apparently `(x-y).total_seconds()` is returning the numeric difference in the
        POSIX timestamps, which approximates the difference in solar time. This is weird
        (I'd expect it to return the difference in atomic time) but it turns out to give
        the correct behavior in this case. */
        time.tv_sec = (as_ptime - epoch).total_seconds();
    } else {
        time.tv_sec = mktime(&t);
        if (time.tv_sec == -1) {
            *errmsg_out = "invalid time";
            return false;
        }
    }
    *out = time;
    return true;
}

with_priority_t::with_priority_t(int priority) {
    rassert(coro_t::self() != NULL);
    previous_priority = coro_t::self()->get_priority();
    coro_t::self()->set_priority(priority);
}
with_priority_t::~with_priority_t() {
    rassert(coro_t::self() != NULL);
    coro_t::self()->set_priority(previous_priority);
}

void *malloc_aligned(size_t size, size_t alignment) {
    void *ptr = NULL;
    int res = posix_memalign(&ptr, alignment, size);  // NOLINT(runtime/rethinkdb_fn)
    if (res != 0) {
        if (res == EINVAL) {
            crash_or_trap("posix_memalign with bad alignment: %zu.", alignment);
        } else if (res == ENOMEM) {
            crash_oom();
        } else {
            crash_or_trap("posix_memalign failed with unknown result: %d.", res);
        }
    }
    return ptr;
}

void *rmalloc(size_t size) {
    void *res = malloc(size);  // NOLINT(runtime/rethinkdb_fn)
    if (res == NULL && size != 0) {
        crash_oom();
    }
    return res;
}

void *rrealloc(void *ptr, size_t size) {
    void *res = realloc(ptr, size);  // NOLINT(runtime/rethinkdb_fn)
    if (res == NULL && size != 0) {
        crash_oom();
    }
    return res;
}

bool risfinite(double arg) {
    // isfinite is a macro on OS X in math.h, so we can't just say std::isfinite.
    using namespace std; // NOLINT(build/namespaces) due to platform variation
    return isfinite(arg);
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

uint64_t rng_t::randuint64(uint64_t n) {
    guarantee(n > 0, "non-positive argument for randint's [0,n) interval");
    uint32_t x_low = jrand48(xsubi);  // NOLINT(runtime/int)
    uint32_t x_high = jrand48(xsubi);  // NOLINT(runtime/int)
    uint64_t x = x_high;
    x <<= 32;
    x += x_low;
    return x % n;
}

double rng_t::randdouble() {
    uint64_t x = rng_t::randuint64(1LL << 53);
    double res = x;
    return res / (1LL << 53);
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
    guarantee(n > 0, "non-positive argument for randint's [0,n) interval");
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

uint64_t randuint64(uint64_t n) {
    guarantee(n > 0, "non-positive argument for randint's [0,n) interval");
    nrand_xsubi_t buffer;
    if (!TLS_get_rng_initialized()) {
        CT_ASSERT(sizeof(buffer.xsubi) == 6);
        get_dev_urandom(&buffer.xsubi, sizeof(buffer.xsubi));
        TLS_set_rng_initialized(true);
    } else {
        buffer = TLS_get_rng_data();
    }
    uint32_t x_low = jrand48(buffer.xsubi);  // NOLINT(runtime/int)
    uint32_t x_high = jrand48(buffer.xsubi);  // NOLINT(runtime/int)
    uint64_t x = x_high;
    x <<= 32;
    x += x_low;
    TLS_set_rng_data(buffer);
    return x % n;
}

size_t randsize(size_t n) {
    guarantee(n > 0, "non-positive argument for randint's [0,n) interval");
    size_t ret = 0;
    size_t i = SIZE_MAX;
    while (i != 0) {
        int x = randint(0x10000);
        ret = ret * 0x10000 + x;
        i /= 0x10000;
    }
    return ret % n;
}

double randdouble() {
    uint64_t x = randuint64(1LL << 53);
    double res = x;
    return res / (1LL << 53);
}

bool begins_with_minus(const char *string) {
    while (isspace(*string)) string++;
    return *string == '-';
}

int64_t strtoi64_strict(const char *string, const char **end, int base) {
    CT_ASSERT(sizeof(long long) == sizeof(int64_t));  // NOLINT(runtime/int)
    long long result = strtoll(string, const_cast<char **>(end), base);  // NOLINT(runtime/int)
    if ((result == LLONG_MAX || result == LLONG_MIN) && get_errno() == ERANGE) {
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
    if (result == ULLONG_MAX && get_errno() == ERANGE) {
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

bool notf(bool x) {
    return !x;
}

std::string vstrprintf(const char *format, va_list ap) {
    printf_buffer_t buf(ap, format);

    return std::string(buf.data(), buf.data() + buf.size());
}

std::string strprintf(const char *format, ...) {
    std::string ret;

    va_list ap;
    va_start(ap, format);

    printf_buffer_t buf(ap, format);

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

bool blocking_read_file(const char *path, std::string *contents_out) {
    scoped_fd_t fd;

    {
        int res;
        do {
            res = open(path, O_RDONLY);
        } while (res == -1 && get_errno() == EINTR);

        if (res == -1) {
            return false;
        }
        fd.reset(res);
    }

    std::string ret;

    char buf[4096];
    for (;;) {
        ssize_t res;
        do {
            res = read(fd.get(), buf, sizeof(buf));
        } while (res == -1 && get_errno() == EINTR);

        if (res == -1) {
            return false;
        }

        if (res == 0) {
            *contents_out = ret;
            return true;
        }

        ret.append(buf, buf + res);
    }
}

std::string blocking_read_file(const char *path) {
    std::string ret;
    bool success = blocking_read_file(path, &ret);
    guarantee(success);
    return ret;
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

int remove_directory_helper(const char *path, UNUSED const struct stat *ptr,
                            UNUSED const int flag, UNUSED FTW *ftw) {
    logNTC("In recursion: removing file %s\n", path);
    int res = ::remove(path);
    guarantee_err(res == 0, "Fatal error: failed to delete '%s'.", path);
    return 0;
}

void remove_directory_recursive(const char *path) {
    // max_openfd is ignored on OS X (which claims the parameter
    // specifies the maximum traversal depth) and used by Linux to
    // limit the number of file descriptors that are open (by opening
    // and closing directories extra times if it needs to go deeper
    // than that).
    const int max_openfd = 128;
    logNTC("Recursively removing directory %s\n", path);
    int res = nftw(path, remove_directory_helper, max_openfd, FTW_PHYS | FTW_MOUNT | FTW_DEPTH);
    guarantee_err(res == 0 || get_errno() == ENOENT, "Trouble while traversing and destroying temporary directory %s.", path);
}

base_path_t::base_path_t(const std::string &path) : path_(path) { }

void base_path_t::make_absolute() {
    char absolute_path[PATH_MAX];
    char *res = realpath(path_.c_str(), absolute_path);
    guarantee_err(res != NULL, "Failed to determine absolute path for '%s'", path_.c_str());
    path_.assign(absolute_path);
}

const std::string& base_path_t::path() const {
    guarantee(!path_.empty());
    return path_;
}

std::string temporary_directory_path(const base_path_t& base_path) {
    return base_path.path() + "/tmp";
}

bool is_rw_directory(const base_path_t& path) {
    if (access(path.path().c_str(), R_OK | F_OK | W_OK) != 0)
        return false;
    struct stat details;
    if (stat(path.path().c_str(), &details) != 0)
        return false;
    return (details.st_mode & S_IFDIR) > 0;
}

void recreate_temporary_directory(const base_path_t& base_path) {
    const base_path_t path(temporary_directory_path(base_path));

    if (is_rw_directory(path) && check_dir_emptiness(path))
        return;
    remove_directory_recursive(path.path().c_str());

    int res;
    do {
        res = mkdir(path.path().c_str(), 0755);
    } while (res == -1 && get_errno() == EINTR);
    guarantee_err(res == 0, "mkdir of temporary directory %s failed",
                  path.path().c_str());

    // Call fsync() on the parent directory to guarantee that the newly
    // created directory's directory entry is persisted to disk.
    warn_fsync_parent_directory(path.path().c_str());
}

// GCC and CLANG are smart enough to optimize out strlen(""), so this works.
// This is the simplist thing I could find that gave warning in all of these
// cases:
// * RETHINKDB_VERSION=
// * RETHINKDB_VERSION=""
// * RETHINKDB_VERSION=1.2
// (the correct case is something like RETHINKDB_VERSION="1.2")
UNUSED static const char _assert_RETHINKDB_VERSION_nonempty = 1/(!!strlen(RETHINKDB_VERSION));
