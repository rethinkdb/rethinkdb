// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/logs/log_writer.hpp"

#include <math.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"
#include "arch/io/disk/filestat.hpp"
#include "arch/io/disk.hpp"
#include "concurrency/promise.hpp"
#include "containers/scoped.hpp"
#include "thread_local.hpp"


RDB_IMPL_SERIALIZABLE_4_SINCE_v1_13(log_message_t, timestamp, uptime, level, message);

// `struct timestamp` has platform-dependent integers, which is not good for
// serialization. We serialize them as fixed types that are compatible with the types
// we have on AMD64 Linux.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#endif
template <cluster_version_t W>
void serialize(write_message_t *wm, const struct timespec &ts) {
    static_assert(
        std::numeric_limits<decltype(ts.tv_sec)>::min()
            >= std::numeric_limits<int64_t>::min()
        && std::numeric_limits<decltype(ts.tv_sec)>::max()
            <= std::numeric_limits<int64_t>::max(),
        "incompatible timespec type");
    serialize<W>(wm, static_cast<int64_t>(ts.tv_sec));
    static_assert(
        std::numeric_limits<decltype(ts.tv_nsec)>::min()
            >= std::numeric_limits<int64_t>::min()
        && std::numeric_limits<decltype(ts.tv_nsec)>::max()
            <= std::numeric_limits<int64_t>::max(),
        "incompatible timespec type");
    serialize<W>(wm, static_cast<int64_t>(ts.tv_nsec));
}
template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, struct timespec *ts_out) {
    archive_result_t res = archive_result_t::SUCCESS;

    int64_t tv_sec;
    res = deserialize<W>(s, &tv_sec);
    if (bad(res)) { return res; }
    if(!(tv_sec >= std::numeric_limits<decltype(ts_out->tv_sec)>::min()
         && tv_sec <= std::numeric_limits<decltype(ts_out->tv_sec)>::max())) {
        return archive_result_t::RANGE_ERROR;
    }
    ts_out->tv_sec = tv_sec;

    int64_t tv_nsec;
    res = deserialize<W>(s, &tv_nsec);
    if (bad(res)) { return res; }
    if(!(tv_nsec >= std::numeric_limits<decltype(ts_out->tv_nsec)>::min()
         && tv_nsec <= std::numeric_limits<decltype(ts_out->tv_nsec)>::max())) {
        return archive_result_t::RANGE_ERROR;
    }
    ts_out->tv_nsec = tv_nsec;

    return res;
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(struct timespec);

std::string format_log_level(log_level_t l) {
    switch (l) {
        case log_level_debug: return "debug";
        case log_level_info: return "info";
        case log_level_notice: return "notice";
        case log_level_warn: return "warn";
        case log_level_error: return "error";
        default: unreachable();
    }
}

log_level_t parse_log_level(const std::string &s) THROWS_ONLY(log_read_exc_t) {
    if (s == "debug")
        return log_level_debug;
    else if (s == "info")
        return log_level_info;
    else if (s == "notice")
        return log_level_notice;
    else if (s == "warn")
        return log_level_warn;
    else if (s == "error")
        return log_level_error;
    else
        throw log_read_exc_t("cannot parse '" + s + "' as log level");
}

std::string format_log_message(const log_message_t &m, bool for_console) {
    // never write an info level message to console
    guarantee(!(for_console && m.level == log_level_info));

    std::string message = m.message;
    std::string message_reformatted;

    std::string prepend;
    if (!for_console) {
        prepend = strprintf("%s %ld.%06llds %s: ",
                            format_time(m.timestamp, local_or_utc_time_t::local).c_str(),
                            m.uptime.tv_sec,
                            m.uptime.tv_nsec / THOUSAND,
                            format_log_level(m.level).c_str());
    } else {
        if (m.level != log_level_info && m.level != log_level_notice) {
            prepend = strprintf("%s: ", format_log_level(m.level).c_str());
        }
    }
    ssize_t prepend_length = prepend.length();

    ssize_t start = 0, end = message.length() - 1;
    while (start < static_cast<ssize_t>(message.length()) && message[start] == '\n') {
        ++start;
    }
    while (end >= 0 && (message[end] == '\n' || message[end] == '\r')) {
        end--;
    }
    for (int i = start; i <= end; i++) {
        if (message[i] == '\n') {
            if (for_console) {
                message_reformatted.push_back('\n');
                message_reformatted.append(prepend_length, ' ');
            } else {
                message_reformatted.append("\\n");
            }
        } else if (message[i] == '\t') {
            if (for_console) {
                message_reformatted.push_back('\t');
            } else {
                message_reformatted.append("\\t");
            }
        } else if (message[i] == '\\') {
            if (for_console) {
                message_reformatted.push_back(message[i]);
            } else {
                message_reformatted.append("\\\\");
            }
        } else if (message[i] < ' ' || message[i] > '~') {
#if !defined(NDEBUG)
             crash("We can't have special characters in log messages because then it "
                   "would be difficult to parse the log file. Message: %s",
                   message.c_str());
#else
            message_reformatted.push_back('?');
#endif
        } else {
            message_reformatted.push_back(message[i]);
        }
    }

    return prepend + message_reformatted;
}



log_message_t parse_log_message(const std::string &s) THROWS_ONLY(log_read_exc_t) {
    const char *p = s.c_str();
    const char *start_timestamp = p;
    while (*p && *p != ' ') p++;
    if (!*p) throw log_read_exc_t("cannot parse log message (1)");
    const char *end_timestamp = p;
    p++;
    const char *start_uptime_ipart = p;
    while (*p && *p != '.') p++;
    if (!*p) throw log_read_exc_t("cannot parse log message (2)");
    const char *end_uptime_ipart = p;
    p++;
    const char *start_uptime_fpart = p;
    while (*p && *p != 's') p++;
    if (!*p) throw log_read_exc_t("cannot parse log message (3)");
    const char *end_uptime_fpart = p;
    p++;
    if (*p != ' ') throw log_read_exc_t("cannot parse log message (4)");
    p++;
    const char *start_level = p;
    while (*p && *p != ':') p++;
    if (*p != ':') throw log_read_exc_t("cannot parse log message (5)");
    const char *end_level = p;
    p++;
    if (*p != ' ') throw log_read_exc_t("cannot parse log message (6)");
    p++;
    const char *start_message = p;
    while (*p) p++;
    const char *end_message = p;

    struct timespec timestamp;
    {
        std::string errmsg;
        if (!parse_time(std::string(start_timestamp, end_timestamp - start_timestamp),
                        local_or_utc_time_t::local, &timestamp, &errmsg)) {
            throw log_read_exc_t(errmsg);
        }
    }
    struct timespec uptime;

    {
        std::string tv_sec_str(start_uptime_ipart, end_uptime_ipart - start_uptime_ipart);
        uint64_t tv_sec;
        if (!strtou64_strict(tv_sec_str, 10, &tv_sec)) {
            throw log_read_exc_t("cannot parse log message (9)");
        }

        uptime.tv_sec = tv_sec;

        std::string tv_nsec_str(start_uptime_fpart, end_uptime_fpart - start_uptime_fpart);
        uint64_t tv_nsec;
        if (!strtou64_strict(tv_nsec_str, 10, &tv_nsec)) {
            throw log_read_exc_t("cannot parse log message (10)");
        }

        // TODO: Seriously?  We assume three decimal places?
        uptime.tv_nsec = THOUSAND * tv_nsec;
    }

    log_level_t level = parse_log_level(std::string(start_level, end_level - start_level));
    std::string message = std::string(start_message, end_message - start_message);

    return log_message_t(timestamp, uptime, level, message);
}

void throw_unless(bool condition, const std::string &where) {
    if (!condition) {
        throw log_read_exc_t("file IO error: " + where + " (errno = " + errno_string(get_errno()).c_str() + ")");
    }
}

file_reverse_reader_t::file_reverse_reader_t(scoped_fd_t &&_fd) :
        fd(std::move(_fd)),
        current_chunk(chunk_size) {
    int64_t fd_filesize = get_file_size(fd.get());
    if (fd_filesize == 0) {
        remaining_in_current_chunk = current_chunk_start = 0;
    } else {
        remaining_in_current_chunk = fd_filesize % chunk_size;
        if (remaining_in_current_chunk == 0) {
            /* We landed right on a chunk boundary; set ourself to read the
            previous whole chunk. */
            remaining_in_current_chunk = chunk_size;
        }
        current_chunk_start = fd_filesize - remaining_in_current_chunk;
        int res = pread(fd.get(), current_chunk.data(), remaining_in_current_chunk, current_chunk_start);
        throw_unless(res == remaining_in_current_chunk, "could not read from file");

        // Ignore the newline and any unfinished statement at the end of the file
        while (current_chunk[remaining_in_current_chunk - 1] != '\n') {
            --remaining_in_current_chunk;
            if (remaining_in_current_chunk == 0) {
                read_chunk();
            }
        }
        --remaining_in_current_chunk;
        if (remaining_in_current_chunk == 0) {
            read_chunk();
        }
    }
}

bool file_reverse_reader_t::get_next(std::string *out) {
    if (remaining_in_current_chunk == 0) {
        guarantee(current_chunk_start == 0);
        return false;
    }

    *out = "";
    while (true) {
        for (int p = remaining_in_current_chunk - 1; p >= 0; --p) {
            if (current_chunk[p] == '\n') {
                *out = std::string(&current_chunk[p] + 1, remaining_in_current_chunk - p - 1) + *out;

                remaining_in_current_chunk = p;
                if (remaining_in_current_chunk == 0) {
                    read_chunk();
                }
                return true;
            }
        }

        *out = std::string(current_chunk.data(), remaining_in_current_chunk) + *out;
        remaining_in_current_chunk = 0;
        if (!read_chunk()) {
            return true; // This is the last (first) line
        }
    }
}

bool file_reverse_reader_t::read_chunk() {
    if (current_chunk_start == 0) {
        return false;
    }

    current_chunk_start -= chunk_size;
    int res = pread(fd.get(), current_chunk.data(), chunk_size, current_chunk_start);
    throw_unless(res == chunk_size, "could not read from file");
    remaining_in_current_chunk = chunk_size;
    return true;
}

/* Most of the logging we do will be through thread_pool_log_writer_t. However,
thread_pool_log_writer_t depends on the existence of a thread pool, which is
not always the case. Thus, fallback_log_writer_t exists to perform logging
operations when thread_pool_log_writer_t cannot be used. */

class fallback_log_writer_t {
public:
    fallback_log_writer_t();
    void install(const std::string &logfile_name);
    friend class thread_pool_log_writer_t;

    log_message_t assemble_log_message(log_level_t level, const std::string &m);

private:
    friend void log_coro(thread_pool_log_writer_t *writer, log_level_t level, const std::string &message, auto_drainer_t::lock_t);
    friend void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...);
    friend void vlog_internal(const char *src_file, int src_line, log_level_t level, const char *format, va_list args);

    bool write(const log_message_t &msg, std::string *error_out);
    void initiate_write(log_level_t level, const std::string &message);
    base_path_t filename;
    struct timespec uptime_reference;
    struct timespec last_msg_timestamp;
    spinlock_t last_msg_timestamp_lock;

#ifdef _WIN32
    // TODO WINDOWS: log locking
#else
    struct flock filelock, fileunlock;
#endif
    scoped_fd_t fd;

    DISABLE_COPYING(fallback_log_writer_t);
} fallback_log_writer;

fallback_log_writer_t::fallback_log_writer_t() :
    filename("-") {
    uptime_reference = clock_monotonic();
    last_msg_timestamp = clock_realtime();

#ifndef _WIN32
    filelock.l_type = F_WRLCK;
    filelock.l_whence = SEEK_SET;
    filelock.l_start = 0;
    filelock.l_len = 0;
    filelock.l_pid = getpid();

    fileunlock.l_type = F_UNLCK;
    fileunlock.l_whence = SEEK_SET;
    fileunlock.l_start = 0;
    fileunlock.l_len = 0;
    fileunlock.l_pid = getpid();
#endif
}

void fallback_log_writer_t::install(const std::string &logfile_name) {
    guarantee(filename.path() == "-", "Attempted to install a fallback_log_writer_t that was already installed.");
    filename = base_path_t(logfile_name);

#ifdef _WIN32
    HANDLE h = CreateFile(filename.path().c_str(), FILE_APPEND_DATA, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    fd.reset(h);

    if (fd.get() == INVALID_FD) {
        throw std::runtime_error(strprintf("Failed to open log file '%s': %s",
                                           logfile_name.c_str(),
                                           winerr_string(GetLastError()).c_str()).c_str());
    }
#else
    int res;
    do {
        res = open(filename.path().c_str(), O_WRONLY|O_APPEND|O_CREAT, 0644);
    } while (res == INVALID_FD && get_errno() == EINTR);

    fd.reset(res);

    if (fd.get() == INVALID_FD) {
        throw std::runtime_error(strprintf("Failed to open log file '%s': %s",
                                           logfile_name.c_str(),
                                           errno_string(errno).c_str()).c_str());
    }
#endif

    // Get the absolute path for the log file, so it will still be valid if
    //  the working directory changes
    filename.make_absolute();

    // For the case that the log file was newly created,
    // call fsync() on the parent directory to guarantee that its
    // directory entry is persisted to disk.
    int sync_res = fsync_parent_directory(filename.path().c_str());
    if (sync_res != 0) {
        char errno_str_buf[250];
        const char *errno_str = errno_string_maybe_using_buffer(sync_res,
            errno_str_buf, sizeof(errno_str_buf));
        logWRN("Parent directory of log file (%s) could not be synced. (%s)\n",
            filename.path().c_str(), errno_str);
    }
}

log_message_t fallback_log_writer_t::assemble_log_message(
        log_level_t level, const std::string &m) {
    struct timespec timestamp = clock_realtime();

    {
        /* Make sure timestamps on log messages are unique. */

        /* We grab the spinlock to avoid races if multiple threads get here at once.
        There's usually no possibility of lock contention because log messages
        originating from within the thread pool will all get sent to thread 0 before
        being written; but the `log*()` functions are supposed to work even outside of
        the thread pool, including in the blocker pool. */
        spinlock_acq_t lock_acq(&last_msg_timestamp_lock);
        struct timespec last_plus = last_msg_timestamp;
        add_to_timespec(&last_plus, 1);
        if (last_plus > timestamp) {
            timestamp = last_plus;
        }
        last_msg_timestamp = timestamp;
    }

    struct timespec uptime = subtract_timespecs(clock_monotonic(), uptime_reference);

    return log_message_t(timestamp, uptime, level, m);
}

// WINDOWS TODO: this function could benefit from some refactoring
bool fallback_log_writer_t::write(const log_message_t &msg, std::string *error_out) {
    std::string formatted = format_log_message(msg) + "\n";

#ifdef _MSC_VER
    static int STDOUT_FILENO = -1;
    static int STDERR_FILENO = -1;
    if (STDOUT_FILENO == -1) {
        STDOUT_FILENO = _open("conout$", _O_RDONLY, 0);
        STDERR_FILENO = STDOUT_FILENO;
    }
#endif

    FILE* write_stream = nullptr;
    int fileno = -1;
    switch (msg.level) {
        case log_level_info:
            // no message on stdout/stderr
            break;
        case log_level_notice:
            write_stream = stdout;
            fileno = STDOUT_FILENO;
            break;
        case log_level_debug:
        case log_level_warn:
        case log_level_error:
            write_stream = stderr;
            fileno = STDERR_FILENO;
            break;
        default:
            unreachable();
    }

    if (msg.level != log_level_info) {
        // Write to stdout/stderr for all log levels but info (#3040)
        std::string console_formatted = format_log_message(msg, true) + "\n";
#ifdef _WIN32
        // WINDOWS TODO
        (void) write_stream;
#else
        flockfile(write_stream);
#endif

#ifdef _WIN32
        size_t write_res = fwrite(console_formatted.data(), 1, console_formatted.length(), stderr);
#else
        ssize_t write_res = ::write(fileno, console_formatted.data(), console_formatted.length());
#endif
        if (write_res != static_cast<decltype(write_res)>(console_formatted.length())) {
            error_out->assign("cannot write to stdout/stderr: " + errno_string(get_errno()));
            return false;
        }


#ifdef _WIN32
        // WINDOWS TODO
#else
        int fsync_res = fsync(fileno);
        if (fsync_res != 0 && !(get_errno() == EROFS || get_errno() == EINVAL ||
                get_errno() == ENOTSUP)) {
            error_out->assign("cannot flush stdout/stderr: " + errno_string(get_errno()));
            return false;
        }

        funlockfile(write_stream);
#endif
    }

    if (fd.get() == INVALID_FD) {
        error_out->assign("logging module is not yet initialized");
        return false;
    }
#ifndef _WIN32
    int fcntl_res = fcntl(fd.get(), F_SETLKW, &filelock);
    if (fcntl_res != 0) {
        error_out->assign("cannot lock log file: " + errno_string(get_errno()));
        return false;
    }
#endif

#ifdef _WIN32
    DWORD bytes_written;
    BOOL res = WriteFile(fd.get(), formatted.data(), formatted.length(), &bytes_written, nullptr);
    if (!res) {
        error_out->assign("cannot write to log file: " + winerr_string(GetLastError()));
        return false;
    }
#else
    ssize_t write_res = ::write(fd.get(), formatted.data(), formatted.length());
    if (write_res != static_cast<ssize_t>(formatted.length())) {
        error_out->assign("cannot write to log file: " + errno_string(get_errno()));
        return false;
    }
#endif

#ifndef _WIN32
    fcntl_res = fcntl(fd.get(), F_SETLK, &fileunlock);
    if (fcntl_res != 0) {
        error_out->assign("cannot unlock log file: " + errno_string(get_errno()));
        return false;
    }
#endif

    return true;
}

void fallback_log_writer_t::initiate_write(log_level_t level, const std::string &message) {
    log_message_t log_msg = assemble_log_message(level, message);
    std::string error_message;
    if (!write(log_msg, &error_message)) {
        fprintf(stderr, "Previous message may not have been written to the log file (%s).\n", error_message.c_str());
    }
}

typedef auto_drainer_t * type;

TLS_with_init(thread_pool_log_writer_t *, global_log_writer, nullptr);
TLS_with_init(auto_drainer_t *, global_log_drainer, nullptr);
TLS_with_init(int, log_writer_block, 0);

thread_pool_log_writer_t::thread_pool_log_writer_t()
        : has_parse_error(false) {
    pmap(
        get_num_threads(),
        boost::bind(&thread_pool_log_writer_t::install_on_thread, this, _1));
}

thread_pool_log_writer_t::~thread_pool_log_writer_t() {
    pmap(get_num_threads(), boost::bind(&thread_pool_log_writer_t::uninstall_on_thread, this, _1));
}

std::vector<log_message_t> thread_pool_log_writer_t::tail(
        int max_lines,
        struct timespec min_timestamp,
        struct timespec max_timestamp,
        signal_t *interruptor) THROWS_ONLY(log_read_exc_t, interrupted_exc_t) {
    assert_thread();

    volatile bool cancel = false;
    class cancel_subscription_t : public signal_t::subscription_t {
    public:
        explicit cancel_subscription_t(volatile bool *c) : cancel(c) { }
        void run() {
            *cancel = true;
        }
        volatile bool *cancel;
    } subscription(&cancel);
    subscription.reset(interruptor);

    std::vector<log_message_t> log_messages;
    std::string error_message;


    bool ok;
    thread_pool_t::run_in_blocker_pool(
        boost::bind(
            &thread_pool_log_writer_t::tail_blocking,
            this,
            max_lines,
            min_timestamp,
            max_timestamp,
            &cancel,
            &log_messages,
            &error_message,
            &ok));
    if (ok) {
        if (cancel) {
            throw interrupted_exc_t();
        } else {
            if (has_parse_error == false && error_message.empty() == false) {
                logERR("%s", error_message.c_str());
                has_parse_error = true;
            }
            return log_messages;
        }
    } else {
        throw log_read_exc_t(error_message);
    }
}

void thread_pool_log_writer_t::install_on_thread(int i) {
    on_thread_t thread_switcher((threadnum_t(i)));
    guarantee(TLS_get_global_log_writer() == nullptr);
    TLS_set_global_log_drainer(new auto_drainer_t);
    TLS_set_global_log_writer(this);
}

void thread_pool_log_writer_t::uninstall_on_thread(int i) {
    on_thread_t thread_switcher((threadnum_t(i)));
    guarantee(TLS_get_global_log_writer() == this);
    TLS_set_global_log_writer(nullptr);
    delete TLS_get_global_log_drainer();
    TLS_set_global_log_drainer(nullptr);
}

void thread_pool_log_writer_t::write(const log_message_t &lm) {
    mutex_t::acq_t write_mutex_acq(&write_mutex);
    std::string error_message;
    bool ok;
    thread_pool_t::run_in_blocker_pool(boost::bind(&thread_pool_log_writer_t::write_blocking, this, lm, &error_message, &ok));
    if (ok) {
        log_write_issue_tracker.report_success();
    } else {
        log_write_issue_tracker.report_error(error_message);
    }
}

void thread_pool_log_writer_t::write_blocking(const log_message_t &msg, std::string *error_out, bool *ok_out) {
    *ok_out = fallback_log_writer.write(msg, error_out);
    return;
}

void thread_pool_log_writer_t::tail_blocking(
        int max_lines,
        timespec min_timestamp,
        timespec max_timestamp,
        volatile bool *cancel,
        std::vector<log_message_t> *messages_out,
        std::string *error_out,
        bool *ok_out) {
    try {
        scoped_fd_t fd;
#ifdef _WIN32
        fd.reset(CreateFile(fallback_log_writer.filename.path().c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
#else
        do {
            fd.reset(open(fallback_log_writer.filename.path().c_str(), O_RDONLY));
        } while (fd.get() == INVALID_FD && get_errno() == EINTR);
#endif
        throw_unless(fd.get() != INVALID_FD,
            strprintf("could not open '%s' for reading.",
                fallback_log_writer.filename.path().c_str()));

        file_reverse_reader_t reader(std::move(fd));
        std::string line;
        while (max_lines-- > 0 && reader.get_next(&line) && !*cancel) {
            if (line.empty()) {
                continue;
            }
            log_message_t lm;
            try {
                lm = parse_log_message(line);
            } catch (const log_read_exc_t &exc) {
                *error_out = strprintf(
                    "Failed to parse one or more lines from the log file, the contents "
                    "of the `logs` system table will be incomplete. The following parse "
                    "error occurred: %s while parsing \"%s\"",
                    exc.what(),
                    line.c_str());
                continue;
            }
            if (lm.timestamp > max_timestamp) {
                continue;
            }
            if (lm.timestamp < min_timestamp) {
                break;
            }
            messages_out->push_back(lm);
        }
        *ok_out = true;
        return;
    } catch (const log_read_exc_t &e) {
        // Any parse errors are handled above, this is for reader-related failures.
        *error_out = e.what();
        *ok_out = false;
        return;
    }
}

void log_coro(thread_pool_log_writer_t *writer, log_level_t level, const std::string &message, auto_drainer_t::lock_t) {
    on_thread_t thread_switcher(writer->home_thread());

    log_message_t log_msg = fallback_log_writer.assemble_log_message(level, message);
    writer->write(log_msg);
}

/* Declared in `logger.hpp`, not `clustering/administration/logs/logger.hpp` like the
other things in this file. */
void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vlog_internal(src_file, src_line, level, format, args);
    va_end(args);
}

void vlog_internal(UNUSED const char *src_file, UNUSED int src_line, log_level_t level, const char *format, va_list args) {
    thread_pool_log_writer_t *writer;
    if ((writer = TLS_get_global_log_writer()) && TLS_get_log_writer_block() == 0) {
        auto_drainer_t::lock_t lock(TLS_get_global_log_drainer());

        std::string message = vstrprintf(format, args);
        coro_t::spawn_sometime(boost::bind(&log_coro, writer, level, message, lock));

    } else {
        std::string message = vstrprintf(format, args);

        fallback_log_writer.initiate_write(level, message);
    }
}

thread_log_writer_disabler_t::thread_log_writer_disabler_t() {
    TLS_set_log_writer_block(TLS_get_log_writer_block() + 1);
}

thread_log_writer_disabler_t::~thread_log_writer_disabler_t() {
    TLS_set_log_writer_block(TLS_get_log_writer_block() - 1);
    guarantee(TLS_get_log_writer_block() >= 0);
}

void install_fallback_log_writer(const std::string &logfile_name) {
    fallback_log_writer.install(logfile_name);
}
