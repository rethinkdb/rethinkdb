#include "clustering/administration/logger.hpp"

#include <fcntl.h>

#include "errors.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

#include "arch/runtime/thread_pool.hpp"   /* for `run_in_blocker_pool()` */
#include "clustering/administration/persist.hpp"
#include "concurrency/promise.hpp"

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
    if (s == "debug") return log_level_debug;
    else if (s == "info") return log_level_info;
    else if (s == "warn") return log_level_warn;
    else if (s == "error") return log_level_error;
    else throw std::runtime_error("cannot parse '" + s + "' as log level");
}

std::string format_log_message(const log_message_t &m) {
#ifndef NDEBUG
    for (int i = 0; i < int(m.message.length()); i++) {
        rassert(m.message[i] != '\n', "We can't have newlines in log messages "
            "because then it would be impossible to parse the log file.");
    }
#endif
    return strprintf("%s %d.%06ds %s: %s\n",
        format_time(m.timestamp).c_str(),
        int(m.uptime.tv_sec),
        int(m.uptime.tv_nsec / 1000),
        format_log_level(m.level).c_str(),
        m.message.c_str()
        );
}

log_message_t parse_log_message(const std::string &s) THROWS_ONLY(std::runtime_error) {
    const char *p = s.c_str();
    const char *start_timestamp = p;
    while (*p && *p != ' ') p++;
    if (!*p) throw std::runtime_error("cannot parse log message");
    const char *end_timestamp = p;
    p++;
    const char *start_uptime_ipart = p;
    while (*p && *p != '.') p++;
    if (!*p) throw std::runtime_error("cannot parse log message");
    const char *end_uptime_ipart = p;
    p++;
    const char *start_uptime_fpart = p;
    while (*p && *p != 's') p++;
    if (!*p) throw std::runtime_error("cannot parse log message");
    const char *end_uptime_fpart = p;
    p++;
    if (*p != ' ') throw std::runtime_error("cannot parse log message");
    p++;
    const char *start_level = p;
    while (*p && *p != ':') p++;
    if (*p != ':') throw std::runtime_error("cannot parse log message");
    const char *end_level = p;
    p++;
    if (*p != ' ') throw std::runtime_error("cannot parse log message");
    const char *start_message = p;
    while (*p && *p != '\n') p++;
    if (!*p) throw std::runtime_error("cannot parse log message");
    const char *end_message = p;
    p++;
    if (*p) throw std::runtime_error("cannot parse log message");

    time_t timestamp = parse_time(std::string(start_timestamp, end_timestamp - start_timestamp));
    struct timespec uptime;
    try {
        uptime.tv_sec = boost::lexical_cast<int>(std::string(start_uptime_ipart, end_uptime_ipart - start_uptime_ipart));
        uptime.tv_nsec = 1e3 * boost::lexical_cast<int>(std::string(start_uptime_fpart, end_uptime_fpart - start_uptime_fpart));
    } catch (boost::bad_lexical_cast) {
        throw std::runtime_error("cannot parse log message");
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
    file_reverse_reader_t(const std::string &filename) :
            fd(open(filename.c_str(), O_RDONLY)),
            current_chunk(new char[chunk_size]) {
        throw_unless(fd.get() != -1, strprintf("could not open '%s' for reading.", filename.c_str()));
        off64_t size = lseek(fd.get(), SEEK_END, 0);
        throw_unless(size >= 0, "could not seek to end of file");
        remaining_in_current_chunk = size % chunk_size;
        current_chunk_start = size - remaining_in_current_chunk;
        int res = pread(fd.get(), current_chunk.get(), remaining_in_current_chunk, current_chunk_start);
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
                    throw_unless(res == remaining_in_current_chunk, "could not read from file");
                    remaining_in_current_chunk = chunk_size;
                } else {
                    remaining_in_current_chunk = 0;
                    return true;
                }
            } else {
                *out = std::string(current_chunk.get() + p, remaining_in_current_chunk - p) + *out;
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

static __thread log_writer_t *global_log_writer = NULL;
static __thread auto_drainer_t *global_log_drainer = NULL;

log_writer_t::log_writer_t(const std::string &filename, local_issue_tracker_t *it) : filename(filename), issue_tracker(it) {

    int res = clock_gettime(CLOCK_MONOTONIC, &uptime_reference);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");

    pmap(get_num_threads(), boost::bind(&log_writer_t::install_on_thread, this, _1));
}

log_writer_t::~log_writer_t() {
    pmap(get_num_threads(), boost::bind(&log_writer_t::uninstall_on_thread, this, _1));
}

std::vector<log_message_t> log_writer_t::tail(int max_lines, time_t min_timestamp, time_t max_timestamp, signal_t *interruptor) THROWS_ONLY(std::runtime_error, interrupted_exc_t) {
    volatile bool cancel = false;
    class cancel_subscription_t : public signal_t::subscription_t {
    public:
        cancel_subscription_t(volatile bool *c) : cancel(c) { }
        void run() {
            *cancel = true;
        }
        volatile bool *cancel;
    } subscription(&cancel);
    subscription.reset(interruptor);

    std::vector<log_message_t> log_messages;
    std::string error_message;
    bool ok = thread_pool_t::run_in_blocker_pool<bool>(
        boost::bind(&log_writer_t::tail_blocking, this, max_lines, min_timestamp, max_timestamp, &cancel, &log_messages, &error_message));
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
    rassert(global_log_writer == NULL);
    global_log_drainer = new auto_drainer_t;
    global_log_writer = this;
}

void log_writer_t::uninstall_on_thread(int i) {
    on_thread_t thread_switcher(i);
    rassert(global_log_writer == this);
    global_log_writer = NULL;
    delete global_log_drainer;
    global_log_drainer = NULL;
}

void log_writer_t::write(const log_message_t &lm) {
    mutex_t::acq_t write_mutex_acq(&write_mutex);
    std::string error_message;
    bool ok = thread_pool_t::run_in_blocker_pool<bool>(
        boost::bind(&log_writer_t::write_blocking, this, lm, &error_message));
    if (ok) {
        issue.reset();
    } else {
        if (!issue) {
            issue.reset(new local_issue_tracker_t::entry_t(
                issue_tracker,
                clone_ptr_t<local_issue_t>(new metadata_persistence::persistence_issue_t(
                    error_message
                    ))
                ));
        }
    }
}

bool log_writer_t::write_blocking(const log_message_t &msg, std::string *error_out) {
    if (fd.get() == -1) {
        fd.reset(open(filename.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0644));
        if (fd.get() == -1) {
            *error_out = std::string("cannot open log file: ") + strerror(errno);
            return false;
        }
    }
    std::string formatted = format_log_message(msg);
    int res = ::write(fd.get(), formatted.data(), formatted.length());
    if (res != 0) {
        *error_out = std::string("cannot write to log file: ") + strerror(errno);
        return false;
    }
    return true;
}

bool log_writer_t::tail_blocking(int max_lines, time_t min_timestamp, time_t max_timestamp, volatile bool *cancel, std::vector<log_message_t> *messages_out, std::string *error_out) {
    try {
        file_reverse_reader_t reader(filename);
        std::string line;
        while (int(messages_out->size()) < max_lines && reader.get_next(&line) && !*cancel) {
            log_message_t lm = parse_log_message(line);
            if (lm.timestamp > max_timestamp) continue;
            if (lm.timestamp < min_timestamp) break;
            messages_out->push_back(lm);
        }
        return true;
    } catch (std::runtime_error &e) {
        *error_out = e.what();
        return false;
    }
}

/* Declared in `logger.hpp`, not `clustering/administration/logger.hpp` like the
other things in this file. */
void log_internal(UNUSED const char *src_file, UNUSED int src_line, log_level_t level, const char *format, ...) {
    if (log_writer_t *writer = global_log_writer) {
        auto_drainer_t::lock_t lock(global_log_drainer);
        on_thread_t thread_switcher(writer->home_thread());

        time_t timestamp = time(NULL);

        struct timespec uptime;
        int res = clock_gettime(CLOCK_MONOTONIC, &uptime);
        guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");
        if (uptime.tv_nsec < writer->uptime_reference.tv_nsec) {
            uptime.tv_nsec += 1000000000;
            uptime.tv_sec -= 1;
        }
        uptime.tv_nsec -= writer->uptime_reference.tv_nsec;
        uptime.tv_sec -= writer->uptime_reference.tv_sec;

        va_list args;
        va_start(args, format);
        std::string message = vstrprintf(format, args);
        va_end(args);

        writer->write(log_message_t(timestamp, uptime, level, message));
    }
}
