
#ifndef __DISTR_HPP__
#define __DISTR_HPP__

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include "load.hpp"

#define SALT1 0xFEEDABE0
#define SALT2 0xBEDFACE8
#define SALT3 0xFEDBABE5


/* Helpful typedefs */
typedef std::pair<char*, size_t> payload_t;

void append(payload_t *p, payload_t *other) {
    p->second += other->second;
    memcpy(p->first + p->second, other->first, other->second);
}

void prepend(payload_t *p, payload_t *other) {
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
        /* compute out the size */
        uint64_t size = seed;
        size ^= SALT1;
        size += size << 19;
        size ^= size >> 17;
        size += size << 3;
        size ^= size >> 37;
        size += size << 5;
        size ^= size >> 11;
        size += size << 2;
        size ^= size >> 45;
        size += size << 13;

        size %= max - min + 1;
        size += min;

        size_t prefix_len = prefix.length();
        char *l = payload->first + prefix_len;

        strncpy(payload->first, prefix.c_str(), prefix_len);
        payload->second = size + prefix_len;

        int _size = size;
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
        
        if (append_client_suffix) {
            // Append the client id as a suffix
            if (max_client_id < 0)
                max_client_id = client_id;
            if (client_id < 0) {
                fprintf(stderr, "Internal error: No client id given to toss().\n");
                exit(-2);
            }
            int remainder = max_client_id;
            int denominator = 1;
            size_t suffix_length = 1;
            while (remainder > 10) {
                denominator *= 10;
                remainder /= 10;
                ++suffix_length;
            }
            remainder = client_id;
            for (size_t i = suffix_length; i > 0; --i) {
                *l++ = '0' + (remainder / denominator);
                remainder = remainder % denominator;
                denominator *= 10;
            }
            payload->second += suffix_length;
        }

    }
    
    size_t calculate_max_length(const int client_id = -1) {
        size_t suffix_length = 0;
        if (append_client_suffix) {
            if (client_id < 0) {
                fprintf(stderr, "Internal error: No client id given to calculate_max_length().\n");
                exit(-2);
            }
            
            int remainder = client_id;
            ++suffix_length;
            while (remainder >= 10) {
                remainder /= 10;
                ++suffix_length;
            }
        }
        return max + suffix_length + prefix.length();
    }

    void parse(char *str) {
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

