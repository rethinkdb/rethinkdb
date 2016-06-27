// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef ARCH_TYPES_HPP_
#define ARCH_TYPES_HPP_

#include <inttypes.h>
#include <string.h>

#include <string>

#include "errors.hpp"
#include "arch/runtime/runtime_utils.hpp"

template <class> class scoped_array_t;
struct iovec;

#define DEFAULT_DISK_ACCOUNT (static_cast<file_account_t *>(0))
#define UNLIMITED_OUTSTANDING_REQUESTS (-1)

// TODO: Remove this from this header.

// The linux_tcp_listener_t constructor can throw this exception
class address_in_use_exc_t : public std::exception {
public:
    address_in_use_exc_t(const char* hostname, int port) throw ();
    explicit address_in_use_exc_t(const std::string &msg) throw ()
        : info(msg) { }

    ~address_in_use_exc_t() throw () { }

    const char *what() const throw () {
        return info.c_str();
    }

private:
    std::string info;
};

class tcp_socket_exc_t : public std::exception {
public:
    explicit tcp_socket_exc_t(int err, int port);

    ~tcp_socket_exc_t() throw () { }

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
    virtual void on_io_failure(int errsv, int64_t offset, int64_t count);
};

class linux_thread_pool_t;
typedef linux_thread_pool_t thread_pool_t;

class file_account_t;

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

class linux_secure_tcp_conn_t;
typedef linux_secure_tcp_conn_t secure_tcp_conn_t;

enum class file_direct_io_mode_t {
    direct_desired,
    buffered_desired
};

// A linux file.  It expects reads and writes and buffers to have an
// alignment of DEVICE_BLOCK_SIZE.
class file_t {
public:
    enum wrap_in_datasyncs_t { NO_DATASYNCS, WRAP_IN_DATASYNCS };

    file_t() { }

    virtual ~file_t() { }
    virtual int64_t get_file_size() = 0;
    virtual void set_file_size(int64_t size) = 0;
    virtual void set_file_size_at_least(int64_t size) = 0;

    virtual void read_async(int64_t offset, size_t length, void *buf,
                            file_account_t *account, linux_iocallback_t *cb) = 0;
    virtual void write_async(int64_t offset, size_t length, const void *buf,
                             file_account_t *account, linux_iocallback_t *cb,
                             wrap_in_datasyncs_t wrap_in_datasyncs) = 0;
    // writev_async doesn't provide the atomicity guarantees of writev.
    virtual void writev_async(int64_t offset, size_t length, scoped_array_t<iovec> &&bufs,
                              file_account_t *account, linux_iocallback_t *cb) = 0;

    virtual void *create_account(int priority, int outstanding_requests_limit) = 0;
    virtual void destroy_account(void *account) = 0;

    virtual bool coop_lock_and_check() = 0;

private:
    DISABLE_COPYING(file_t);
};

class file_account_t {
public:
    file_account_t(file_t *f, int p, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS);
    ~file_account_t();
    void *get_account() { return account; }

private:
    file_t *parent;

    // When used with a linux_file_t, account is a `accounting_diskmgr_t::account_t
    // *`.  When used with a mock_file_t, it's a unused non-null pointer to some kind
    // of irrelevant object.
    void *account;

    DISABLE_COPYING(file_account_t);
};


#endif  // ARCH_TYPES_HPP_
