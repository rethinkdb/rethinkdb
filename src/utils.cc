#include "utils.hpp"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "arch/arch.hpp"

void generic_crash_handler(int signum) {
    if (signum == SIGSEGV) {
        fail("Segmentation fault.");
    } else {
        fail("Unexpected signal: %d\n", signum);
    }
}

void ignore_crash_handler(int signum) {}

void install_generic_crash_handler() {
    struct sigaction action;
    bzero(&action, sizeof(action));
    action.sa_handler = generic_crash_handler;
    int res = sigaction(SIGSEGV, &action, NULL);
    check("Could not install SEGV handler", res < 0);

    bzero(&action, sizeof(action));
    action.sa_handler = ignore_crash_handler;
    res = sigaction(SIGPIPE, &action, NULL);
    check("Could not install PIPE handler", res < 0);
}

// fast non-null terminated string comparison
int sized_strcmp(const char *str1, int len1, const char *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0)
        res = len1-len2;
    return res;
}

void print_hd(const void *vbuf, size_t offset, size_t ulength) {

    flockfile(stderr);

    const char *buf = (const char *)vbuf;
    ssize_t length = ulength;

    char bd_sample[16] = { 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 
                           0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD, 0xBD };
    char zero_sample[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char ff_sample[16] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                           0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    bool skipped_last = false;
    while (length > 0) {

        bool skip = memcmp(buf, bd_sample, 16) == 0 ||
                    memcmp(buf, zero_sample, 16) == 0 ||
                    memcmp(buf, ff_sample, 16) == 0;
        if (skip) {
            if (!skipped_last) fprintf(stderr, "*\n");
        } else {
            fprintf(stderr, "%.8x  ", (unsigned int)offset);
            for (int i = 0; i < 16; i++) {
                if (i < (int)length) fprintf(stderr, "%.2hhx ", buf[i]);
                else fprintf(stderr, "   ");
            }
            fprintf(stderr, "| ");
            for (int i = 0; i < 16; i++) {
                if (i < (int)length) {
                    if (isprint(buf[i])) fputc(buf[i], stderr);
                    else fputc('.', stderr);
                } else {
                    fputc(' ', stderr);
                }
            }
            fprintf(stderr, "\n");
        }
        skipped_last = skip;

        offset += 16;
        buf += 16;
        length -= 16;
    }

    funlockfile(stderr);
}

