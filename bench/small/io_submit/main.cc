
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

#define REQUEST_COUNT  500
#define REQUEST_BURSTS 10
#define BLOCK_SIZE     512
#define USE_AIO        1

static io_context_t ctx;

void* aio_poll_worker(void *arg) {
    int res;
    io_event events[1000];
    int count = 0;
    do {
        res = io_getevents(ctx, 1, sizeof(events), events, NULL);
        check("Waiting for AIO events failed", res < 0);
        for(int i = 0; i < res; i++) {
            check("Bad result", events[i].res != BLOCK_SIZE);
            count++;
        }
    } while(count < REQUEST_COUNT);
}

int main() {
    srand(time(NULL));
    ctx = 0;
    int rpb = REQUEST_COUNT / REQUEST_BURSTS;
    int res;
    if(USE_AIO) {
        res = io_setup(rpb, &ctx);
        check("io_setup failed", res != 0);
    }

    char device[] = "/dev/sdb";
    off64_t length = get_device_length(device);
    int fd = open(device, O_DIRECT | O_NOATIME | O_RDONLY);
    check("Couldn't open device", fd == -1);

    iocb request[REQUEST_COUNT];
    char *buf;
    res = posix_memalign((void**)&buf, 512, BLOCK_SIZE);
    check("Couldn't alloc buffer", res != 0);

    // start the thread
    pthread_t listener;
    res = pthread_create(&listener, NULL, aio_poll_worker, NULL);
    check("Could not start thread", res != 0);

    float times[REQUEST_BURSTS];
    float max_time = 0;
    for(int j = 0; j < REQUEST_BURSTS; j++) {
        timeval tvb, tve;
        gettimeofday(&tvb, NULL);
        
        for(int i = 0; i < rpb; i++) {
            if(USE_AIO) {
                io_prep_pread(&request[j * rpb + i],
                              fd, buf, BLOCK_SIZE,
                              get_rnd_block_offset(BLOCK_SIZE, length));
                request->data = NULL;
                iocb* requests[1];
                requests[0] = &request[j * rpb + i];
                res = 0;
                do {
                    if(res == -EAGAIN) {
                        //sleep(1);
                    }
                    res = io_submit(ctx, 1, requests);
                } while(res == -EAGAIN);
                if(res < 1) {
                    errno = -res;
                    check("Could not submit IO request", res < 1);
                }
            } else {
                res = pread(fd, buf, BLOCK_SIZE, get_rnd_block_offset(BLOCK_SIZE, length));
                check("Could not do IO request", res != BLOCK_SIZE);
            }
        }
        
        gettimeofday(&tve, NULL);

        long bu = tvb.tv_sec * 1000000L + tvb.tv_usec;
        long eu = tve.tv_sec * 1000000L + tve.tv_usec;
        float diff = ((float)(eu - bu)) / (float)rpb;
        times[j] = diff;
        if(diff > max_time)
            max_time = diff;
    }

    res = pthread_join(listener, NULL);
    check("Could not join threads", res != 0);

    float sum = 0;
    for(int i = 0; i < REQUEST_BURSTS; i ++) {
        printf("burst: %d, Time/request: %fus\n", i, times[i]);
        sum += times[i];
    }

    printf("max: %f.2us, avg: %fus\n", max_time, sum / (float)REQUEST_BURSTS);
    
    if(USE_AIO) {
        io_destroy(ctx);
    }

    close(fd);
}
