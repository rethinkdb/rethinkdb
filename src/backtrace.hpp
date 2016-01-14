// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BACKTRACE_HPP_
#define BACKTRACE_HPP_

#include <stdio.h>
#include <time.h>

#include <map>
#include <string>
#include <vector>

#include "containers/scoped.hpp"

/* `demangle_cpp_name()` attempts to de-mangle the given symbol name. If it
succeeds, it returns the result as a `std::string`. If it fails, it throws
`demangle_failed_exc_t`. */
struct demangle_failed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Could not demangle C++ name.";
    }
};
std::string demangle_cpp_name(const char *mangled_name);

#ifndef _WIN32
class address_to_line_t {
public:
    address_to_line_t() { }

    /* Returns an empty string if the line could not be found */
    std::string address_to_line(const std::string &executable, const void *address);

private:
    bool run_addr2line(const std::string &executable, const void *address, char *line, int line_size);

    // Internal helper class:
    class addr2line_t {
    public:
        explicit addr2line_t(const char *executable);
        ~addr2line_t();

        FILE *input, *output;
        bool bad;
    private:
        int child_in[2], child_out[2];
        pid_t pid;
        DISABLE_COPYING(addr2line_t);
    };

    // "Cache" to re-use addr2line processes
    std::map<std::string, scoped_ptr_t<addr2line_t> > procs;

    DISABLE_COPYING(address_to_line_t);
};
#endif

std::string format_backtrace();

// An individual backtrace frame
class backtrace_frame_t {
public:
    explicit backtrace_frame_t(const void *_addr);

    // Initializes symbols_line, filename, function and offset strings.
    void initialize_symbols();

    std::string get_filename() const;
    std::string get_name() const;
    std::string get_demangled_name() const;
    std::string get_offset() const;
    std::string get_symbols_line() const;
    const void *get_addr() const;
private:
    std::string filename, function, offset, symbols_line;
    bool symbols_initialized;
    const void *addr;
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
