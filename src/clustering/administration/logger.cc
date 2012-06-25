#include "clustering/administration/logger.hpp"

#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "errors.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

#include "arch/runtime/thread_pool.hpp"   /* for `run_in_blocker_pool()` */
#include "clustering/administration/persist.hpp"
#include "concurrency/promise.hpp"
#include "thread_local.hpp"

std::string format_log_level(log_level_t l) {
    switch (l) {
        case log_level_debug: return "debug";
        case log_level_info: return "info";
        case log_level_warn: return "warn";
        case log_level_error: return "error";
        default: unreachable();
    }
}

log_level_t parse_log_level(const std::string &s) THROWS_ONLY(std::runtime_error) {
    if (s == "debug")
        return log_level_debug;
    else if (s == "info")
        return log_level_info;
    else if (s == "warn")
        return log_level_warn;
    else if (s == "error")
        return log_level_error;
    else
        throw std::runtime_error("cannot parse '" + s + "' as log level");
}

std::string format_log_message(const log_message_t &m) {

    std::string message = m.message;
    for (int i = 0; i < int(message.length()); i++) {
        if (message[i] < ' ' || message[i] > '~') {
#ifndef NDEBUG
            crash("We can't have newlines, tabs or special characters in log "
                "messages because then it would be difficult to parse the log "
                "file. Message: %s", message.c_str());
#else
            message[i] = '?';
#endif
        }
    }
    return strprintf("%s %d.%06ds %s: %s\n",
        format_time(m.timestamp).c_str(),
        int(m.uptime.tv_sec),
        int(m.uptime.tv_nsec / 1000),
        format_log_level(m.level).c_str(),
        message.c_str()
        );
}

log_message_t parse_log_message(const std::string &s) THROWS_ONLY(std::runtime_error) {
    const char *p = s.c_str();
    const char *start_timestamp = p;
    while (*p && *p != ' ') p++;
    if (!*p) throw std::runtime_error("cannot parse log message (1)");
    const char *end_timestamp = p;
    p++;
    const char *start_uptime_ipart = p;
    while (*p && *p != '.') p++;
    if (!*p) throw std::runtime_error("cannot parse log message (2)");
    const char *end_uptime_ipart = p;
    p++;
    const char *start_uptime_fpart = p;
    while (*p && *p != 's') p++;
    if (!*p) throw std::runtime_error("cannot parse log message (3)");
    const char *end_uptime_fpart = p;
    p++;
    if (*p != ' ') throw std::runtime_error("cannot parse log message (4)");
    p++;
    const char *start_level = p;
    while (*p && *p != ':') p++;
    if (*p != ':') throw std::runtime_error("cannot parse log message (5)");
    const char *end_level = p;
    p++;
    if (*p != ' ') throw std::runtime_error("cannot parse log message (6)");
    p++;
    const char *start_message = p;
    while (*p && *p != '\n') p++;
    if (!*p) throw std::runtime_error("cannot parse log message (7)");
    const char *end_message = p;
    p++;
    if (*p) throw std::runtime_error("cannot parse log message (8)");

    struct timespec timestamp = parse_time(std::string(start_timestamp, end_timestamp - start_timestamp));
    struct timespec uptime;
    try {
        uptime.tv_sec = boost::lexical_cast<int>(std::string(start_uptime_ipart, end_uptime_ipart - start_uptime_ipart));
        uptime.tv_nsec = 1e3 * boost::lexical_cast<int>(std::string(start_uptime_fpart, end_uptime_fpart - start_uptime_fpart));
    } catch (boost::bad_lexical_cast) {
        throw std::runtime_error("cannot parse log message (9)");
    }
    log_level_t level = parse_log_level(std::string(start_level, end_level - start_level));
    std::string message = std::string(start_message, end_message - start_message);

    return log_message_t(timestamp, uptime, level, message);
}

static void throw_unless(bool condition, const std::string &where) {
    if (!condition) {
        throw std::runtime_error("file IO error: " + where + " (errno = " + strerror(errno) + ")");
    }
}

class file_reverse_reader_t {
public:
    explicit file_reverse_reader_t(const std::string &filename) :
            fd(open(filename.c_str(), O_RDONLY)),
            current_chunk(new char[chunk_size]) {
        throw_unless(fd.get() != -1, strprintf("could not open '%s' for reading.", filename.c_str()));
        struct stat64 stat;
        int res = fstat64(fd.get(), &stat);
        throw_unless(res == 0, "could not determine size of log file");
        remaining_in_current_chunk = stat.st_size % chunk_size;
        current_chunk_start = stat.st_size - remaining_in_current_chunk;
        res = pread(fd.get(), current_chunk.get(), remaining_in_current_chunk, current_chunk_start);
        throw_unless(res == remaining_in_current_chunk, "could not read from file");
    }

    bool get_next(std::string *out) {
        if (remaining_in_current_chunk == 0) {
            rassert(current_chunk_start == 0);
            return false;
        }
        *out = "";
        while (true) {
            int p = remaining_in_current_chunk - 1;
            while (p > 0 && current_chunk[p - 1] != '\n') {
                p--;
            }
            if (p == 0) {
                *out = std::string(current_chunk.get(), remaining_in_current_chunk) + *out;
                if (current_chunk_start != 0) {
                    current_chunk_start -= chunk_size;
                    int res = pread(fd.get(), current_chunk.get(), chunk_size, current_chunk_start);
                    throw_unless(res == chunk_size, "could not read from file");
                    remaining_in_current_chunk = chunk_size;
                } else {
                    remaining_in_current_chunk = 0;
                    return true;
                }
            } else {
                *out = std::string(current_chunk.get() + p, remaining_in_current_chunk - p) + *out;
                remaining_in_current_chunk = p;
                return true;
            }
        }
    }

private:
    static const int chunk_size = 4096;
    scoped_fd_t fd;
    boost::scoped_array<char> current_chunk;
    int remaining_in_current_chunk;
    off64_t current_chunk_start;
};

TLS_with_init(log_writer_t *, global_log_writer, NULL);
TLS_with_init(auto_drainer_t *, global_log_drainer, NULL);

log_writer_t::log_writer_t(const std::string &filename, local_issue_tracker_t *it) : filename(filename), issue_tracker(it) {

    int res = clock_gettime(CLOCK_MONOTONIC, &uptime_reference);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");

    pmap(get_num_threads(), boost::bind(&log_writer_t::install_on_thread, this, _1));
}

log_writer_t::~log_writer_t() {
    pmap(get_num_threads(), boost::bind(&log_writer_t::uninstall_on_thread, this, _1));
}

std::vector<log_message_t> log_writer_t::tail(int max_lines, struct timespec min_timestamp, struct timespec max_timestamp, signal_t *interruptor) THROWS_ONLY(std::runtime_error, interrupted_exc_t) {
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
    thread_pool_t::run_in_blocker_pool(boost::bind(&log_writer_t::tail_blocking, this, max_lines, min_timestamp, max_timestamp, &cancel, &log_messages, &error_message, &ok));
    if (ok) {
        if (cancel) {
            throw interrupted_exc_t();
        } else {
            return log_messages;
        }
    } else {
        throw std::runtime_error(error_message);
    }
}

void log_writer_t::install_on_thread(int i) {
    on_thread_t thread_switcher(i);
    rassert(TLS_get_global_log_writer() == NULL);
    TLS_set_global_log_drainer(new auto_drainer_t);
    TLS_set_global_log_writer(this);
}

void log_writer_t::uninstall_on_thread(int i) {
    on_thread_t thread_switcher(i);
    rassert(TLS_get_global_log_writer() == this);
    TLS_set_global_log_writer(NULL);
    delete TLS_get_global_log_drainer();
    TLS_set_global_log_drainer(NULL);
}

void log_writer_t::write(const log_message_t &lm) {
    mutex_t::acq_t write_mutex_acq(&write_mutex);
    std::string error_message;
    bool ok;
    thread_pool_t::run_in_blocker_pool(boost::bind(&log_writer_t::write_blocking, this, lm, &error_message, &ok));
    if (ok) {
        issue.reset();
    } else {
        if (!issue) {
            issue.reset(new local_issue_tracker_t::entry_t(
                issue_tracker,
                local_issue_t("LOGFILE_WRITE_ERROR", true, error_message)
                ));
        }
    }
}

void log_writer_t::write_blocking(const log_message_t &msg, std::string *error_out, bool *ok_out) {
    if (fd.get() == -1) {
        fd.reset(open(filename.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0644));
        if (fd.get() == -1) {
            *error_out = std::string("cannot open log file: ") + strerror(errno);
            *ok_out = false;
            return;
        }
    }
    std::string formatted = format_log_message(msg);
    int res = ::write(fd.get(), formatted.data(), formatted.length());
    if (res != int(formatted.length())) {
        *error_out = std::string("cannot write to log file: ") + strerror(errno);
        *ok_out = false;
        return;
    }
    *ok_out = true;
    return;
}

bool operator<(const struct timespec &t1, const struct timespec &t2) {
    return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec);
}

bool operator>(const struct timespec &t1, const struct timespec &t2) {
    return t2 < t1;
}

bool operator<=(const struct timespec &t1, const struct timespec &t2) {
    return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec <= t2.tv_nsec);
}

bool operator>=(const struct timespec &t1, const struct timespec &t2) {
    return t2 <= t1;
}

void log_writer_t::tail_blocking(int max_lines, struct timespec min_timestamp, struct timespec max_timestamp, volatile bool *cancel, std::vector<log_message_t> *messages_out, std::string *error_out, bool *ok_out) {
    try {
        file_reverse_reader_t reader(filename);
        std::string line;
        while (int(messages_out->size()) < max_lines && reader.get_next(&line) && !*cancel) {
            if (line == "" || line[line.length() - 1] != '\n') {
                continue;
            }
            log_message_t lm = parse_log_message(line);
            if (lm.timestamp >= max_timestamp) continue;
            if (lm.timestamp <= min_timestamp) break;
            messages_out->push_back(lm);
        }
        *ok_out = true;
        return;
    } catch (std::runtime_error &e) {
        *error_out = e.what();
        *ok_out = false;
        return;
    }
}

void log_coro(log_writer_t *writer, log_level_t level, const std::string &message, auto_drainer_t::lock_t) {
    on_thread_t thread_switcher(writer->home_thread());

    int res;

    struct timespec timestamp;
    res = clock_gettime(CLOCK_REALTIME, &timestamp);
    guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");

    struct timespec uptime;
    res = clock_gettime(CLOCK_MONOTONIC, &uptime);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");
    if (uptime.tv_nsec < writer->uptime_reference.tv_nsec) {
        uptime.tv_nsec += 1000000000;
        uptime.tv_sec -= 1;
    }
    uptime.tv_nsec -= writer->uptime_reference.tv_nsec;
    uptime.tv_sec -= writer->uptime_reference.tv_sec;

    writer->write(log_message_t(timestamp, uptime, level, message));
}

/* Declared in `logger.hpp`, not `clustering/administration/logger.hpp` like the
other things in this file. */
void log_internal(UNUSED const char *src_file, UNUSED int src_line, log_level_t level, const char *format, ...) {
    if (log_writer_t *writer = TLS_get_global_log_writer()) {
        auto_drainer_t::lock_t lock(TLS_get_global_log_drainer());

        va_list args;
        va_start(args, format);
        std::string message = vstrprintf(format, args);
        va_end(args);

        coro_t::spawn_sometime(boost::bind(&log_coro, writer, level, message, lock));

    }
}
