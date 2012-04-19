#include "clustering/administration/logger.hpp"

#include "errors.hpp"
#include <boost/scoped_array.hpp>

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
        m.uptime.tv_sec,
        m.uptime.tv_nsec / 1e3,
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

    time_t timestamp = parse_timestamp(std::string(start_timestamp, end_timestamp - start_timestamp));
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
            fd(open(filename.c_str(), "r")),
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
                    if (res != remaining_in_current_chunk) {
                        throw file_io_exc_t("could not read from file", errno);
                    }
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
    boost::scoped_array<char> current_chunk;
    int remaining_in_current_chunk;
    off64_t current_chunk_start;
};

static __thread log_file_writer_t *global_log_writer = NULL;
static __thread auto_drainer_t *global_log_drainer = NULL;

log_file_writer_t::log_file_writer_t(const std::string &filename) : filename(filename), last_time(time(NULL)) {
    pmap(get_num_threads(), boost::bind(&log_file_writer_t::install_on_thread, this, _1));
}

log_file_writer_t::~log_file_writer_t() {
    pmap(get_num_threads(), boost::bind(&log_file_writer_t::uninstall_on_thread, this, _1));
}

void log_file_writer_t::install_on_thread(int i) {
    on_thread_t thread_switcher(i);
    rassert(global_log_writer == NULL);
    global_log_drainer = new auto_drainer_t;
    global_log_writer = this;
}

void log_file_writer_t::uninstall_on_thread(int i) {
    on_thread_t thread_switcher(i);
    rassert(global_log_writer == this);
    global_log_writer = NULL;
    delete global_log_drainer;
    global_log_drainer = NULL;
}

void log_file_writer_t::write_log_message(const log_message_t &lm) {
    if (fd.get() == -1) {
        fd.reset(open(filename.c_str(), "a"));
        if (fd.get() == -1) {
            if (!issue.get()) {
                issue.reset(new local_issue_tracker_t::entry_t(
                    issue_tracker,
                    clone_ptr_t<local_issue_t>(new metadata_persistence::persistence_issue_t(
                        std::string("cannot open log file: ") + strerror(errno)
                        ));
                    ));
            }
            return;
        } else {
            issue.reset();
        }
    }
    std::string formatted = format_log_message(lm);
    int res = write(fd.get(), formatted.data(), formatted.length());
    if (res != 0) {
        if (!issue.get()) {
            issue.reset(new local_issue_tracker_t::entry_t(
                issue_tracker,
                clone_ptr_t<local_issue_t>(new metadata_persistence::persistence_issue_t(
                    std::string("cannot write to log file: ") + strerror(errno)
                    ));
                ));
        }
    } else {
        issue.reset();
    }
}

/* Declared in `logger.hpp`, not `clustering/administration/logger.hpp` like the
other things in this file. */
void log_internal(UNUSED const char *src_file, UNUSED int src_line, log_level_t level, const char *format, ...) {
    if (log_file_writer_t *writer = global_log_writer) {
        auto_drainer_t::lock_t lock(global_log_drainer);
        on_thread_t thread_switcher(writer->home_thread());

        time_t timestamp = time(NULL);

        struct timespec uptime;
        res = clock_gettime(CLOCK_MONOTONIC, &uptime);
        guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");

        va_list args;
        va_begin(format, args);
        std::string message = vstrprintf(format, args);
        va_end(args);

        writer->write(log_message_t(timestamp, uptime, level, message));
    }
}


void read_log_file_blocking(const std::string &filename,
        int max_lines, time_t min_timestamp, time_t max_timestamp,
        std::vector<log_message_t> *messages_out, std::string *error_out, bool *result_out) {
    try {
        file_reverse_reader_t reader;
        std::string line;
        while (messages_out->size() < max_lines && reader.get_next(&line)) {
            log_message_t lm = parse_log_message(line);
            if (lm.timestamp > max_timestamp) continue;
            if (lm.timestamp < min_timestamp) break;
            messages_out->push_back(lm);
        }
        *result_out = true;
    } catch (std::runtime_error &e) {
        *error_out = e.what();
        *result_out = false;
    }
}

std::vector<log_message_t> read_log_file(const std::string &filename,
        int max_lines, precise_time_t min_date, precise_time_t max_date) {
    
}

log_file_server_t::log_file_server_t(mailbox_manager_t *mm, log_file_writer_t *lfw) :
    mailbox_manager(mm), writer(lfw),
    mailbox(mailbox_manager, boost::bind(&log_file_server_t::handle_request, this, _1, auto_drainer_t::lock_t(&drainer)))
    { }

mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> log_file_server_t::get_business_card() {
    return mailbox.get_address();
}

void log_file_server_t::handle_request(const mailbox_addr_t<void(std::vector<std::string>)> &cont, auto_drainer_t::lock_t) {
    send(mailbox_manager, cont, writer->get_lines());
}

std::vector<std::string> fetch_log_file(
        mailbox_manager_t *mm,
        const mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> &mbox,
        signal_t *interruptor) THROWS_ONLY(resource_lost_exc_t, interrupted_exc_t) {
    promise_t<std::vector<std::string> > promise;
    mailbox_t<void(std::vector<std::string>)> reply_mailbox(mm, boost::bind(&promise_t<std::vector<std::string> >::pulse, &promise, _1));
    disconnect_watcher_t dw(mm->get_connectivity_service(), mbox.get_peer());
    send(mm, mbox, reply_mailbox.get_address());
    wait_any_t waiter(promise.get_ready_signal(), &dw);
    wait_interruptible(&waiter, interruptor);
    if (promise.get_ready_signal()->is_pulsed()) {
        return promise.get_value();
    } else {
        throw resource_lost_exc_t();
    }
}
