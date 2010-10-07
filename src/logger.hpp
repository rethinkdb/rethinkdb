#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "utils.hpp"



enum log_level_t {
#ifndef NDEBUG
    DBG = 0,
#endif
    INF = 1,
    WRN,
    ERR
};

// Log a message in one chunk. You still have to provide '\n'.

void _logf(const char *src_file, int src_line, log_level_t level, const char *format, ...);
#define logf(lvl, fmt, args...) (_logf(__FILE__, __LINE__, (lvl), (fmt) , ##args))

// Log a message in pieces.

void _mlog_start(const char *src_file, int src_line, log_level_t level);
#define mlog_start(lvl) (_mlog_start(__FILE__, __LINE__, (lvl)))

void mlogf(const char *format, ...);

void mlog_end();



/* server_t creates one log_controller_t for the entire thread pool. The
log_controller_t takes care of starting and shutting down the per-thread
logger_t objects. */

class start_logger_message_t;
class shutdown_logger_message_t;

class log_controller_t :
    public home_cpu_mixin_t
{
    friend class start_logger_message_t;
    friend class shutdown_logger_message_t;

public:
    log_controller_t(cmd_config_t *cmd_config);
    
    struct ready_callback_t {
        virtual void on_logger_ready() = 0;
    };
    bool start(ready_callback_t *cb);
    
    struct shutdown_callback_t {
        virtual void on_logger_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);
    
    ~log_controller_t();
    
    void writef(const char *format, ...);
    void write(const char *str);

private:
    void finish_shutting_down();
    
    enum state_t {
        state_off,
        state_starting_loggers,
        state_ready,
        state_shutting_down_loggers
    } state;
    cmd_config_t *cmd_config;
    FILE *log_file;
    
    ready_callback_t *ready_callback;
    shutdown_callback_t *shutdown_callback;
    
    /* The number of start_logger_message_ts or shutdown_logger_message_ts in existence,
    NOT the number of log_msg_ts in existence */
    int messages_out;
};

#endif // __LOGGER_HPP__
