
#ifndef __DISTR_HPP__
#define __DISTR_HPP__

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include "utils.hpp"

#define SALT1 0xFEEDABE0
#define SALT2 0xBEDFACE8
#define SALT3 0xFEDBABE5


/* Helpful typedefs */
typedef std::pair<char*, size_t> payload_t;

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
        : min(_min), max(_max), append_client_suffix(false), prefix("")
        {}

    distr_t()
        : min(8), max(16), append_client_suffix(false), prefix("")
        {}

    // Generates a random payload within given bounds. It is the
    // user's responsibility to clean up the payload.
    void toss(payload_t *payload, uint64_t seed, const int client_id = -1, int max_client_id = -1) {

        // First part of payload: user-specified prefix, if any

        strncpy(payload->first, prefix.c_str(), prefix.length());
        payload->second = prefix.length();

        // Second part of payload: randomly generated payload body

        int body_min = min, body_max = max;
        body_min -= prefix.length();
        body_max -= prefix.length();
        int client_suffix_length;
        if (append_client_suffix) {
            if (max_client_id < 0 || client_id < 0) {
                fprintf(stderr, "Internal error: No client id given to toss().\n");
                exit(-2);
            }
            client_suffix_length = count_decimal_digits(max_client_id);
            body_min -= client_suffix_length;
            body_max -= client_suffix_length;
        }

        if (body_min < 0) {
            fprintf(stderr, "Lower bound for length is specified to be %d, but prefix and suffix "
                "together are %d bytes, making lower bound unattainable.\n",
                min, min - body_min);
            exit(-1);
        } else if (body_max < 0) {
            fprintf(stderr, "Specified maximum length is %d bytes, but prefix and suffix together "
                "are %d bytes, making it impossible to generate a valid payload.\n",
                max, max - body_max);
            exit(-1);
        }

        /* compute out the size */
        uint64_t body_size = seed;
        body_size ^= SALT1;
        body_size += body_size << 19;
        body_size ^= body_size >> 17;
        body_size += body_size << 3;
        body_size ^= body_size >> 37;
        body_size += body_size << 5;
        body_size ^= body_size >> 11;
        body_size += body_size << 2;
        body_size ^= body_size >> 45;
        body_size += body_size << 13;

        body_size %= body_max - body_min + 1;
        body_size += body_min;

        char *l = payload->first + prefix.length();
        int _size = body_size;
        do {
            uint64_t hash = seed;
            hash ^= ((uint64_t)_size << 7);
            hash += SALT2;
            hash ^= SALT3;

            hash += hash << 43;
            hash ^= hash >> 27;
            hash ^= hash >> 13;
            hash ^= hash << 11;

            unsigned char *hash_head = (unsigned char *) &hash;

            for(int j = 0; j < std::min(_size, 4); j++) {
                *l++ = (*hash_head++ & 31) + 'A';
            }
            _size -= 4;
        } while(_size > 0);

        payload->second += body_size;

        // Third part of payload: per-client suffix

        if (append_client_suffix) {
            int remainder = client_id;
            for (int i = client_suffix_length - 1; i >= 0; --i) {
                l[i] = '0' + remainder % 10;
                remainder /= 10;
            }
            payload->second += client_suffix_length;
        }
    }

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
        if (append_client_suffix)
            printf(", per-client suffix");
    }

public:
    int min, max;
    bool append_client_suffix;
    std::string prefix;
};

#endif // __DISTR_HPP__

