// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <stdio.h>
#include <string>

enum log_level_t { log_level_debug = 0, log_level_info = 1, log_level_notice, log_level_warn, log_level_error };

/* These functions are implemented in `clustering/administration/logs/log_writer.cc`.
This header file exists so that anything can call them without having to include the same
things that `clustering/administration/logs/log_writer.hpp` does. */

void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...)
    __attribute__ ((format (printf, 4, 5)));

void vlog_internal(const char *src_file, int src_line, log_level_t level, const char *format, va_list args);

#ifndef NDEBUG
#define logDBG(fmt, ...) log_internal(__FILE__, __LINE__, log_level_debug, (fmt), ##__VA_ARGS__)
#define vlogDBG(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_debug, (fmt), (args))
#else
#define logDBG(fmt, ...) ((void)0)
#define vlogDBG(fmt, args) ((void)0)
#endif

#define logINF(fmt, ...) log_internal(__FILE__, __LINE__, log_level_info, (fmt), ##__VA_ARGS__)
#define logNTC(fmt, ...) log_internal(__FILE__, __LINE__, log_level_notice, (fmt), ##__VA_ARGS__)
#define logWRN(fmt, ...) log_internal(__FILE__, __LINE__, log_level_warn, (fmt), ##__VA_ARGS__)
#define logERR(fmt, ...) log_internal(__FILE__, __LINE__, log_level_error, (fmt), ##__VA_ARGS__)

#define vlogINF(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_info, (fmt), (args))
#define vlogNTC(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_notice, (fmt), (args))
#define vlogWRN(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_warn, (fmt), (args))
#define vlogERR(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_error, (fmt), (args))

#endif // LOGGER_HPP_
