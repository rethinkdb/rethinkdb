#ifndef ARCH_IO_DISK_FILESTAT_HPP_
#define ARCH_IO_DISK_FILESTAT_HPP_

#include <stdint.h>

#include "arch/runtime/runtime_utils.hpp"

int64_t get_file_size(fd_t fd);

#endif  // ARCH_IO_DISK_FILESTAT_HPP_
