// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BACKTRACE_HPP_
#define BACKTRACE_HPP_

#include <execinfo.h>
#include <stdio.h>
#include <time.h>

#include <string>

#include "errors.hpp"

/* `demangle_cpp_name()` attempts to de-mangle the given symbol name. If it
succeeds, it returns the result as a `std::string`. If it fails, it throws
`demangle_failed_exc_t`. */
struct demangle_failed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Could not demangle C++ name.";
    }
};
std::string demangle_cpp_name(const char *mangled_name);

std::string print_frames(void **stack_frames, int size, bool use_addr2line);
std::string format_backtrace(bool use_addr2line = true);

// Stores the backtrace from when it was constructed for later printing.
class lazy_backtrace_t {
public:
    lazy_backtrace_t();
    std::string addrs(); // A well-formateed backtrace with source lines (slow)
    std::string lines(); // A raw backtrace with assembly addresses (fast)
private:
    static const int max_frames = 100;
    void *stack_frames[max_frames];
    int size;
    std::string cached_addrs, cached_lines;

    time_t timestamp;
    std::string timestr;

    DISABLE_COPYING(lazy_backtrace_t);
};

#endif /* BACKTRACE_HPP_ */
