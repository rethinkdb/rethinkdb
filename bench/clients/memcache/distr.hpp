
#ifndef __DISTR_HPP__
#define __DISTR_HPP__

#include <stdio.h>
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
        
        // malloc and fill memory
        char *l = (char*)malloc(s);
        for(int i = 0; i < s; i++) {
            l[i] = random(65, 90);
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

