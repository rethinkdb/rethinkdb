// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_TYPES_HPP_
#define ARCH_TYPES_HPP_

#include <string>

#include "utils.hpp"

#define DEFAULT_DISK_ACCOUNT (static_cast<file_account_t *>(0))
#define UNLIMITED_OUTSTANDING_REQUESTS (-1)

// TODO: Remove this from this header.

// The linux_tcp_listener_t constructor can throw this exception
class address_in_use_exc_t : public std::exception {
public:
    address_in_use_exc_t(const char* hostname, int port) throw () :
        info(strprintf("The address at %s:%d is reserved or already in use", hostname, port)) { }
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

class file_account_t;

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

// A linux file.  It expects reads and writes and buffers to have an
// alignment of DEVICE_BLOCK_SIZE.
class file_t {
public:
    virtual ~file_t() { }
    virtual bool exists() = 0;
    virtual bool is_block_device() = 0;
    virtual uint64_t get_size() = 0;
    virtual void set_size(size_t size) = 0;
    virtual void set_size_at_least(size_t size) = 0;

    virtual void read_async(size_t offset, size_t length, void *buf, file_account_t *account, linux_iocallback_t *cb) = 0;
    virtual void write_async(size_t offset, size_t length, const void *buf, file_account_t *account, linux_iocallback_t *cb) = 0;

    virtual void read_blocking(size_t offset, size_t length, void *buf) = 0;
    virtual void write_blocking(size_t offset, size_t length, const void *buf) = 0;

    virtual void *create_account(int priority, int outstanding_requests_limit) = 0;
    virtual void destroy_account(void *account) = 0;

    virtual bool coop_lock_and_check() = 0;
};

class file_account_t {
public:
    file_account_t(file_t *f, int p, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS);
    ~file_account_t();
    void *get_account() { return account; }

private:
    file_t *parent;
    /* account is internally a pointer to a accounting_diskmgr_t::account_t object. It has to be
       a void* because accounting_diskmgr_t is a template, so its actual type depends on what
       IO backend is chosen. */
    // Maybe accounting_diskmgr_t shouldn't be a templated class then.

    void *account;

    DISABLE_COPYING(file_account_t);
};


#endif  // ARCH_TYPES_HPP_
