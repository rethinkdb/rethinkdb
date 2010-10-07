#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "config/alloc.hpp"
#include "logger.hpp"
#include <string.h>
#include "utils.hpp"
#include "arch/arch.hpp"

struct log_msg_t;

/* Each thread has a logger_t that is responsible for log messages originating
on that thread. All of the logger_ts are tied to the master log_controller_t. */

struct logger_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, logger_t>,
    public home_cpu_mixin_t
{

    logger_t(log_controller_t *controller)
        : msg(NULL), controller(controller), active_msg_count(0), shutting_down(false) { }
    
    // The message we are currently composing; non-NULL between mlog_start() and mlog_end()
    log_msg_t *msg;
    
    log_controller_t *controller;
    
    /* Shutting down a logger_t consists of waiting for any outstanding messages
    to come back. */
    
    struct shutdown_callback_t {
        virtual void on_logger_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb) {
        shutting_down = true;
        if (active_msg_count == 0) {
            return true;
        } else {
            shutdown_callback = cb;
            return false;
        }
    }
    
    int active_msg_count;   // How many outstanding log_msg_ts there are on this core
    bool shutting_down;
    shutdown_callback_t *shutdown_callback;
    
    static __thread logger_t *logger;   // The logger instance for this thread
};

__thread logger_t *logger_t::logger;

/* log_msg_t represents a log message. It originates on some core, travels to the core
where the log_controller_t resides, writes itself, travels back to the original core,
and then deletes itself. */

struct log_msg_t :
    public cpu_message_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, log_msg_t>
{
public:
    log_msg_t(logger_t *logger)
        : logger(logger)
    {
        *str = '\0';
        del = false;
        assert(!logger->shutting_down);
        logger->active_msg_count++;
    }

    void on_cpu_switch() {
        
        if (del) {
            /* We have returned to the core we were created on */
            delete this;
            
        } else {
            /* We have reached the core where the log controller is */
            logger->controller->writef("(%s)Q%d:%s:%d:", level_str(), logger->home_cpu, src_file, src_line);
            logger->controller->write(str);
            
            /* Now go back to be deleted */
            del = true;
            if (continue_on_cpu(logger->home_cpu, this)) on_cpu_switch();
        }
    }
    
    const char *level_str() {
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
    
    ~log_msg_t() {
        logger->active_msg_count--;
        if (logger->shutting_down && logger->active_msg_count == 0) {
            logger->shutdown_callback->on_logger_shutdown();
        }
    }
    
public:
    char str[MAX_LOG_MSGLEN];
    log_level_t level;
    const char *src_file;
    int src_line;
    
    bool del;
    logger_t *logger;
};

/* Functions to actually do the logging */

static void vmlogf(const char *format, va_list arg) {
    int pos = strlen(logger_t::logger->msg->str);
    vsnprintf((char *)logger_t::logger->msg->str + pos, (size_t) MAX_LOG_MSGLEN - pos, format, arg);
}

void _logf(const char *src_file, int src_line, log_level_t level, const char *format, ...) {
    _mlog_start(src_file, src_line, level);

    va_list arg;
    va_start(arg, format);
    vmlogf(format, arg);
    va_end(arg);

    mlog_end();
}

void _mlog_start(const char *src_file, int src_line, log_level_t level) {
    assert(!logger_t::logger->msg);
    log_msg_t *msg = logger_t::logger->msg = new log_msg_t(logger_t::logger);
    msg->level = level;
    msg->src_file = src_file;
    msg->src_line = src_line;
    msg->logger = logger_t::logger;
}

void mlogf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    vmlogf(format, arg);
    va_end(arg);
}

void mlog_end() {
    log_msg_t *msg = logger_t::logger->msg;
    assert(msg);
    if (continue_on_cpu(logger_t::logger->controller->home_cpu, msg)) msg->on_cpu_switch();
    logger_t::logger->msg = NULL;
}

/* The log_controller_t is responsible for starting and shutting down the per-thread
logger_t objects, and for actually writing the log messages. */

log_controller_t::log_controller_t(cmd_config_t *cmd_config)
    : state(state_off), cmd_config(cmd_config), log_file(NULL) { }

log_controller_t::~log_controller_t() {
    assert(state == state_off);
    assert(log_file == NULL);
}

struct start_logger_message_t :
    public cpu_message_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, start_logger_message_t>
{
    enum state_t {
        state_go_to_cpu,
        state_return_from_cpu
    } state;
    log_controller_t *controller;
    
    void on_cpu_switch() {
        switch (state) {
        
            case state_go_to_cpu:
                logger_t::logger = new logger_t(controller);
                state = state_return_from_cpu;
                if (continue_on_cpu(controller->home_cpu, this)) on_cpu_switch();
                break;
            
            case state_return_from_cpu:
            
                controller->messages_out--;
                if (controller->messages_out == 0) {
                    assert(controller->state == log_controller_t::state_starting_loggers);
                    controller->state = log_controller_t::state_ready;
                    if (controller->ready_callback) controller->ready_callback->on_logger_ready();
                }
                    
                delete this;
                break;
            
            default: fail("Bad state");
        }
    }
};

bool log_controller_t::start(ready_callback_t *ready_cb) {
    
    assert(state == state_off);
    state = state_starting_loggers;
    
    // Open the log file
    
    if (*cmd_config->log_file_name) {
        log_file = fopen(cmd_config->log_file_name, "w");
    } else {
        log_file = stderr;
    }
    
    // Start the per-thread logger objects
    
    ready_callback = NULL;   // In case all of the start_logger_message_ts finish immediately
    
    messages_out = get_num_cpus();
    for (int i = 0; i < get_num_cpus(); i++) {
        start_logger_message_t *m = new start_logger_message_t();
        m->state = start_logger_message_t::state_go_to_cpu;
        m->controller = this;
        if (continue_on_cpu(i, m)) m->on_cpu_switch();
    }
    
    if (messages_out == 0) {
        // This happens when there is only one thread: the one we are on
        return true;
    } else {
        ready_callback = ready_cb;
        return false;
    }
}

void log_controller_t::writef(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    vfprintf(log_file, format, arg);
    va_end(arg);
}

void log_controller_t::write(const char *str) {
    writef("%s", str);
}

struct shutdown_logger_message_t :
    public cpu_message_t,
    public logger_t::shutdown_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, shutdown_logger_message_t>
{
    enum state_t {
        state_go_to_cpu,
        state_wait_for_logger,
        state_return_from_cpu
    } state;
    log_controller_t *controller;
    
    void on_cpu_switch() {
        switch (state) {
        
            case state_go_to_cpu:
                state = state_wait_for_logger;
                if (logger_t::logger->shutdown(this)) on_logger_shutdown();
                break;
            
            case state_return_from_cpu:
            
                controller->messages_out--;
                if (controller->messages_out == 0) controller->finish_shutting_down();
                    
                delete this;
                break;
            
            default: fail("Bad state");
        }
    }
    
    void on_logger_shutdown() {
        assert(state == state_wait_for_logger);
        
        delete logger_t::logger;
        logger_t::logger = NULL;
        
        state = state_return_from_cpu;
        if (continue_on_cpu(controller->home_cpu, this)) on_cpu_switch();
    }
};

bool log_controller_t::shutdown(shutdown_callback_t *cb) {
    
    assert(state == state_ready);
    state = state_shutting_down_loggers;
    
    shutdown_callback = NULL;   // In case all of the shutdown_logger_message_ts finish immediately
    
    messages_out = get_num_cpus();
    for (int i = 0; i < get_num_cpus(); i++) {
        shutdown_logger_message_t *m = new shutdown_logger_message_t();
        m->state = shutdown_logger_message_t::state_go_to_cpu;
        m->controller = this;
        if (continue_on_cpu(i, m)) m->on_cpu_switch();
    }
    
    if (messages_out == 0) {
        return true;
    } else {
        shutdown_callback = cb;
        return false;
    }
}

void log_controller_t::finish_shutting_down() {
    
    assert(state == state_shutting_down_loggers);
    state = state_off;
    
    if (log_file && log_file != stderr) {
        fclose(log_file);
    }
    log_file = NULL;
    
    if (shutdown_callback) shutdown_callback->on_logger_shutdown();
}