#include "utils.hpp"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "arch/arch.hpp"

void generic_crash_handler(int signum) {
    fail("Internal crash detected.");
}

void ignore_crash_handler(int signum) {}

void install_generic_crash_handler() {
    struct sigaction action;
    bzero(&action, sizeof(action));
    action.sa_handler = generic_crash_handler;
    int res = sigaction(SIGSEGV, &action, NULL);
    guarantee_err(res == 0, "Could not install SEGV handler");

    bzero(&action, sizeof(action));
    action.sa_handler = ignore_crash_handler;
    res = sigaction(SIGPIPE, &action, NULL);
    guarantee_err(res == 0, "Could not install PIPE handler");
}

// fast non-null terminated string comparison
int sized_strcmp(const char *str1, int len1, const char *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0)
        res = len1-len2;
    return res;
}

void print_hd(const void *buf, size_t offset, size_t length) {
    
    flockfile(stderr);
    
    size_t count = 0;
    char *_buf = (char*)buf;
    char bd_sample[16] = { 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 
                           0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD };
    char zero_sample[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char ff_sample[16] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                           0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    int line = 0;
    bool skip_last = false;
    while(1) {
        // Check a line for bds
        bool skip = memcmp(_buf + count, bd_sample, 16) == 0 ||
                    memcmp(_buf + count, zero_sample, 16) == 0 ||
                    memcmp(_buf + count, ff_sample, 16) == 0;
        
        // Print a line
        if(!skip) {
            // print the current offset
            fprintf(stderr, "%.8x  ", (unsigned int)(offset + count));

            skip_last = false;
            for(int j = 0; j < 2; j++) {
                // Print a column
                for(int i = 0; i < 8; i++) {
                    fprintf(stderr, "%.2hhx ", _buf[count]);
                    count++;
                    if(count >= length) {
                        fprintf(stderr, "\n");
                        funlockfile(stderr);
                        return;
                    }
                }
                fprintf(stderr, " ");
            }
            // Print a char representation
            fprintf(stderr, "|");
            for(int i = 0; i < 16; i++) {
                char c = _buf[count - 16 + i];
                if(isprint(c))
                    fprintf(stderr, "%c", c);
                else
                    fprintf(stderr, ".");
            }
            fprintf(stderr, "|\n");
            line++;
            if(line == 8) {
                fprintf(stderr, "\n");
                line = 0;
            }
        } else {
            if(!skip_last) {
                skip_last = true;
                fprintf(stderr, "*\n");
            }
            count += 16;
            if(count >= length) {
                fprintf(stderr, "\n");
                funlockfile(stderr);
                return;
            }
        }
    }
    
    funlockfile(stderr);
}
