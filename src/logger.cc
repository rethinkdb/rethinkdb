#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "logger.hpp"
#include <string.h>
#include "utils.hpp"
#include "arch/arch.hpp"

FILE *log_file = stderr;

/* As an optimization, during the main phase of the server's running we route all log messages
to one thread to write them. */

struct log_controller_t;

static __thread log_controller_t *log_controller = NULL;   // Non-NULL if we are routing log messages

struct log_controller_t :
    public home_thread_mixin_t
{
    /* Startup process */
    
    log_controller_t() {
        shutting_down = false;
        for (int i = 0; i < get_num_threads(); i++) {
            do_on_thread(i, this, &log_controller_t::install);
        }
    }
    
    bool install() {
        assert(log_controller == NULL);
        log_controller = this;
        messages_out[get_thread_id()] = 0;
        return true;
    }
    
    /* Shutdown process */
    
    bool shutting_down;
    int num_threads_up;
    int messages_out[MAX_THREADS];
    logger_shutdown_callback_t *shutdown_callback;
    
    void shutdown(logger_shutdown_callback_t *cb) {
        shutting_down = true;
        num_threads_up = get_num_threads();
        shutdown_callback = cb;
        for (int i = 0; i < get_num_threads(); i++) {
            do_on_thread(i, this, &log_controller_t::uninstall);
        }
    }
    
    bool uninstall() {
        assert(log_controller == this);
        log_controller = NULL;
        if (messages_out[get_thread_id()] == 0)
            do_on_thread(home_thread, this, &log_controller_t::have_uninstalled);
        return true;
    }
    
    bool have_uninstalled() {
        num_threads_up--;
        if (num_threads_up == 0) {
            shutdown_callback->on_logger_shutdown();
            delete this;
        }
        return true;
    }
    
    /* Log writing process */
    
    void write(const char *ptr, size_t length) {
        char *msg = strndup(ptr, length);
        messages_out[get_thread_id()]++;
        do_on_thread(home_thread, this, &log_controller_t::do_write, msg, get_thread_id());
    }
    
    bool do_write(char *msg, int return_thread) {
        fprintf(log_file, "%s", msg);
        do_on_thread(return_thread, this, &log_controller_t::done, msg);
        return true;
    }
    
    bool done(char *msg) {
        free(msg);
        messages_out[get_thread_id()]--;
        if (shutting_down && messages_out[get_thread_id()] == 0) {
            do_on_thread(home_thread, this, &log_controller_t::have_uninstalled);
        }
        return true;
    }
};

void logger_start(logger_ready_callback_t *cb) {
    new log_controller_t();
    
    /* Call callback immediately because it's OK if log messages get written before the log
    controller finishes installing itself--there might be lock contention on the log file,
    but everything will still work. */
    cb->on_logger_ready();
}

void logger_shutdown(logger_shutdown_callback_t *cb) {
    assert(log_controller);
    log_controller->shutdown(cb);
}

/* Functions to actually do the logging */

static __thread bool logging = false;
static __thread int message_len;
static __thread char message[MAX_LOG_MSGLEN];

void mlog_flush();

static void vmlogf(const char *format, va_list arg) {
    assert(logging);
    /* If the incoming message will overflow our buffer, we need to flush
     * the message. Unfortunately we don't know whether it will until we
     * run vsnprintf, so we have to copy the argument list no matter what.*/
    char *buf = (char *) calloc(MAX_LOG_MSGLEN, sizeof(char));

    size_t max_bytes = MAX_LOG_MSGLEN - message_len;
    size_t nbytes = vsnprintf(buf, MAX_LOG_MSGLEN, format, arg);

    assert(nbytes < MAX_LOG_MSGLEN); // don't send messages > MAX_LOG_MSGLEN

    if (nbytes < max_bytes) {
        strncpy(message + message_len, buf, nbytes);
    }
    else {
        mlog_flush();
        message_len = 0;
        strncpy(message, buf, nbytes);
    }
    message_len += nbytes;
    free(buf);
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
    assert(!logging);
    logging = true;
    
    message_len = 0;
    message[0] = '\0';
    
    const char *level_str;
    switch (level) {
        case DBG: level_str = "debug"; break;
        case INF: level_str = "info"; break;
        case WRN: level_str = "warn"; break;
        case ERR: level_str = "error"; break;
        default:  unreachable();
    };
    
    /* If the log controller hasn't been started yet, then assume the thread pool hasn't been
    started either, so don't write which core the message came from. */
    
    if (log_controller) mlogf("%s (Q%d, %s:%d): ", level_str, get_thread_id(), src_file, src_line);
    else mlogf("%s (%s:%d): ", level_str, src_file, src_line);
}

void mlogf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    vmlogf(format, arg);
    va_end(arg);
}

void mlog_flush() {
    if (log_controller) {
        // Send the message to the log controller to be routed to the appropriate thread
        log_controller->write(message, message_len);
    } else {
        // Write directly to the log file from whatever thread we're on
        fwrite(message, 1, message_len, log_file);
    }
}

void mlog_end() {
    assert(logging);
    logging = false;

    mlog_flush();
}
