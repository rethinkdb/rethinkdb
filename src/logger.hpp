#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "utils.hpp"
#include "log.hpp"

/* server_t creates one log_controller_t for the entire thread pool. The
log_controller_t takes care of starting and shutting down the per-thread
logger_t objects. */

class logger_t;

class log_controller_t :
    public home_cpu_mixin_t
{
    friend class logger_t;

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
    bool start_a_logger();   // Called on each thread
    bool have_started_a_logger();   // Called on main thread
    bool shutdown_a_logger();   // Called on each thread
    bool a_logger_has_shutdown();   // Called on each thread
    bool have_shutdown_a_logger();   // Called on main thread
    
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
