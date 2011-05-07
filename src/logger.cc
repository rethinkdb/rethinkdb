#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "logger.hpp"
#include <string.h>
#include "utils.hpp"
#include "arch/arch.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/pmap.hpp"

FILE *log_file = stderr;

/* As an optimization, during the main phase of the server's running we route all log messages
to one thread to write them. */

static __thread log_controller_t *log_controller = NULL;   // Non-NULL if we are routing log messages
static __thread rwi_lock_t *log_controller_lock = NULL;

static void install_log_controller(log_controller_t *lc, int thread) {
    on_thread_t thread_switcher(thread);
    rassert(!log_controller);
    log_controller = lc;
    log_controller_lock = new rwi_lock_t;
}

log_controller_t::log_controller_t() {
    home_thread = get_thread_id();
    pmap(get_num_threads(), boost::bind(&install_log_controller, this, _1));
}

static void uninstall_log_controller(UNUSED log_controller_t *lc, int thread) {
    on_thread_t thread_switcher(thread);
    rassert(log_controller == lc);
    log_controller = NULL;
    log_controller_lock->co_lock(rwi_write);
    delete log_controller_lock;
}

log_controller_t::~log_controller_t() {
    pmap(get_num_threads(), boost::bind(&uninstall_log_controller, this, _1));
}

/* Log message class. */

struct log_message_t
{
    std::vector<char> contents;   // Not NUL-terminated

    void vprintf(const char *format, va_list arg) {

        /* We don't know how much space it will take to format the message. So we initially guess
        a reasonable size, and if that's insufficient then we try again with exactly as much space
        as is needed. */
        static const int chunk_length_guess = 200;

        /* vsnprintf mutates its 'arg' parameter, so we need to duplicate it in case we will be
        calling it a second time. */
        va_list arg2;
        va_copy(arg2, arg);

        int old_length = contents.size();
        contents.resize(old_length + chunk_length_guess + 1);   // +1 for NUL-terminator
        int chunk_length = vsnprintf(
            contents.data() + old_length,
            chunk_length_guess + 1,   // +1 for NUL-terminator
            format, arg);

        if (chunk_length == -1) {
            /* Abort; something went wrong somehow */
            contents.resize(old_length);

        } else if (chunk_length <= chunk_length_guess) {
            /* Our guess was sufficient; the entire message was printed. Now shrink the
            array again to remove any unnecessary space and to cut off the NUL-terminator. */
            contents.resize(old_length + chunk_length);

        } else {
            /* Our guess was insufficient; grow the buffer and print it again. Fortunately,
            'chunk_length' is now exactly as much space as we need. */
            contents.resize(old_length + chunk_length + 1);   // +1 for NUL-terminator
            int chunk_length_2 __attribute__((unused)) = vsnprintf(
                contents.data() + old_length,
                chunk_length + 1,   // +1 for NUL-terminator
                format, arg2);
            rassert(chunk_length_2 == chunk_length);   /* snprintf should be deterministic... */

            /* Cut off NUL-terminator */
            contents.resize(old_length + chunk_length);
        }
    }
};

/* Functions to actually do the logging */

static __thread log_message_t *current_message;

void _logf(const char *src_file, int src_line, log_level_t level, const char *format, ...) {

    log_message_t *old_message = current_message;
    current_message = NULL;

    _mlog_start(src_file, src_line, level);
    va_list arg;
    va_start(arg, format);
    current_message->vprintf(format, arg);
    va_end(arg);
    mlog_end();
    
    current_message = old_message;
}

void _mlog_start(const char *src_file, int src_line, log_level_t level) {

    rassert(!current_message);
    current_message = new log_message_t;

    const char *level_str;
    switch (level) {
        case DBG: level_str = "debug"; break;
        case INF: level_str = "info"; break;
        case WRN: level_str = "warn"; break;
        case ERR: level_str = "error"; break;
        default:  unreachable();
    };

    precise_time_t t = get_time_now();
    char formatted_time[formatted_precise_time_length+1];
    format_precise_time(t, formatted_time, sizeof(formatted_time));

    /* If the log controller hasn't been started yet, then assume the thread pool hasn't been
    started either, so don't write which core the message came from. */
    if (log_controller) mlogf("%s %s (Q%d, %s:%d): ", formatted_time, level_str, get_thread_id(), src_file, src_line);
    else mlogf("%s %s (%s:%d): ", formatted_time, level_str, src_file, src_line);
}

void mlogf(const char *format, ...) {
    rassert(current_message);
    va_list arg;
    va_start(arg, format);
    current_message->vprintf(format, arg);
    va_end(arg);
}

void write_log_message(log_message_t *msg, log_controller_t *lc) {

    {
        on_thread_t thread_switcher(lc->home_thread);
        fwrite(msg->contents.data(), 1, msg->contents.size(), log_file);
    }

    delete msg;
    log_controller_lock->unlock();
}

void mlog_end() {
    rassert(current_message);
    if (!log_controller) {
        fwrite(current_message->contents.data(), 1, current_message->contents.size(), log_file);
        delete current_message;
    } else {
        log_controller_lock->co_lock(rwi_read);
        coro_t::spawn(boost::bind(&write_log_message, current_message, log_controller));
    }
    current_message = NULL;
}
