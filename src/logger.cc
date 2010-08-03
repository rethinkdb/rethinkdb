#include <stdio.h>
#include <stdarg.h>
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

void log_writer_t::writef(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    vfprintf(log_file, format, arg);
    va_end(arg);
}

void log_writer_t::write(const char *str) {
    writef("%s", str);
}

// log_msg_t

log_msg_t::log_msg_t() : cpu_message_t(cpu_message_t::mt_log) {
    *str = '\0';
    del = false;
}

const char *log_msg_t::level_str() {
    switch (level) {
#ifndef NDEBUG
        case DBG:
            return "DD";
            break;
#endif
        case INF:
            return "II";
            break;
        case WRN:
            return "WW";
            break;
        case ERR:
            return "EE";
            break;
        default:
            return "??";
            break;
    }
}

// logger_t

logger_t::logger_t() {
    msg = NULL;
}

void logger_t::_logf(const char *src_file, int src_line, log_level_t level, const char *format, ...) {
    _mlog_start(src_file, src_line, level);

    va_list arg;
    va_start(arg, format);
    _vmlogf(format, arg);
    va_end(arg);

    _mlog_end();
}

void logger_t::_mlog_start(const char *src_file, int src_line, log_level_t level) {
    assert(!msg);
    msg = new log_msg_t();
    msg->level = level;
    msg->src_file = src_file;
    msg->src_line = src_line;
}

void logger_t::_vmlogf(const char *format, va_list arg) {
    int pos = strlen(msg->str);
    vsnprintf((char *) msg->str + pos, (size_t) MAX_LOG_MSGLEN - pos, format, arg);
}

void logger_t::_mlogf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    _vmlogf(format, arg);
    va_end(arg);
}

// Sends the message to worker LOG_WORKER.
void logger_t::_mlog_end() {
    assert(msg);
    worker_t *worker = get_cpu_context()->worker;
    msg->return_cpu = worker->workerid;
    worker->event_queue->message_hub.store_message(LOG_WORKER, msg);
    msg = NULL;
}
