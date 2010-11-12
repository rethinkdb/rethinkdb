
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

        size %= max - min;
        size += min;

        char *l = (char*) malloc(size);
        char *head = l;
        for(;size > 4; size -= 4) {
            uint64_t hash = seed;
            hash ^= (size << 7);
            hash += SALT2;
            hash ^= SALT3;

            hash += hash << 43;
            hash ^= hash >> 27;
            hash += hash << 47;
            hash ^= hash >> 1;

            char *hash_head = (char *) &hash;

            switch (size) {
                default:
                    *head++ = *hash_head++;
                case (3):
                    *head++ = *hash_head++;
                case (2):
                    *head++ = *hash_head++;
                case (1):
                    *head++ = *hash_head++;
                    break;
            }
        }
        // fill the payload
        payload->first = l;
        payload->second = size;
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

