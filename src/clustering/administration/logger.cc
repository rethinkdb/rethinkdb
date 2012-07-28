#include "clustering/administration/logger.hpp"

#include <math.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#include "arch/runtime/thread_pool.hpp"   /* for `run_in_blocker_pool()` */
#include "clustering/administration/persist.hpp"
#include "concurrency/promise.hpp"
#include "containers/scoped.hpp"
#include "thread_local.hpp"

primary_log_writer_t primary_log_writer;

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
    std::string message_reformatted;

    int start = 0, end = int(message.length()) - 1;
    while (start < int(message.length()) && message[start] == '\n') {
        start++;
    }
    while (end >= 0 && message[end] == '\n') {
        end--;
    }
    for (int i = start; i <= end; i++) {
        if (message[i] == '\n') {
            message_reformatted.append(LOGGER_NEWLINE);
        } else if (message[i] < ' ' || message[i] > '~') {
#ifndef NDEBUG
            crash("We can't have tabs or other special characters in log "
                "messages because then it would be difficult to parse the log "
                "file. Message: %s", message.c_str());
#else
            message_reformatted.push_back("?");
#endif
        } else {
            message_reformatted.push_back(message[i]);
        }
    }
    return strprintf("%s %d.%06ds %s: %s\n",
        format_time(m.timestamp).c_str(),
        int(m.uptime.tv_sec),
        int(m.uptime.tv_nsec / 1000),
        format_log_level(m.level).c_str(),
        message_reformatted.c_str()
        );
}

std::string format_log_message_for_console(const log_message_t &m) {
    std::string message = m.message;
    std::string message_reformatted;

    int prepend_length = format_time(m.timestamp).length() + 12;
    prepend_length += format_log_level(m.level).length() + 1;
    if (int(m.uptime.tv_sec) != 0) {
        prepend_length += static_cast<int>(log10(int(m.uptime.tv_sec)));
    }

    int start = 0, end = int(message.length()) - 1;
    while (start < int(message.length()) && message[start] == '\n') {
        start++;
    }
    while (end >= 0 && message[end] == '\n') {
        end--;
    }
    for (int i = start; i <= end; i++) {
        if (message[i] == '\n') {
            message_reformatted.push_back('\n');
            message_reformatted.append(prepend_length, ' ');
        } else if (message[i] < ' ' || message[i] > '~') {
#ifndef NDEBUG
            crash("We can't have newlines, tabs or special characters in log "
                "messages because then it would be difficult to parse the log "
                "file. Message: %s", message.c_str());
#else
            message_reformatted.push_back('?');
#endif
        } else {
            message_reformatted.push_back(message[i]);
        }
    }

    return strprintf("%s %d.%06ds %s: %s\n",
        format_time(m.timestamp).c_str(),
        int(m.uptime.tv_sec),
        int(m.uptime.tv_nsec / 1000),
        format_log_level(m.level).c_str(),
        message_reformatted.c_str()
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

    {
        std::string tv_sec_str(start_uptime_ipart, end_uptime_ipart - start_uptime_ipart);
        uint64_t tv_sec;
        if (!strtou64_strict(tv_sec_str, 10, &tv_sec)) {
            throw std::runtime_error("cannot parse log message (9)");
        }

        uptime.tv_sec = tv_sec;

        std::string tv_nsec_str(start_uptime_fpart, end_uptime_fpart - start_uptime_fpart);
        uint64_t tv_nsec;
        if (!strtou64_strict(tv_nsec_str, 10, &tv_nsec)) {
            throw std::runtime_error("cannot parse log message (10)");
        }

        // TODO: Seriously?  We assume three decimal places?
        uptime.tv_nsec = 1000 * tv_nsec;
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
            current_chunk(chunk_size) {
        throw_unless(fd.get() != -1, strprintf("could not open '%s' for reading.", filename.c_str()));
        struct stat64 stat;
        int res = fstat64(fd.get(), &stat);
        throw_unless(res == 0, "could not determine size of log file");
        if (stat.st_size == 0) {
            remaining_in_current_chunk = current_chunk_start = 0;
        } else {
            remaining_in_current_chunk = stat.st_size % chunk_size;
            if (remaining_in_current_chunk == 0) {
                /* We landed right on a chunk boundary; set ourself to read the
                previous whole chunk. */
                remaining_in_current_chunk = chunk_size;
            }
            current_chunk_start = stat.st_size - remaining_in_current_chunk;
            res = pread(fd.get(), current_chunk.data(), remaining_in_current_chunk, current_chunk_start);
            throw_unless(res == remaining_in_current_chunk, "could not read from file");
        }
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
                *out = std::string(current_chunk.data(), remaining_in_current_chunk) + *out;
                if (current_chunk_start != 0) {
                    current_chunk_start -= chunk_size;
                    int res = pread(fd.get(), current_chunk.data(), chunk_size, current_chunk_start);
                    throw_unless(res == chunk_size, "could not read from file");
                    remaining_in_current_chunk = chunk_size;
                } else {
                    remaining_in_current_chunk = 0;
                    return true;
                }
            } else {
                *out = std::string(current_chunk.data() + p, remaining_in_current_chunk - p) + *out;
                remaining_in_current_chunk = p;
                return true;
            }
        }
    }

private:
    static const int chunk_size = 4096;
    scoped_fd_t fd;
    scoped_array_t<char> current_chunk;
    int remaining_in_current_chunk;
    off64_t current_chunk_start;

    DISABLE_COPYING(file_reverse_reader_t);
};

TLS_with_init(log_writer_t *, global_log_writer, NULL);
TLS_with_init(auto_drainer_t *, global_log_drainer, NULL);
TLS_with_init(int *, log_writer_block, 0);

log_writer_t::log_writer_t(local_issue_tracker_t *it) : filename(primary_log_writer.filename), issue_tracker(it) {
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
    TLS_set_log_writer_block(0);
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
        if (!issue.has()) {
            issue.init(new local_issue_tracker_t::entry_t(
                issue_tracker,
                local_issue_t("LOGFILE_WRITE_ERROR", true, error_message)
                ));
        }
    }
}

void log_writer_t::write_blocking(const log_message_t &msg, std::string *error_out, bool *ok_out) {
    int res, output_fileno;
    std::string formatted = format_log_message(msg);
    std::string console_formatted = format_log_message_for_console(msg);

    // Print the log message to stderr or stdout (stdout only if it's an info log)
    if (msg.level == log_level_info) {
        output_fileno = STDOUT_FILENO;
        flockfile(stdout);
    } else {
        output_fileno = STDERR_FILENO;
        flockfile(stderr);
    }
    res = ::write(output_fileno, console_formatted.data(), console_formatted.length());
    if (res != int(console_formatted.length())) {
        *error_out = std::string("cannot write to standard error: ") + strerror(errno);
        *ok_out = false;
        return;
    }
    if (msg.level == log_level_info) {
        funlockfile(stdout);
    } else {
        funlockfile(stderr);
    }

    if (fd.get() == -1) {
        fd.reset(open(filename.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0644));
        if (fd.get() == -1) {
            *error_out = std::string("cannot open log file: ") + strerror(errno);
            *ok_out = false;
            return;
        }
    }
    res = ::write(fd.get(), formatted.data(), formatted.length());
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
    } catch (const std::runtime_error &e) {
        *error_out = e.what();
        *ok_out = false;
        return;
    }
}

primary_log_writer_t::primary_log_writer_t() : shutdown(false) {
    int res = clock_gettime(CLOCK_MONOTONIC, &uptime_reference);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");

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

    pthread_create(&worker_thread, NULL, &write_action, (void*) this);
}

primary_log_writer_t::~primary_log_writer_t() {
    shutdown = true;
    pthread_join(worker_thread, NULL);
}

void primary_log_writer_t::install(const std::string &logfile_name) {
    rassert(filename == "", "Attempted to install a primary_log_writer_t that was already installed.");
    filename = logfile_name;
}

void *write_action(void *arg) {
    primary_log_writer_t *writer = reinterpret_cast<primary_log_writer_t*>(arg);
    while (!writer->shutdown) {
        if (writer->message_list.size() > 0) {
            writer->write_in_thread(writer->message_list.front());
            writer->message_list.pop();
        }
    }
    return NULL;
}

bool primary_log_writer_t::write_in_thread(const log_message_t &msg) {
    int res, output_fileno;
    std::string formatted = format_log_message(msg);
    std::string console_formatted = format_log_message_for_console(msg);

    // print the log message to stderr or stdout (stdout only if it's an info log)
    if (msg.level == log_level_info) {
        output_fileno = STDOUT_FILENO;
        flockfile(stdout);
    } else {
        output_fileno = STDERR_FILENO;
        flockfile(stderr);
    }
    res = ::write(output_fileno, console_formatted.data(), console_formatted.length());
    if (res != int(console_formatted.length())) {
        return false;
    }
    if (msg.level == log_level_info) {
        funlockfile(stdout);
    } else {
        funlockfile(stderr);
    }

    if (fd.get() == -1) {
        if (filename != "") {
            fd.reset(open(filename.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0644));
        }
        if (fd.get() == -1) {
            fprintf(stderr, "Error: the log writer has not been assigned a log file. The previous message will not be recorded in the log.\n");
            return false;
        }
    }

    res = fcntl(fd.get(), F_SETLKW, &filelock);
    if (res != 0) {
        return false;
    }

    res = ::write(fd.get(), formatted.data(), formatted.length());
    if (res != int(formatted.length())) {
        return false;
    }

    res = fcntl(fd.get(), F_SETLK, &fileunlock);
    if (res != 0) {
        return false;
    }

    return true;
}

void primary_log_writer_t::write(log_level_t level, const std::string &msg) {
    mutex_t::acq_t write_mutex_acq(&write_mutex);

    int res;

    struct timespec timestamp;
    res = clock_gettime(CLOCK_REALTIME, &timestamp);
    guarantee_err(res == 0, "clock_gettime(CLOCK_REALTIME) failed");

    struct timespec uptime;
    res = clock_gettime(CLOCK_MONOTONIC, &uptime);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");
    if (uptime.tv_nsec < uptime_reference.tv_nsec) {
        uptime.tv_nsec += 1000000000;
        uptime.tv_sec -= 1;
    }

    uptime.tv_nsec -= uptime_reference.tv_nsec;
    uptime.tv_sec -= uptime_reference.tv_sec;

    log_message_t log_msg(timestamp, uptime, level, msg);
    message_list.push(log_msg);
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
    if (uptime.tv_nsec < primary_log_writer.uptime_reference.tv_nsec) {
        uptime.tv_nsec += 1000000000;
        uptime.tv_sec -= 1;
    }
    uptime.tv_nsec -= primary_log_writer.uptime_reference.tv_nsec;
    uptime.tv_sec -= primary_log_writer.uptime_reference.tv_sec;

    writer->write(log_message_t(timestamp, uptime, level, message));
}

/* Declared in `logger.hpp`, not `clustering/administration/logger.hpp` like the
other things in this file. */
void log_internal(UNUSED const char *src_file, UNUSED int src_line, log_level_t level, const char *format, ...) {
    log_writer_t *writer;
    if ((writer = TLS_get_global_log_writer()) && TLS_get_log_writer_block() == 0) {
        auto_drainer_t::lock_t lock(TLS_get_global_log_drainer());

        va_list args;
        va_start(args, format);
        std::string message = vstrprintf(format, args);
        va_end(args);

        coro_t::spawn_sometime(boost::bind(&log_coro, writer, level, message, lock));

    } else {
        va_list args;
        va_start(args, format);
        std::string message = vstrprintf(format, args);
        va_end(args);

        primary_log_writer.write(level, message);
    }
}

void vlog_internal(UNUSED const char *src_file, UNUSED int src_line, log_level_t level, const char *format, va_list args) {
    log_writer_t *writer;
    if ((writer = TLS_get_global_log_writer()) && TLS_get_log_writer_block() == 0) {
        auto_drainer_t::lock_t lock(TLS_get_global_log_drainer());

        std::string message = vstrprintf(format, args);
        coro_t::spawn_sometime(boost::bind(&log_coro, writer, level, message, lock));

    } else {
        std::string message = vstrprintf(format, args);

        primary_log_writer.write(level, message);
    }
}

thread_log_writer_disabler_t::thread_log_writer_disabler_t() {
    TLS_set_log_writer_block(TLS_get_log_writer_block() + 1);
}

thread_log_writer_disabler_t::~thread_log_writer_disabler_t() {
    TLS_set_log_writer_block(TLS_get_log_writer_block() - 1);
    rassert(TLS_get_log_writer_block() >= 0);
}

void install_primary_log_writer(const std::string &logfile_name) {
    primary_log_writer.install(logfile_name);
}
