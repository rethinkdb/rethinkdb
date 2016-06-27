// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "backtrace.hpp"

#ifdef _WIN32
#define OPTIONAL // This macro is undefined in "windows.hpp" but necessary for <DbgHelp.h>
#include <DbgHelp.h>
#include <atomic>
#else
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/wait.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <string>

#include "arch/runtime/coroutines.hpp"
#include "containers/scoped.hpp"
#include "logger.hpp"
#include "rethinkdb_backtrace.hpp"
#include "utils.hpp"

#ifndef _WIN32
static bool parse_backtrace_line(char *line, char **filename, char **function, char **offset, char **address) {
    /*
    backtrace() gives us lines in one of the following two forms:
       ./path/to/the/binary(function+offset) [address]
       ./path/to/the/binary [address]
    */

    *filename = line;

    // Check if there is a function present
    if (char *paren1 = strchr(line, '(')) {
        char *paren2 = strchr(line, ')');
        if (!paren2) return false;
        *paren1 = *paren2 = '\0';   // Null-terminate the offset and the filename
        *function = paren1 + 1;
        char *plus = strchr(*function, '+');
        if (!plus) return false;
        *plus = '\0';   // Null-terminate the function name
        *offset = plus + 1;
        line = paren2 + 1;
        if (*line != ' ') return false;
        line += 1;
    } else {
        *function = nullptr;
        *offset = nullptr;
        char *bracket = strchr(line, '[');
        if (!bracket) return false;
        line = bracket - 1;
        if (*line != ' ') return false;
        *line = '\0';   // Null-terminate the file name
        line += 1;
    }

    // We are now at the opening bracket of the address
    if (*line != '[') return false;
    line += 1;
    *address = line;
    line = strchr(line, ']');
    if (!line || line[1] != '\0') return false;
    *line = '\0';   // Null-terminate the address

    return true;
}

/* There has been some trouble with abi::__cxa_demangle.

Originally, demangle_cpp_name() took a pointer to the mangled name, and returned a
buffer that must be free()ed. It did this by calling __cxa_demangle() and passing NULL
and 0 for the buffer and buffer-size arguments.

There were complaints that print_backtrace() was smashing memory. Shachaf observed that
pieces of the backtrace seemed to be ending up overwriting other structs, and filed
issue #100.

Daniel Mewes suspected that the memory smashing was related to calling malloc().
In December 2010, he changed demangle_cpp_name() to take a static buffer, and fill
this static buffer with the demangled name. See 284246bd.

abi::__cxa_demangle expects a malloc()ed buffer, and if the buffer is too small it
will call realloc() on it. So the static-buffer approach worked except when the name
to be demangled was too large.

In March 2011, Tim and Ivan got tired of the memory allocator complaining that someone
was trying to realloc() an unallocated buffer, and changed demangle_cpp_name() back
to the way it was originally.

Please don't change this function without talking to the people who have already
been involved in this. */

std::string demangle_cpp_name(const char *mangled_name) {
    int res;
    char *name_as_c_str = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &res);
    if (res == 0) {
        std::string name_as_std_string(name_as_c_str);
        free(name_as_c_str);
        return name_as_std_string;
    } else {
        throw demangle_failed_exc_t();
    }
}

int set_o_cloexec(int fd) {
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        return flags;
    }
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

address_to_line_t::addr2line_t::addr2line_t(const char *executable) : input(nullptr), output(nullptr), bad(false), pid(-1) {
    if (pipe(child_in) || set_o_cloexec(child_in[0]) || set_o_cloexec(child_in[1]) || pipe(child_out) || set_o_cloexec(child_out[0]) || set_o_cloexec(child_out[1])) {
        bad = true;
        return;
    }

    if ((pid = fork())) {
        input = fdopen(child_in[1], "w");
        output = fdopen(child_out[0], "r");
        close(child_in[0]);
        close(child_out[1]);
    } else {
        dup2(child_in[0], 0);   // stdin
        dup2(child_out[1], 1);  // stdout

        const char *args[] = {"addr2line", "-s", "-e", executable, nullptr};

        execvp("addr2line", const_cast<char *const *>(args));
        // A normal `exit` can get stuck here it seems. Maybe some `atexit` handler?
        _exit(EXIT_FAILURE);
    }
}

address_to_line_t::addr2line_t::~addr2line_t() {
    if (input) {
        fclose(input);
    }
    if (output) {
        fclose(output);
    }
    if (pid != -1) {
        waitpid(pid, nullptr, 0);
    }
}

bool address_to_line_t::run_addr2line(const std::string &executable, const void *address, char *line, int line_size) {
    addr2line_t *proc;

    std::map<std::string, scoped_ptr_t<addr2line_t> >::iterator iter
        = procs.find(executable);

    if (iter != procs.end()) {
        proc = iter->second.get();
    } else {
        proc = new addr2line_t(executable.c_str());
        auto p = procs.insert(
                std::make_pair(executable,
                               make_scoped<addr2line_t>(executable.c_str()))).first;
        proc = p->second.get();
    }

    if (proc->bad) {
        return false;
    }

    fprintf(proc->input, "%p\n", address);
    fflush(proc->input);

    char *result = fgets(line, line_size, proc->output);

    if (result == nullptr) {
        proc->bad = true;
        return false;
    }

    line[line_size - 1] = '\0';

    int len = strlen(line);
    if (line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }

    if (!strcmp(line, "??:0")) return false;

    return true;
}

std::string address_to_line_t::address_to_line(const std::string &executable, const void *address) {
    char line[255];
    bool success = run_addr2line(executable, address, line, sizeof(line));

    if (!success) {
        return "";
    } else {
        return std::string(line);
    }
}
#endif

std::string format_backtrace(bool use_addr2line) {
    lazy_backtrace_formatter_t bt;
    return use_addr2line ? bt.lines() : bt.addrs();
}

backtrace_t::backtrace_t() {
    scoped_array_t<void *> stack_frames(new void*[max_frames], max_frames); // Allocate on heap in case stack space is scarce
    int size = rethinkdb_backtrace(stack_frames.data(), max_frames);

#ifdef CROSS_CORO_BACKTRACES
    if (coro_t::self() != nullptr) {
        int space_remaining = max_frames - size;
        rassert(space_remaining >= 0);
        size += coro_t::self()->copy_spawn_backtrace(stack_frames.data() + size,
                                                     space_remaining);
    }
#endif

    frames.reserve(static_cast<size_t>(size));
    for (int i = 0; i < size; ++i) {
        frames.push_back(backtrace_frame_t(stack_frames[i]));
    }
}

backtrace_frame_t::backtrace_frame_t(const void* _addr) :
    symbols_initialized(false),
    addr(_addr) {

}

void backtrace_frame_t::initialize_symbols() {
#ifndef _WIN32
    void *addr_array[1] = {const_cast<void *>(addr)};
    char **symbols = backtrace_symbols(addr_array, 1);
    if (symbols != nullptr) {
        symbols_line = std::string(symbols[0]);
        char *c_filename;
        char *c_function;
        char *c_offset;
        char *c_address;
        if (parse_backtrace_line(symbols[0], &c_filename, &c_function, &c_offset, &c_address)) {
            if (c_filename != nullptr) {
                filename = std::string(c_filename);
            }
            if (c_function != nullptr) {
                function = std::string(c_function);
            }
            if (c_offset != nullptr) {
                offset = std::string(c_offset);
            }
        }
        free(symbols);
    }
#endif
    symbols_initialized = true;
}

std::string backtrace_frame_t::get_name() const {
    rassert(symbols_initialized);
    return function;
}

std::string backtrace_frame_t::get_symbols_line() const {
    rassert(symbols_initialized);
    return symbols_line;
}

std::string backtrace_frame_t::get_demangled_name() const {
    rassert(symbols_initialized);
#ifdef _WIN32
    return function;
#else
    return demangle_cpp_name(function.c_str());
#endif
}

std::string backtrace_frame_t::get_filename() const {
    rassert(symbols_initialized);
    return filename;
}

std::string backtrace_frame_t::get_offset() const {
    rassert(symbols_initialized);
    return offset;
}

const void *backtrace_frame_t::get_addr() const {
    return addr;
}

lazy_backtrace_formatter_t::lazy_backtrace_formatter_t() :
    backtrace_t(),
    timestamp(time(nullptr)),
    timestr(time2str(timestamp)) {
}

std::string lazy_backtrace_formatter_t::addrs() {
    if (cached_addrs == "") {
        cached_addrs = timestr + "\n" + print_frames(false);
    }
    return cached_addrs;
}

std::string lazy_backtrace_formatter_t::lines() {
    if (cached_lines == "") {
        cached_lines = timestr + "\n" + print_frames(true);
    }
    return cached_lines;
}

#ifdef _WIN32
void initialize_dbghelp() {
    static std::atomic<bool> initialised = false;
    if (!initialised.exchange(true)) {
        DWORD options = SymGetOptions();
        options |= SYMOPT_LOAD_LINES; // Load line information
        options |= SYMOPT_DEFERRED_LOADS; // Lazy symbol loading
        SymSetOptions(options);

        // Initialize and load the symbol tables
        BOOL ret = SymInitialize(GetCurrentProcess(), nullptr, true);
        if (!ret) {
            logWRN("Unable to load symbols: %s", winerr_string(GetLastError()).c_str());
        }
    }
}
#endif

std::string backtrace_t::print_frames(bool use_addr2line) const {
#ifdef _WIN32
    initialize_dbghelp();

    std::string output;
    for (size_t i = 0; i < get_num_frames(); i++) {
        output.append(strprintf("%d: ", static_cast<int>(i+1)));
        DWORD64 offset;
        const int MAX_SYMBOL_LENGTH = 2048; // An arbitrary number
        auto symbol_info = scoped_malloc_t<SYMBOL_INFO>(sizeof(SYMBOL_INFO) - 1 + MAX_SYMBOL_LENGTH);
        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen = MAX_SYMBOL_LENGTH;
        BOOL ret = SymFromAddr(GetCurrentProcess(), reinterpret_cast<DWORD64>(get_frame(i).get_addr()), &offset, symbol_info.get());
        if (!ret) {
            output.append(strprintf("0x%p [%s]", get_frame(i).get_addr(), winerr_string(GetLastError()).c_str()));
        } else {
            output.append(strprintf("%s", symbol_info->Name));
            if (offset != 0) {
                output.append(strprintf("+%llu", offset));
            }
        }
        DWORD line_offset;
        IMAGEHLP_LINE64 line_info;
        ret = SymGetLineFromAddr64(GetCurrentProcess(), reinterpret_cast<DWORD64>(get_frame(i).get_addr()), &line_offset, &line_info);
        if (!ret) {
            // output.append(" (no line info)");
        } else {
            output.append(strprintf(" at %s:%u", line_info.FileName, line_info.LineNumber));
        }
        output.append("\n");
    }
    return output;
#else
    address_to_line_t address_to_line;
    std::string output;
    for (size_t i = 0; i < get_num_frames(); i++) {
        backtrace_frame_t current_frame = get_frame(i);
        current_frame.initialize_symbols();

        output.append(strprintf("%d [%p]: ", static_cast<int>(i+1), current_frame.get_addr()));

        try {
            output.append(current_frame.get_demangled_name());
        } catch (const demangle_failed_exc_t &) {
            if (!current_frame.get_name().empty()) {
                output.append(current_frame.get_name() + "+" + current_frame.get_offset());
            } else if (!current_frame.get_symbols_line().empty()) {
                output.append(current_frame.get_symbols_line());
            } else {
                output.append("<unknown function>");
            }
        }

        output.append(" at ");

        std::string some_other_line;
        if (use_addr2line) {
            if (!current_frame.get_filename().empty()) {
                some_other_line = address_to_line.address_to_line(current_frame.get_filename(), current_frame.get_addr());
            }
        }
        if (!some_other_line.empty()) {
            output.append(some_other_line);
        } else {
            output.append(strprintf("%p", current_frame.get_addr()) + " (" + current_frame.get_filename() + ")");
        }

        output.append("\n");
    }

    return output;
#endif
}
