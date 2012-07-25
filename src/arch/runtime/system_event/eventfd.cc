#if defined(LEGACY_LINUX) && !defined(NO_EVENTFD)

#include <sys/syscall.h>

// Wrappers for eventfd(), since CentOS backported the system
// calls but not the libc wrappers.

#include "arch/runtime/system_event/eventfd.hpp"
#include "utils.hpp"

int eventfd(int count, UNUSED int flags) {
    rassert(flags == 0); // Legacy kernel doesn't have eventfd2.
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
