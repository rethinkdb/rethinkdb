#ifndef __ARCH_ARCH_HPP__
#define __ARCH_ARCH_HPP__

/* Cross-platform stuff */

#include <stdint.h>

typedef uint64_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

typedef char byte;

/* Select platform-specific stuff */

/* #if WE_ARE_ON_LINUX */

#include "arch/linux/arch.hpp"
typedef linux_io_config_t platform_io_config_t;

/* #elif WE_ARE_ON_WINDOWS

#include "arch/win32/arch.hpp"
typedef win32_io_config_t platform_io_config_t

#elif ...

...

#endif */

/* Optionally mock the IO layer */

#define MOCK_IO_LAYER

#ifndef MOCK_IO_LAYER

typedef platform_io_config_t io_config_t;

#else

#include "arch/mock/arch.hpp"
typedef mock_io_config_t<platform_io_config_t> io_config_t;

#endif

/* Move stuff into global namespace */

typedef io_config_t::event_queue_t event_queue_t;
typedef io_config_t::direct_file_t direct_file_t;
typedef io_config_t::net_listener_t net_listener_t;
typedef io_config_t::net_conn_t net_conn_t;
typedef io_config_t::iocallback_t iocallback_t;
typedef io_config_t::net_conn_callback_t net_conn_callback_t;

#endif /* __ARCH_ARCH_HPP__ */
