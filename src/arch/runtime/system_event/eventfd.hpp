// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_SYSTEM_EVENT_EVENTFD_HPP_
#define ARCH_RUNTIME_SYSTEM_EVENT_EVENTFD_HPP_

#ifndef LEGACY_LINUX

#include <sys/eventfd.h>

#else

#include <stdint.h>

typedef uint64_t eventfd_t;

int eventfd(int count, int flags);
int eventfd_read(int fd, eventfd_t *value);
int eventfd_write(int fd, eventfd_t value);

#endif /* LEGACY_LINUX */

#endif /* ARCH_RUNTIME_SYSTEM_EVENT_EVENTFD_HPP_ */
