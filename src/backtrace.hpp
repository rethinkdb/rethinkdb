// Copyright 2010-2014 RethinkDB, all rights reserved.

/**
 * @file backtrace.hpp
 * @brief Stack trace functionality for debugging and error reporting.
 *
 * Provides utilities for capturing, formatting, and displaying stack traces.
 * Includes C++ symbol demangling and address-to-line translation using addr2line.
 * Debug backtrace functionality can be disabled at compile time via RDB_NO_BACKTRACE.
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
 * @defgroup Backtrace Stack Trace Utilities
 * @brief Stack trace capture and formatting.
 */

/**
 * @ingroup Backtrace
 * @brief Exception thrown when C++ symbol demangling fails.
 *
 * Indicates that the demangling of a C++ mangled symbol name was unsuccessful.
 */
struct demangle_failed_exc_t : public std::exception {
    /**
     * @brief Returns the error message.
     * @return "Could not demangle C++ name."
     */
    const char *what() const throw () {
        return "Could not demangle C++ name.";
    }
};

/**
 * @ingroup Backtrace
 * @brief Demangles a C++ symbol name.
 *
 * Attempts to convert a C++ mangled symbol name (produced by the compiler)
 * into a human-readable demangled form. This is useful when reading backtraces
 * and symbol names from debugging information.
 *
 * @param mangled_name The mangled C++ symbol name to demangle.
 * @return The demangled symbol name.
 * @throw demangle_failed_exc_t if demangling fails.
 *
 * @code
 * try {
 *     std::string demangled = demangle_cpp_name("_ZN10MyNamespace5MyClassC1Ev");
 *     // demangled might be "MyNamespace::MyClass::MyClass()"
 * } catch (const demangle_failed_exc_t &e) {
 *     // Failed to demangle symbol
 * }
 * @endcode
 *
 * @see address_to_line_t for converting addresses to source code locations.
 */
std::string demangle_cpp_name(const char *mangled_name);

#if !defined(_WIN32) && !defined(RDB_NO_BACKTRACE)
/**
 * @ingroup Backtrace
 * @brief Translates memory addresses to source code locations.
 *
 * Converts instruction addresses to source file names and line numbers using
 * the addr2line utility. Maintains a cache of addr2line processes to improve
 * performance when translating multiple addresses from the same executable.
 *
 * @note Only available on non-Windows systems (requires addr2line utility).
 * @note This class is not available if RDB_NO_BACKTRACE is defined.
 */
class address_to_line_t {
public:
    /**
     * @brief Constructs an address translator with an empty cache.
     */
    address_to_line_t() { }

    /**
     * @brief Translates a memory address to a source code location.
     *
     * Uses addr2line to convert an executable address to source file and line number.
     * Results are not cached at this level (caching happens internally via process reuse).
     *
     * @param executable The path to the executable or shared library containing the address.
     * @param address The memory address to look up.
     * @return A string describing the source location (e.g., "myfile.cpp:42").
     *         Returns an empty string if the location could not be determined.
     *
     * @code
     * address_to_line_t translator;
     * std::string location = translator.address_to_line(
     *     "/usr/bin/myapp",
     *     reinterpret_cast<void*>(0x400c60)
     * );
     * // location might be "main.cpp:15"
     * @endcode
     */
    std::string address_to_line(const std::string &executable, const void *address);

private:
    /**
     * @brief Internal wrapper for communicating with an addr2line process.
     *
     * Manages the subprocess communication for addr2line utility.
     */
    class addr2line_t {
    public:
        /**
         * @brief Starts an addr2line process for the given executable.
         * @param executable The path to the executable to analyze.
         */
        explicit addr2line_t(const char *executable);

        /**
         * @brief Closes the addr2line process.
         */
        ~addr2line_t();

        FILE *input;    ///< Input pipe to addr2line
        FILE *output;   ///< Output pipe from addr2line
        bool bad;       ///< True if the process failed to start or died
    private:
        int child_in[2], child_out[2];  ///< Pipe file descriptors
        pid_t pid;                      ///< Child process ID
        DISABLE_COPYING(addr2line_t);
    };

    // "Cache" to re-use addr2line processes
    std::map<std::string, scoped_ptr_t<addr2line_t> > procs;  ///< Map of executable path to addr2line process

    DISABLE_COPYING(address_to_line_t);
};
#endif

/**
 * @ingroup Backtrace
 * @brief Formats the current call stack as a human-readable string.
 *
 * Captures the current call stack and formats it with optional address-to-line
 * translation for better readability.
 *
 * @param use_addr2line If true, attempts to translate addresses to source code
 *                      locations using addr2line (slower but more informative).
 *                      If false, uses only symbol information from the binary.
 * @return A formatted backtrace string suitable for logging or display.
 *
 * @code
 * std::string trace = format_backtrace(true);
 * logERR("Unhandled error: %s", trace.c_str());
 * @endcode
 */
std::string format_backtrace(bool use_addr2line = true);

#if !defined(RDB_NO_BACKTRACE)
/**
 * @ingroup Backtrace
 * @brief Represents a single frame in a stack trace.
 *
 * Encapsulates information about one level in the call stack, including
 * the memory address and symbol information.
 */
class backtrace_frame_t {
public:
    /**
     * @brief Constructs a frame from a memory address.
     *
     * @param _addr The memory address of the instruction in this frame.
     */
    explicit backtrace_frame_t(const void *_addr);

    /**
     * @brief Resolves symbol information for this frame.
     *
     * Parses the /proc/self/maps or equivalent to resolve the filename,
     * function name, and offset for this frame's address. This must be called
     * before calling the getter methods (except get_addr()).
     */
    void initialize_symbols();

    /**
     * @brief Gets the filename where this frame's code is located.
     * @return The executable or shared library filename (empty if unknown).
     */
    std::string get_filename() const;

    /**
     * @brief Gets the mangled function name.
     * @return The mangled C++ function name (empty if unknown).
     */
    std::string get_name() const;

    /**
     * @brief Gets the demangled function name.
     * @return The demangled C++ function name. Returns the mangled name if
     *         demangling fails.
     */
    std::string get_demangled_name() const;

    /**
     * @brief Gets the offset within the function.
     * @return A string describing the offset (e.g., "0x1234").
     */
    std::string get_offset() const;

    /**
     * @brief Gets the raw symbols line from the debugging information.
     * @return The complete symbols line for this frame.
     */
    std::string get_symbols_line() const;

    /**
     * @brief Gets the instruction address for this frame.
     * @return The memory address of the instruction.
     */
    const void *get_addr() const;

private:
    std::string filename, function, offset, symbols_line;  ///< Resolved symbol information
    bool symbols_initialized;                               ///< Whether initialize_symbols() has been called
    const void *addr;                                       ///< Memory address of the instruction
};
#endif  // !defined(RDB_NO_BACKTRACE)

/**
 * @ingroup Backtrace
 * @brief Represents a complete stack trace.
 *
 * Encapsulates a snapshot of the call stack at a point in time.
 * Can be formatted or printed for debugging purposes.
 */
class backtrace_t {
public:
    /**
     * @brief Captures the current call stack.
     *
     * Records up to max_frames (100) frames from the current call stack.
     */
    backtrace_t();

#if !defined(RDB_NO_BACKTRACE)
    /**
     * @brief Returns the number of frames in this backtrace.
     * @return The number of stack frames captured.
     */
    size_t get_num_frames() const { return frames.size(); }

    /**
     * @brief Gets a specific frame from this backtrace.
     * @param i The frame index (0 is the outermost/oldest frame).
     * @return A const reference to the requested backtrace_frame_t.
     */
    const backtrace_frame_t &get_frame(const size_t i) const { return frames[i]; }
#endif

    /**
     * @brief Formats this backtrace as a human-readable string.
     *
     * @param use_addr2line If true, translates addresses to source code locations
     *                      (slower but more informative).
     * @return A formatted backtrace string suitable for logging or display.
     */
    std::string print_frames(bool use_addr2line) const;

private:
#if !defined(RDB_NO_BACKTRACE)
    static const int max_frames = 100;                      ///< Maximum number of frames to capture
    std::vector<backtrace_frame_t> frames;                  ///< Captured stack frames
#endif
};

/**
 * @ingroup Backtrace
 * @brief Lazy-evaluated backtrace with caching.
 *
 * Captures a backtrace and caches its formatted output. The actual formatting
 * is deferred until the first call to addrs() or lines(), improving performance
 * when the backtrace is never displayed.
 *
 * This class stores both fast (assembly addresses) and slow (source line info)
 * formatted versions with lazy evaluation.
 */
class lazy_backtrace_formatter_t : public backtrace_t {
public:
    /**
     * @brief Captures the current call stack and timestamp.
     */
    lazy_backtrace_formatter_t();

    /**
     * @brief Returns a fast backtrace with raw assembly addresses.
     *
     * Formats the backtrace with only assembly addresses, which is fast.
     * The result is cached on first call.
     *
     * @return A formatted backtrace string with addresses only.
     *
     * @code
     * lazy_backtrace_formatter_t trace;
     * logERR("Error occurred: %s", trace.addrs().c_str());
     * @endcode
     */
    std::string addrs();

    /**
     * @brief Returns a well-formatted backtrace with source code line information.
     *
     * Translates addresses to source file names and line numbers, which is slower
     * but more informative. The result is cached on first call.
     *
     * @return A formatted backtrace string with source location information.
     *
     * @code
     * lazy_backtrace_formatter_t trace;
     * logERR("Error occurred: %s", trace.lines().c_str());
     * @endcode
     */
    std::string lines();

private:
    std::string cached_addrs;   ///< Cached output of addrs() (populated lazily)
    std::string cached_lines;   ///< Cached output of lines() (populated lazily)

    time_t timestamp;           ///< When this backtrace was captured
    std::string timestr;        ///< Formatted timestamp string

    DISABLE_COPYING(lazy_backtrace_formatter_t);
};

#endif /* BACKTRACE_HPP_ */
