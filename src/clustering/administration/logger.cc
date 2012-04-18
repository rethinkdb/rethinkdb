#include "clustering/administration/logger.hpp"

#include "errors.hpp"
#include <boost/scoped_array.hpp>

#include "concurrency/promise.hpp"

static const int max_log_messages_in_memory = 1000;

static __thread log_file_writer_t *global_log_writer = NULL;
static __thread auto_drainer_t *global_log_drainer = NULL;

log_file_writer_t::log_file_writer_t(const std::string &filename) : lines(max_log_messages_in_memory) {
    file = fopen(filename.c_str(), "a+");
    guarantee_err(file, "Could not open log file.");
    static const int max_line = 10000;
    boost::scoped_array<char> line(new char[max_line]);
    int ok = fseek(file, 0, SEEK_SET);
    guarantee_err(ok == 0, "Could not seek to beginning of log file.");
    while (true) {
        /* TODO: Be nonblocking */
        char *res = fgets(line.get(), max_line, file);
        if (res != line.get()) {
            guarantee_err(!ferror(file), "Could not read from log file.");
            rassert(feof(file));
            break;
        }
        lines.push_back(line.get());
    }
    pmap(get_num_threads(), boost::bind(&log_file_writer_t::install_on_thread, this, _1));
}

log_file_writer_t::~log_file_writer_t() {
    pmap(get_num_threads(), boost::bind(&log_file_writer_t::uninstall_on_thread, this, _1));
    fclose(file);
}

std::vector<std::string> log_file_writer_t::get_lines() {
    return std::vector<std::string>(lines.begin(), lines.end());
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

/* Declared in `logger.hpp`, not `clustering/administration/logger.hpp` like the
other things in this file. */
void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...) {
    const char *level_str;
    switch (level) {
        case DBG: level_str = "debug"; break;
        case INF: level_str = "info"; break;
        case WRN: level_str = "warn"; break;
        case ERR: level_str = "error"; break;
        default:  unreachable();
    };

    precise_time_t t = get_time_now();
    char formatted_time[formatted_precise_time_length+1];
    format_precise_time(t, formatted_time, sizeof(formatted_time));

    va_list arg;
    va_start(arg, format);
    std::string msg = strprintf("%s %s (%s:%d): ", formatted_time, level_str, src_file, src_line) + vstrprintf(format, arg);
    va_end(arg);

    if (log_file_writer_t *writer = global_log_writer) {
        auto_drainer_t::lock_t lock(global_log_drainer);
        on_thread_t thread_switcher(writer->home_thread());
        writer->lines.push_back(msg);
        fputs(msg.c_str(), stderr);
        /* TODO: Be nonblocking */
        fputs(msg.c_str(), writer->file);
        guarantee_err(!ferror(writer->file), "Could not write to log file.");
        int res = fflush(writer->file);
        guarantee_err(res == 0, "Could not flush log file.");
    }
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
