
#include <pthread.h>
#include <time.h>
#include <libaio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

void check(const char *str, int error) {
    if (error) {
        if(errno == 0)
            errno = EINVAL;
        perror(str);
        exit(-1);
    }
}

off64_t get_device_length(const char* device) {
    int fd;
    off64_t length;
    
    fd = open64(device, O_RDONLY);
    check("Error opening device", fd == -1);

    length = lseek64(fd, 0, SEEK_END);
    check("Error computing device size", length == -1);

    close(fd);

    return length;
}

off64_t get_rnd_block_offset(size_t block_size, off64_t device_length) {
    off64_t off = block_size * (rand() % (device_length / block_size) - 1);
    return off;
}

#define REQUEST_COUNT  100000
#define BLOCK_SIZE     512

int main(int argc, char *argv[]) {
    int res;
    srand(time(NULL));

    check("Not enough parameters", argc < 2);

    char *device = argv[1];
    off64_t length = get_device_length(device);
    int fd = open(device, O_DIRECT | O_NOATIME | O_RDONLY);
    check("Couldn't open device", fd == -1);

    char *buf;
    res = posix_memalign((void**)&buf, 512, BLOCK_SIZE);
    check("Couldn't alloc buffer", res != 0);

    // start the thread
    timeval tvb, tve;
    gettimeofday(&tvb, NULL);
    for(int j = 0; j < REQUEST_COUNT; j++) {
        
        res = pread(fd, buf, BLOCK_SIZE, get_rnd_block_offset(BLOCK_SIZE, length));
        check("Could not do IO request", res != BLOCK_SIZE);
        
    }
    gettimeofday(&tve, NULL);

    long bu = tvb.tv_sec * 1000000L + tvb.tv_usec;
    long eu = tve.tv_sec * 1000000L + tve.tv_usec;

    float tpr = ((float)(eu - bu)) / (float)REQUEST_COUNT;
    printf("Time/request: %fus\n", tpr);

    float rps = (float)REQUEST_COUNT / ((float)(eu - bu) / 1000000.0f);
    printf("IOPS: %fus\n", rps);
    
    close(fd);
}
