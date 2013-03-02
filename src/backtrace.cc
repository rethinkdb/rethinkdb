// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <backtrace.hpp>

#include <cxxabi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <string>

#include "errors.hpp"
#include <boost/tokenizer.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "containers/scoped.hpp"
#include "logger.hpp"
#include "utils.hpp"




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
        *function = NULL;
        *offset = NULL;
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
    char *name_as_c_str = abi::__cxa_demangle(mangled_name, NULL, 0, &res);
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

class addr2line_t {
public:
    explicit addr2line_t(const char *executable) : input(NULL), output(NULL), bad(false), pid(-1) {
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

            const char *args[] = {"addr2line", "-s", "-e", executable, NULL};

            execvp("addr2line", const_cast<char *const *>(args));
            exit(EXIT_FAILURE);
        }
    }

    ~addr2line_t() {
        if (input) {
            fclose(input);
        }
        if (output) {
            fclose(output);
        }
        if (pid != -1) {
            waitpid(pid, NULL, 0);
        }
    }

    FILE *input, *output;
    bool bad;
private:
    int child_in[2], child_out[2];
    pid_t pid;
    DISABLE_COPYING(addr2line_t);
};

static bool run_addr2line(boost::ptr_map<std::string, addr2line_t> *procs, const char *executable, const char *address, char *line, int line_size) {
    std::string exe(executable);

    addr2line_t* proc;

    boost::ptr_map<std::string, addr2line_t>::iterator iter = procs->find(exe);

    if (iter != procs->end()) {
        proc = iter->second;
    } else {
        proc = new addr2line_t(executable);
        procs->insert(exe, proc);
    }

    if (proc->bad) {
        return false;
    }

    fprintf(proc->input, "%s\n", address);
    fflush(proc->input);

    char *result = fgets(line, line_size, proc->output);

    if (result == NULL) {
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

std::string format_backtrace(bool use_addr2line) {
    lazy_backtrace_t bt;
    return use_addr2line ? bt.lines() : bt.addrs();
}

std::string print_frames(void **stack_frames, int size, bool use_addr2line) {
    boost::ptr_map<std::string, addr2line_t> procs;
    char **symbols = backtrace_symbols(stack_frames, size);
    std::string output;
    if (symbols) {
        for (int i = 0; i < size; i ++) {
            // Parse each line of the backtrace
            scoped_malloc_t<char> line(symbols[i], symbols[i] + (strlen(symbols[i]) + 1));
            char *executable, *function, *offset, *address;

            output.append(strprintf("%d: ", i+1));

            if (!parse_backtrace_line(line.get(), &executable, &function, &offset, &address)) {
                output.append(strprintf("%s\n", symbols[i]));
            } else {
                if (function) {
                    try {
                        std::string demangled = demangle_cpp_name(function);
                        output.append(demangled);
                    } catch (demangle_failed_exc_t) {
                        output.append(strprintf("%s+%s", function, offset));
                    }
                } else {
                    output.append("?");
                }

                output.append(" at ");

                char some_other_line[255] = {0};
                if (use_addr2line && run_addr2line(&procs, executable, address, some_other_line, sizeof(some_other_line))) {
                    output.append(some_other_line);
                } else {
                    output.append(strprintf("%s (%s)", address, executable));
                }

                output.append("\n");
            }
        }

        free(symbols);

        return output;
    } else {
        output = "(too little memory for backtrace)";

        return output;
    }
}

lazy_backtrace_t::lazy_backtrace_t() : timestamp(time(0)), timestr(time2str(timestamp)) {
    size = backtrace(stack_frames, max_frames);
}

std::string lazy_backtrace_t::addrs() {
    if (cached_addrs == "") {
        cached_addrs = timestr + "\n" + print_frames(stack_frames, size, false);
    }
    return cached_addrs;
}

std::string lazy_backtrace_t::lines() {
    if (cached_lines == "") {
        cached_lines = timestr + "\n" + print_frames(stack_frames, size, true);
    }
    return cached_lines;
}
