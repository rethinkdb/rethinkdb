#include <stdio.h>
#include <stdlib.h>
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "cpu_context.hpp"
#include "worker_pool.hpp"
#include "logger.hpp"
#include <string.h>

// log_writer_t

log_writer_t::log_writer_t() {
    log_file = NULL;
}

log_writer_t::~log_writer_t() {
    if (log_file) {
        fclose(log_file);
    }
}

void log_writer_t::start(cmd_config_t *cmd_config) {
    log_file = stderr;
    if (*cmd_config->log_file_name) {
        log_file = fopen(cmd_config->log_file_name, "w");
    }
}

void log_writer_t::write_str(const char *str) {
    fprintf(log_file, "%s", str);
}

// log_msg_t

log_msg_t::log_msg_t() : cpu_message_t(cpu_message_t::mt_log) {
    del = false;
    pos = 0;
}

// logger_t

logger_t::logger_t() {
    msg = NULL;
}

void logger_t::_log1(const char *src_file, int src_line, log_level_t level, const char *format, ...) {
    _logn_header(src_file, src_line, level);
    va_list arg;
    va_start(arg, format);
    _vlogn(format, arg);
    va_end(arg);
    _logn_end();
}

void logger_t::_logn_header(const char *src_file, int src_line, log_level_t level) {
    _logn_end();
    _logn("(%c)%s:%d:", level, src_file, src_line);
}

void logger_t::_logn(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    _vlogn(format, arg);
    va_end(arg);
}

void logger_t::_logn_end() {
    // Sends the message to worker LOG_WORKER.
    // Should be idempotent? If so we can call it in _log1 (or maybe get rid of it completely).
    if (msg && msg->pos != 0) {
        event_queue_t *queue = get_cpu_context()->event_queue;
        msg->return_cpu = queue->queue_id;
        queue->message_hub.store_message(LOG_WORKER, msg);
    }
    msg = new log_msg_t(); // msg will be sent back to be deleted later.
}

void logger_t::_vlogn(const char *format, va_list arg) {
    msg->pos += vsnprintf((char *) msg->str + msg->pos, (size_t) MAX_LOG_MSGLEN - msg->pos, format, arg);
}
