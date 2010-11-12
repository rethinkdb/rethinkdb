
#ifndef __DISTR_HPP__
#define __DISTR_HPP__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "load.hpp"

#define SALT1 0xFEEDABE0
#define SALT2 0xBEDFACE8
#define SALT3 0xFEDBABE5

/* Helpful typedefs */
typedef std::pair<char*, size_t> payload_t;

/* Defines a distribution of values, from min to max. */
struct distr_t {
public:
    distr_t(int _min, int _max)
        : min(_min), max(_max)
        {}

    distr_t()
        : min(8), max(16)
        {}

    // Generates a random payload within given bounds. It is the
    // user's responsibility to clean up the payload.
    void toss(payload_t *payload, uint64_t seed) {
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
        size ^= size >> 47;
        size += size << 13;

        size %= max - min + 1;
        size += min;

        char *l = payload->first;
        payload->second = size;
        int _size = size;
        do {
            uint64_t hash = seed;
            hash ^= ((uint64_t)_size << 7);
            hash += SALT2;
            hash ^= SALT3;

            hash += hash << 43;
            hash ^= hash >> 27;

            unsigned char *hash_head = (unsigned char *) &hash;

            for(int j = 0; j < std::min(_size, 4); j++) {
                *l++ = (*hash_head++ % ('z' - 'a' + 1)) + 'a';
            }
            _size -= 4;
        } while(_size > 0);
    }

    void parse(char *str) {
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
                fprintf(stderr, "Invalid distr format (use MIN-MAX)\n");
                exit(-1);
                break;
            }
            tok = strtok(NULL, "-");
            c++;
        }
        if(c < 2) {
            fprintf(stderr, "Invalid distr format (use MIN-MAX)\n");
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

