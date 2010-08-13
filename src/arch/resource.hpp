
#ifndef __ARCH_COMMON_HPP__
#define __ARCH_COMMON_HPP__

#include <stdint.h>

typedef int fd_t;
#define INVALID_FD fd_t(-1)

typedef fd_t resource_t;

typedef uint64_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

typedef char byte;

#endif // __ARCH_COMMON_HPP__

