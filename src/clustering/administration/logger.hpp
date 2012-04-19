#ifndef CLUSTERING_ADMINISTRATION_LOGGER_HPP_
#define CLUSTERING_ADMINISTRATION_LOGGER_HPP_

#include <stdio.h>

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/circular_buffer.hpp>

#include "clustering/resource.hpp"
#include "logger.hpp"
#include "rpc/mailbox/typed.hpp"
#include "utils.hpp"

class log_message_t {
public:
    log_message_t(time_t t, struct timestamp u, log_level_t l, std::string m) :
        timestamp(t), uptime(u), level(l), message(m) { }
    time_t timestamp;
    struct timespec uptime;
    log_level_t level;
    std::string message;
    RDB_MAKE_ME_SERIALIZABLE_4(timestamp, uptime, log_level, message);
};

class log_issue_t : public local_issue_t {
public:
    log_issue_t() { }   // for serialization

    explicit log_issue_t(const std::string &_message) : message(_message) { }

    std::string get_description() const {
        return "There was a problem when trying to write to the log file "
            "locally: " + message;
    }
    cJSON *get_json_description() const {
        issue_json_t json;
        json.critical = true;
        json.description = get_description();
        json.time = get_secs();
        json.type.issue_type = LOG_ISSUE;

        return render_as_json(&json, 0);
    }

    log_issue_t *clone() const {
        return new log_issue_t(message);
    }

    std::string message;

    RDB_MAKE_ME_SERIALIZABLE_1(message);
};

std::string format_log_level(log_level_t l);
log_level_t parse_log_level(const std::string &s) THROWS_ONLY(std::runtime_error);

std::string format_log_message(const log_message_t &m);
log_message_t parse_log_message(const std::string &s) THROWS_ONLY(std::runtime_error);

class log_file_writer_t : public home_thread_mixin_t {
public:
    log_file_writer_t(const std::string &filename, local_issue_tracker_t *issue_tracker);
    ~log_file_writer_t();

    std::vector<log_message_t> tail(int max_lines, time_t min_timestamp, time_t max_timestamp);

private:
    friend void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...);
    void install_on_thread(int i);
    void uninstall_on_thread(int i);
    void write(const log_message_t &msg);
    std::string filename;
    mutex_t write_mutex;
    scoped_fd_t fd;
    local_issue_tracker_t *issue_tracker;
    boost::scoped_ptr<local_issue_t> issue;
    time_t last_time;
};

class log_file_server_t {
public:
    log_file_server_t(mailbox_manager_t *mm, std::string filename);
    mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> get_business_card();
private:
    void handle_request(const mailbox_addr_t<void(std::vector<std::string>)> &, auto_drainer_t::lock_t);
    mailbox_manager_t *mailbox_manager;
    log_file_writer_t *writer;
    auto_drainer_t drainer;
    mailbox_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> mailbox;
};

std::vector<std::string> fetch_log_file(
    mailbox_manager_t *mailbox_manager,
    const mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> &mbox,
    signal_t *interruptor) THROWS_ONLY(resource_lost_exc_t, interrupted_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_LOGGER_HPP_ */
