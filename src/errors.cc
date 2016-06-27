// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "errors.hpp"

#ifndef _MSC_VER
#include <cxxabi.h>
#endif

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
#include "clustering/administration/logs/log_writer.hpp"
#include "arch/timing.hpp"
#include "arch/runtime/thread_pool.hpp"

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
    logERR("Error in thread %d in %s at line %d:", get_thread_id().threadnum, file, line);
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
#ifdef __GLIBC__
    return strerror_r(errsv, buf, buflen);
#elif defined(_WIN32)
    UNUSED errno_t res = strerror_s(buf, buflen, errsv);
    return buf;
#else
    // The result is either 0 or ERANGE (if the buffer is too small) or EINVAL (if the error number
    // is invalid), but in either case a friendly nul-terminated buffer is written.
    UNUSED int res = strerror_r(errsv, buf, buflen);
    return buf;
#endif
}

#ifdef _WIN32
MUST_USE const std::string winerr_string(DWORD winerr) {
    char *errmsg;
    DWORD res = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, winerr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errmsg, 0, nullptr);
    if (res != 0) {
        size_t end = strlen(errmsg);
        while (end > 0 && isspace(errmsg[end-1])) {
            end--;
        }
        errmsg[end] = 0;
        std::string ret(errmsg);
        LocalFree(errmsg);
        return ret;
    } else {
        return strprintf("Unkown error 0x%x (error formatting the error: 0x%x)", winerr, GetLastError());
    }
}
#endif

/* Handlers for various signals and for unexpected exceptions or calls to std::terminate() */

#ifndef _WIN32
NORETURN void generic_crash_handler(int signum) {
    if (signum == SIGSEGV) {
        crash("Segmentation fault.");
    } else if (signum == SIGBUS) {
        crash("Bus error.");
    } else {
        crash("Unexpected signal: %d", signum);
    }
}
#endif

NORETURN void rdb_terminate_handler() {
#ifdef _WIN32
    bool t = std::uncaught_exception();
#else
    std::type_info *t = abi::__cxa_current_exception_type();
#endif
    if (t) {
#ifdef _WIN32
        // On windows, uncaught exceptions are handled in windows_crash_handler
        std::string name = "unknown";
#else
        std::string name;
        try {
            name = demangle_cpp_name(t->name());
        } catch (const demangle_failed_exc_t &) {
            name = t->name();
        }
#endif
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

#ifdef _WIN32
LONG WINAPI windows_crash_handler(EXCEPTION_POINTERS *exception) {
    std::string message;
    EXCEPTION_RECORD *record = exception->ExceptionRecord;
    switch (record->ExceptionCode) {
    case 0xe06d7363:
        try {
            _CxxThrowException(reinterpret_cast<void*>(record->ExceptionInformation[1]),
                               reinterpret_cast<_ThrowInfo*>(record->ExceptionInformation[2]));
        } catch (std::exception &e) {
            message = strprintf("Unhandled C++ exception: %s: %s", typeid(e).name(), e.what());
        } catch (...) {
            message = "Unhandled C++ exception";
        }
        break;
    case EXCEPTION_BREAKPOINT:
        return EXCEPTION_EXECUTE_HANDLER;
    case EXCEPTION_ACCESS_VIOLATION:
        message = "ACCESS_VIOLATION: The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        message = "ARRAY_BOUNDS_EXCEEDED: The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        message = "DATATYPE_MISALIGNMENT: The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        message = "FLT_DENORMAL_OPERAND: One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        message = "FLT_DIVIDE_BY_ZERO: The thread tried to divide a floating-point value by a floating-point divisor of zero.";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        message = "FLT_INEXACT_RESULT: The result of a floating-point operation cannot be represented exactly as a decimal fraction.";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        message = "FLT_INVALID_OPERATION: This exception represents any floating-point exception not included in this list.";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        message = "FLT_OVERFLOW: The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        message = "FLT_STACK_CHECK: The stack overflowed or underflowed as the result of a floating-point operation.";
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        message = "FLT_UNDERFLOW: The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.";
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        message = "ILLEGAL_INSTRUCTION: The thread tried to execute an invalid instruction.";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        message = "IN_PAGE_ERROR: The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        message = "INT_DIVIDE_BY_ZERO: The thread tried to divide an integer value by an integer divisor of zero.";
        break;
    case EXCEPTION_INT_OVERFLOW:
        message = "INT_OVERFLOW: The result of an integer operation caused a carry out of the most significant bit of the result.";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        message = "INVALID_DISPOSITION: An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        message = "NONCONTINUABLE_EXCEPTION: The thread tried to continue execution after a noncontinuable exception occurred.";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        message = "PRIV_INSTRUCTION: The thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
        break;
    case EXCEPTION_SINGLE_STEP:
        message = "SINGLE_STEP: A trace trap or other single-instruction mechanism signaled that one instruction has been executed.";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        message = "STACK_OVERFLOW: The thread used up its stack.";
        break;
    default:
        message = "Unkown exception.";
    }

    logERR("Windows exception 0x%x: %s", exception->ExceptionRecord->ExceptionCode, message.c_str());
    logERR("backtrace:\n%s", format_backtrace().c_str());

    // This usually results in process termination
    return EXCEPTION_EXECUTE_HANDLER;
}

int windows_runtime_debug_failure_handler(int type, char *message, int *retval) {
    logERR("run-time debug failure:\n%s", message);
    logERR("backtrace:\n%s", format_backtrace().c_str());

    return false;
}

BOOL WINAPI windows_ctrl_handler(DWORD type) {
    thread_pool_t::interrupt_handler(type);
    return true;
}
#endif

void install_generic_crash_handler() {
#ifdef _WIN32
    // TODO WINDOWS: maybe call SetErrorMode
    SetUnhandledExceptionFilter(windows_crash_handler);
    SetConsoleCtrlHandler(windows_ctrl_handler, true);
#ifdef _MSC_VER
    _CrtSetReportHook(windows_runtime_debug_failure_handler);
#endif
#else
#ifndef VALGRIND
    {
        struct sigaction sa = make_sa_handler(0, generic_crash_handler);

        int res = sigaction(SIGSEGV, &sa, nullptr);
        guarantee_err(res == 0, "Could not install SEGV signal handler");
        res = sigaction(SIGBUS, &sa, nullptr);
        guarantee_err(res == 0, "Could not install BUS signal handler");
    }
#endif

    struct sigaction sa = make_sa_handler(0, SIG_IGN);
    int res = sigaction(SIGPIPE, &sa, nullptr);
    guarantee_err(res == 0, "Could not install PIPE handler");
#endif

    std::set_terminate(&rdb_terminate_handler);
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
