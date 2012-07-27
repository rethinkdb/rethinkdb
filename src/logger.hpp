#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <string>

#include <stdio.h>

enum log_level_t { log_level_debug = 0, log_level_info = 1, log_level_warn, log_level_error };

std::string get_logfilepath(const std::string &filepath);

void install_primary_log_writer(const std::string &logfile_name);

/* These functions are implemented in `clustering/administration/logger.cc`.
This header file exists so that anything can call them without having to include
the same things that `clustering/administration/logger.hpp` does. */

void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...)
    __attribute__ ((format (printf, 4, 5)));

void vlog_internal(const char *src_file, int src_line, log_level_t level, const char *format, va_list args);

#ifndef NDEBUG
#define logDBG(fmt, args...) log_internal(__FILE__, __LINE__, log_level_debug, (fmt), ##args)
#define vlogDBG(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_debug, (fmt), (args))
#else
#define logDBG(fmt, args...) ((void)0)
#define vlogDBG(fmt, args) ((void)0)
#endif

#define logINF(fmt, args...) log_internal(__FILE__, __LINE__, log_level_info, (fmt), ##args)
#define logWRN(fmt, args...) log_internal(__FILE__, __LINE__, log_level_warn, (fmt), ##args)
#define logERR(fmt, args...) log_internal(__FILE__, __LINE__, log_level_error, (fmt), ##args)

#define vlogINF(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_info, (fmt), (args))
#define vlogWRN(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_warn, (fmt), (args))
#define vlogERR(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_error, (fmt), (args))

#endif // LOGGER_HPP_
