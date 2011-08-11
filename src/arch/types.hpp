#ifndef ARCH_TYPES_HPP_
#define ARCH_TYPES_HPP_

class linux_thread_pool_t;
typedef linux_thread_pool_t thread_pool_t;

class linux_file_t;
typedef linux_file_t file_t;

class linux_direct_file_t;
typedef linux_direct_file_t direct_file_t;

class linux_nondirect_file_t;
typedef linux_nondirect_file_t nondirect_file_t;

class linux_iocallback_t;
typedef linux_iocallback_t iocallback_t;

enum linux_io_backend_t;
typedef linux_io_backend_t io_backend_t;

class linux_tcp_listener_t;
typedef linux_tcp_listener_t tcp_listener_t;

class linux_tcp_conn_t;
typedef linux_tcp_conn_t tcp_conn_t;



#endif  // ARCH_TYPES_HPP_
