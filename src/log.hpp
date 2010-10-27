#ifndef __LOG_HPP__
#define __LOG_HPP__

/* These functions log things. The logging infrastructure is defined in logger.hpp, and these
functions are defined in logger.cc; the reason why they are a separate file is because the
allocator system needs to be able to call the logging system, but the logging system needs to be
able to call the allocator system. */

enum log_level_t {
#ifndef NDEBUG
    DBG = 0,
#endif
    INF = 1,
    WRN,
    ERR
};

// Log a message in one chunk. You still have to provide '\n'.

// logf is a standard library function in <math.h>.  So we use Logf.

void _Logf(const char *src_file, int src_line, log_level_t level, const char *format, ...);
#define Logf(lvl, fmt, args...) (_Logf(__FILE__, __LINE__, (lvl), (fmt) , ##args))

// Log a message in pieces.

void _mLog_start(const char *src_file, int src_line, log_level_t level);
#define mLog_start(lvl) (_mLog_start(__FILE__, __LINE__, (lvl)))

void mLogf(const char *format, ...);

void mLog_end();

#endif /* __LOG_HPP__ */
