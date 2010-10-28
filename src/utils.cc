
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "utils.hpp"
#include "arch/arch.hpp"

void *malloc_aligned(size_t size, size_t alignment) {
    void *ptr = NULL;
    int res = posix_memalign(&ptr, alignment, size);
    if(res != 0)
        return NULL;
    else
        return ptr;
}

// fast non-null terminated string comparison
int sized_strcmp(const char *str1, int len1, const char *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0)
        res = len1-len2;
    return res;
}

void print_hd(void *buf, size_t offset, size_t length) {
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
            printf("%.8x  ", (unsigned int)(offset + count));

            skip_last = false;
            for(int j = 0; j < 2; j++) {
                // Print a column
                for(int i = 0; i < 8; i++) {
                    printf("%.2hhx ", _buf[count]);
                    count++;
                    if(count >= length) {
                        printf("\n");
                        return;
                    }
                }
                printf(" ");
            }
            // Print a char representation
            printf("|");
            for(int i = 0; i < 16; i++) {
                char c = _buf[count - 16 + i];
                if(isprint(c))
                    printf("%c", c);
                else
                    printf(".");
            }
            printf("|\n");
            line++;
            if(line == 8) {
                printf("\n");
                line = 0;
            }
        } else {
            if(!skip_last) {
                skip_last = true;
                printf("*\n");
            }
            count += 16;
            if(count >= length) {
                printf("\n");
                return;
            }
        }
    }
}
