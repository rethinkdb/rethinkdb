// Copyright 2010-2014 RethinkDB, all rights reserved.

/**
 * @file backtrace.hpp
 * @brief Stack trace capture and symbol demangling utilities.
 *
 * Provides functions for capturing, formatting, and printing stack traces
 * with optional source line information via addr2line.
 */

#ifndef BACKTRACE_HPP_
#define BACKTRACE_HPP_

#include <stdio.h>
#include <time.h>

#include <map>
#include <string>
#include <vector>

#include "containers/scoped.hpp"

/**
 * @defgroup Backtracing Stack Trace Utilities
 * @brief Backtrace capture and formatting
 */

/**
 * @ingroup Backtracing
 * @brief Exception thrown when C++ symbol demangling fails.
 */
struct demangle_failed_exc_t : public std::exception {
    /**
     * @brief Get the error message.
     *
     * @return "Could not demangle C++ name."
     */
    const char *what() const throw () {
        return "Could not demangle C++ name.";
    }
};

/**
 * @ingroup Backtracing
 * @brief Demangle a C++ mangled symbol name.
 *
 * Converts an ABI-mangled name (as from dlsym() or nm) back to the
 * original C++ declaration.
 *
 * @param mangled_name The mangled symbol name.
 * @return The demangled name.
 * @throw demangle_failed_exc_t if demangling fails.
 *
 * Example:
 * @code
 * std::string demangled = demangle_cpp_name("_ZN3std6vectorIiE5sizeEv");
 * // Returns: "std::vector<int>::size()"
 * @endcode
 */
std::string demangle_cpp_name(const char *mangled_name);

/**
 * @ingroup Backtracing
 * @brief Convert addresses to source code lines using addr2line.
 *
 * (Only available on non-Windows platforms)
 *
 * Uses the addr2line utility to convert instruction addresses to
 * source file locations. Caches addr2line processes for efficiency.
 */
#if !defined(_WIN32) && !defined(RDB_NO_BACKTRACE)
class address_to_line_t {
public:
    /**
     * @brief Construct an address-to-line converter.
     */
    address_to_line_t() { }

    /**
     * @brief Convert an instruction address to source code location.
     *
     * @param executable The path to the executable with debug symbols.
     * @param address The instruction address to look up.
     * @return The source location (filename:line), or empty string if not found.
     */
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

/**
 * @ingroup Backtracing
 * @brief Format the current stack trace as a string.
 *
 * @param use_addr2line Whether to use addr2line for source lines (slower but more informative).
 * @return A formatted stack trace string.
 */
std::string format_backtrace(bool use_addr2line = true);

/**
 * @ingroup Backtracing
 * @brief A single frame in a stack trace.
 *
 * (Only available when RDB_NO_BACKTRACE is not defined)
 *
 * Represents one function call in a stack trace with address,
 * filename, function name, and optional source line information.
 */
#if !defined(RDB_NO_BACKTRACE)
class backtrace_frame_t {
public:
    /**
     * @brief Construct a frame from an instruction address.
     *
     * @param _addr The instruction pointer address.
     */
    explicit backtrace_frame_t(const void *_addr);

    /**
     * @brief Look up and initialize symbol information.
     *
     * Resolves filename, function name, and offset from the address.
     * Can be slow if symbol information is not available.
     */
    void initialize_symbols();

    /**
     * @brief Get the source filename.
     *
     * @return The filename containing this function, or empty string.
     */
    std::string get_filename() const;

    /**
     * @brief Get the mangled function name.
     *
     * @return The symbol name from the symbol table.
     */
    std::string get_name() const;

    /**
     * @brief Get the demangled function name.
     *
     * @return The C++ function signature, or mangled name if demangling fails.
     */
    std::string get_demangled_name() const;

    /**
     * @brief Get the offset within the function.
     *
     * @return The offset as a hex string.
     */
    std::string get_offset() const;

    /**
     * @brief Get the raw symbols line from nm/addr2line.
     *
     * @return The raw symbol lookup result.
     */
    std::string get_symbols_line() const;

    /**
     * @brief Get the instruction address.
     *
     * @return The address pointer.
     */
    const void *get_addr() const;
private:
    std::string filename, function, offset, symbols_line;
    bool symbols_initialized;
    const void *addr;
};
#endif  // !defined(RDB_NO_BACKTRACE)

/**
 * @ingroup Backtracing
 * @brief A complete stack trace.
 *
 * Contains multiple frames captured at a point in time.
 * Can be formatted for printing with optional source line lookup.
 */
class backtrace_t {
public:
    /**
     * @brief Capture the current stack trace.
     */
    backtrace_t();

#if !defined(RDB_NO_BACKTRACE)
    /**
     * @brief Get the number of captured frames.
     *
     * @return The frame count.
     */
    size_t get_num_frames() const { return frames.size(); }

    /**
     * @brief Get a specific frame.
     *
     * @param i The frame index (0 is innermost).
     * @return Reference to the frame.
     */
    const backtrace_frame_t &get_frame(const size_t i) const { return frames[i]; }
#endif

    /**
     * @brief Format all frames for printing.
     *
     * @param use_addr2line Whether to look up source lines.
     * @return A formatted backtrace string.
     */
    std::string print_frames(bool use_addr2line) const;
private:
#if !defined(RDB_NO_BACKTRACE)
    static const int max_frames = 100;
    std::vector<backtrace_frame_t> frames;
#endif
};

/**
 * @ingroup Backtracing
 * @brief Lazily-formatted backtrace with caching.
 *
 * Captures the backtrace at construction, but defers formatting
 * until requested. Caches both raw address and full source-line versions.
 *
 * Example:
 * @code
 * auto bt = lazy_backtrace_formatter_t();
 * std::cout << bt.addrs();   // Fast: just addresses
 * std::cout << bt.lines();   // Slow: with source lines
 * @endcode
 */
class lazy_backtrace_formatter_t : public backtrace_t {
public:
    /**
     * @brief Capture and store the current backtrace.
     */
    lazy_backtrace_formatter_t();

    /**
     * @brief Get raw backtrace with addresses (fast).
     *
     * @return Formatted backtrace with addresses only.
     */
    std::string addrs();

    /**
     * @brief Get formatted backtrace with source lines (slow).
     *
     * @return Formatted backtrace with symbol and source information.
     */
    std::string lines();
private:
    std::string cached_addrs, cached_lines;

    time_t timestamp;
    std::string timestr;

    DISABLE_COPYING(lazy_backtrace_formatter_t);
};

#endif /* BACKTRACE_HPP_ */
