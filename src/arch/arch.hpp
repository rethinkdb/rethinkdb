// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_ARCH_HPP_
#define ARCH_ARCH_HPP_

#include "arch/runtime/runtime.hpp"

/* The original plan was to have all Linux-specific code in the `linux`
directory and to have corresponding directories for other OSes. This was a bad
idea because so much code is shared between OSes, so it was (incompletely)
changed back. We'll sort this mess out when it's time to support another OS. */

#include "arch/io/arch.hpp"

typedef linux_io_backend_t io_backend_t;

#include "arch/types.hpp"

void co_read(file_t *file, size_t offset, size_t length, void *buf, file_account_t *account);
void co_write(file_t *file, size_t offset, size_t length, void *buf, file_account_t *account);

#include "arch/spinlock.hpp"
#include "arch/timing.hpp"

#endif /* ARCH_ARCH_HPP_ */
