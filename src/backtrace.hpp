// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BACKTRACE_HPP_
#define BACKTRACE_HPP_

#include <stdio.h>
#include <time.h>

#include <string>
#include <vector>

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

std::string format_backtrace(bool use_addr2line = true);

// An individual backtrace frame
class backtrace_frame_t {
public:
    explicit backtrace_frame_t(void *_addr);
    std::string get_filename() const;
    std::string get_name() const;
    std::string get_demangled_name() const;
    std::string get_offset() const;
    void *get_addr() const;
private:
    std::string filename, function, offset;
    void *addr;
};

// A backtrace, consisting of backtrace_frame_t
class backtrace_t {
public:
    backtrace_t();
    size_t get_num_frames() const { return frames.size(); }
    const backtrace_frame_t &get_frame(const size_t i) const { return frames[i]; }
private:
    static const int max_frames = 100;
    std::vector<backtrace_frame_t> frames;
};

// Stores the backtrace from when it was constructed for later printing.
class lazy_backtrace_formatter_t : public backtrace_t {
public:
    lazy_backtrace_formatter_t();
    std::string addrs(); // A well-formatted backtrace with source lines (slow)
    std::string lines(); // A raw backtrace with assembly addresses (fast)
private:
    std::string cached_addrs, cached_lines;

    time_t timestamp;
    std::string timestr;
    
    std::string print_frames(bool use_addr2line);

    DISABLE_COPYING(lazy_backtrace_formatter_t);
};

#endif /* BACKTRACE_HPP_ */
