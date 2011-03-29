#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// The file to write log messages to. It defaults to stderr, but main() may set it to something
// different.
extern FILE *log_file;

/* These functions log things. */

enum log_level_t { DBG = 0, INF = 1, WRN, ERR };

// Log a message in one chunk. You still have to provide '\n'.

// logf is a standard library function in <math.h>.  So we use _logf.

void _logf(const char *src_file, int src_line, log_level_t level, const char *format, ...)
    __attribute__ ((format (printf, 4, 5)));

#ifndef NDEBUG
#define logDBG(fmt, args...) _logf(__FILE__, __LINE__, DBG, (fmt), ##args)
#else
#define logDBG(fmt, args...) ((void)0)
#endif

#define logINF(fmt, args...) _logf(__FILE__, __LINE__, INF, (fmt), ##args)
#define logWRN(fmt, args...) _logf(__FILE__, __LINE__, WRN, (fmt), ##args)
#define logERR(fmt, args...) _logf(__FILE__, __LINE__, ERR, (fmt), ##args)

#ifndef NDEBUG
#define log_call(fn, args...) do {                                          \
        debugf("%s:%u: %s: entered\n", __FILE__, __LINE__, stringify(fn));  \
        fn(args);                                                           \
        debugf("%s:%u: %s: returned\n", __FILE__, __LINE__, stringify(fn)); \
    } while (0)
#else
#define log_call(fn, args...) fn(args)
#endif

// Log a message in pieces.

void _mlog_start(const char *src_file, int src_line, log_level_t level);
#define mlog_start(lvl) (_mlog_start(__FILE__, __LINE__, (lvl)))

void mlogf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

void mlog_end();

/* The logger has two modes. During the main phase of the server running, it will send all log
messages to one thread and write them to the file from there; this makes it so that we don't block
on the log file's lock. During the startup and shutdown phases, we just write messages directly
from the file and don't care about the performance hit from the lock. To enter the high-performance
mode, construct a log_controller_t from within a coroutine in a thread pool. */

class log_controller_t
{

public:
    // The constructor and destructor are potentially blocking.
    log_controller_t();
    ~log_controller_t();

    int home_thread;
};

#endif // __LOGGER_HPP__
