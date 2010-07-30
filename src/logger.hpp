#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "message_hub.hpp"
#include "alloc/alloc_mixin.hpp"



enum log_level_t { // For now these are just used directly as characters. This isn't a good idea.
#ifndef NDEBUG
    DBG = 'D',
#endif
    INF = 'I',
    WRN = 'W',
    ERR = 'E'
};


// The class that actually writes the logs
class log_writer_t {
public:
    log_writer_t();
    ~log_writer_t();
    void start(cmd_config_t *cmd_config);
    void write_str(const char *str);
private:
    FILE *log_file;
};


// Log messages

struct log_msg_t : public cpu_message_t,
                   public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, log_msg_t> {
public:
    log_msg_t();

public:
    char str[MAX_LOG_MSGLEN];
    int pos;
    bool del;
};

struct logger_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, logger_t> {
public:
    logger_t();
public:
    void _log1(const char *src_file, int src_line, log_level_t level, const char *format, ...);
    void _logn_header(const char *src_file, int src_line, log_level_t level);
    void _logn(const char *format, ...);
    void _logn_end();
private:
    void _vlogn(const char *format, va_list arg);
    log_msg_t *msg;
};

#define CURRENT_LOGGER (get_cpu_context()->event_queue->logger)

// Log a line. You still have to provide '\n'.
#define log1(lvl, fmt, args...) (CURRENT_LOGGER._log1(__FILE__, __LINE__, (lvl), (fmt) , ##args))
#define logn_header(lvl) (CURRENT_LOGGER._logn_header(__FILE__, __LINE__, (lvl)))
#define logn(fmt, args...) (CURRENT_LOGGER._logn((fmt) , ##args))
#define logn_end() (CURRENT_LOGGER._logn_end())

#endif
