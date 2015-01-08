// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "errors.hpp"

#include <cxxabi.h>

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <typeinfo>

#include "logger.hpp"
#include "utils.hpp"
#include "backtrace.hpp"
#include "thread_local.hpp"
#include "clustering/administration/log_writer.hpp"
#include "arch/timing.hpp"

TLS_with_init(bool, crashed, false); // to prevent crashing within crashes

NOINLINE int get_errno() {
    return errno;
}
NOINLINE void set_errno(int new_errno) {
    errno = new_errno;
}

NORETURN void crash_oom() {
    const char *message = "rethinkdb: Memory allocation failed. This usually means "
                          "that we have run out of RAM. Aborting.\n";
    UNUSED size_t res = fwrite(message, 1, strlen(message), stderr);
    abort();
}

void report_user_error(const char *msg, ...) {
    fprintf(stderr, "Version: %s\n", RETHINKDB_VERSION_STR);
    if (TLS_get_crashed()) {
        va_list args;
        va_start(args, msg);
        fprintf(stderr, "Crashing while already crashed. Printing error message to stderr.\n");
        vfprintf(stderr, msg, args);
        va_end(args);
        return;
    }

    TLS_set_crashed(true);

    thread_log_writer_disabler_t disabler;

    va_list args;
    va_start(args, msg);
    vlogERR(msg, args);
    va_end(args);
}

void report_fatal_error(const char *file, int line, const char *msg, ...) {
    fprintf(stderr, "Version: %s\n", RETHINKDB_VERSION_STR);
    if (TLS_get_crashed()) {
        va_list args;
        va_start(args, msg);
        fprintf(stderr, "Crashing while already crashed. Printing error message to stderr.\n");
        vfprintf(stderr, msg, args);
        va_end(args);
        return;
    }

    TLS_set_crashed(true);

    thread_log_writer_disabler_t disabler;

    va_list args;
    va_start(args, msg);
    logERR("Error in %s at line %d:", file, line);
    vlogERR(msg, args);
    va_end(args);

    /* Don't print backtraces in valgrind mode because valgrind issues lots of spurious
    warnings when print_backtrace() is run. */
#if !defined(VALGRIND)
    logERR("Backtrace:");
    logERR("%s", format_backtrace().c_str());
#endif

    logERR("Exiting.");
}

const char *errno_string_maybe_using_buffer(int errsv, char *buf, size_t buflen) {
#ifdef _GNU_SOURCE
    return strerror_r(errsv, buf, buflen);
#else
    // The result is either 0 or ERANGE (if the buffer is too small) or EINVAL (if the error number
    // is invalid), but in either case a friendly nul-terminated buffer is written.
    UNUSED int res = strerror_r(errsv, buf, buflen);
    return buf;
#endif  // _GNU_SOURCE
}

/* Handlers for various signals and for unexpected exceptions or calls to std::terminate() */

NORETURN void generic_crash_handler(int signum) {
    if (signum == SIGSEGV) {
        crash("Segmentation fault.");
    } else {
        crash("Unexpected signal: %d", signum);
    }
}

NORETURN void terminate_handler() {
    std::type_info *t = abi::__cxa_current_exception_type();
    if (t) {
        std::string name;
        try {
            name = demangle_cpp_name(t->name());
        } catch (const demangle_failed_exc_t &) {
            name = t->name();
        }
        try {
            /* This will rethrow whatever unexpected exception was thrown. */
            throw;
        } catch (const std::exception& e) {
            crash("Uncaught exception of type \"%s\"\n  what(): %s", name.c_str(), e.what());
        } catch (...) {
            crash("Uncaught exception of type \"%s\"", name.c_str());
        }
        unreachable();
    } else {
        crash("std::terminate() called without any exception.");
    }
}

void install_generic_crash_handler() {

#ifndef VALGRIND
    {
        struct sigaction sa = make_sa_handler(0, generic_crash_handler);

        int res = sigaction(SIGSEGV, &sa, NULL);
        guarantee_err(res == 0, "Could not install SEGV handler");
    }
#endif

    struct sigaction sa = make_sa_handler(0, SIG_IGN);
    int res = sigaction(SIGPIPE, &sa, NULL);
    guarantee_err(res == 0, "Could not install PIPE handler");

    std::set_terminate(&terminate_handler);
}

/* If a call to `operator new()` or `operator new[]()` fails, we have to crash
immediately.
The default behavior of this handler is to throw an `std::bad_alloc` exception.
However that is dangerous because it unwinds the stack and can cause side-effects
including data corruption. */
NORETURN void new_oom_handler() {
    crash_oom();
}

void install_new_oom_handler() {
    std::set_new_handler(&new_oom_handler);
}


/* Boost will call this function when an assertion fails. */

namespace boost {

void assertion_failed(char const * expr, char const * function, char const * file, long line) { // NOLINT(runtime/int)
    report_fatal_error(file, line, "BOOST_ASSERT failure in '%s': %s", function, expr);
    BREAKPOINT;
}

void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line) { // NOLINT(runtime/int)
    report_fatal_error(file, line, "BOOST_ASSERT_MSG failure in '%s': %s (%s)", function, expr, msg);
    BREAKPOINT;
}

}  // namespace boost
