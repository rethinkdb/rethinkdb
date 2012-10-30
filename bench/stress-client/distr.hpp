// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef __STRESS_CLIENT_DISTR_HPP__
#define __STRESS_CLIENT_DISTR_HPP__

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
        memcpy(first, p.first, buffer_size);
    }
    payload_t &operator=(const payload_t &p) {
        delete[] first;
        first = new char[p.buffer_size];
        second = p.second;
        buffer_size = p.buffer_size;
        memcpy(first, p.first, buffer_size);
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
        std::string param(const_str);
        size_t hyphenIndex = param.find("-");

        // Make sure the parameter only contains numerals and the (optional) hyphen
        if (param.find_first_not_of("0123456789-") != std::string::npos) {
            fprintf(stderr, "Invalid distr format (use NUM or MIN-MAX), brackets and +/- signs are not supported\n");
            exit(-1);
        }
        
        if (hyphenIndex == std::string::npos) {
            min = max = atoi(param.c_str());
            return;
        }
        
        // Protection against more than one hyphen
        if (param.rfind("-") != hyphenIndex) {
            fprintf(stderr, "Invalid distr format (use NUM or MIN-MAX), +/- signs are not supported\n");
            exit(-1);
        }

        min = atoi(param.substr(0, hyphenIndex).c_str());
        max = atoi(param.substr(hyphenIndex + 1, std::string::npos).c_str());

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

#endif  // __STRESS_CLIENT_DISTR_HPP__

