#include "errors.hpp"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef NDEBUG

#include <execinfo.h>

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

void print_backtrace(FILE *out) {
    
    fprintf(out, "\nBacktrace:\n");
    
    // Get a backtrace
    static const int max_frames = 100;
    void *stack_frames[max_frames];
    int size = backtrace(stack_frames, max_frames);
    char **symbols = backtrace_symbols(stack_frames, size);
    
    if (symbols) {        
        for (int i = 0; i < size; i ++) {
        
            // Parse each line of the backtrace
            char *line = strdup(symbols[i]);
            char *filename, *function, *offset, *address;
            
            fprintf(out, "%d: ", i+1);
            
            if (!parse_backtrace_line(line, &filename, &function, &offset, &address)) {
                fprintf(out, "%s\n", symbols[i]);
                
            } else if (function) {
                if (char *demangled = demangle_cpp_name(function)) {
                    char cmd_buf[255], line[255], exec_name[255];
                    // Make valgrind happy
                    bzero((void*)cmd_buf, sizeof(cmd_buf));
                    bzero((void*)line, sizeof(line));
                    bzero((void*)exec_name, sizeof(exec_name));
                    // Get current executable path
                    size_t exec_name_size = readlink( "/proc/self/exe", exec_name, 255);
                    exec_name[exec_name_size] = '\0';
                    // Generate and run addr2line command
                    snprintf(cmd_buf, sizeof(cmd_buf), "addr2line -s -e %s %s",
                             exec_name, address);
                    FILE *fline = popen(cmd_buf, "r");
                    int __attribute__((__unused__)) res = fread(line, sizeof(char), sizeof(line), fline);
                    pclose(fline);
                    // Output the result
                    fprintf(out, "%s at %s", demangled, line);
                    free(demangled);
                } else {
                    fprintf(out, "[ %s(%s+%s) [%s] ]\n", filename, function, offset, address);
                }
            
            } else {
                fprintf(out, "[ %s [%s] ]\n", filename, address);
                
            }
            
            free(line);
        }
        
        free(symbols);
        
    } else {
        fprintf(out, "(too little memory for backtrace)\n");
    }
}

#endif

void _fail(const char *file, int line, const char *msg, ...) {
    
    flockfile(stderr);
    
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "\nError in %s at line %d:\n", file, line);
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
    
#ifndef NDEBUG
    print_backtrace();
#endif
    
    fprintf(stderr, "\nExiting.\n\n");
    
    funlockfile(stderr);
    
    abort();
}

#ifndef NDEBUG

#include <cxxabi.h>
char *demangle_cpp_name(const char *mangled_name) {
    int res;
    char *name = abi::__cxa_demangle(mangled_name, NULL, NULL, &res);
    if (res == 0) {
        return name;
    } else {
        return NULL;
    }
}

#endif
