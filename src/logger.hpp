#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "message_hub.hpp"
#include "alloc/alloc_mixin.hpp"


enum log_level_t { // For now these are just used directly as characters. This isn't a good idea.
#ifndef NDEBUG
    DBG = 0,
#endif
    INF = 1, WRN, ERR
};


// The class that actually writes the logs
class log_writer_t {
public:
    log_writer_t();
    ~log_writer_t();
    void start(cmd_config_t *cmd_config);
    void writef(const char *format, ...);
    void write(const char *str);
private:
    FILE *log_file;
};


// Log messages

struct log_msg_t : public cpu_message_t,
                   public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, log_msg_t> {
public:
    log_msg_t();
    const char *level_str();

public:
    char str[MAX_LOG_MSGLEN];
    log_level_t level;
    const char *src_file;
    int src_line;
    bool del;
};

// Per-worker logger

struct logger_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, logger_t> {
public:
    logger_t();
    void _logf(const char *src_file, int src_line, log_level_t level, const char *format, ...);
    void _mlog_start(const char *src_file, int src_line, log_level_t level);
    void _mlogf(const char *format, ...);
    void _mlog_end();
private:
    void _vmlogf(const char *format, va_list arg);
    const char *level_to_str(log_level_t level);
    log_msg_t *msg;
};

// XXX We include this down here because event_queue.hpp refers to logger_t
// (and we need get_cpu_context() for our macro -- though I guess it could be
// replaced with a function). Is there a better way of doing it?
#include "cpu_context.hpp"

#define CURRENT_LOGGER (get_cpu_context()->event_queue->logger)

// Log a message in pieces.
#define mlog_start(lvl) (CURRENT_LOGGER._mlog_start(__FILE__, __LINE__, (lvl)))
#define mlogf(fmt, args...) (CURRENT_LOGGER._mlogf((fmt) , ##args))
#define mlog_end() (CURRENT_LOGGER._mlog_end())

// Log a message in one chunk. You still have to provide '\n'.
#define logf(lvl, fmt, args...) (CURRENT_LOGGER._logf(__FILE__, __LINE__, (lvl), (fmt) , ##args))

#endif // __LOGGER_HPP__
