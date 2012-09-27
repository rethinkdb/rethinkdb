#ifndef ARCH_TYPES_HPP_
#define ARCH_TYPES_HPP_

#include <string>

#include "utils.hpp"

#define DEFAULT_DISK_ACCOUNT (static_cast<linux_file_account_t *>(0))
#define UNLIMITED_OUTSTANDING_REQUESTS (-1)

// TODO: Remove this from this header.

// The linux_tcp_listener_t constructor can throw this exception
class address_in_use_exc_t : public std::exception {
public:
    address_in_use_exc_t(const char* hostname, int port) throw () :
        info(strprintf("The address at %s:%d is already in use", hostname, port)) { }
    ~address_in_use_exc_t() throw () { }

    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

class tcp_conn_read_closed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Network connection read end closed";
    }
};

struct tcp_conn_write_closed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Network connection write end closed";
    }
};


class linux_iocallback_t {
public:
    virtual ~linux_iocallback_t() {}
    virtual void on_io_complete() = 0;

    //TODO Remove this default implementation and actually handle io errors.
    virtual void on_io_failure() {
        crash("I/O operation failed. Exiting.\n");
    }
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

class linux_repeated_nonthrowing_tcp_listener_t;
typedef linux_repeated_nonthrowing_tcp_listener_t repeated_nonthrowing_tcp_listener_t;

class linux_tcp_conn_descriptor_t;
typedef linux_tcp_conn_descriptor_t tcp_conn_descriptor_t;

class linux_tcp_conn_t;
typedef linux_tcp_conn_t tcp_conn_t;



#endif  // ARCH_TYPES_HPP_
