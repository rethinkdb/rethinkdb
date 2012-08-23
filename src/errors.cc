#include "errors.hpp"

#include <cxxabi.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <typeinfo>

#include "logger.hpp"
#include "utils.hpp"
#include "backtrace.hpp"
#include "clustering/administration/logger.hpp"

static __thread bool crashed = false; // to prevent crashing within crashes

void report_user_error(const char *msg, ...) {
    if (crashed) {
        va_list args;
        va_start(args, msg);
        fprintf(stderr, "Crashing while already crashed. Printing error message to stderr.\n");
        vfprintf(stderr, msg, args);
        va_end(args);
        return;
    }

    crashed = true;

    thread_log_writer_disabler_t disabler;

    va_list args;
    va_start(args, msg);
    vlogERR(msg, args);
    va_end(args);
}

void report_fatal_errno_error(const char *file, int line, int errsv, const char *msg, ...) {
    // TODO: Umm, duplicate code.
    if (crashed) {
        va_list args;
        va_start(args, msg);
        fprintf(stderr, "Crashing while already crashed (at %s:%d). Printing error message to stderr.\n", file, line);
        vfprintf(stderr, msg, args);
        va_end(args);
        return;
    }

    crashed = true;

    thread_log_writer_disabler_t disabler;

    va_list args;
    va_start(args, msg);
    logERR("Error in %s at line %d (errno %d - %s:", file, line, errsv, strerror(errsv));
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

void report_fatal_error(const char *file, int line, const char *msg, ...) {
    if (crashed) {
        va_list args;
        va_start(args, msg);
        fprintf(stderr, "Crashing while already crashed. Printing error message to stderr.\n");
        vfprintf(stderr, msg, args);
        va_end(args);
        return;
    }

    crashed = true;

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

/* Handlers for various signals and for unexpected exceptions or calls to std::terminate() */

NORETURN void generic_crash_handler(int signum) {
    if (signum == SIGSEGV) {
        crash("Segmentation fault.");
    } else {
        crash("Unexpected signal: %d", signum);
    }
}

void ignore_crash_handler(UNUSED int signum) { }

NORETURN void terminate_handler() {
    std::type_info *t = abi::__cxa_current_exception_type();
    if (t) {
        std::string name;
        try {
            name = demangle_cpp_name(t->name());
        } catch (demangle_failed_exc_t) {
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

    struct sigaction action;
    int res;

#ifndef VALGRIND
    bzero(&action, sizeof(action));
    action.sa_handler = generic_crash_handler;
    res = sigaction(SIGSEGV, &action, NULL);
    guaranteef_err(res == 0, "Could not install SEGV handler");
#endif

    bzero(&action, sizeof(action));
    action.sa_handler = ignore_crash_handler;
    res = sigaction(SIGPIPE, &action, NULL);
    guaranteef_err(res == 0, "Could not install PIPE handler");

    std::set_terminate(&terminate_handler);
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

}
