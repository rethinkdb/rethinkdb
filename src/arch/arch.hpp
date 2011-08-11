#ifndef __ARCH_ARCH_HPP__
#define __ARCH_ARCH_HPP__

#include "arch/runtime/runtime.hpp"

/* The original plan was to have all Linux-specific code in the `linux`
directory and to have corresponding directories for other OSes. This was a bad
idea because so much code is shared between OSes, so it was (incompletely)
changed back. We'll sort this mess out when it's time to support another OS. */

#include "arch/io/arch.hpp"

typedef linux_io_backend_t io_backend_t;

#include "arch/types.hpp"

void co_read(direct_file_t *file, size_t offset, size_t length, void *buf, direct_file_t::account_t *account);
void co_write(direct_file_t *file, size_t offset, size_t length, void *buf, direct_file_t::account_t *account);

#include "arch/spinlock.hpp"
#include "arch/timing.hpp"

#endif /* __ARCH_ARCH_HPP__ */
