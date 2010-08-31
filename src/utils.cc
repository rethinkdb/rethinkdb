
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <new>
#include <exception>
#include <bfd.h>
#include "config/args.hpp"
#include "config/code.hpp"
#include "utils.hpp"
#include <algorithm>
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

void print_backtrace() {
    
    fprintf(stderr, "\nBacktrace:\n");
    
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
            
            fprintf(stderr, "%d: ", i+1);
            
            if (!parse_backtrace_line(line, &filename, &function, &offset, &address)) {
                fprintf(stderr, "%s\n", symbols[i]);
                
            } else if (function) {
                if (char *demangled = demangle_cpp_name(function)) {
                    fprintf(stderr, "%s in %s at %s\n", address, demangled, filename);
                    free(demangled);
                } else {
                    fprintf(stderr, "[ %s(%s+%s) [%s] ]\n", filename, function, offset, address);
                }
            
            } else {
                fprintf(stderr, "[ %s [%s] ]\n", filename, address);
                
            }
            
            free(line);
        }
    
    } else {
        fprintf(stderr, "(too little memory for backtrace)\n");
    }
    free(symbols);
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

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

long get_available_ram() {
    return (long)sysconf(_SC_AVPHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

long get_total_ram() {
    return (long)sysconf(_SC_PHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

void* operator new(size_t size) throw(std::bad_alloc) {
#ifndef NDEBUG
    fail("You are using builtin operator new. Please use RethinkDB allocator system instead.");
#else
    // Provide an implementation in case we do this in release mode.
    void *ptr = NULL;
    int res = posix_memalign(&ptr, 64, size);
    if(res != 0)
        throw std::bad_alloc();
    else
        return ptr;
#endif
}

void operator delete(void *p) {
    free(p);
}

void *malloc_aligned(size_t size, size_t alignment) {
    void *ptr = NULL;
    int res = posix_memalign(&ptr, alignment, size);
    if(res != 0)
        return NULL;
    else
        return ptr;
}

// fast non-null terminated string comparison
int sized_strcmp(const char *str1, int len1, const char *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0)
        res = len1-len2;
    return res;
}

void *_gmalloc(size_t size) {
    void *ptr = NULL;
    int res = posix_memalign((void**)&ptr, 64, size);
    if(res != 0)
        return NULL;
    else
        return ptr;
}

void _gfree(void *ptr) {
    free(ptr);
}
