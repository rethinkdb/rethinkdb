#ifndef __ARCH_ARCH_HPP__
#define __ARCH_ARCH_HPP__

/* Cross-platform stuff */

#include <stdint.h>

typedef uint64_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

typedef char byte;

/* Select platform-specific stuff */

#ifndef MOCK_IO_LAYER

#include "arch/linux/arch.hpp"
typedef linux_event_queue_t event_queue_t;
typedef linux_direct_file_t direct_file_t;
typedef linux_net_listener_t net_listener_t;
typedef linux_net_conn_t net_conn_t;
typedef linux_iocallback_t iocallback_t;
typedef linux_net_conn_callback_t net_conn_callback_t;

#else

#include "arch/mock/arch.hpp"
typedef ...;

#endif

#endif /* __ARCH_ARCH_HPP__ */
