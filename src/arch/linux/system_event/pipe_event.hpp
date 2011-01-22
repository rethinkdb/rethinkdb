
#ifndef __ARCH_LINUX_PIPE_EVENT_HPP__
#define __ARCH_LINUX_PIPE_EVENT_HPP__

#include <stdint.h>

// An event API implemented in terms of eventfd. May not be available
// on older kernels.
struct pipe_event_t {
public:
    pipe_event_t();
    ~pipe_event_t();

    uint64_t read();
    void write(uint64_t value);

    int get_notify_fd() {
        return pipefd[0];
    }

private:
    int pipefd[2];
};


#endif // __ARCH_LINUX_PIPE_EVENT_HPP__

