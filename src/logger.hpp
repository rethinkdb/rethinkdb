#ifndef LOGGER_HPP_
#define LOGGER_HPP_

/* These functions are implemented in `clustering/administration/logger.cc`.
This header file exists so that anything can call them without having to include
the same things that `clustering/administration/logger.hpp` does. */

enum log_level_t { DBG = 0, INF = 1, WRN, ERR };

void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...)
    __attribute__ ((format (printf, 4, 5)));

#ifndef NDEBUG
#define logDBG(fmt, args...) log_internal(__FILE__, __LINE__, DBG, (fmt), ##args)
#else
#define logDBG(fmt, args...) ((void)0)
#endif

#define logINF(fmt, args...) log_internal(__FILE__, __LINE__, INF, (fmt), ##args)
#define logWRN(fmt, args...) log_internal(__FILE__, __LINE__, WRN, (fmt), ##args)
#define logERR(fmt, args...) log_internal(__FILE__, __LINE__, ERR, (fmt), ##args)

#endif // LOGGER_HPP_
