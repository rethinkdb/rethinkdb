// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_LOGGER_HPP_
#define CLUSTERING_ADMINISTRATION_LOGGER_HPP_

#include <stdio.h>
#include <fcntl.h>

#include <queue>
#include <string>
#include <vector>

#include "arch/io/io_utils.hpp"
#include "containers/scoped.hpp"
#include "logger.hpp"
#include "rpc/mailbox/typed.hpp"
#include "utils.hpp"
#include "clustering/administration/issues/log_write.hpp"

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(log_level_t, int, log_level_debug, log_level_error);
RDB_DECLARE_SERIALIZABLE(struct timespec);

class log_message_t {
public:
    log_message_t() { }
    log_message_t(struct timespec t, struct timespec u, log_level_t l, std::string m) :
        timestamp(t), uptime(u), level(l), message(m) { }
    struct timespec timestamp;
    struct timespec uptime;
    log_level_t level;
    std::string message;
};

RDB_DECLARE_SERIALIZABLE(log_message_t);

std::string format_log_level(log_level_t l);
log_level_t parse_log_level(const std::string &s) THROWS_ONLY(std::runtime_error);

std::string format_log_message(const log_message_t &m, bool for_console = false);
log_message_t parse_log_message(const std::string &s) THROWS_ONLY(std::runtime_error);

log_message_t assemble_log_message(log_level_t level, const std::string &message, struct timespec uptime);

void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...) __attribute__((format (printf, 4, 5)));
void vlog_internal(const char *src_file, int src_line, log_level_t level, const char *format, va_list args) __attribute__((format (printf, 4, 0)));

class thread_pool_log_writer_t : public home_thread_mixin_t {
public:
    explicit thread_pool_log_writer_t(local_issue_aggregator_t *local_issue_aggregator);
    ~thread_pool_log_writer_t();

    std::vector<log_message_t> tail(int max_lines, struct timespec min_timestamp, struct timespec max_timestamp, signal_t *interruptor) THROWS_ONLY(std::runtime_error, interrupted_exc_t);

private:
    friend void log_coro(thread_pool_log_writer_t *writer, log_level_t level, const std::string &message, auto_drainer_t::lock_t lock);
    friend void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...);
    friend void vlog_internal(const char *src_file, int src_line, log_level_t level, const char *format, va_list args);
    void install_on_thread(int i);
    void uninstall_on_thread(int i);
    void write(const log_message_t &msg);
    void write_blocking(const log_message_t &msg, std::string *error_out, bool *ok_out);
    void tail_blocking(int max_lines,
                       struct timespec min_timestamp,
                       struct timespec max_timestamp,
                       volatile bool *cancel,
                       std::vector<log_message_t> *messages_out,
                       std::string *error_out,
                       bool *ok_out);

    mutex_t write_mutex;
    log_write_issue_tracker_t log_write_issue_tracker;

    DISABLE_COPYING(thread_pool_log_writer_t);
};

void install_fallback_log_writer(const std::string &logfile_name);

class thread_log_writer_disabler_t {
public:
    thread_log_writer_disabler_t();
    ~thread_log_writer_disabler_t();
};

#endif /* CLUSTERING_ADMINISTRATION_LOGGER_HPP_ */
