
#ifndef __DISTR_HPP__
#define __DISTR_HPP__

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include "utils.hpp"

/* payload_t represents just a string of characters. It's designed so that its internal
buffer can be reused over and over. */

struct payload_t {

    /* "first" is the buffer and "second" is how many bytes of the buffer are in use. The
    names are a holdover from when payload_t was defined as a std::pair<char*, size_t>. */
    char *first;
    size_t second;
    int buffer_size;

    payload_t() :
        first(NULL), second(0), buffer_size(0) { }
    payload_t(int bs) :
        first(new char[bs]), second(0), buffer_size(bs) { }
    payload_t(const payload_t &p) :
        first(new char[p.buffer_size]), second(p.second), buffer_size(p.buffer_size) {
        memcpy(first, p.first, second);
    }
    payload_t &operator=(const payload_t &p) {
        delete[] first;
        first = new char[p.buffer_size];
        second = p.second;
        buffer_size = p.buffer_size;
        memcpy(first, p.first, second);
    }
    ~payload_t() {
        delete[] first;
    }

    /* If the payload's buffer has less than this much space, then expand the
    payload's buffer. The contents of the payload's buffer after this operation
    are undefined. */
    void grow_to(int size) {
        if (buffer_size < size) {
            delete[] first;
            first = new char[size];
            buffer_size = size;
        }
    }
};

inline void append(payload_t *p, payload_t *other) {
    p->second += other->second;
    memcpy(p->first + p->second, other->first, other->second);
}

inline void prepend(payload_t *p, payload_t *other) {
    p->second += other->second;
    memmove(p->first + p->second, p->first, p->second);
    memcpy(p->first, other->first, other->second);
}

/* Defines a distribution of values, from min to max. */
struct distr_t {
public:
    distr_t(int _min, int _max)
        : min(_min), max(_max)
        {}

    distr_t()
        : min(8), max(16)
        {}

    void parse(const char *const_str) {
        char str[200];
        strncpy(str, const_str, sizeof(str));
        if (!strchr(str, '-')) {
            min = max = atoi(str);
            return;
        }
        char *tok = strtok(str, "-");
        int c = 0;
        while(tok != NULL) {
            switch(c) {
            case 0:
                min = atoi(tok);
                break;
            case 1:
                max = atoi(tok);
                break;
            default:
                fprintf(stderr, "Invalid distr format (use NUM or MIN-MAX)\n");
                exit(-1);
                break;
            }
            tok = strtok(NULL, "-");
            c++;
        }
        if(c < 2) {
            fprintf(stderr, "Invalid distr format (use NUM or MIN-MAX)\n");
            exit(-1);
        }
        if (min > max) {
            fprintf(stderr, "Invalid distr format (use NUM or MIN-MAX, where MIN <= MAX)\n");
            exit(-1);
        }
    }

    void print() {
        printf("%d-%d", min, max);
    }

public:
    int min, max;
};

#endif // __DISTR_HPP__

