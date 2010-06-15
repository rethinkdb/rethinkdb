
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <new>
#include <exception>
#include "utils.hpp"

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

long get_available_ram() {
    return (long)sysconf(_SC_AVPHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

long get_total_ram() {
    return (long)sysconf(_SC_PHYS_PAGES) * (long)sysconf(_SC_PAGESIZE); }

// Redefine operator new to do cache-lines alignment
void* operator new(size_t size) throw(std::bad_alloc) {
    void *ptr = NULL;
    int res = posix_memalign(&ptr, 64, size);
    if(res != 0)
        throw std::bad_alloc();
    else
        return ptr;
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

/**
 * Tokenizer
 */
bool is_delim(char c, const char *delims) {
    while(*delims != '\0') {
        if(c == *delims) {
            return true;
        }
        ++delims;
    }
    return false;
}

char* tokenize(char *str, unsigned int size,
               const char *delims, unsigned int *token_size)
{
    return (char*)tokenize((const char*)str, size, delims, token_size);
}

const char* tokenize(const char *str, unsigned int size,
                     const char *delims, unsigned int *token_size)
{
    unsigned int pos = 0;
    
    // Skip delimeters
    for(; pos < size; pos++) {
        if(!is_delim(str[pos], delims))
            break;
    }
    if(pos == size)
        return NULL;
    
    // Determine token length
    unsigned int end = pos;
    for(; end < size; end++) {
        if(is_delim(str[end], delims))
            break;
    }

    // Return
    *token_size = end - pos;
    return str + pos;
}

bool token_eq(const char *str, char *token, int token_size) {
    int len = strlen(str);
    return len == token_size && strncmp(token, str, len) == 0;
}

bool contains_tokens(char *start, char *end, const char *delims) {
    while (start < end) {
        if (!is_delim(*start, delims))
            return true;
        start++;
    }
    return false;
}
