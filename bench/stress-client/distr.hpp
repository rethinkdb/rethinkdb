
#ifndef __DISTR_HPP__
#define __DISTR_HPP__

#include <stdio.h>
#include <string.h>
#include "load.hpp"

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
    void toss(payload_t *payload) {
        // generate the size
        int s = random(min, max);
        
        char *l = (char*)malloc(s);
        
        /* The random number generator is slow, so we randomly generate the first 8 bytes and then
        pad the rest with '*'. */
        int i;
        for (i = 0; i < s && i < 8; i++) {
            l[i] = random('A', 'Z');
        }
        for (; i < s; i++) {
            l[i] = '*';
        }
        
        // fill the payload
        payload->first = l;
        payload->second = s;
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

