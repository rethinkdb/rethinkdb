#ifdef LEGACY_LINUX

// Wrappers for eventfd(), since CentOS backported the system
// calls but not the libc wrappers.

#include "arch/linux/eventfd.hpp"
#include "utils.hpp"
#include <sys/syscall.h>

int eventfd(int count, int flags) {
    assert(flags == 0); // Legacy kernel doesn't have eventfd2.
    return syscall(SYS_eventfd, count);
}

int eventfd_read(int fd, eventfd_t *value) {
    int res = read(fd, value, sizeof(eventfd_t));
    return (res == sizeof(eventfd_t)) ? 0 : -1;
}

int eventfd_write(int fd, eventfd_t value) {
  int res = write(fd, &value, sizeof(eventfd_t));
  return (res == sizeof(eventfd_t)) ? 0 : -1;
}

#endif
