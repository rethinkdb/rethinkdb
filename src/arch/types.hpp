#ifndef ARCH_TYPES_HPP_
#define ARCH_TYPES_HPP_

#define DEFAULT_DISK_ACCOUNT (static_cast<linux_file_account_t *>(0))
#define UNLIMITED_OUTSTANDING_REQUESTS (-1)

class linux_iocallback_t {
public:
    virtual ~linux_iocallback_t() {}
    virtual void on_io_complete() = 0;
};

class linux_thread_pool_t;
typedef linux_thread_pool_t thread_pool_t;

class linux_file_t;
typedef linux_file_t file_t;

class linux_file_account_t;
typedef linux_file_account_t file_account_t;

class linux_direct_file_t;
typedef linux_direct_file_t direct_file_t;

class linux_nondirect_file_t;
typedef linux_nondirect_file_t nondirect_file_t;

class linux_iocallback_t;
typedef linux_iocallback_t iocallback_t;

class linux_tcp_bound_socket_t;
typedef linux_tcp_bound_socket_t tcp_bound_socket_t;

class linux_nonthrowing_tcp_listener_t;
typedef linux_nonthrowing_tcp_listener_t non_throwing_tcp_listener_t;

class linux_tcp_listener_t;
typedef linux_tcp_listener_t tcp_listener_t;

class repeated_linux_tcp_listener_t;
typedef repeated_linux_tcp_listener_t repeated_tcp_listener_t;

class linux_nascent_tcp_conn_t;
typedef linux_nascent_tcp_conn_t nascent_tcp_conn_t;

class linux_tcp_conn_t;
typedef linux_tcp_conn_t tcp_conn_t;



#endif  // ARCH_TYPES_HPP_
