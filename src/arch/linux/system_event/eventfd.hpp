#ifndef __ARCH_LINUX_EVENTFD_HPP__
#define __ARCH_LINUX_EVENTFD_HPP__

#ifndef LEGACY_LINUX

#include <sys/eventfd.h>

#else

#include <stdint.h>

typedef uint64_t eventfd_t;

int eventfd(int count, int flags);
int eventfd_read(int fd, eventfd_t *value);
int eventfd_write(int fd, eventfd_t value);

#endif /* LEGACY_LINUX */

#endif /* __ARCH_LINUX_EVENTFD_HPP__ */
