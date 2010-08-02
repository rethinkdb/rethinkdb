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
    //pos = 0;
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

void logger_t::_logf(const char *src_file, int src_line, log_level_t level, const char *format, ...) {
    _mlog_start(src_file, src_line, level);

    va_list arg;
    va_start(arg, format);
    _mlogf(format, arg);
    va_end(arg);

    _mlog_end();
}

void logger_t::_mlog_start(const char *src_file, int src_line, log_level_t level) {
    msg = new log_msg_t();
    msg->level = level;
    msg->src_file = src_file;
    msg->src_line = src_line;
}

void logger_t::_mlogf(const char *format, ...) {
    int len = strlen(msg->str);
    va_list arg;
    va_start(arg, format);
    vsnprintf((char *) msg->str + len, (size_t) MAX_LOG_MSGLEN - len, format, arg);
    //msg->pos += vsnprintf((char *) msg->str + msg->pos, (size_t) MAX_LOG_MSGLEN - msg->pos, format, arg);
    va_end(arg);
}

// Sends the message to worker LOG_WORKER.
void logger_t::_mlog_end() {
    assert(msg);
    event_queue_t *queue = get_cpu_context()->event_queue;
    msg->return_cpu = queue->queue_id;
    queue->message_hub.store_message(LOG_WORKER, msg);
}
